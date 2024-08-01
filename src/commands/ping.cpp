// Copyright (c) SunnyCase. All rights reserved.
// Licensed under the Apache license. See LICENSE file in the project root for full license information.
#include "commands.h"
#include <array>
#include <cerrno>
#include <chino/os/ioapi.h>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

using namespace chino;
using namespace chino::os;
using namespace chino::shell;

/**
 * PING_DEBUG: Enable debugging for PING.
 */
#ifndef PING_DEBUG
#define PING_DEBUG LWIP_DBG_ON
#endif

/** ping receive timeout - in milliseconds */
#ifndef PING_RCV_TIMEO
#define PING_RCV_TIMEO 1000
#endif

/** ping delay - in milliseconds */
#ifndef PING_DELAY
#define PING_DELAY 1000
#endif

/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID 0xAFAF
#endif

/** ping additional data size to include in the packet */
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif

/** ping result action - no default action */
#ifndef PING_RESULT
#define PING_RESULT(ping_ok)
#endif

/* ping variables */
static const ip_addr_t *ping_target;
static u16_t ping_seq_num;
// #ifdef LWIP_DEBUG
static u32_t ping_time;
// #endif /* LWIP_DEBUG */

/** Prepare a echo ICMP request */
static void ping_prepare_echo(struct icmp_echo_hdr *iecho, u16_t len) {
    size_t i;
    size_t data_len = len - sizeof(struct icmp_echo_hdr);

    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id = PING_ID;
    iecho->seqno = lwip_htons(++ping_seq_num);

    /* fill the additional data buffer with some data */
    for (i = 0; i < data_len; i++) {
        ((char *)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
    }

    iecho->chksum = inet_chksum(iecho, len);
}

/* Ping using the socket ip */
static err_t ping_send(int s, const ip_addr_t *addr) {
    int err;
    struct icmp_echo_hdr *iecho;
    struct sockaddr_storage to;
    size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;
    LWIP_ASSERT("ping_size is too big", ping_size <= 0xffff);

#if LWIP_IPV6
    if (IP_IS_V6(addr) && !ip6_addr_isipv4mappedipv6(ip_2_ip6(addr))) {
        /* todo: support ICMP6 echo */
        return ERR_VAL;
    }
#endif /* LWIP_IPV6 */

    iecho = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)ping_size);
    if (!iecho) {
        return ERR_MEM;
    }

    ping_prepare_echo(iecho, (u16_t)ping_size);

#if LWIP_IPV4
    if (IP_IS_V4(addr)) {
        struct sockaddr_in *to4 = (struct sockaddr_in *)&to;
        to4->sin_family = AF_INET;
        //inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(addr));
    }
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
    if (IP_IS_V6(addr)) {
        struct sockaddr_in6 *to6 = (struct sockaddr_in6 *)&to;
        to6->sin6_len = sizeof(*to6);
        to6->sin6_family = AF_INET6;
        inet6_addr_from_ip6addr(&to6->sin6_addr, ip_2_ip6(addr));
    }
#endif /* LWIP_IPV6 */

    err = sendto(s, iecho, ping_size, 0, (struct sockaddr *)&to, sizeof(to));

    mem_free(iecho);

    return (err ? ERR_OK : ERR_VAL);
}

static void ping_recv(int s) {
    char buf[64];
    int len;
    struct sockaddr_storage from;
    int fromlen = sizeof(from);

    while ((len = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&from, (socklen_t *)&fromlen)) > 0) {
        if (len >= (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr))) {
            ip_addr_t fromaddr;
            memset(&fromaddr, 0, sizeof(fromaddr));

#if LWIP_IPV4
            if (from.ss_family == AF_INET) {
                struct sockaddr_in *from4 = (struct sockaddr_in *)&from;
                fromaddr.addr = from4->sin_addr.s_addr;
            }
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
            if (from.ss_family == AF_INET6) {
                struct sockaddr_in6 *from6 = (struct sockaddr_in6 *)&from;
                inet6_addr_to_ip6addr(ip_2_ip6(&fromaddr), &from6->sin6_addr);
                IP_SET_TYPE_VAL(fromaddr, IPADDR_TYPE_V6);
            }
#endif /* LWIP_IPV6 */

            LWIP_DEBUGF(PING_DEBUG, ("ping: recv "));
            ip_addr_debug_print_val(PING_DEBUG, fromaddr);
            LWIP_DEBUGF(PING_DEBUG, (" %" U32_F " ms\n", (sys_now() - ping_time)));

            /* todo: support ICMP6 echo */
#if LWIP_IPV4
            if (IP_IS_V4_VAL(fromaddr)) {
                struct ip_hdr *iphdr;
                struct icmp_echo_hdr *iecho;

                iphdr = (struct ip_hdr *)buf;
                iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));
                if ((iecho->id == PING_ID) && (iecho->seqno == lwip_htons(ping_seq_num))) {
                    /* do some ping result processing */
                    PING_RESULT((ICMPH_TYPE(iecho) == ICMP_ER));
                    return;
                } else {
                    LWIP_DEBUGF(PING_DEBUG, ("ping: drop\n"));
                }
            }
#endif /* LWIP_IPV4 */
        }
        fromlen = sizeof(from);
    }

    if (len == 0) {
        LWIP_DEBUGF(PING_DEBUG, ("ping: recv - %" U32_F " ms - timeout\n", (sys_now() - ping_time)));
    }

    /* do some ping result processing */
    PING_RESULT(0);
}

static void commands::ping(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <address>\n", argv[0]);
        return;
    }

    auto s = open("/dev/net/tcp", O_RDWR);
    if (s == -1) {
        fprintf(stderr, "%s: Error on opening %s: %s\n", argv[0], argv[1], strerror(errno));
        return;
    }

    int ret;

#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
    int timeout = PING_RCV_TIMEO;
#else
    struct timeval timeout;
    timeout.tv_sec = PING_RCV_TIMEO / 1000;
    timeout.tv_usec = (PING_RCV_TIMEO % 1000) * 1000;
#endif

#if LWIP_IPV6
    if (IP_IS_V4(ping_target) || ip6_addr_isipv4mappedipv6(ip_2_ip6(ping_target))) {
        s = lwip_socket(AF_INET6, SOCK_RAW, IP_PROTO_ICMP);
    } else {
        s = lwip_socket(AF_INET6, SOCK_RAW, IP6_NEXTH_ICMP6);
    }
#else
    s = socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
#endif
    if (s < 0) {
        return;
    }

    ret = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    LWIP_ASSERT("setting receive timeout failed", ret == 0);

    while (ping_target != NULL) {
        if (ping_send(s, ping_target) == ERR_OK) {
            LWIP_DEBUGF(PING_DEBUG, ("ping: send "));
            ip_addr_debug_print(PING_DEBUG, ping_target);
            LWIP_DEBUGF(PING_DEBUG, ("\n"));

#ifdef LWIP_DEBUG
            ping_time = sys_now();
#endif /* LWIP_DEBUG */
            ping_recv(s);
        } else {
            LWIP_DEBUGF(PING_DEBUG, ("ping: send "));
            ip_addr_debug_print(PING_DEBUG, ping_target);
            LWIP_DEBUGF(PING_DEBUG, (" - error\n"));
        }
        sys_msleep(PING_DELAY);
    }

    close(s);
}
