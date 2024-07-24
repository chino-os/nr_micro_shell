// Copyright (c) SunnyCase. All rights reserved.
// Licensed under the Apache license. See LICENSE file in the project root for full license information.
#include "commands.h"
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

using namespace chino;
using namespace chino::shell;

static void commands::serial(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <device> [baudrate=115200]\n", argv[0]);
        return;
    }

    auto sfd = open(argv[1], O_RDWR);
    if (sfd == -1) {
        fprintf(stderr, "%s: Error on opening %s: %s\n", argv[0], argv[1], strerror(errno));
        return;
    }

    struct termios tty;
    // Read in existing settings, and handle any error
    if (tcgetattr(sfd, &tty) != 0) {
        fprintf(stderr, "Error %i from tcgetattr: %s\n", errno, strerror(errno));
    }

    tty.c_cflag &= ~PARENB;        // Clear parity bit, disabling parity (most common)
    tty.c_cflag &= ~CSTOPB;        // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag &= ~CSIZE;         // Clear all bits that set the data size
    tty.c_cflag |= CS8;            // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS;       // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1; // timeout 100ms

    speed_t baudrate = 115200;
    if (argc >= 3) {
        baudrate = atoi(argv[2]);
    }
    cfsetspeed(&tty, baudrate);
    tcflush(sfd, TCIOFLUSH);
    tcsetattr(sfd, TCSAFLUSH, &tty);

    while (true) {
        auto c = getchar();
        if (c == -1) {
            break;
        }
        write(sfd, &c, 1);

        while (true) {
            auto ret = read(sfd, &c, 1);
            if (ret == -1) {
                fprintf(stderr, "Error %i from read: %s\n", errno, strerror(errno));
                goto end;
            } else if (ret) {
                if (c == 0x3) { // CTRL+C
                    puts("");
                    goto end;
                }
                write(STDOUT_FILENO, &c, 1);
            } else {
                break;
            }
        }
    }
end:
    close(sfd);
}
