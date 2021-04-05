#include <stdarg.h>
#include <stdio.h>
#include "nr_micro_shell.h"

#define NR_SHELL_PRINT_BUF_SIZE 128

vcons_st *cur_vcons;

void nr_printf(char *fmt, ...)
{
	int res = 0;
	int i;
	char out_buf[NR_SHELL_PRINT_BUF_SIZE];
	if(cur_vcons == NULL)
		return;
	va_list args;
	va_start(args, fmt);
	res = vsnprintf(out_buf, NR_SHELL_PRINT_BUF_SIZE, fmt, args);
	for(i = 0; i < res; i++)
	{
		write_to_console(cur_vcons, out_buf[i]);
	}
	va_end(args);
}


