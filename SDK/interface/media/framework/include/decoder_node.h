#ifndef DECODER_NODE_MGR_H
#define DECODER_NODE_MGR_H

#include "jlstream.h"
#include "media/audio_base.h"
#include "media/audio_splicing.h"
#include "system/task.h"

struct decoder_hdl;

struct decoder_file_ops {
    void *file;
    int (*fread)(void *file, u8 *buf, u32 fpos, int len);
};

struct decoder_get_fmt {
    struct decoder_file_ops fops;
    struct stream_fmt *fmt;
};


struct decoder_plug_ops {
    int coding_type;
    void *(*init)(struct decoder_hdl *dec_hdl);
    int (*run)(void *);
    int (*ioctl)(void *, int, int);
    void (*release)(void *);
};

struct decoder_hdl {
    struct stream_node *node;
    void *decoder;
    int (*run)(void *);
    enum stream_scene scene;
    u8 start;
    u8 subpackage;
    u8 output_frame_cnt;
    u8 timestamp_flag;
    u8 pause;
    u8 no_data;
    u8 channel_mode;
    u16 coding_type;
    u16 frame_min_len;
    u16 frame_max_len;

    u32 cur_time;
    u32 timestamp;
    u32 file_len;  //解码文件的长度
    u32 fpos;
    struct decoder_file_ops fops;

    OS_MUTEX mutex;
    struct jlstream_fade fade;
    struct stream_frame *frame;
    struct audio_dec_breakpoint *breakpoint;
    const char *task_name;
    const struct decoder_plug_ops *plug;
};

#define REGISTER_DECODER_PLUG(plug) \
    const struct decoder_plug_ops plug sec(.decoder_plug)

int decoder_plug_output_data(void *_hdl, u8 *data, u16 len, u8 channel_mode, void *priv);

int decoder_plug_read_data(void *_hdl, u32 fpos, u8 *data, u16 len);

int decoder_plug_get_data_len(void *_hdl);

struct stream_frame *decoder_plug_pull_frame(void *_hdl);

void decoder_plug_free_frame(void *_hdl, struct stream_frame *frame);

#endif

