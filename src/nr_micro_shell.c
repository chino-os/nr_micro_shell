#include <stdint.h>
#include <stdarg.h>
#include "nr_micro_shell.h"

#define NR_SHELL_CMD_LINE_MAX_LENGTH 128

#define pr_db(fmt, ...) printf(fmt"\r\n", ##__VA_ARGS__)

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
		pr_db("res %d, i %d",res, i);
		write_to_console(cur_shell->cons, out_buf[i]);
	}
	va_end(args);
}

void shell_parser(nr_shell_st *shell)
{
	/* todo: parser the last cmd line. */
	char cur_cmd[NR_SHELL_CMD_LINE_MAX_LENGTH];

	printf("I am working\r\n");
	nr_printf("--------\r\n");
	nr_printf("%s:", shell->user_name);
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
		cons->need_wrap = 0;
	} else {
		cons->y++;
		cons->pos += cons->col_size - cons->x;
		cons->x = 0;
	}
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

static void bell(vcons_st *cons)
{
	pr_db("bell");
	dump_src_mem(cons);
}

static void csi_A(vcons_st *cons, int d)
{
	pr_db("up");
}

static void csi_B(vcons_st *cons, int d)
{
	pr_db("down");
}

static void csi_C(vcons_st *cons, int d)
{
	pr_db("front");
}

static void csi_D(vcons_st *cons, int d)
{
	pr_db("back");
}

static void vt(vcons_st *cons)
{
	/* todo: complete the code */
}

vcons_ops_st shell_vcons_ops = 
{
	.lf = lf,
	.bell = bell,
};
#define NR_MICRO_SHELL_VERSION "2.0.0"
void nr_shell_init(nr_shell_st *shell)
{
	nr_printf(" _   _ ____    __  __ _                  ____  _          _ _ \r\n");
	nr_printf("| \\ | |  _ \\  |  \\/  (_) ___ _ __ ___   / ___|| |__   ___| | |\r\n");
	nr_printf("|  \\| | |_) | | |\\/| | |/ __| '__/ _ \\  \\___ \\| '_ \\ / _ \\ | |\r\n");
	nr_printf("| |\\  |  _ <  | |  | | | (__| | | (_) |  ___) | | | |  __/ | |\r\n");
	nr_printf("|_| \\_|_| \\_\\ |_|  |_|_|\\___|_|  \\___/  |____/|_| |_|\\___|_|_|\r\n");
	nr_printf("                                                              \r\n");
	nr_printf("author: Ji Youzhou - jyz_nrush@163.com.\r\n");
	nr_printf("version: %s\r\n", NR_MICRO_SHELL_VERSION);
	nr_printf("build info: %s %s\r\n", __DATE__, __TIME__);
	nr_printf("%s:", shell->user_name);

}

