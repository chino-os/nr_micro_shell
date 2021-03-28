/**
 * @file      nr_micro_shell.h
 * @author    Ji Youzhou
 * @version   V2.0
 * @date      28 Mar 2021
 * @brief     [brief]
 * *****************************************************************************
 * @attention
 * 
 * MIT License
 * 
 * Copyright (C) 2021 Ji Youzhou. or its affiliates.  All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __NR_MICRO_SHELL_H
#define __NR_MICRO_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include "nr_micro_shell_config.h"

#define MAX_NR_CSI_PARA 16

/* escape sequence mode */
enum {
	ESnormal,
	ESesc,
	ESsquare,
	ESgetpars,
	ESgotpars,
	ESfunckey,
	EShash,
	ESsetG0,
	ESsetG1,
	ESignore
};

enum { CONS_ENABLE = 1, CONS_DISABLE = 0 };

typedef struct scr_ops_struct {
	void (*gotoxy)(uint8_t id, int x, int y);
}scr_ops_st;

typedef struct virtual_console_struct {
	uint32_t x;
	uint32_t y;
	uint32_t end_x;
	uint32_t end_y;
	uint8_t *pos;

	uint8_t *scr_mem_start;

	uint8_t row_size;
	uint8_t col_size;

	const uint8_t *trans_table;

	uint8_t state;
	uint8_t npara;
	uint16_t para[MAX_NR_CSI_PARA];

	uint32_t en : 1;
	uint32_t echo_en : 1;
	uint32_t need_wrap : 1;
	uint32_t auto_wrap : 1;
	uint32_t insert : 1;

	scr_ops_st* src_ops;

	uint8_t id;
} v_cons_st;

typedef struct nr_shell_struct
{
	v_cons_st *cons;
}nr_shell_st;

void write_to_console(v_cons_st *cons, uint8_t c);
void dump_src_mem(v_cons_st *cons);

extern const char vt100[];

#define VCONS_DEFAULT_COLS 80
#define VCONS_DEFAULT_ROWS 80
#define  DECLARE_AND_DEFAULT_NR_SHELL(name)\
uint8_t name##_scr_buf[VCONS_DEFAULT_COLS*VCONS_DEFAULT_ROWS];\
v_cons_st name##_vcons = {\
	.x = 0,\
	.y = 0,\
	.pos = name##_scr_buf,\
	.scr_mem_start = name##_scr_buf,\
	.row_size = VCONS_DEFAULT_ROWS,\
	.col_size = VCONS_DEFAULT_COLS,\
	.trans_table = vt100,\
	.state = ESnormal,\
	.en = CONS_ENABLE,\
	.id = 0,\
};\
nr_shell_st name = {\
	.cons = &name##_vcons,\
}

#ifdef __cplusplus
}
#endif

#endif
/******************* (C) COPYRIGHT 2021 Ji Youzhou *****END OF FILE*****************/