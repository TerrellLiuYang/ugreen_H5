#include "jlstream.h"
#include "esco_recoder.h"
#include "encoder_node.h"
#include "sdk_config.h"
#include "media/includes.h"
#include "app_config.h"


#if TCFG_AUDIO_DUT_ENABLE
#include "audio_dut_control.h"
#endif

struct esco_recoder {
    struct jlstream *stream;
};

static struct esco_recoder *g_esco_recoder = NULL;


static void esco_recoder_callback(void *private_data, int event)
{
    struct esco_recoder *recoder = g_esco_recoder;
    struct jlstream *stream = (struct jlstream *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
        break;
    }
}

int esco_recoder_open(u8 link_type)
{
    int err;
    struct encoder_fmt enc_fmt;
    struct esco_recoder *recoder;

    if (g_esco_recoder) {
        return -EBUSY;
    }
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"esco");
    if (uuid == 0) {
        return -EFAULT;
    }

    recoder = malloc(sizeof(*recoder));
    if (!recoder) {
        return -ENOMEM;
    }


    recoder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_ADC);
    if (!recoder->stream) {
        recoder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_PDM_MIC);
    }
    if (!recoder->stream) {
        recoder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_IIS_RX);
    }
    if (!recoder->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

    //设置ADC的中断点数
    err = jlstream_node_ioctl(recoder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, 256);
    if (link_type == JL_DOGLE_ACL) { //连接方式为ACL  msbc 编码需要用软件; aec 参考采样率为48000;
        enc_fmt.sw_hw_option = 1; //连接方式时ACL msbc 编码需要用软件
        u16 ref_sr = 48000;           //aec 参考采样率为48000;
        //设置编码参数
        err = jlstream_node_ioctl(recoder->stream, NODE_UUID_ENCODER, NODE_IOC_SET_PRIV_FMT, (int)(&enc_fmt));
        //根据回音消除的类型，将配置传递到对应的节点
#ifdef TCFG_AUDIO_CVP_SMS_ANS_MODE			/*单MIC+ANS通话*/
        err = jlstream_node_ioctl(recoder->stream, NODE_UUID_CVP_SMS_ANS, NODE_IOC_SET_FMT, (int)ref_sr);
#elif (defined TCFG_AUDIO_CVP_SMS_DNS_MODE) /*单MIC+DNS通话*/
        err = jlstream_node_ioctl(recoder->stream, NODE_UUID_CVP_SMS_DNS, NODE_IOC_SET_FMT, (int)ref_sr);
#elif (defined TCFG_AUDIO_CVP_DMS_ANS_MODE) /*双MIC+ANS通话*/
        err = jlstream_node_ioctl(recoder->stream, NODE_UUID_CVP_DMS_ANS, NODE_IOC_SET_FMT, (int)ref_sr);
#elif (defined TCFG_AUDIO_CVP_DMS_DNS_MODE) /*双MIC+DNS通话*/
        err = jlstream_node_ioctl(recoder->stream, NODE_UUID_CVP_DMS_DNS, NODE_IOC_SET_FMT, (int)ref_sr);
#endif/*TCFG_AUDIO_CVP_DMS_DNS_MODE*/
    }

    if (err) {
        goto __exit1;
    }
    jlstream_set_callback(recoder->stream, recoder->stream, esco_recoder_callback);
    jlstream_set_scene(recoder->stream, STREAM_SCENE_ESCO);
    err = jlstream_start(recoder->stream);
    if (err) {
        goto __exit1;
    }

    g_esco_recoder = recoder;

    return 0;

__exit1:
    jlstream_release(recoder->stream);
__exit0:
    free(recoder);
    return err;
}

void esco_recoder_close()
{
    struct esco_recoder *recoder = g_esco_recoder;

    if (!recoder) {
        return;
    }
    jlstream_stop(recoder->stream, 0);
    jlstream_release(recoder->stream);

    free(recoder);
    g_esco_recoder = NULL;

    jlstream_event_notify(STREAM_EVENT_CLOSE_RECODER, (int)"esco");
#if TCFG_AUDIO_DUT_ENABLE
    //退出通话默认设置为算法模式
    cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
#endif
}


extern bool esco_player_runing();
int esco_recoder_switch(u8 en)
{
    if (!esco_player_runing()) {
        printf("esco player close now, non-operational!");
        return 1;
    }
    if (en) {
        //固定LINK_SCO类型，dongle不跑switch
        esco_recoder_open(COMMON_SCO);
    } else {
        esco_recoder_close();
    }
    return 0;
}

//esco_recoder复位流程，目前提供产测使用
int esco_recoder_reset(void)
{
    if (!esco_player_runing()) {
        printf("esco player close now, non-operational!");
        return 1;
    }
    //产测流程，固定LINK_SCO类型
    esco_recoder_close();
    esco_recoder_open(COMMON_SCO);
    return 0;
}




