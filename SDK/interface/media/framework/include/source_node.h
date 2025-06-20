#ifndef SOURCE_NODE_MGR_H
#define SOURCE_NODE_MGR_H

#include "jlstream.h"
#include "media/audio_base.h"

enum {
    SRC_NODE_ERR_FILE_END = 1,
};


struct source_node_plug {
    u16 uuid;

    u16 frame_size;

    void *(*init)(void *, struct stream_node *node);

    enum stream_node_state(*read)(void *, struct stream_frame *);

    enum stream_node_state(*seek)(void *, u32 pos);

    enum stream_node_state(*get_frame)(void *, struct stream_frame **);

    int (*ioctl)(void *, int cmd, int arg);

    void (*release)(void *);
};



#define REGISTER_SOURCE_NODE_PLUG(plug) \
    const struct source_node_plug plug sec(.source_node_plug)

struct stream_frame *source_plug_get_output_frame(void *, int len);

void source_plug_put_output_frame(void *, struct stream_frame *);

void source_plug_free_frame(void *, struct stream_frame *);

#endif
