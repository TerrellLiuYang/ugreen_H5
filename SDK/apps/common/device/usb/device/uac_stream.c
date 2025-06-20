#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "app_config.h"
#include "system/includes.h"
#include "printf.h"
#include "usb/usb_config.h"
#include "usb/device/usb_stack.h"
#include "generic/circular_buf.h"
#include "system/event.h"

#include "usb/device/uac_audio.h"
#include "uac_stream.h"
#include "audio_config.h"
#include "pc_spk_player.h"
#include "pc_spk_file.h"
#include "pc_mic_recoder.h"


#define LOG_TAG_CONST       USB
#define LOG_TAG             "[UAC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


static volatile u8 speaker_stream_is_open = 0;
struct uac_speaker_handle {
    cbuffer_t cbuf;
    volatile u8 need_resume;
    u8 channel;
    //void (*rx_handler)(int, void *, int);
};

static void (*uac_rx_handler)(int, void *, int) = NULL;

static u16 speaker_close_tid = 0;

static struct uac_speaker_handle *uac_speaker = NULL;

#if USB_MALLOC_ENABLE
#else
static struct uac_speaker_handle uac_speaker_handle SEC(.usb.data.bss.exchange);
#endif


void set_uac_speaker_rx_handler(void *priv, void (*rx_handler)(int, void *, int))
{
    uac_rx_handler = rx_handler;
    /* if (uac_speaker) { */
    /* uac_speaker->rx_handler = rx_handler; */
    /* } */
}


//这里起中断往后面推数
void uac_speaker_stream_write(const u8 *obuf, u32 len)
{
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    pc_spk_data_isr_cb((void *)obuf, len);
#endif
}

void uac_speaker_stream_get_volume(u16 *l_vol, u16 *r_vol)
{
    uac_get_cur_vol(0, l_vol, r_vol);
}

void uac_speaker_stream_open(u32 samplerate, u32 ch, u32 bitwidth)
{
    u32 last_sr = 0;

#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    last_sr = pc_spk_get_fmt_sample_rate();
#endif
    if (speaker_stream_is_open) {
        if (speaker_close_tid) {
            sys_hi_timeout_del(speaker_close_tid);
            speaker_close_tid = 0;
        }
        if (last_sr != samplerate) {
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
            pc_spk_set_fmt(ch, 0, samplerate);
            pcspk_restart_player_by_taskq();
#endif
        }
        return;
    }
    log_info("%s", __func__);

    if (!uac_speaker) {
#if USB_MALLOC_ENABLE
        uac_speaker = zalloc(sizeof(struct uac_speaker_handle));
        if (!uac_speaker) {
            return;
        }
#else
        uac_speaker = &uac_speaker_handle;
        memset(uac_speaker, 0, sizeof(struct uac_speaker_handle));
#endif
        uac_speaker->channel = ch;
    }
    //uac_speaker->rx_handler = NULL;

    speaker_stream_is_open = 1;

#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    y_printf(">> Case : USB_AUDIO_PLAY_OPEN\n");
    pc_spk_set_fmt(ch, 0, samplerate);
    pcspk_open_player_by_taskq();
#endif

}

void uac_speaker_stream_close_delay()
{
    speaker_close_tid = 0;
    if (speaker_stream_is_open == 0) {
        return;
    }

    log_info("%s", __func__);
    speaker_stream_is_open = 0;

    if (uac_speaker) {
#if USB_MALLOC_ENABLE
        free(uac_speaker);
#endif
        uac_speaker = NULL;
    }
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    y_printf(">> Case : USB_AUDIO_PLAY_CLOSE\n");
    pcspk_close_player_by_taskq();
#endif
}

void uac_speaker_stream_close()
{
    if (speaker_stream_is_open == 0) {
        return;
    }

    if (speaker_close_tid == 0) {
        speaker_close_tid = sys_hi_timeout_add(NULL, uac_speaker_stream_close_delay, 1000);
    } else {
        sys_hi_timeout_modify(speaker_close_tid, 1000);
    }
}

int uac_get_spk_vol()
{
    int max_vol = app_audio_volume_max_query(AppVol_USB);
    int vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    if (vol * 100 / max_vol < 100) {
        return vol * 100 / max_vol;
    } else {
        return 99;
    }
    return 0;
}

static volatile u32 mic_stream_is_open;
u8 uac_get_mic_stream_status(void)
{
    return mic_stream_is_open;
}

void uac_mute_volume(u32 type, u32 l_vol, u32 r_vol)
{
    static u32 last_spk_l_vol = (u32) - 1, last_spk_r_vol = (u32) - 1;
    static u32 last_mic_vol = (u32) - 1;

    switch (type) {
    case MIC_FEATURE_UNIT_ID: //MIC
        if (mic_stream_is_open == 0) {
            return ;
        }
        if (l_vol == last_mic_vol) {
            return;
        }
        last_mic_vol = l_vol;
        //TODO
        break;
    case SPK_FEATURE_UNIT_ID: //SPK
        if (speaker_stream_is_open == 0) {
            return;
        }
        if (l_vol == last_spk_l_vol && r_vol == last_spk_r_vol) {
            return;
        }
        last_spk_l_vol = l_vol;
        last_spk_r_vol = r_vol;

        //TODO
        //left and right seperate
        /* y_printf(">> PC, r_vol:%d, l_vol:%d\n", r_vol, l_vol); */
        /* app_audio_set_volume(APP_AUDIO_STATE_MUSIC, (r_vol + l_vol) / 2, 1); */
        break;
    default:
        break;
    }
}


static int (*mic_tx_handler)(void *, void *, int) = NULL;
static void *mic_tx_handler_priv = NULL;
static u32 mic_close_tid = 0;

int uac_mic_stream_read(u8 *buf, u32 len)
{
    if (mic_stream_is_open == 0) {
        return 0;
    }
#if 0//48K 1ksin
    const s16 sin_48k[] = {
        0, 2139, 4240, 6270, 8192, 9974, 11585, 12998,
        14189, 15137, 15826, 16244, 16384, 16244, 15826, 15137,
        14189, 12998, 11585, 9974, 8192, 6270, 4240, 2139,
        0, -2139, -4240, -6270, -8192, -9974, -11585, -12998,
        -14189, -15137, -15826, -16244, -16384, -16244, -15826, -15137,
        -14189, -12998, -11585, -9974, -8192, -6270, -4240, -2139
    };
    u16 *l_ch = (u16 *)buf;
    u16 *r_ch = (u16 *)buf;
    r_ch++;
    for (int i = 0; i < len / 4; i++) {
        *l_ch = sin_48k[i];
        *r_ch = sin_48k[i];
        l_ch += 2;
        r_ch += 2;
    }
    return len;
#else
    if (mic_tx_handler) {
        return mic_tx_handler(mic_tx_handler_priv, buf, len);
    } else {
        //putchar('N');
    }
    return 0;
#endif
}

void set_uac_mic_tx_handler(void *priv, int (*tx_handler)(void *, void *, int))
{
    mic_tx_handler = tx_handler;
    mic_tx_handler_priv = priv;
}

void uac_mic_stream_get_volume(u16 *vol)
{
    *vol = uac_get_mic_vol(0);
}

u32 uac_mic_stream_open(u32 samplerate, u32 ch, u32 bitwidth)
{
    u32 last_sr = 0;

#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    last_sr = pc_mic_get_fmt_sample_rate();
    pc_mic_set_fmt(ch, 0, samplerate);
#endif

    if (mic_stream_is_open) {
        if (mic_close_tid) {
            sys_hi_timeout_del(mic_close_tid);
            mic_close_tid = 0;
        }
        if (last_sr != samplerate) {
            mic_tx_handler = NULL;	//添加测试

#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
            //重启mic recorder
            pc_mic_recoder_restart_by_taskq();
#endif
        }
        return 0;
    }

    mic_tx_handler = NULL;	//这里需要赋值为NULL

    log_info("%s", __func__);

    mic_stream_is_open = 1;

#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    y_printf("## Func:%s, Line:%d, Open Mic!!\n", __func__, __LINE__);
    pc_mic_recoder_open_by_taskq();
#endif

    return 0;
}

static void uac_mic_stream_close_delay()
{
    mic_close_tid = 0;
    log_info("%s", __func__);
    mic_stream_is_open = 0;

#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    y_printf("## Func:%s, Line:%d, Close Mic!!\n", __func__, __LINE__);
    pc_mic_recoder_close_by_taskq();
#endif
}

void uac_mic_stream_close()
{
    if (mic_stream_is_open == 0) {
        return ;
    }
    //FIXME:
    //未知原因出现频繁开关mic，导致出现audio或者蓝牙工作异常，
    //收到mic关闭命令后延时1s再发消息通知audio模块执行关闭动作
    //如果在1s之内继续收到usb下发的关闭命令，则继续推迟1s。
    if (mic_close_tid == 0) {
        mic_close_tid = sys_hi_timeout_add(NULL, uac_mic_stream_close_delay, 1000);
    } else {
        sys_hi_timeout_modify(mic_close_tid, 1000);
    }
}

