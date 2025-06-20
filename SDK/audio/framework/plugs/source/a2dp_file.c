#include "btstack/a2dp_media_codec.h"
#include "btstack/avctp_user.h"
#include "source_node.h"
#include "classic/tws_api.h"
#include "system/timer.h"
#include "sync/audio_syncts.h"
#include "media/bt_audio_timestamp.h"
#include "os/os_api.h"
#include "jiffies.h"
#include "a2dp_streamctrl.h"
#include "reference_time.h"

#define A2DP_TIMESTAMP_ENABLE       1

struct a2dp_file_hdl {
    u8 start;
    /*u8 reassemble;*/
    u16 timer;
    void *file;
    int media_type;
    struct stream_node *node;

    void *ts_handle;
    u32 sample_rate;
    u8 channel_num;
    u16 seqn;
    u32 base_time;
    u32 timestamp;
    u32 ts_sample_rate;
    u32 dts;//total frams

    u16 delay_time;
    u8 sync_step;
    u8 reference;
    struct a2dp_media_frame frame;
    int frame_len;
    void *stream_ctrl;
    u8 bt_addr[6];
    u8 tws_case;
    u8 handshake_state;
    u32 request_timeout;
    u32 handshake_timeout;
    /*struct stream_frame *reassembled_frame;*/

    u8 link_jl_dongle; //连接jl_dongle
    u8 rtp_ts_en; //使用rtp的时间戳
    u16 jl_dongle_latency ;
};

extern const uint32_t CONFIG_A2DP_DELAY_TIME_AAC;
extern const uint32_t CONFIG_A2DP_DELAY_TIME_SBC;
extern const int CONFIG_JL_DONGLE_PLAYBACK_LATENCY;
extern const int CONFIG_JL_DONGLE_PLAYBACK_DYNAMIC_LATENCY_ENABLE;
extern const int CONFIG_A2DP_DELAY_TIME_LO;
extern const int CONFIG_A2DP_SBC_DELAY_TIME_LO;
extern const int CONFIG_BTCTLER_TWS_ENABLE;
extern const int CONFIG_DONGLE_SPEAK_ENABLE;

extern void bt_audio_reference_clock_select(void *addr, u8 network);
extern u32 bt_audio_reference_clock_time(u8 network);
extern int a2dp_get_packet_pcm_frames(u8 codec_type, u8 *data, int len);
static int a2dp_stream_ts_enable_detect(struct a2dp_file_hdl *hdl, u8 *packet, int *drop);
static void a2dp_frame_pack_timestamp(struct a2dp_file_hdl *hdl, struct stream_frame *frame, u8 *data, int pcm_frames);
static void a2dp_file_timestamp_setup(struct a2dp_file_hdl *hdl);

static u8 a2dp_low_latency = 0;

#define msecs_to_bt_time(m)     (((m + 1)* 1000) / 625)
#define a2dp_seqn_before(a, b)  ((a < b && (u16)(b - a) < 1000) || (a > b && (u16)(a - b) > 1000))
#define RB16(b)    (u16)(((u8 *)b)[0] << 8 | (((u8 *)b))[1])
#define RB32(b)    (u32)(((u8 *)b)[0] << 24 | (((u8 *)b))[1] << 16 | (((u8 *)b))[2] << 8 | (((u8 *)b))[3])

#include "a2dp_handshake.c"
/*#include "a2dp_aac_demuxer.c"*/

void a2dp_file_low_latency_enable(u8 enable)
{
    a2dp_low_latency = enable;
}

static void abandon_a2dp_data(void *p)
{
    struct a2dp_file_hdl *hdl = (struct a2dp_file_hdl *)p;
    struct a2dp_media_frame _frame;
    while (a2dp_media_try_get_packet(hdl->file, &_frame) > 0) {
        // printf("drop : %d\n", RB16((u8 *)_frame.packet + 2));
        a2dp_media_free_packet(hdl->file, _frame.packet);
    }
    /*a2dp_media_clear_packet_before_seqn(hdl->file, 0);*/
}

static void a2dp_file_start_abandon_data(struct a2dp_file_hdl *hdl)
{
    int role = TWS_ROLE_MASTER;

    /*if (CONFIG_BTCTLER_TWS_ENABLE) {
        role = tws_api_get_role();
    }*/
    if (role == TWS_ROLE_MASTER) {
        if (hdl->timer == 0) {
            hdl->timer = sys_timer_add(hdl, abandon_a2dp_data, 100);
            puts("start_abandon_a2dp_data\n");
        }
    }
}

static void a2dp_file_stop_abandon_data(struct a2dp_file_hdl *hdl)
{
    if (hdl->timer) {
        sys_timer_del(hdl->timer);
        hdl->timer = 0;
    }
}


static void a2dp_source_wake_jlstream_run(void *_hdl)
{
    struct a2dp_file_hdl *hdl = (struct a2dp_file_hdl *)_hdl;

    if (hdl->start && (hdl->node->state & NODE_STA_SOURCE_NO_DATA)) {
        jlstream_try_run_by_snode(hdl->node);
    }
}

static enum stream_node_state a2dp_get_frame(void *_hdl, struct stream_frame **pframe)
{
    struct a2dp_file_hdl *hdl = (struct a2dp_file_hdl *)_hdl;
    struct a2dp_media_frame _frame;
    int drop = 0;
    int stream_error = 0;

    *pframe = NULL;

#if A2DP_TIMESTAMP_ENABLE
    if (!hdl->rtp_ts_en && !hdl->ts_handle) {
        int err = a2dp_tws_media_handshake(hdl);
        if (err) {
            return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
        }
        a2dp_file_timestamp_setup(hdl);
    }
#endif
    if (!hdl->ts_handle && hdl->start == 0) {
        int delay = a2dp_media_get_remain_play_time(hdl->file, 1);
        if (delay < 300) {
            return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
        }
        hdl->start = 1;
    }

    int len = hdl->frame_len;
    if (len == 0) {
        if (hdl->stream_ctrl) {
            stream_error = a2dp_stream_control_pull_frame(hdl->stream_ctrl, &_frame, &len);
        } else {
            len = a2dp_media_try_get_packet(hdl->file, &_frame);
        }
        if (len <= 0) {
            return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
        }
        hdl->frame_len = len;
        memcpy(&hdl->frame, &_frame, sizeof(struct a2dp_media_frame));
    } else {
        memcpy(&_frame, &hdl->frame, sizeof(struct a2dp_media_frame));
    }

    hdl->seqn = RB16((u8 *)_frame.packet + 2);
    int err = a2dp_stream_ts_enable_detect(hdl, _frame.packet, &drop);
    if (err) {
        if (drop) {
            if (hdl->stream_ctrl) {
                a2dp_stream_control_free_frame(hdl->stream_ctrl, &_frame);
            } else {
                a2dp_media_free_packet(hdl->file, _frame.packet);
            }
            hdl->frame_len = 0;
        }
        a2dp_tws_media_try_handshake_ack(0, hdl->seqn);
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }

    int head_len = a2dp_media_get_rtp_header_len(hdl->media_type, _frame.packet, len);

    struct stream_frame *frame;
    int frame_len = len - head_len;
    frame = jlstream_get_frame(hdl->node->oport, frame_len);
    if (frame == NULL) {
        return NODE_STA_RUN;
    }
    frame->len          = frame_len;
    frame->timestamp    = _frame.clkn;
    frame->flags        |= (stream_error);
    a2dp_frame_pack_timestamp(hdl, frame, _frame.packet + 4,  //时间戳的地址
                              a2dp_get_packet_pcm_frames(hdl->media_type,
                                      _frame.packet + head_len, frame_len));

    a2dp_tws_media_try_handshake_ack(1, hdl->seqn);

    memcpy(frame->data, _frame.packet + head_len, frame_len);

    if (hdl->stream_ctrl) {
        a2dp_stream_control_free_frame(hdl->stream_ctrl, &_frame);
    } else {
        a2dp_media_free_packet(hdl->file, _frame.packet);
    }
    hdl->frame_len = 0;
    hdl->start = 1;

    ASSERT(frame);
    *pframe = frame;

    return NODE_STA_RUN;
}

static void *a2dp_init(void *priv, struct stream_node *node)
{
    struct a2dp_file_hdl *hdl = zalloc(sizeof(*hdl));
    hdl->node = node;
    return hdl;
}

static const u32 aac_sample_rates[] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000,
};

static const u16 sbc_sample_rates[] = {16000, 32000, 44100, 48000};

static int a2dp_ioc_get_fmt(struct a2dp_file_hdl *hdl, struct stream_fmt *fmt)
{
    struct a2dp_media_frame _frame;
    int type = a2dp_media_get_codec_type(hdl->file);

    if (type == A2DP_CODEC_SBC) {
        fmt->coding_type = AUDIO_CODING_SBC;
    } else if (type == A2DP_CODEC_MPEG24) {
        fmt->coding_type = AUDIO_CODING_AAC;
    }
    if (hdl->sample_rate) {
        fmt->sample_rate = hdl->sample_rate;
        fmt->channel_mode = hdl->channel_num == 2 ? AUDIO_CH_LR : AUDIO_CH_MIX;
        printf("******a2dp_media_get_codec_type --- type == %d ******\n",type);
        return 0;
    }
    hdl->media_type = type;

__again:
    int len = a2dp_media_try_get_packet(hdl->file, &_frame);
    if (len <= 0) {
        printf("******len < 0 a2dp_media_try_get_packet --- len == %d ******\n",len);
        return -EAGAIN;
    }
    u8 *packet = _frame.packet;

    int head_len = a2dp_media_get_rtp_header_len(type, packet, len);
    if (head_len >= len) {
        printf("******head_len >= len --- len = %d,head_len = %d ******\n",len,head_len);
        a2dp_media_free_packet(hdl->file, packet);
        goto __again;
    }
    /*put_buf(packet, head_len + 8);*/
    u8 *frame = packet + head_len;
    if (frame[0] == 0x47) {    				//常见mux aac格式
        u8 sr = (frame[5] & 0x3C) >> 2;
        u8 ch = ((frame[5] & 0x3) << 2) | ((frame[6] & 0xC0) >> 6);
        fmt->channel_mode = AUDIO_CH_LR;
        fmt->sample_rate  = aac_sample_rates[sr];
    } else if (frame[0] == 0x20) {			//特殊LATM aac格式
        u8 sr = ((frame[2] & 0x7) << 1) | ((frame[3] & 0x80) >> 7);
        u8 ch = ((frame[3] & 0x78) >> 3) ;
        fmt->channel_mode = AUDIO_CH_LR;
        fmt->sample_rate = aac_sample_rates[sr];
    } else if (frame[0] == 0x9C) {          //sbc 格式
        /*
         * 检查数据是否为AAC格式,
         * 可以切换AAC和SBC格式的手机可能切换后数据格式和type不对应
         */
        head_len = a2dp_media_get_rtp_header_len(A2DP_CODEC_MPEG24, packet, len);
        if (head_len < len) {
            if (packet[head_len] == 0x47 || packet[head_len] == 0x20) {
                a2dp_media_free_packet(hdl->file, packet);
                printf("******head_len < len --- len = %d,head_len = %d ******\n",len,head_len);
                goto __again;
            }
        }

        u8 sr = (frame[1] >> 6) & 0x3;
        u8 ch = (frame[1] >> 2) & 0x3;
        if (ch == 0) {
            fmt->channel_mode = AUDIO_CH_MIX;
        } else {
            fmt->channel_mode = AUDIO_CH_LR;
        }
        fmt->sample_rate  = sbc_sample_rates[sr];
    } else {
        /*
         * 小米8手机先播sbc,暂停后切成AAC格式点播放,有时第一包数据还是sbc格式
         * 导致这里获取头信息错误
         */
        a2dp_media_free_packet(hdl->file, packet);
        goto __again;
    }
    a2dp_media_put_packet(hdl->file, packet);

    hdl->sample_rate = fmt->sample_rate;
    hdl->channel_num = (fmt->channel_mode == AUDIO_CH_LR) ? 2 : 1;
    printf("a2dp %d, format %s\n", hdl->sample_rate,
           type == A2DP_CODEC_SBC ? "sbc" : "aac");
    return 0;
}



static int a2dp_ioc_set_bt_addr(struct a2dp_file_hdl *hdl, u8 *bt_addr)
{
    y_printf("a2dp_ioc_set_bt_addr");
    put_buf(bt_addr, 6);
    hdl->file = a2dp_open_media_file(bt_addr);
    if (!hdl->file) {
        printf("open_file_faild\n");
        put_buf(bt_addr, 6);
        return -EINVAL;
    }
    memcpy(hdl->bt_addr, bt_addr, 6);

    if (CONFIG_DONGLE_SPEAK_ENABLE) {
        if (btstack_get_dev_type_for_addr(hdl->bt_addr) == REMOTE_DEV_DONGLE_SPEAK) {
            hdl->link_jl_dongle = 1;
            hdl->jl_dongle_latency = CONFIG_JL_DONGLE_PLAYBACK_LATENCY;
            if (!CONFIG_JL_DONGLE_PLAYBACK_DYNAMIC_LATENCY_ENABLE) {
                hdl->rtp_ts_en = 1;
            }
        }
    }

    return 0;
}

static void a2dp_ioc_stop(struct a2dp_file_hdl *hdl)
{
    /*hdl->sample_rate = 0;*/
    if (hdl->frame_len) {
        if (hdl->stream_ctrl) {
            a2dp_stream_control_free_frame(hdl->stream_ctrl, &hdl->frame);
        } else {
            a2dp_media_free_packet(hdl->file, hdl->frame.packet);
        }
        hdl->frame_len = 0;
    }
    a2dp_file_stop_abandon_data(hdl);
    /*a2dp_media_clear_packet_before_seqn(hdl->file, 0);*/
    a2dp_media_stop_play(hdl->file);
    hdl->start = 0;
}

static int sbc_get_packet_pcm_frames(u8 *data, int len)
{
    data++;

    u8 ch = (data[0] >> 2) & 0x3;
    u8 subbands = (data[0] & 0x01) ? 8 : 4;
    u8 blocks = (((data[0] >> 4) & 0x03) + 1) * 4;
    u8 channels = ch == 0x0 ? 1 : 2;
    u8 joint = ch == 0x3 ? 1 : 0;
    u8 bitpool = data[1] & 0xff;
    int frame_len = 4 + ((4 * subbands * channels) >> 3);
    if (ch >= 0x2) {
        frame_len += (((joint ? subbands : 0) + blocks * bitpool) + 7) >> 3;
    } else {
        frame_len += ((blocks * channels * bitpool) + 7) >> 3;
    }

    return (len / frame_len) * (blocks * subbands);
}

static int a2dp_get_packet_pcm_frames(u8 codec_type, u8 *data, int len)
{
    u8 unit = 1;
    u32 frames = 0;

    if (codec_type == A2DP_CODEC_SBC) {
        /*data--;*/
        /*u8 frame_num = data[0] & 0xf;*/
        frames = sbc_get_packet_pcm_frames(data, len);//frame_num * 128 * (unit);
    } else if (codec_type == A2DP_CODEC_MPEG24) {
        u32 audio_mux_element = 0xffffffff;
        memcpy(&audio_mux_element, data, len >= sizeof(audio_mux_element) ? sizeof(audio_mux_element) : len);
        if (audio_mux_element == 0xFC47 || audio_mux_element == 0x10120020) {
            frames = 1024 * (unit);
        }
    } else if (0) {//codec_type == A2DP_CODEC_APTX) {
        log_e("unsupport: aptx\n");
    }
    /* printf("frames %d\n", frames); */
    return frames;
}


static void a2dp_stream_control_open(struct a2dp_file_hdl *hdl)
{
    /*
     * 策略选择：
     * 1、是否低延时
     * 2、解码格式？
     * 3、策略方案 - 默认0，其他为定制方案
     */
    if (hdl->stream_ctrl) {
        return;
    }
    if (hdl->link_jl_dongle) {
        hdl->stream_ctrl = a2dp_stream_control_plan_select(hdl->file, a2dp_low_latency, hdl->media_type, A2DP_STREAM_JL_DONGLE_CONTROL);

    } else {
        hdl->stream_ctrl = a2dp_stream_control_plan_select(hdl->file, a2dp_low_latency, hdl->media_type, 0);
    }
    if (hdl->stream_ctrl) {
        hdl->delay_time = a2dp_stream_control_delay_time(hdl->stream_ctrl);
        a2dp_stream_control_set_underrun_callback(hdl->stream_ctrl, hdl, a2dp_source_wake_jlstream_run);
    }
}

static void a2dp_stream_control_close(struct a2dp_file_hdl *hdl)
{
    if (hdl->stream_ctrl) {
        a2dp_stream_control_free(hdl->stream_ctrl);
        hdl->stream_ctrl = NULL;
    }
}

static u32 a2dp_stream_update_base_time(struct a2dp_file_hdl *hdl)
{
    struct a2dp_media_frame frame;
    int distance_time = 0;
    int len = a2dp_media_try_get_packet(hdl->file, &frame);
    if (len > 0) {
        a2dp_media_put_packet(hdl->file, frame.packet);
        // return frame.clkn + msecs_to_bt_time((hdl->delay_time < 100 ? 100 : hdl->delay_time));
        // return bt_audio_reference_clock_time(0) + msecs_to_bt_time((hdl->delay_time < 250 ? 250 : hdl->delay_time));
        if (!a2dp_low_latency) {
            return frame.clkn + msecs_to_bt_time((hdl->delay_time < 100 ? 100 : hdl->delay_time));
        } else {
            return bt_audio_reference_clock_time(0) + msecs_to_bt_time((hdl->delay_time < 250 ? 250 : hdl->delay_time));
        }
    }
    distance_time = a2dp_low_latency ? hdl->delay_time : (hdl->delay_time - a2dp_media_get_remain_play_time(hdl->file, 1));
    if (!a2dp_low_latency) {
        distance_time = hdl->delay_time;
    } else if (distance_time < 100) {
        distance_time = 100;
    }
    /*printf("distance time : %d, %d, %d\n", a2dp_delay_time, a2dp_media_get_remain_play_time(hdl->file, 1), distance_time);*/
    return bt_audio_reference_clock_time(0) + msecs_to_bt_time(distance_time);
}


void a2dp_ts_handle_create(struct a2dp_file_hdl *hdl)
{
    if (!hdl || (hdl->rtp_ts_en)) {
        return;
    }
    if (hdl->ts_handle) {
        return;
    }
#if A2DP_TIMESTAMP_ENABLE
    int err = stream_node_ioctl(hdl->node, NODE_UUID_BT_AUDIO_SYNC, NODE_IOC_SYNCTS, 0);
    if (err) {
        return;
    }

    hdl->base_time = a2dp_stream_update_base_time(hdl);
    printf("a2dp timestamp base time : %d, %d\n", hdl->base_time, bt_audio_reference_clock_time(0));
    hdl->ts_handle = a2dp_audio_timestamp_create(hdl->sample_rate, hdl->base_time, TIMESTAMP_US_DENOMINATOR);
    hdl->sync_step = 0;
    hdl->frame_len = 0;
    hdl->dts = 0;
#endif
}

void a2dp_ts_handle_release(struct a2dp_file_hdl *hdl)
{
    if (!hdl) {
        return;
    }
#if A2DP_TIMESTAMP_ENABLE
    if (hdl->ts_handle) {
        a2dp_audio_timestamp_close(hdl->ts_handle);
        hdl->ts_handle = NULL;
    }
#endif
}

static void a2dp_frame_pack_timestamp(struct a2dp_file_hdl *hdl, struct stream_frame *frame, u8 *data, int pcm_frames)
{
    if (CONFIG_DONGLE_SPEAK_ENABLE) {
        if (hdl->link_jl_dongle && hdl->rtp_ts_en) {
            u32 ts = RB32(data);
            frame->timestamp    = ts + hdl->jl_dongle_latency * 1000 * 32;
            frame->flags        |= (FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_UPDATE_TIMESTAMP);
            /* printf("ts : %u, %u,  %u\n",ts,frame->timestamp, bt_audio_reference_clock_time(0)); */
            return;
        }
    }
    if (!hdl->ts_handle || pcm_frames == 0) {
        frame->flags &= ~FRAME_FLAG_TIMESTAMP_ENABLE;
        return;
    }

    if (CONFIG_BTCTLER_TWS_ENABLE && (frame->flags & FRAME_FLAG_RESET_TIMESTAMP_BIT)) {
        /*printf("----stream error : resume----\n");*/
        tws_a2dp_share_timestamp(hdl->ts_handle);
    }
    u32 timestamp = a2dp_audio_update_timestamp(hdl->ts_handle, hdl->seqn, hdl->dts);
    int delay_time = hdl->stream_ctrl ? a2dp_stream_control_delay_time(hdl->stream_ctrl) : hdl->delay_time;
    int frame_delay = (timestamp - (frame->timestamp * 625 * TIME_US_FACTOR)) / 1000 / TIME_US_FACTOR;
    /*int distance_time = (int)(timestamp - (frame->timestamp * 625 * TIME_US_FACTOR)) / 1000 / TIME_US_FACTOR - delay_time;*/
    int distance_time = frame_delay - delay_time;
    a2dp_audio_delay_offset_update(hdl->ts_handle, distance_time);
    frame->flags |= (FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_UPDATE_TIMESTAMP | FRAME_FLAG_UPDATE_DRIFT_SAMPLE_RATE);
    frame->timestamp = timestamp;
    frame->d_sample_rate = a2dp_audio_sample_rate(hdl->ts_handle) - hdl->sample_rate;
    /*printf("drift : %d\n", frame->d_sample_rate);*/
    hdl->dts += pcm_frames;
    a2dp_stream_mark_next_timestamp(hdl->stream_ctrl, timestamp + PCM_SAMPLE_TO_TIMESTAMP(pcm_frames, hdl->sample_rate));
    a2dp_stream_bandwidth_detect_handler(hdl->stream_ctrl, pcm_frames, hdl->sample_rate);
}

static int a2dp_stream_ts_enable_detect(struct a2dp_file_hdl *hdl, u8 *packet, int *drop)
{
    int err = 0;

    if (hdl->sync_step) {
        return 0;
    }

    if (!drop) {
        printf("wrong argument 'drop'!\n");
    }
    if (CONFIG_BTCTLER_TWS_ENABLE && hdl->ts_handle) {
        if (hdl->tws_case != 1 && \
            !a2dp_audio_timestamp_is_available(hdl->ts_handle, hdl->seqn, 0, drop)) {
            if (*drop) {
                hdl->base_time = a2dp_stream_update_base_time(hdl);
                a2dp_audio_set_base_time(hdl->ts_handle, hdl->base_time);
            }
            return -EINVAL;
        }
    }
    hdl->sync_step = 2;
    return 0;
}

static void a2dp_media_reference_time_setup(struct a2dp_file_hdl *hdl)
{
#if A2DP_TIMESTAMP_ENABLE
    int err = stream_node_ioctl(hdl->node, NODE_UUID_BT_AUDIO_SYNC, NODE_IOC_SYNCTS, 0);
    if (err) {
        return;
    }
    hdl->reference = audio_reference_clock_select(hdl->bt_addr, 0);//0 - a2dp主机，1 - tws, 2 - BLE
#endif
}

static void a2dp_media_reference_time_close(struct a2dp_file_hdl *hdl)
{
#if A2DP_TIMESTAMP_ENABLE
    audio_reference_clock_exit(hdl->reference);
#endif
}

static void a2dp_file_timestamp_setup(struct a2dp_file_hdl *hdl)
{
    a2dp_stream_control_open(hdl);
    a2dp_ts_handle_create(hdl);
}

static void a2dp_file_timestamp_close(struct a2dp_file_hdl *hdl)
{
    a2dp_tws_media_handshake_exit(hdl);
    a2dp_stream_control_close(hdl);
    a2dp_ts_handle_release(hdl);
}

static int a2dp_ioctl(void *_hdl, int cmd, int arg)
{
    int err = 0;
    struct a2dp_file_hdl *hdl = (struct a2dp_file_hdl *)_hdl;

    switch (cmd) {
    case NODE_IOC_SET_BTADDR:
        y_printf("NODE_IOC_SET_BTADDR");
        err = a2dp_ioc_set_bt_addr(hdl, (u8 *)arg);
        break;
    case NODE_IOC_GET_BTADDR:
        memcpy((u8 *)arg, hdl->bt_addr, 6);
        break;
    case NODE_IOC_GET_FMT:
        err = a2dp_ioc_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_PRIV_FMT:
        break;
    case NODE_IOC_START:
        a2dp_media_start_play(hdl->file);
        a2dp_media_set_rx_notify(hdl->file, hdl, a2dp_source_wake_jlstream_run);
        a2dp_file_stop_abandon_data(hdl);
        a2dp_media_reference_time_setup(hdl);
        break;
    case NODE_IOC_SUSPEND:
        /*hdl->sample_rate = 0;*/
        a2dp_media_set_rx_notify(hdl->file, NULL, NULL);
        a2dp_ioc_stop(hdl);
        a2dp_file_timestamp_close(hdl);
        a2dp_media_reference_time_close(hdl);
        a2dp_file_start_abandon_data(hdl);
        break;
    case NODE_IOC_STOP:
        a2dp_media_set_rx_notify(hdl->file, NULL, NULL);
        a2dp_ioc_stop(hdl);
        a2dp_file_timestamp_close(hdl);
        a2dp_media_reference_time_close(hdl);
        break;
    }

    return err;
}

static void a2dp_release(void *_hdl)
{
    struct a2dp_file_hdl *hdl = (struct a2dp_file_hdl *)_hdl;

    a2dp_close_media_file(hdl->file);
    a2dp_file_stop_abandon_data(hdl);

    free(hdl);
}


REGISTER_SOURCE_NODE_PLUG(a2dp_file_plug) = {
    .uuid       = NODE_UUID_A2DP_RX,
    .frame_size = 1024,
    .init       = a2dp_init,
    .get_frame  = a2dp_get_frame,
    .ioctl      = a2dp_ioctl,
    .release    = a2dp_release,
};









