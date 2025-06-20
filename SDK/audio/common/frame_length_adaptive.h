#ifndef _FRAME_LENGTH_ADAPTIVE_H_
#define _FRAME_LENGTH_ADAPTIVE_H_

#include "system/includes.h"
#include "circular_buf.h"

struct frame_length_adaptive_hdl {

    u16 adj_len;
    u8 *sw_buf;  //长度转换buf
    cbuffer_t sw_cbuf;
};

struct frame_length_adaptive_hdl *frame_length_adaptive_open(u16 adj_len);//adj_len是调整后 需要输出的数据长度；

int frame_length_adaptive_run(void *_hdl, void *in_data, void *out_data, u16 in_len);

void frame_length_adaptive_close(void *_hdl);


#endif/*FRAME_LENGTH_ADAPTIVE_H_*/

