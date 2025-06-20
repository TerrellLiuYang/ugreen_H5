#include "jlstream.h"
#include "app_config.h"
#include "audio_config.h"
#include "effects/effects_adj.h"
#include "frame_length_adaptive.h"
#include "audio_agc.h"


#if 1
#define agc_log	printf
#else
#define agc_log(...)
#endif/*log_en*/

#define AGC_FRAME_SIZE   128

#ifdef TCFG_AGC_NODE_ENABLE

struct agc_cfg_t {
    float AGC_max_lvl;          //最大幅度压制，range[0 : -90] dB
    float AGC_fade_in_step;     //淡入步进，range[0.1 : 5] dB
    float AGC_fade_out_step;    //淡出步进，range[0.1 : 5] dB
    float AGC_max_gain;         //放大上限, range[-90 : 40] dB
    float AGC_min_gain;         //放大下限, range[-90 : 40] dB
    float AGC_speech_thr;       //放大阈值, range[-70 : -40] dB
} __attribute__((packed));

struct agc_node_hdl {
    u16 sr;
    void *agc;
    struct stream_frame *out_frame;
    struct stream_node *node;		//节点句柄
    agc_param_t parm;
    struct frame_length_adaptive_hdl *olen_adaptive;//输出长度适配
};

int agc_param_cfg_read(struct stream_node *node)
{
    struct agc_cfg_t config;
    struct agc_node_hdl *hdl = (struct agc_node_hdl *)node->private_data;
    if (!hdl) {
        return -1 ;
    }
    /*
     *获取配置文件内的参数,及名字
     * */
    if (!jlstream_read_node_data_new(NODE_UUID_AGC, node->subid, (void *)&config, NULL)) {
        printf("%s, read node data err\n", __FUNCTION__);
        return -1 ;
    }

    memcpy(&hdl->parm, &config, sizeof(config));

    agc_log("AGC_max_lvl %d/10\n", (int)(hdl->parm.AGC_max_lvl * 10));
    agc_log("AGC_fade_in_step %d/10\n", (int)(hdl->parm.AGC_fade_in_step * 10));
    agc_log("AGC_fade_out_step %d/10\n", (int)(hdl->parm.AGC_fade_out_step * 10));
    agc_log("AGC_max_gain  %d/10\n", (int)(hdl->parm.AGC_max_gain * 10));
    agc_log("AGC_min_gain    %d/10\n", (int)(hdl->parm.AGC_min_gain * 10));
    agc_log("AGC_speech_thr     %d/10\n", (int)(hdl->parm.AGC_speech_thr * 10));

    return 0;
}

/*节点输出回调处理，可处理数据或post信号量*/
static void agc_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct agc_node_hdl *hdl = (struct agc_node_hdl *)iport->private_data;
    struct stream_node *node = iport->node;
    struct stream_frame *in_frame;
    int wlen;
    u8 trigger = 1;

    while (1) {
        in_frame = jlstream_pull_frame(iport, note);		//从iport读取数据
        if (!in_frame) {
            break;
        }

        if (!hdl->out_frame) {
            int out_frame_len = in_frame->len > (AGC_FRAME_SIZE << 1) ? in_frame->len : AGC_FRAME_SIZE << 1;
            hdl->out_frame = jlstream_get_frame(node->oport, out_frame_len);
            if (!hdl->out_frame) {
                return;
            }
        }
        wlen = audio_agc_run(hdl->agc, (s16 *)in_frame->data,
                             (s16 *)hdl->out_frame->data, in_frame->len);

        /*保证节点输入输出长度一样*/
        if (!hdl->olen_adaptive) {
            hdl->olen_adaptive = frame_length_adaptive_open(in_frame->len);
        }
        int len = frame_length_adaptive_run(hdl->olen_adaptive, (s16 *)hdl->out_frame->data, (s16 *)hdl->out_frame->data, wlen);
        if (len) {
            hdl->out_frame->len = len;
            jlstream_push_frame(node->oport, hdl->out_frame);	//将数据推到oport
            hdl->out_frame = NULL;
        }
        jlstream_free_frame(in_frame);	//释放iport资源
    }
}

/*节点预处理-在ioctl之前*/
static int agc_adapter_bind(struct stream_node *node, u16 uuid)
{
    struct agc_node_hdl *hdl = zalloc(sizeof(*hdl));

    hdl->node = node;
    node->private_data = hdl;	//保存私有信息
    agc_param_cfg_read(node);

    return 0;
}

/*打开改节点输入接口*/
static void agc_ioc_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = agc_handle_frame;				//注册输出回调
    iport->private_data = iport->node->private_data;	//保存节点私有句柄
}

/*节点参数协商*/
static int agc_ioc_negotiate(struct stream_iport *iport)
{
    return 0;
}

/*节点start函数*/
static void agc_ioc_start(struct agc_node_hdl *hdl)
{
    struct stream_fmt *fmt = &hdl->node->oport->fmt;
    hdl->parm.AGC_samplerate = fmt->sample_rate;
    hdl->parm.AGC_frame_size = AGC_FRAME_SIZE;
    agc_log("AGC_samplerate : %d", hdl->parm.AGC_samplerate);
    agc_log("AGC_frame_size : %d", hdl->parm.AGC_frame_size);
    hdl->agc = audio_agc_open(&hdl->parm);
}

/*节点stop函数*/
static void agc_ioc_stop(struct agc_node_hdl *hdl)
{
    if (hdl->agc) {
        audio_agc_close(hdl->agc);
        hdl->agc = NULL;
    }
    if (hdl->olen_adaptive) {
        frame_length_adaptive_close(hdl->olen_adaptive);
        hdl->olen_adaptive = NULL;
    }
    if (hdl->out_frame) {
        jlstream_free_frame(hdl->out_frame);
        hdl->out_frame = NULL;
    }
}


/*节点ioctl函数*/
static int agc_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct agc_node_hdl *hdl = (struct agc_node_hdl *)iport->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        agc_ioc_open_iport(iport);
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= agc_ioc_negotiate(iport);
        break;
    case NODE_IOC_START:
        agc_ioc_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        agc_ioc_stop(hdl);
        break;
    }

    return ret;
}

/*节点用完释放函数*/
static void agc_adapter_release(struct stream_node *node)
{
    struct agc_node_hdl *hdl = (struct agc_node_hdl *)node->private_data;
    if (!hdl) {
        return;
    }
    free(hdl);
}

/*节点adapter 注意需要在sdk_used_list声明，否则会被优化*/
REGISTER_STREAM_NODE_ADAPTER(agc_node_adapter) = {
    .name       = "agc",
    .uuid       = NODE_UUID_AGC,
    .bind       = agc_adapter_bind,
    .ioctl      = agc_adapter_ioctl,
    .release    = agc_adapter_release,
};

//注册工具在线调试
REGISTER_ONLINE_ADJUST_TARGET(agc_process) = {
    .uuid = NODE_UUID_AGC,
};

#endif /*TCFG_AGC_NODE_ENABLE*/

