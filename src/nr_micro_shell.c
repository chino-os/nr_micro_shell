#include <stdint.h>
#include <stdarg.h>
#include "nr_micro_shell.h"

#define NR_SHELL_CMD_LINE_MAX_LENGTH 128

void shell_parser(nr_shell_st *shell)
{
	/* todo: parser the last cmd line. */
	char cur_cmd[NR_SHELL_CMD_LINE_MAX_LENGTH];

	printf("I am working\r\n");
}

void shell_complete_cmd(nr_shell_st *shell)
{
	printf("I am trying to complete\r\n");
}

void nr_shell_get_char(nr_shell_st *sh, char c)
{
	write_to_console(sh->cons, c);
	sync_with_real_screen(sh->cons);
	if (c == '\n')
		shell_parser(sh);
	if (c == '\t')
		shell_complete_cmd(sh);
}

static void lf(vcons_st *cons)
{
	/* todo: scroll up effiently */
	if(cons->y >= cons->row_size - 1) {
		/* just creat a new page */
		cons->pos = cons->scr_mem_start;
		cons->y = 0;
		cons->x = 0;
	} else {
		cons->y++;
		cons->pos += cons->col_size - cons->x;
		cons->x = 0;
	}

	cons->need_wrap = 0;
}

void delete_char(vcons_st *cons, int del_p)
{
	uint16_t i = del_p;
	uint8_t *p = (uint8_t *)cons->pos;
	while(++i < cons->col_size) {
		*p = *(p+1);
		p++;
	}
}

static void bs(vcons_st *cons)
{
	int i;
	if (cons->x) {
		cons->pos--;
		cons->x--;	
	}
	cons->need_wrap = 0;
}


#define NR_SHELL_PRINT_BUF_SIZE 128

nr_shell_st *cur_shell;

void nr_printf(char *fmt, ...)
{
	int res = 0;
	int i;
	char out_buf[NR_SHELL_PRINT_BUF_SIZE];
	if(cur_shell == NULL || cur_shell->cons == NULL)
		return;
	va_list args;
	va_start(args, fmt);
	res = vsnprintf(out_buf, NR_SHELL_PRINT_BUF_SIZE, fmt, args);
	for(i = 0; i < res; i++)
	{
		write_to_console(cur_shell->cons, out_buf[i]);
	}
	va_end(args);
}

static void vt(vcons_st *cons)
{
	/* todo: complete the code */
}

vcons_ops_st shell_vcons_ops = 
{
	.lf = lf,
};

