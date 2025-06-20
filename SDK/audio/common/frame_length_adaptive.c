#include "frame_length_adaptive.h"


int frame_length_adaptive_run(void *_hdl, void *in_data, void *out_data, u16 in_len)
{
    struct frame_length_adaptive_hdl *hdl = (struct frame_length_adaptive_hdl *)_hdl;
    if (!hdl) {
        return 0;
    }
    int	wlen, rlen = 0;
    wlen = cbuf_write(&hdl->sw_cbuf, in_data, in_len);
    if (wlen != in_len) {
        putchar('w');
    }
    if (cbuf_get_data_len(&hdl->sw_cbuf) >= hdl->adj_len) {
        rlen = cbuf_read(&hdl->sw_cbuf, out_data, hdl->adj_len);
        if (rlen != hdl->adj_len) {
            putchar('W');
        }
    }
    return rlen;
}


struct frame_length_adaptive_hdl *frame_length_adaptive_open(u16 adj_len)
{
    struct frame_length_adaptive_hdl *hdl = zalloc(sizeof(struct frame_length_adaptive_hdl));
    if (!hdl) {
        return NULL;
    }
    printf("frame_length_adaptive open---------adj_len = %d\n", adj_len);
    hdl->adj_len = adj_len;
    hdl->sw_buf = zalloc(adj_len * 2);
    cbuf_init(&hdl->sw_cbuf, hdl->sw_buf, adj_len * 2);

    u32 len;
    void *obuf = cbuf_write_alloc(&hdl->sw_cbuf, &len); //先填一帧的数据,
    memset(obuf, 0, adj_len);
    cbuf_write_updata(&hdl->sw_cbuf, adj_len);

    return hdl;
}

void frame_length_adaptive_close(void *_hdl)
{
    struct frame_length_adaptive_hdl *hdl = (struct frame_length_adaptive_hdl *)_hdl;
    if (hdl) {
        if (hdl->sw_buf) {
            free(hdl->sw_buf);
        }
        free(hdl);
    }
}


