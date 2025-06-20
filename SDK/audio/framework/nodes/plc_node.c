#include "jlstream.h"
#include "audio_plc.h"
#include "overlay_code.h"
#include "tech_lib/LFaudio_plc_api.h"
#include "app_config.h"

#ifdef TCFG_PLC_NODE_ENABLE

extern int tws_api_get_low_latency_state();
struct music_plc {
    LFaudio_PLC_API *plc_ops;
    void *plc_mem;
};

struct music_plc *music_plc_open(u8 ch_num)
{
    struct music_plc *plc = zalloc(sizeof(struct music_plc));
    if (plc) {
        int plc_mem_size;
        plc->plc_ops = get_lfaudioPLC_api();
        plc_mem_size = plc->plc_ops->need_buf(ch_num); // 3660bytes，请衡量是否使用该空间换取PLC处理
        plc->plc_mem = malloc(plc_mem_size);
        if (!plc->plc_mem) {
            plc->plc_ops = NULL;
            free(plc);
            return NULL;
        }
        plc->plc_ops->open(plc->plc_mem, ch_num, tws_api_get_low_latency_state() ? 4 : 0); //4是延时最低 16个点
    }
    return plc;
}


void music_plc_run(struct music_plc *plc, s16 *data, u16 len, u8 repair)
{
    if (plc && plc->plc_ops) {
        plc->plc_ops->run(plc->plc_mem, data, data, len >> 1, repair ? 2 : 0);
    }
}

void music_plc_close(struct music_plc *plc)
{
    if (plc) {
        if (plc->plc_mem) {
            free(plc->plc_mem);
            plc->plc_mem = NULL;
        }
        free(plc);
    }
}

struct plc_node_hdl {
    u8 start;
    u8 channel_num;
    enum stream_scene scene;	//当前场景
    struct music_plc *plc;
};

static void plc_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct stream_frame *frame;
    struct stream_node *node = iport->node;
    struct plc_node_hdl *hdl = (struct plc_node_hdl *)iport->private_data;
    u8 repair_flag = 0;
    while (1) {
        frame = jlstream_pull_frame(iport, note);
        if (!frame) {
            break;
        }
        if ((frame->flags & (FRAME_FLAG_FILL_PACKET & ~FRAME_FLAG_FLUSH_OUT)) == (FRAME_FLAG_FILL_PACKET & ~FRAME_FLAG_FLUSH_OUT)) {
            repair_flag = 1;
            //putchar('Q');
        }
        if (hdl->scene == STREAM_SCENE_A2DP) {
            music_plc_run(hdl->plc, (s16 *)frame->data, frame->len, repair_flag);
        } else {
            audio_plc_run((s16 *)frame->data, frame->len, repair_flag);
        }
        repair_flag = 0;
        if (node->oport) {
            jlstream_push_frame(node->oport, frame);
        } else {
            jlstream_free_frame(frame);
        }
    }
}

/*节点预处理-在ioctl之前*/
static int plc_adapter_bind(struct stream_node *node, u16 uuid)
{
    struct plc_node_hdl *hdl = malloc(sizeof(*hdl));
    memset(hdl, 0, sizeof(*hdl));
    node->private_data = hdl;
    return 0;
}

/*打开改节点输入接口*/
static void plc_ioc_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = plc_handle_frame;
    iport->private_data = iport->node->private_data;
}

/*节点参数协商*/
static int plc_ioc_negotiate(struct stream_iport *iport)
{
    return 0;
}

/*节点start函数*/
static void plc_ioc_start(u32 sr)
{
    //JLstream decoder里打开overlay_aec
    /* overlay_load_code(OVERLAY_AEC); */
    audio_plc_open(sr);
}

/*节点stop函数*/
static void plc_ioc_stop(void)
{
    audio_plc_close();
}

/*节点ioctl函数*/
static int plc_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct plc_node_hdl *hdl = (struct plc_node_hdl *)iport->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        plc_ioc_open_iport(iport);
        break;
    case NODE_IOC_SET_SCENE:
        hdl->scene = (enum stream_scene)arg;
        break;

    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= plc_ioc_negotiate(iport);
        break;
    case NODE_IOC_START:
        hdl->start = 1;
        if (hdl->scene == STREAM_SCENE_A2DP) {
            struct stream_fmt *in_fmt = &iport->prev->fmt;
            hdl->plc = music_plc_open(AUDIO_CH_NUM(in_fmt->channel_mode));
        } else {
            plc_ioc_start(iport->prev->fmt.sample_rate);
        }
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        if (hdl->start) {
            hdl->start = 0;
            if (hdl->scene == STREAM_SCENE_A2DP) {
                if (hdl->plc) {
                    music_plc_close(hdl->plc);
                    hdl->plc = NULL;
                }
            } else {
                plc_ioc_stop();
            }
        }
        break;
    }
    return ret;
}

/*节点用完释放函数*/
static void plc_adapter_release(struct stream_node *node)
{
    free(node->private_data);
}

/*节点adapter 注意需要在sdk_used_list声明，否则会被优化*/
REGISTER_STREAM_NODE_ADAPTER(plc_node_adapter) = {
    .name       = "plc",
    .uuid       = NODE_UUID_PLC,
    .bind       = plc_adapter_bind,
    .ioctl      = plc_adapter_ioctl,
    .release    = plc_adapter_release,
};

#endif

