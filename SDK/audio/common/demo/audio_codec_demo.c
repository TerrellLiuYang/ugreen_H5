#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "audio_base.h"

#include "decoder_node.h"
#include "tech_lib/opus_codec_api.h"

static int opus_enc_get_data(void *priv, void *data, int len);
static int opus_dec_get_data(void *priv, void *data, int len);
static int  opus_enc_put_output_data(void *priv, void *data, int len);
static int opus_dec_put_output_data(void *priv, void *data, int len);
static int opus_enc_pcm_data_get(void *priv, void *data, int len);

#define OPUS_ENCODER_TEST   0
#define OPUS_CODEC_TEST 	1

static u8 test_type = OPUS_ENCODER_TEST;
/*******************************************/
/**************   编码   ***************/
/*******************************************/
//编码
struct opus_encoder {
    OPUS_ENC_OPS *ops;
    OPUS_EN_FILE_IO IO;
    void *priv;
    void *enc_buf;
    u8 start;
    u8  *obuf_remain_ptr;
    u16 obuf_remain;

    u8 quality;
    u32 sample_rate;
    u8 output_frame[1024];//根据实际设置buf大小
};

struct opus_encoder *g_opus_enc = NULL;

//编码open
static void *opus_encoder_open(void *priv)
{
    struct opus_encoder *opus_enc;

    opus_enc = zalloc(sizeof(struct opus_encoder));
    if (opus_enc) {
        opus_enc->ops = get_opus_enc_ops();
        ASSERT(opus_enc->ops != NULL);
        opus_enc->priv = priv;
        g_opus_enc = opus_enc;
        return opus_enc;
    }
    return NULL;
}

//读取pcm数据接口 注意这里的参数len 是点数
static u16 opus_enc_input_data(void *priv, s16 *buf, u16 len)
{
    struct opus_encoder *opus_enc = (struct opus_encoder *)priv;

    int rlen = 0;

    if (test_type == OPUS_CODEC_TEST) {
        rlen = opus_enc_get_data(opus_enc->priv, buf, len << 1); //获取需要编码的pcm 数据;
    } else {
        rlen = opus_enc_pcm_data_get(opus_enc->priv, buf, len << 1);
    }
    if (!rlen) {
        return 0;
    }

    /* printf("enc input len = %d\n", len); */
    return (rlen >> 1);

}

//输出编码数据
static void  opus_enc_output_data(void *priv, u8 *buf, u16 len)
{
    struct opus_encoder *opus_enc = (struct opus_encoder *)priv;

    memcpy(opus_enc->output_frame, buf, len);

    int wlen = opus_enc_put_output_data(opus_enc->priv, opus_enc->output_frame, len);	//编码数据输出
    if (wlen != len) {
        putchar('w');
    }

    opus_enc->obuf_remain_ptr = (u8 *)opus_enc->output_frame + wlen;
    opus_enc->obuf_remain = len - wlen;
}


//设置编码参数
static int opus_encoder_set_fmt(void *_opus, struct audio_fmt *fmt)
{
    struct opus_encoder *opus_enc = (struct opus_encoder *)_opus;
    opus_enc->sample_rate = fmt->sample_rate;
    opus_enc->quality = fmt->quality;
    printf("\n opus enc quality:%d sr:%d\n", opus_enc->quality, opus_enc->sample_rate);
    return 0;
}




static int opus_encoder_start(void *_enc)
{
    struct opus_encoder *opus_enc = (struct opus_encoder *)_enc;

    if (opus_enc) {
        u32 need_buf_size = opus_enc->ops->need_buf(opus_enc->sample_rate);
        printf("opus: buf_size %x\n", need_buf_size);
        opus_enc->enc_buf = zalloc(need_buf_size);
        if (opus_enc->enc_buf) {
            u8 quality = (opus_enc->quality & 0x3) > 2 ? 2 : opus_enc->quality;
            opus_enc->IO.input_data  = opus_enc_input_data;
            opus_enc->IO.output_data = opus_enc_output_data;
            opus_enc->IO.priv = opus_enc;
            opus_enc->ops->open(opus_enc->enc_buf, &opus_enc->IO, quality, opus_enc->sample_rate);

            opus_enc->start = 1;
            return 0;
        } else {
            return -ENOMEM;
        }
    } else {
        return -EINVAL;
    }
}


static int opus_encoder_run(void *_opus)
{
    struct opus_encoder *opus_enc = (struct opus_encoder *)_opus;
    if (opus_enc->start == 0) {
        opus_encoder_start(opus_enc);
    }
    if (opus_enc->ops) {
        ///上一次的数据如果尚未输出完成， 先输出完，再run
        if (opus_enc->obuf_remain) {
            u16 wlen;
            wlen = opus_enc_put_output_data(opus_enc, opus_enc->obuf_remain_ptr, opus_enc->obuf_remain);
            opus_enc->obuf_remain -= wlen;
            opus_enc->obuf_remain_ptr += wlen;
            return 0;
        }
        opus_enc->ops->run(opus_enc->enc_buf);
    }
    return 0;
}

int opus_encoder_close(void *_opus)
{
    struct opus_encoder *opus_enc = (struct opus_encoder *)_opus;
    opus_enc->start = 0;
    if (opus_enc) {
#ifndef CONFIG_OPUS_STATIC
        if (opus_enc->enc_buf) {
            free(opus_enc->enc_buf);
        }
#endif
        free(opus_enc);
        g_opus_enc = NULL;
    }
    return 0;
}

/*******************************************/
/**************   解码   ***************/
/*******************************************/

#define OPUS_SR_8000_OUT_POINTS			(160)
#define OPUS_SR_16000_OUT_POINTS		(320)

extern decoder_ops_t *get_opusdec_ops();
extern const int OPUS_SRINDEX ;; //选择opus解码文件的帧大小，0代表一帧40字节，1代表一帧80字节，2代表一帧160字节


struct opus_decoder {
    void *priv;
    u8 *dc_buf;
    int sr;
    u8 channel;
    u8 start;
    u16 out_remain : 1;

    opuslib_dec_ops *opus_ops;
    s16 *obuf;
    u16 out_frame_len;
    u16 remain_len;
};


static void *opus_decoder_open(void *priv)
{
    printf("\n===== opus_decoder_open =====\n");
    struct opus_decoder *opus_dec = NULL;
    opus_dec = malloc(sizeof(struct opus_decoder));
    ASSERT(opus_dec != NULL);
    memset(opus_dec, 0, sizeof(struct opus_decoder));


    opus_dec->priv = priv; /* ops的第一参数 */
    opus_dec->dc_buf = NULL;

    opus_dec->sr = 16000; //与编码的一致
    opus_dec->channel = 1; //输出声道数

    opus_dec->opus_ops = getopuslibdec_ops();//get_opus_dec_obj();
    ASSERT(opus_dec->opus_ops != NULL);

    int dcbuf_size = opus_dec->opus_ops->need_buf();

    opus_dec->dc_buf = zalloc(dcbuf_size);


    if (opus_dec->sr == 8000) {
        opus_dec->out_frame_len = OPUS_SR_8000_OUT_POINTS * 2;
    } else if (opus_dec->sr == 16000) {
        opus_dec->out_frame_len = OPUS_SR_16000_OUT_POINTS * 2;
    }

    opus_dec->obuf = zalloc(opus_dec->out_frame_len);

    u32 srindex = OPUS_SRINDEX;
    opus_dec->opus_ops->open((u32 *)opus_dec->dc_buf, srindex);


    return opus_dec;
}


static int opus_decoder_get_data(void *_opus, void *data, int len)
{

    struct opus_decoder *opus_dec = (struct opus_decoder *)_opus;

    return opus_dec_get_data(opus_dec->priv, data, len);

}

static int opus_decoder_output(void *_opus, void *data, int len)
{
    struct opus_decoder *opus_dec = (struct opus_decoder *)_opus;

    //这里固定输出单声道，需要声道变化,可以先变换 在向后级输出 但是需要处理输出不完的情况

    return opus_dec_put_output_data(opus_dec->priv, data, len);

}

static int opus_decoder_run(void *_opus)
{
    struct opus_decoder *opus_dec = (struct opus_decoder *)_opus;
    u32 ret = -1;

    u8 *data = NULL;
    int len = opus_decoder_get_data(opus_dec, data, 40);//输入一帧编码数据，根据OPUS_SRINDEX参数可知编码帧的长度

    if (len > 0) {
        //返回解码输出点数, 解码固定输出单声道数据
        ret = opus_dec->opus_ops->run((u32 *)opus_dec->dc_buf, (char *)data, opus_dec->obuf);
    }

    if (ret != opus_dec->out_frame_len / 2) {
        //处理解码出错的情况
    }

    int olen = opus_decoder_output(opus_dec, opus_dec->obuf, opus_dec-> out_frame_len);

    return 0;
}


int opus_decoder_close(void *_opus)
{
    struct opus_decoder *dec_hdl = (struct opus_decoder *)_opus;

    printf("\n===== opus_decoder_close =====\n");

    if (dec_hdl->dc_buf) {
        free(dec_hdl->dc_buf);
        dec_hdl->dc_buf = NULL;
    }
    free(dec_hdl);
    return 0;
}




/*******************************************/
/************   test demo   ***********/
/*******************************************/
struct opus_codec_demo {
    struct opus_decoder *opus_dec;
    struct opus_encoder *opus_enc;
    u32 sample_rate;
    u8 ch_num;
    u8 quality;
    u8 start;


    cbuffer_t test_cbuf;
    int test_buf[128 * 4 / 4];
    int dec_buf[128 * 4 / 4];
};

struct opus_codec_demo *g_opus_demo = NULL;


static short const tsin_16k[16] = {
    0x0000, 0x30fd, 0x5a83, 0x7641, 0x7fff, 0x7642, 0x5a82, 0x30fc, 0x0000, 0xcf04, 0xa57d, 0x89be,	0x8000, 0x89be, 0xa57e, 0xcf05,
};
static u16 tx_s_cnt = 0;

static int get_sine_16k_data(u16 *s_cnt, s16 *data, u16 points, u8 ch)
{
    while (points--) {
        if (*s_cnt >= 16) {
            *s_cnt = 0;
        }
        *data++ = tsin_16k[*s_cnt];
        if (ch == 2) {
            *data++ = tsin_16k[*s_cnt];
        }
        (*s_cnt)++;
    }
    return 0;
}


static int opus_enc_get_data(void *priv, void *data, int len)
{
    struct opus_codec_demo *opus_demo = (struct opus_codec_demo *)priv;

    get_sine_16k_data(&tx_s_cnt, data, len / 2, 1);//单声道
    /* get_sine_16k_data(&tx_s_cnt, buf, len / 2 / 2, 2); //双声道*/
    return len;
}


static int  opus_enc_put_output_data(void *priv, void *data, int len)
{
    struct opus_codec_demo *opus_demo = (struct opus_codec_demo *)priv;
    if (!opus_demo || !opus_demo->start) {
        return 0;
    }
    /* printf("enc output len = %d\n", len); */
    int wlen = 0;
    if (test_type == OPUS_CODEC_TEST) {
        wlen = cbuf_write(&opus_demo->test_cbuf, data, len);
        if (wlen != len) {
            putchar('w');
        }
    } else {
        return len;
    }

    return wlen;
}


static int opus_dec_get_data(void *priv, void *data, int len)
{
    struct opus_codec_demo *opus_demo = (struct opus_codec_demo *)priv;

    if (!opus_demo || !opus_demo->start) {
        return 0;
    }

    if (cbuf_get_data_size(&opus_demo->test_cbuf) < len) {
        return 0;
    }

    int rlen = cbuf_read(&opus_demo->test_cbuf, opus_demo->dec_buf, len);
    data = opus_demo->dec_buf;

    return rlen;

}


static int opus_dec_put_output_data(void *priv, void *data, int len)
{

    //data_deal
    printf("dec out len = %d\n", len);

    return len;

}


static void opus_codec_test_task(void *priv)
{
    struct opus_codec_demo *opus_demo = (struct opus_codec_demo *)priv;
    int res = 0;
    int msg[16];
    while (1) {
        res = os_taskq_accept(ARRAY_SIZE(msg), msg);

        opus_encoder_run(opus_demo->opus_enc);  //编码主函数
        os_time_dly(20);
        opus_decoder_run(opus_demo->opus_dec);  //解码主函数
    }

}


void opus_codec_test_init()
{
    if (g_opus_demo) {
        return;
    }
    printf("opus codec test init \n");
    struct opus_codec_demo *opus_demo = NULL;

    struct audio_fmt fmt = {0};

    opus_demo = zalloc(sizeof(struct opus_codec_demo));
    ASSERT(opus_demo != NULL);

    g_opus_demo = opus_demo;

    test_type  = OPUS_CODEC_TEST;
    /*设置编码参数，要保证解码时的参数和编码一致*/
    opus_demo->sample_rate = 16000; //数据采样率
    opus_demo->quality = 0; //编码质量

    fmt.quality = opus_demo->quality;
    fmt.sample_rate = opus_demo->sample_rate;
    cbuf_init(&opus_demo->test_cbuf, opus_demo->test_buf, 128 * 4);

    opus_demo->opus_enc = opus_encoder_open(opus_demo);
    opus_encoder_set_fmt(opus_demo->opus_enc, &fmt);
    opus_encoder_start(opus_demo->opus_enc);


    opus_demo->opus_dec = opus_decoder_open(opus_demo);

    opus_demo->start = 1;
    printf("opus codec test start \n");
    int ret = os_task_create(opus_codec_test_task, opus_demo, 5, 512, 0, "opus_codec_task");

}


void opus_codec_test_close()
{

    if (!g_opus_demo) {
        return;
    }
    g_opus_demo->start = 0;

    opus_encoder_close(g_opus_demo->opus_enc);
    opus_decoder_close(g_opus_demo->opus_dec);

    free(g_opus_demo);
    g_opus_demo = NULL;
}

/*******************************************/
/**********  iis_out opus_enc demo *******/
/*******************************************/


struct opus_enc_demo {
    struct opus_encoder *opus_enc;
    u32 sample_rate; //编码只支持16k 采样率
    u8 ch_num;
    u8 quality;
    u8 start;

    cbuffer_t pcm_in_cbuf;
    u8 pcm_in_buf[1024];
};

struct opus_enc_demo *g_opus_enc_demo = NULL;



int opus_enc_pcm_data_write(void *priv, void *data, int len)
{
    if (!g_opus_enc_demo) {
        return 0;
    }
    int wlen = cbuf_write(&g_opus_enc_demo->pcm_in_cbuf, data, len);
    if (wlen != len) {
        putchar('~');
    }
    return wlen;

}

int opus_enc_pcm_data_get(void *priv, void *data, int len)
{
    if (!g_opus_enc_demo) {
        return 0;
    }
    if (cbuf_get_data_size(&g_opus_enc_demo->pcm_in_cbuf) < len) {
        return 0;
    }
    int wlen = cbuf_read(&g_opus_enc_demo->pcm_in_cbuf, data, len);
    if (wlen != len) {
        putchar('~');
    }
    return wlen;

}



void opus_encoder_run_demo()
{
    if (!g_opus_enc_demo || !g_opus_enc_demo->start) {
        printf("run err !!!!!!!!\n");
        return;
    }

    opus_encoder_run(g_opus_enc_demo->opus_enc);
}

void opus_enc_open_demo()
{
    if (g_opus_enc_demo) {
        printf("opus enc have opened\n");
        return;
    }
    test_type  = OPUS_ENCODER_TEST;
    struct opus_enc_demo *opus_enc_demo = NULL;
    struct audio_fmt fmt = {0};
    opus_enc_demo = zalloc(sizeof(struct opus_enc_demo));
    ASSERT(opus_enc_demo != NULL);

    g_opus_enc_demo = opus_enc_demo;

    /*设置编码参数，要保证解码时的参数和编码一致*/
    opus_enc_demo->sample_rate = 16000; //数据采样率
    opus_enc_demo->quality = 0; //编码质量

    fmt.quality = opus_enc_demo->quality;
    fmt.sample_rate = opus_enc_demo->sample_rate;
    cbuf_init(&opus_enc_demo->pcm_in_cbuf, opus_enc_demo->pcm_in_buf, 1024);

    opus_enc_demo->opus_enc = opus_encoder_open(opus_enc_demo);
    opus_encoder_set_fmt(opus_enc_demo->opus_enc, &fmt);
    opus_encoder_start(opus_enc_demo->opus_enc);
    g_opus_enc_demo->start = 1;
    printf("opus enc test start \n");

}

void opus_enc_close_demo()
{

    if (!g_opus_enc_demo) {
        return;
    }
    g_opus_enc_demo->start = 0;

    opus_encoder_close(g_opus_enc_demo->opus_enc);

    free(g_opus_enc_demo);
    g_opus_enc_demo = NULL;
}






