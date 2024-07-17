// Copyright (c) SunnyCase. All rights reserved.
// Licensed under the Apache license. See LICENSE file in the project root for full license information.
#include "commands.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <sys/errno.h>
#include <sys/file.h>
#include <unistd.h>

using namespace chino;
using namespace chino::shell;

static void commands::cat(int argc, char *argv[]) {
    for (size_t i = 1; i < argc; i++) {
        auto fd = open(argv[i], O_RDONLY);
        if (fd != -1) {
            char buf[64];
            while (1) {
                auto size = read(fd, buf, std::size(buf));
                if (size < 0) {
                    fprintf(stderr, "cat: %s: %s\n", argv[i], strerror(errno));
                } else {
                    write(STDOUT_FILENO, buf, size);
                    if (size < std::size(buf))
                        break;
                }
            }
            close(fd);
        } else {
            fprintf(stderr, "cat: %s: %s\n", argv[i], strerror(errno));
        }
    }
}
