#include "btstack/avctp_user.h"
#include "app_main.h"
#include "jl_kws/jl_kws_api.h"
#include "app_config.h"
#include "audio_cvp.h"


#if TCFG_CALL_KWS_SWITCH_ENABLE
extern int audio_phone_call_kws_start(void);
extern int audio_phone_call_kws_close(void);
extern void audio_smart_voice_detect_close(void);
extern bool esco_player_runing();
#endif

static void jl_call_kws_handler(int event)
{
    if (event == BT_STATUS_PHONE_INCOME) {
#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
        audio_aec_reboot(1);
        jl_kws_speech_recognition_open();
        jl_kws_speech_recognition_start();
#endif

#if TCFG_CALL_KWS_SWITCH_ENABLE
        audio_aec_reboot(1);
        audio_phone_call_kws_start();
#endif /* #if TCFG_CALL_KWS_SWITCH_ENABLE */
    } else if (event == BT_STATUS_PHONE_ACTIVE) {
#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
        jl_kws_speech_recognition_close();
        audio_aec_reboot(0);
#endif
#ifdef CONFIG_BOARD_AISPEECH_VAD_ASR
        printf("----aispeech_state phone active");
        ais_platform_asr_close();
        esco_mic_reset();
#endif /*CONFIG_BOARD_AISPEECH_VAD_ASR*/
#if TCFG_CALL_KWS_SWITCH_ENABLE
        audio_smart_voice_detect_close();
        audio_aec_reboot(0);
#endif /* TCFG_CALL_KWS_SWITCH_ENABLE */
    } else if (event == BT_STATUS_PHONE_HANGUP) {
#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
        jl_kws_speech_recognition_close();
#endif
#if TCFG_CALL_KWS_SWITCH_ENABLE
        if (!esco_player_runing()) {
            audio_phone_call_kws_close();
        }
#endif /* TCFG_CALL_KWS_SWITCH_ENABLE */
    }
}


static int kws_btstack_msg_handler(int *msg)
{
    struct bt_event *bt = (struct bt_event *)msg;

    switch (bt->event) {
    case BT_STATUS_PHONE_INCOME:
        jl_call_kws_handler(BT_STATUS_PHONE_INCOME);
        break;
    case BT_STATUS_PHONE_ACTIVE:
        jl_call_kws_handler(BT_STATUS_PHONE_ACTIVE);
        break;
    case BT_STATUS_PHONE_HANGUP:
        jl_call_kws_handler(BT_STATUS_PHONE_HANGUP);
        break;
    default:
        break;
    }
    return 0;
}
APP_MSG_PROB_HANDLER(call_kws_btstack_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = kws_btstack_msg_handler,
};

