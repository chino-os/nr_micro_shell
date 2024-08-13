// Copyright (c) SunnyCase. All rights reserved.
// Licensed under the Apache license. See LICENSE file in the project root for full license information.
#include "commands.h"
#include <cassert>
#include <cerrno>
#include <chino/os/ioapi.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

using namespace chino;
using namespace chino::os;
using namespace chino::shell;

/** ping receive timeout - in milliseconds */
#ifndef PING_RCV_TIMEO
#define PING_RCV_TIMEO 1000
#endif

/** ping delay - in milliseconds */
#ifndef PING_DELAY
#define PING_DELAY 1000
#endif

/** ping identifier - must fit on a uint16_t */
#ifndef PING_ID
#define PING_ID 0xAFAF
#endif

/** ping additional data size to include in the packet */
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif

constexpr size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;
static_assert(ping_size <= 0xffff);

/** ping result action - no default action */
#ifndef PING_RESULT
#define PING_RESULT(ping_ok)
#endif

/** Swap the bytes in an u16_t: much like lwip_htons() for little-endian */
#ifndef SWAP_BYTES_IN_WORD
#define SWAP_BYTES_IN_WORD(w) (((w) & 0xff) << 8) | (((w) & 0xff00) >> 8)
#endif /* SWAP_BYTES_IN_WORD */

/** Split an u32_t in two u16_ts and add them up */
#ifndef FOLD_U32T
#define FOLD_U32T(u) ((uint32_t)(((u) >> 16) + ((u) & 0x0000ffffUL)))
#endif

static uint32_t current_ms() {
    timespec cnt_time;
    timespec_get(&cnt_time, TIME_UTC);
    return cnt_time.tv_sec * 1000 + cnt_time.tv_nsec / 1000000;
}

/**
 * An optimized checksum routine. Basically, it uses loop-unrolling on
 * the checksum loop, treating the head and tail bytes specially, whereas
 * the inner loop acts on 8 bytes at a time.
 *
 * @arg start of buffer to be checksummed. May be an odd byte address.
 * @len number of bytes in the buffer to be checksummed.
 * @return host order (!) lwip checksum (non-inverted Internet sum)
 *
 * by Curt McDowell, Broadcom Corp. December 8th, 2005
 */
static uint16_t inet_chksum(const void *dataptr, int len) {
    const uint8_t *pb = (const uint8_t *)dataptr;
    const uint16_t *ps;
    uint16_t t = 0;
    const uint32_t *pl;
    uint32_t sum = 0, tmp;
    /* starts at odd byte address? */
    int odd = ((uintptr_t)pb & 1);

    if (odd && len > 0) {
        ((uint8_t *)&t)[1] = *pb++;
        len--;
    }

    ps = (const uint16_t *)(const void *)pb;

    if (((uintptr_t)ps & 3) && len > 1) {
        sum += *ps++;
        len -= 2;
    }

    pl = (const uint32_t *)(const void *)ps;

    while (len > 7) {
        tmp = sum + *pl++; /* ping */
        if (tmp < sum) {
            tmp++; /* add back carry */
        }

        sum = tmp + *pl++; /* pong */
        if (sum < tmp) {
            sum++; /* add back carry */
        }

        len -= 8;
    }

    /* make room in upper bits */
    sum = FOLD_U32T(sum);

    ps = (const uint16_t *)pl;

    /* 16-bit aligned word remaining? */
    while (len > 1) {
        sum += *ps++;
        len -= 2;
    }

    /* dangling tail byte remaining? */
    if (len > 0) { /* include odd byte */
        ((uint8_t *)&t)[0] = *(const uint8_t *)ps;
    }

    sum += t; /* add end bytes */

    /* Fold 32-bit sum to 16 bits
       calling this twice is probably faster than if statements... */
    sum = FOLD_U32T(sum);
    sum = FOLD_U32T(sum);

    if (odd) {
        sum = SWAP_BYTES_IN_WORD(sum);
    }

    return ~(uint16_t)sum;
}

class ping_context {
  public:
    /** Prepare a echo ICMP request */
    void ping_prepare_echo(struct icmp_echo_hdr *iecho, uint16_t len) {
        size_t i;
        size_t data_len = len - sizeof(struct icmp_echo_hdr);

        iecho->type = ICMP_ECHO;
        iecho->code = 0;
        iecho->chksum = 0;
        iecho->id = PING_ID;
        iecho->seqno = htons(++ping_seq_num_);

        /* fill the additional data buffer with some data */
        for (i = 0; i < data_len; i++) {
            ((char *)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
        }

        iecho->chksum = inet_chksum(iecho, len);
    }

    /* Ping using the socket ip */
    result<void> ping_send(int s, in_addr_t addr) {
        struct icmp_echo_hdr *iecho;
        struct sockaddr_storage to {};
        uint8_t ping_buf[ping_size];

        iecho = (struct icmp_echo_hdr *)ping_buf;
        if (!iecho) {
            return err(error_code::out_of_memory);
        }

        ping_prepare_echo(iecho, (uint16_t)ping_size);

        struct sockaddr_in *to4 = (struct sockaddr_in *)&to;
        to4->sin_family = AF_INET;
        to4->sin_addr.s_addr = addr;
        to4->sin_port = 0;

        ping_time_ = current_ms();
        return sendto(s, iecho, ping_size, 0, (struct sockaddr *)&to, sizeof(sockaddr_in)) > 0
                   ? ok()
                   : err(error_code::invalid_argument);
    }

    void ping_recv(int s) {
        char buf[64];
        int len;
        struct sockaddr_storage from;
        int fromlen = sizeof(from);

        while ((len = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&from, (socklen_t *)&fromlen)) > 0) {
            if (len >= (int)(sizeof(struct iphdr) + sizeof(struct icmp_echo_hdr))) {
                in_addr_t fromaddr = 0;

                if (from.ss_family == AF_INET) {
                    struct sockaddr_in *from4 = (struct sockaddr_in *)&from;
                    fromaddr = from4->sin_addr.s_addr;
                } else {
                    break;
                }

                auto addr_le = (uint32_t)ntohl(fromaddr);

                struct iphdr *iphdr;
                struct icmp_echo_hdr *iecho;

                iphdr = (struct iphdr *)buf;
                iecho = (struct icmp_echo_hdr *)(buf + iphdr->ihl * 4);
                auto data_len = len - iphdr->ihl * 4 - sizeof(icmp_echo_hdr);
                if ((iecho->id == PING_ID) && (iecho->seqno == htons(ping_seq_num_))) {
                    printf("%" PRIu32 "bytes from %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32
                           ": icmp_seq=%u ttl=%" PRIi8 " time=%" PRIu32 "ms\n",
                           (uint32_t)data_len, addr_le >> 24, (addr_le >> 16) & 0xFF, (addr_le >> 8) & 0xFF,
                           addr_le & 0xFF, htons(iecho->seqno), iphdr->ttl, current_ms() - ping_time_);
                    return;
                } else {
                    printf("ping: drop\n");
                }
            }
            fromlen = sizeof(from);
        }

        if (len == 0) {
            printf("ping: recv - %" PRIu32 " ms - timeout\n", current_ms() - ping_time_);
        }
    }

  private:
    uint16_t ping_seq_num_ = 0;
    uint32_t ping_time_;
};

void commands::ping(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <address>\n", argv[0]);
        return;
    }

    in_addr_t ping_target = 0;
    if (inet_pton(AF_INET, argv[1], &ping_target) != 1) {
        fprintf(stderr, "%s: invalid address: %s\n", argv[0], argv[1]);
        return;
    }

    printf("PING %s %d bytes of data.\n", argv[1], (int)PING_DATA_SIZE);

    struct timeval timeout;
    timeout.tv_sec = PING_RCV_TIMEO / 1000;
    timeout.tv_usec = (PING_RCV_TIMEO % 1000) * 1000;

    auto s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (s < 0) {
        fprintf(stderr, "%s: open socket failed: %s\n", argv[0], strerror(errno));
        return;
    }

    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0) {
        fprintf(stderr, "%s: setting receive timeout failed: %s\n", argv[0], strerror(errno));
        return;
    }

    async_io_result stdin_io{};
    char in_c;
    (void)read_async(STDIN_FILENO, {reinterpret_cast<std::byte *>(&in_c), 1}, 0, stdin_io);

    ping_context pc;
    while (1) {
        if (pc.ping_send(s, ping_target).is_ok()) {
            pc.ping_recv(s);
        } else {
            printf("error: %s\n", strerror(errno));
        }

        if (stdin_io.bytes_transferred == 1) {
            if (in_c == 0x3) { // CTRL+C
                puts("");
                break;
            } else {
                stdin_io = {};
                (void)read_async(STDIN_FILENO, {reinterpret_cast<std::byte *>(&in_c), 1}, 0, stdin_io);
            }
            (void)wait_queued_io();
        }
        sleep(1);
    }

    close(s);
}
