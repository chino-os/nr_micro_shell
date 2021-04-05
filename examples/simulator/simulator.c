#include "/home/nrush/workspace/nr_micro_shell/inc/nr_micro_shell.h"
#include <stdio.h>
#include "stdlib.h"
#include "string.h"
#include <curses.h>

DECLARE_AND_DEFAULT_NR_SHELL(shell);

int main(void)
{
    initscr();
    cur_shell = &shell;
    nr_printf("hello world!");
    dump_src_mem(shell.cons);
    fflush(stdout);
    char c;
    while(1)
    {
        c = getch();
        printf("-->%x; ",c);
        fflush(stdout);
        nr_shell_get_char(cur_shell,c);
    }
    endwin();
    return 0;
}