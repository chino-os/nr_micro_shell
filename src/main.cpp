// Copyright (c) SunnyCase. All rights reserved.
// Licensed under the Apache license. See LICENSE file in the project root for full license information.
#include <nr_micro_shell.h>

#include <Windows.h>
#include <stdio.h>
#include <sys/cdefs.h>

extern "C" {
/*
 * Dummy stdio hooks. This allows programs to link without requiring
 * any system-dependent functions. This is only used if the program
 * doesn't provide its own version of stdin, stdout, stderr
 */

static int dummy_putc(char c, FILE *file) {
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), &c, 1, nullptr, nullptr);
    return (unsigned char)c;
}

static int dummy_getc(FILE *file) {
    char c;
    DWORD count;
    ReadConsoleA(GetStdHandle(STD_INPUT_HANDLE), &c, 1, &count, nullptr);
    return count ? c : EOF;
}

static int dummy_flush(FILE *file) {
    (void)file;
    FlushFileBuffers(GetStdHandle(STD_OUTPUT_HANDLE));
    return 0;
}

static FILE __stdio = FDEV_SETUP_STREAM(dummy_putc, dummy_getc, dummy_flush, _FDEV_SETUP_RW);

#ifdef __strong_reference
#define STDIO_ALIAS(x) __strong_reference(stdin, x);
#else
#define STDIO_ALIAS(x) FILE *const x = &__stdio;
#endif

FILE *const stdin = &__stdio;
STDIO_ALIAS(stdout);
STDIO_ALIAS(stderr);

const static_cmd_st static_cmd[] = {};
}

int main(int argc, char *argv[]) {
    char c;
    shell_init();
    while (1) {
        c = getchar();
        shell(c);
    }
    return 0;
}
