// Copyright (c) SunnyCase. All rights reserved.
// Licensed under the Apache license. See LICENSE file in the project root for full license information.
#include "commands.h"
#include <nr_micro_shell.h>

using namespace chino::shell;

extern "C" const static_cmd_st static_cmd[] = {
    {"cat", commands::cat}, {"echo", commands::echo}, {"uname", commands::uname}, {"\0", NULL}};

int main(int argc, char *argv[]) {
    char c;
    shell_init();
    while (1) {
        c = getchar();
        shell(c);
    }
    return 0;
}
