#include "jlstream.h"
#include "media/audio_base.h"
#include "effects/effects_adj.h"
#include "app_config.h"


#ifdef TCFG_EFFECT_DEV3_NODE_ENABLE

#define LOG_TAG_CONST EFFECTS
#define LOG_TAG     "[EFFECT_dev3-NODE]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma   code_seg(".effect_dev3.text")
#pragma   data_seg(".effect_dev3.data")
#pragma  const_seg(".effect_dev3.text.const")
#endif


struct effect_dev3_node_hdl {
    char name[16];
    struct stream_node *node;	//节点句柄
    void *effect_dev3;
    u32 sample_rate;
    u8 ch_num;
};

/* 自定义算法，初始化
 * sample_rate:采样率
 * ch_num:通道数，单声道 1，立体声 2, 四声道 4
 **/
static void audio_effect_dev3_init(u32 sample_rate, u8 ch_num)
{
    //do something
}

/* 自定义算法，运行
 * sample_rate:采样率
 * ch_num:通道数，单声道 1，立体声 2, 四声道 4
 * *data:输入输出数据同地址,位宽16bit
 * data_len :输入数据长度,byte
 * */
static void audio_effect_dev3_run(u32 sample_rate, u8 ch_num,  s16 *data, u32 data_len)
{
    //do something
    /* printf("effect dev3 do something here\n"); */
}
/* 自定义算法，关闭
 **/
static void audio_effect_dev3_exit()
{
    //do something

}
/*节点输出回调处理，可处理数据或post信号量*/
static void effect_dev3_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct stream_frame *in_frame, *out_frame;

    struct stream_frame *frame;
    struct effect_dev3_node_hdl *hdl = (struct effect_dev3_node_hdl *)iport->private_data;
    struct stream_node *node = iport->node;

    while (1) {
        frame = jlstream_pull_frame(iport, note);		//从iport读取数据
        if (!frame) {
            break;
        }
        audio_effect_dev3_run(hdl->sample_rate, hdl->ch_num, (s16 *)frame->data, frame->len);

        if (node->oport) {
            jlstream_push_frame(node->oport, frame);	//将数据推到oport
        } else {
            jlstream_free_frame(frame);	//释放iport资源
        }
    }
}

/*节点预处理-在ioctl之前*/
static int effect_dev3_adapter_bind(struct stream_node *node, u16 uuid)
{
    struct effect_dev3_node_hdl *hdl = malloc(sizeof(*hdl));

    memset(hdl, 0, sizeof(*hdl));

    hdl->node = node;
    node->private_data = hdl;	//保存私有信息

    return 0;
}

/*打开改节点输入接口*/
static void effect_dev3_ioc_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = effect_dev3_handle_frame;				//注册输出回调
    iport->private_data = iport->node->private_data;	//保存节点私有句柄
}

/*节点参数协商*/
static int effect_dev3_ioc_negotiate(struct stream_iport *iport)
{
    struct stream_oport *oport = iport->node->oport;
    struct stream_fmt *in_fmt = &iport->prev->fmt;
    struct effect_dev3_node_hdl *hdl = (struct effect_dev3_node_hdl *)iport->private_data;

    return 0;
}

/*节点start函数*/
static void effect_dev3_ioc_start(struct effect_dev3_node_hdl *hdl)
{
    struct stream_fmt *fmt = &hdl->node->oport->fmt;
    struct jlstream *stream = jlstream_for_node(hdl->node);


    hdl->sample_rate = fmt->sample_rate;
    hdl->ch_num = AUDIO_CH_NUM(fmt->channel_mode);
    audio_effect_dev3_init(hdl->sample_rate, hdl->ch_num);
}


/*节点stop函数*/
static void effect_dev3_ioc_stop(struct effect_dev3_node_hdl *hdl)
{
    audio_effect_dev3_exit();
}

static int effect_dev3_ioc_update_parm(struct effect_dev3_node_hdl *hdl, int parm)
{
    int ret = false;
    return ret;
}
static int get_effect_dev3_ioc_parm(struct effect_dev3_node_hdl *hdl, int parm)
{
    int ret = 0;
    return ret;
}

/*节点ioctl函数*/
static int effect_dev3_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct effect_dev3_node_hdl *hdl = (struct effect_dev3_node_hdl *)iport->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        effect_dev3_ioc_open_iport(iport);
        break;
    case NODE_IOC_OPEN_OPORT:
        break;
    case NODE_IOC_CLOSE_IPORT:
        break;
    case NODE_IOC_SET_SCENE:
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= effect_dev3_ioc_negotiate(iport);
        break;
    case NODE_IOC_START:
        effect_dev3_ioc_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        effect_dev3_ioc_stop(hdl);
        break;
    }

    return ret;
}

/*节点用完释放函数*/
static void effect_dev3_adapter_release(struct stream_node *node)
{
    free(node->private_data);
}

/*节点adapter 注意需要在sdk_used_list声明，否则会被优化*/
REGISTER_STREAM_NODE_ADAPTER(effect_dev3_node_adapter) = {
    .name       = "effect_dev3",
    .uuid       = NODE_UUID_EFFECT_DEV3,
    .bind       = effect_dev3_adapter_bind,
    .ioctl      = effect_dev3_adapter_ioctl,
    .release    = effect_dev3_adapter_release,
};

/* REGISTER_ONLINE_ADJUST_TARGET(effect_dev3) = { */
/* .uuid = NODE_UUID_effect_dev3, */
/* }; */

#endif

