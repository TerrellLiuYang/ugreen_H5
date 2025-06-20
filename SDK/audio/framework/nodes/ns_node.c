
#include "jlstream.h"
#include "audio_config.h"
#include "media/audio_base.h"
#include "audio_ns.h"
#include "btstack/avctp_user.h"
#include "effects/effects_adj.h"
#include "frame_length_adaptive.h"
#include "app_config.h"

#if 0
#define ns_log	printf
#else
#define ns_log(...)
#endif/*log_en*/

#ifdef TCFG_NS_NODE_ENABLE

enum {
    AUDIO_NS_TYPE_ESCO_DL = 1,	//下行降噪类型
    AUDIO_NS_TYPE_GENERAL,		//通用类型
};

struct ns_cfg_t {
    u8 ns_type;
    u8 call_active_trigger;//接通电话触发标志, 只有通话下行降噪会使用
    u8 mode;               //降噪模式(0,1,2:越大越耗资源，效果越好)
    float aggressfactor;   //降噪强度(越大越强:1~2)
    float minsuppress;     //降噪最小压制(越小越强:0~1)
    float noiselevel;      //初始噪声水平(评估初始噪声，加快收敛)
} __attribute__((packed));

struct ns_node_hdl {
    u16 sr;
    void *ns;
    struct stream_frame *out_frame;
    struct stream_node *node;		//节点句柄
    struct ns_cfg_t cfg;
    struct frame_length_adaptive_hdl *olen_adaptive;//输出长度适配
};

extern int db2mag(int db, int dbQ, int magDQ);//10^db/20
int ns_param_cfg_read(struct stream_node *node)
{
    struct ns_cfg_t config;
    struct ns_node_hdl *hdl = (struct ns_node_hdl *)node->private_data;
    if (!hdl) {
        return -1 ;
    }
    /*
     *获取配置文件内的参数,及名字
     * */
    if (!jlstream_read_node_data_new(NODE_UUID_NOISE_SUPPRESSOR, node->subid, (void *)&config, NULL)) {
        printf("%s, read node data err\n", __FUNCTION__);
        return -1 ;
    }

    hdl->cfg = config;

    ns_log("type %d\n", hdl->cfg.ns_type);
    ns_log("call_active_trigger %d\n", hdl->cfg.call_active_trigger);
    ns_log("mode                %d\n", hdl->cfg.mode);
    ns_log("aggressfactor  %d/1000\n", (int)(hdl->cfg.aggressfactor * 1000.f));
    ns_log("minsuppress    %d/1000\n", (int)(hdl->cfg.minsuppress * 1000.f));
    ns_log("noiselevel     %d/1000\n", (int)(hdl->cfg.noiselevel * 1000.f));

    return 0;
}

/*节点输出回调处理，可处理数据或post信号量*/
static void ns_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct ns_node_hdl *hdl = (struct ns_node_hdl *)iport->private_data;
    struct stream_node *node = iport->node;
    struct stream_frame *in_frame;
    int wlen;
    u8 trigger = 1;

    while (1) {
        in_frame = jlstream_pull_frame(iport, note);		//从iport读取数据
        if (!in_frame) {
            break;
        }

        if (hdl->cfg.ns_type == AUDIO_NS_TYPE_ESCO_DL) {
            if (hdl->cfg.call_active_trigger) {
                if (bt_get_call_status() != BT_CALL_ACTIVE) {
                    trigger = 0;
                }
            }
        }
        if (trigger) {
            /*接通的时候再开始做降噪*/
            if (!hdl->out_frame) {
                hdl->out_frame = jlstream_get_frame(node->oport, NS_OUT_POINTS_MAX << 1);
                if (!hdl->out_frame) {
                    return;
                }
            }
            wlen = audio_ns_run(hdl->ns, (s16 *)in_frame->data,
                                (s16 *)hdl->out_frame->data, in_frame->len);

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
        } else {
            jlstream_push_frame(node->oport, in_frame);	//将数据推到oport
        }
    }
}

/*节点预处理-在ioctl之前*/
static int ns_adapter_bind(struct stream_node *node, u16 uuid)
{
    struct ns_node_hdl *hdl = malloc(sizeof(*hdl));

    memset(hdl, 0, sizeof(*hdl));

    hdl->node = node;
    node->private_data = hdl;	//保存私有信息
    ns_param_cfg_read(node);

    return 0;
}

/*打开改节点输入接口*/
static void ns_ioc_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = ns_handle_frame;				//注册输出回调
    iport->private_data = iport->node->private_data;	//保存节点私有句柄
}

/*节点参数协商*/
static int ns_ioc_negotiate(struct stream_iport *iport)
{
    return 0;
}

/*节点start函数*/
static void ns_ioc_start(struct ns_node_hdl *hdl)
{
    struct stream_fmt *fmt = &hdl->node->oport->fmt;
    hdl->ns = audio_ns_open(fmt->sample_rate, hdl->cfg.mode, hdl->cfg.noiselevel, hdl->cfg.aggressfactor, hdl->cfg.minsuppress);
}

/*节点stop函数*/
static void ns_ioc_stop(struct ns_node_hdl *hdl)
{
    if (hdl->ns) {
        audio_ns_close(hdl->ns);
        hdl->ns = NULL;
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
static int ns_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct ns_node_hdl *hdl = (struct ns_node_hdl *)iport->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        ns_ioc_open_iport(iport);
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= ns_ioc_negotiate(iport);
        break;
    case NODE_IOC_START:
        ns_ioc_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        ns_ioc_stop(hdl);
        break;
    }

    return ret;
}

/*节点用完释放函数*/
static void ns_adapter_release(struct stream_node *node)
{
    struct ns_node_hdl *hdl = (struct ns_node_hdl *)node->private_data;
    if (!hdl) {
        return;
    }
    free(hdl);
}

/*节点adapter 注意需要在sdk_used_list声明，否则会被优化*/
REGISTER_STREAM_NODE_ADAPTER(ns_node_adapter) = {
    .name       = "ns",
    .uuid       = NODE_UUID_NOISE_SUPPRESSOR,
    .bind       = ns_adapter_bind,
    .ioctl      = ns_adapter_ioctl,
    .release    = ns_adapter_release,
};

//注册工具在线调试
REGISTER_ONLINE_ADJUST_TARGET(noise_suppressor) = {
    .uuid = NODE_UUID_NOISE_SUPPRESSOR,
};

#endif/* TCFG_AUDIO_NS_ENABLE*/

