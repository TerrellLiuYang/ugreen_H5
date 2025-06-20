#ifndef ENCODER_NODE_MGR_H
#define ENCODER_NODE_MGR_H

#include "jlstream.h"
#include "media/audio_base.h"

struct encoder_plug_ops {
    int coding_type;
    void *(*init)(void *priv);
    int (*run)(void *);
    int (*ioctl)(void *, int, int);
    void (*release)(void *);
};

struct encoder_fmt {
    u8 quality;
    u8 complexity;
    u8 sw_hw_option;
    u8 ch_num;
    u32 bit_rate;
    u32 sample_rate;
};

#define REGISTER_ENCODER_PLUG(plug) \
    const struct encoder_plug_ops plug sec(.encoder_plug)

int encoder_plug_output_data(void *_hdl, u8 *data, u16 len);

int encoder_plug_read_data(void *_hdl, u8 *data, u16 len);

struct stream_frame *encoder_plug_pull_frame(void *_hdl);

struct stream_frame *encoder_plug_get_output_frame(void *_hdl, u16);

void encoder_plug_put_output_frame(void *_hdl, struct stream_frame *);

void encoder_plug_free_frame(void *_hdl, struct stream_frame *);

#endif

