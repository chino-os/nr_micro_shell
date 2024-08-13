// Copyright (c) SunnyCase. All rights reserved.
// Licensed under the Apache license. See LICENSE file in the project root for full license information.
#include "commands.h"
#include <cerrno>
#include <chino/os/ioapi.h>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

using namespace chino;
using namespace chino::os;
using namespace chino::shell;

void commands::serial(int argc, char *argv[]) {
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
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1; // timeout 100ms

    speed_t baudrate = 115200;
    if (argc >= 3) {
        baudrate = atoi(argv[2]);
    }
    cfsetspeed(&tty, baudrate);
    tcflush(sfd, TCIOFLUSH);
    tcsetattr(sfd, TCSAFLUSH, &tty);

    char stdin_c;
    char serial_in_c;
    bool stdin_ready = false;
    bool serial_in_ready = false;

    async_io_result stdin_io;
    async_io_result serial_in_io;

    if (read_async(STDIN_FILENO, {reinterpret_cast<std::byte *>(&stdin_c), 1}, 0, stdin_io).is_ok()) {
        stdin_ready = true;
    }

    if (read_async(sfd, {reinterpret_cast<std::byte *>(&serial_in_c), 1}, 0, serial_in_io).is_ok()) {
        serial_in_ready = true;
    }

    while (true) {
        while (stdin_ready) {
            stdin_ready = false;
            if (stdin_c == 0x3) { // CTRL+C
                puts("");
                goto end;
            }
            write(sfd, &stdin_c, 1);
            if (read_async(STDIN_FILENO, {reinterpret_cast<std::byte *>(&stdin_c), 1}, 0, stdin_io).is_ok()) {
                stdin_ready = true;
            }
        }

        while (serial_in_ready) {
            serial_in_ready = false;
            write(STDOUT_FILENO, &serial_in_c, 1);
            if (read_async(sfd, {reinterpret_cast<std::byte *>(&serial_in_c), 1}, 0, serial_in_io).is_ok()) {
                serial_in_ready = true;
            }
        }

        auto ready_io = wait_queued_io().unwrap();
        if (ready_io == &stdin_io)
            stdin_ready = true;
        else if (ready_io == &serial_in_io)
            serial_in_ready = true;
    }

end:
    close(sfd);
}
