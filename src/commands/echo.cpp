// Copyright (c) SunnyCase. All rights reserved.
// Licensed under the Apache license. See LICENSE file in the project root for full license information.
#include "commands.h"
#include <cstdio>

using namespace chino;
using namespace chino::shell;

static void commands::echo(int argc, char *argv[]) {
    for (size_t i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
}
