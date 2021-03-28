#include <stdint.h>
#include "nr_micro_shell.h"

/* ref form Linus Torvalds */

#define NUL 0x00
#define SOH 0x01
#define STX 0x02
#define ETX 0x03
#define EOT 0x04
#define ENQ 0x05
#define ACK 0x06
#define BEL 0x07
#define BS 0x08
#define HT 0x09
#define LF 0x0A
#define VT 0x0B
#define FF 0x0C
#define CR 0x0D
#define SO 0x0E
#define SI 0x0F
#define DLE 0x10
#define DC1 0x11
#define DC2 0x12
#define DC3 0x13
#define DC4 0x14
#define NAK 0x15
#define SYN 0x16
#define ETB 0x17
#define CAN 0x18
#define EM 0x19
#define SUB 0x1A
#define ESC 0x1B
#define FS 0x1C
#define GS 0x1D
#define RS 0x1E
#define US 0x1F
#define DEL 0x7F
#define CSI 0x9B

const char vt100[] = {
	"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
	"\0\0\0\0\0\0\0\0\0\0\376\0\0\0\0\0"
	" !\"#$%&'()*+,-./0123456789:;<=>?"
	"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
	"`abcdefghijklmnopqrstuvwxyz{|}~\0"
	"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
	"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
	"\377\255\233\234\376\235\174\025\376\376\246\256\252\055\376\376"
	"\370\361\375\376\376\346\024\371\376\376\247\257\254\253\376\250"
	"\376\376\376\376\216\217\222\200\376\220\376\376\376\376\376\376"
	"\376\245\376\376\376\376\231\376\350\376\376\376\232\376\376\341"
	"\205\240\203\376\204\206\221\207\212\202\210\211\215\241\214\213"
	"\376\244\225\242\223\376\224\366\355\227\243\226\201\376\376\230",
};

static void cons_put_chars(v_cons_st *cons, char *str, int size)
{
	int i = 0;
	for (i = 0; i < size; i++) {
		nr_shell_putc(*(str + i), cons->id);
	}
}

static void cons_put_str(v_cons_st *cons, char *str)
{
	int i = 0;
	for (i = 0; str[i] != '\0'; i++)
		nr_shell_putc(str[i], cons->id);
}

static void screen_clean(v_cons_st *cons, int x)
{
	cons->pos = cons->scr_mem_start;
	cons->x = 0;
	cons->y = 0;
}

static void screen_new(v_cons_st *cons, int x)
{
}

static void screen_up(v_cons_st *cons, int x)
{
}

static void screen_down(v_cons_st *cons, int x)
{
}

/* CR move the cursor to the left */
static void cr(v_cons_st *cons)
{
	cons->pos -= cons->x;
	cons->x = 0;
	cons->need_wrap = 0;
}

/* LF move the cursor to next line */
static void lf(v_cons_st *cons)
{
	if (cons->y + 1 < cons->row_size) {
		cons->y++;
		cons->pos += cons->row_size;
	} else {
		screen_up(cons, 1);
		cons->need_wrap = 0;
	}
}

/* BS move the cursor forward one space */
static void bs(v_cons_st *cons)
{
	if (cons->x) {
		cons->pos--;
		cons->x--;
		cons->need_wrap = 0;
	}
}

static void del(v_cons_st *cons)
{
	if (cons->x) {
	}
}

static void insert_char(v_cons_st *cons)
{
}

static void gotoxy(v_cons_st *cons, int x, int y)
{
	x = x < 0 ? 0 : x;
	x = x >= cons->col_size ? cons->col_size - 1 : x;

	y = y < 0 ? 0 : y;
	y = y >= cons->row_size ? cons->row_size - 1 : x;

}

static void csi_J(v_cons_st *cons, int par)
{
}

static void csi_K(v_cons_st *cons, int par)
{
}

void dump_src_mem(v_cons_st *cons)
{
	int i = 0;
	int j = 0;
	int k = 0;
	printf("\r\n");
	printf("\r\n===========================================================================================\r\n");

	for (i = 0; i < cons->y; i++) {
		for (j = 0; j < cons->col_size; j++) {
			nr_shell_putc(cons->scr_mem_start[k], cons->id);
			k++;
		}
		printf("\r\n");
	}

	for (j = 0; j <= cons->x; j++) {
		nr_shell_putc(cons->scr_mem_start[k], cons->id);
		k++;
	}
	printf("\r\n===========================================================================================\r\n");
	printf("x:%d, y:%d\n", cons->x, cons->y);
	printf("\r\n");
	fflush(stdout);
}

__weak void bell(void)
{
}

void write_to_console(v_cons_st *cons, uint8_t c)
{
	if (cons->en == CONS_DISABLE)
		return;

	if (cons->state == ESnormal && cons->trans_table[c]) {
		if (cons->insert)
			insert_char(cons);
		c = cons->trans_table[c];
		*cons->pos = c;
		if (cons->x == cons->col_size)
			cons->need_wrap = cons->auto_wrap;
		else {
			cons->x++;
			cons->pos++;
		}
		return;
	}

	switch (c) {
	case BEL:
		bell();
		return;
	case BS:
		bs(cons);
		return;
	case LF:
		lf(cons);
		dump_src_mem(cons);
		return;
	case CR:
		cr(cons);
		return;
	case CAN:
	case SUB:
		cons->state = ESnormal;
		return;
	case ESC:
		cons->state = ESesc;
		return;
	case DEL:
		del(cons);
		return;
	case CSI:
		cons->state = ESsquare;
		return;
	}

	switch (cons->state) {
	case ESesc:
		cons->state = ESnormal;
		switch (c) {
		case '[':
			cons->state = ESsquare;
			return;
		}
		return;
	case ESsquare:
		for (cons->npara = 0; cons->npara < MAX_NR_CSI_PARA;
		     cons->npara++)
			cons->para[cons->npara] = 0;
		cons->npara = 0;
		cons->state = ESgetpars;
	case ESgetpars:
		if (c == ';' && cons->npara < MAX_NR_CSI_PARA - 1) {
			cons->npara++;
			return;
		} else if (c >= '0' && c <= '9') {
			cons->para[cons->npara] *= 10;
			cons->para[cons->npara] += c - '0';
			return;
		} else
			cons->state = ESgotpars;
	case ESgotpars:
		switch (c) {
		case 'G':
		case '`':
			if (cons->para[0])
				cons->para[0]--;
			gotoxy(cons, cons->para[0], cons->y);
			return;
		case 'A':
			if (!cons->para[0])
				cons->para[0]++;
			gotoxy(cons, cons->x, cons->y - cons->para[0]);
			return;
		case 'B':
		case 'e':
			if (!cons->para[0])
				cons->para[0]++;
			gotoxy(cons, cons->x, cons->y + cons->para[0]);
			return;
		case 'C':
		case 'a':
			if (!cons->para[0])
				cons->para[0]++;
			gotoxy(cons, cons->x + cons->para[0], cons->y);
			return;
		case 'D':
			if (!cons->para[0])
				cons->para[0]++;
			gotoxy(cons, cons->x - cons->para[0], cons->y);
			return;
		case 'E':
			if (!cons->para[0])
				cons->para[0]++;
			gotoxy(cons, 0, cons->y + cons->para[0]);
			return;
		case 'F':
			if (!cons->para[0])
				cons->para[0]++;
			gotoxy(cons, 0, cons->y - cons->para[0]);
			return;
		case 'd':
			if (cons->para[0])
				cons->para[0]--;
			gotoxy(cons, cons->x, cons->para[0]);
			return;
		case 'H':
		case 'f':
			if (cons->para[0])
				cons->para[0]--;
			if (cons->para[1])
				cons->para[1]--;
			gotoxy(cons, cons->para[1], cons->para[0]);
			return;
		case 'J':
			csi_J(cons, cons->para[0]);
			return;
		case 'K':
			csi_K(cons, cons->para[0]);
			return;
		}
		return;
	default:
		cons->state = ESnormal;
	}
}