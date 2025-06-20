#include "ability_uuid.h"
#include "app_main.h"
#include "bt_tws.h"
#include "btstack/avctp_user.h"
#include "audio_config.h"
#include "vol_sync.h"
#include "audio_anc.h"
#include "app_tone.h"
#include "audio_manager.h"
#include "audio_voice_changer_api.h"
#include "app_config.h"

#if RCSP_MODE && RCSP_ADV_KEY_SET_ENABLE
#include "adv_anc_voice_key.h"
#endif

#if TCFG_AUDIO_SPATIAL_EFFECT_ENABLE
#include "spatial_effects_process.h"
#endif

extern BT_DEV_ADDR user_bt_dev_addr;
#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
#include "icsd_adt_app.h"
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

#if TCFG_APP_KEY_DUT_ENABLE
#include "app_key_dut.h"
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

extern struct bt_mode_var g_bt_hdl;

static const struct scene_event audio_event_table[] = {
    [0] = {
        .event = AUDIO_EVENT_VOL_MAX,
    },
    [1] = {
        .event = AUDIO_EVENT_VOL_MIN,
    },
    [2] = {
        .event = AUDIO_EVENT_ANC_ON,
    },
    [3] = {
        .event = AUDIO_EVENT_ANC_OFF,
    },
    [4] = {
        .event = AUDIO_EVENT_ANC_TRANS,
    },
    [5] = {
        .event = AUDIO_EVENT_A2DP_START,
    },
    [6] = {
        .event = AUDIO_EVENT_A2DP_STOP,
    },
    [7] = {
        .event = AUDIO_EVENT_ESCO_START,
    },
    [8] = {
        .event = AUDIO_EVENT_ESCO_STOP,
    },

    { 0xffffffff, NULL },
};

static bool audio_is_vol_max(struct scene_param *param)
{
    return (app_audio_get_volume(APP_AUDIO_CURRENT_STATE) == app_audio_get_max_volume());
}

static bool audio_is_vol_min(struct scene_param *param)
{
    return (!app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
}

static bool audio_is_anc_mode_on(struct scene_param *param)
{
#if TCFG_AUDIO_ANC_ENABLE
    return (anc_mode_get() == ANC_ON);
#endif/*TCFG_AUDIO_ANC_ENABLE*/
    return 0;
}

static bool audio_is_anc_mode_trans(struct scene_param *param)
{
#if TCFG_AUDIO_ANC_ENABLE
    return (anc_mode_get() == ANC_TRANSPARENCY);
#endif/*TCFG_AUDIO_ANC_ENABLE*/
    return 0;
}

static bool audio_is_anc_mode_off(struct scene_param *param)
{
#if TCFG_AUDIO_ANC_ENABLE
    return (anc_mode_get() == ANC_OFF);
#endif/*TCFG_AUDIO_ANC_ENABLE*/
    return 0;
}

static bool audio_is_mic_open(struct scene_param *param)
{
    return (!audio_common_mic_mute_en_get());
}

static bool audio_is_mic_close(struct scene_param *param)
{
    return (audio_common_mic_mute_en_get());
}


static const struct scene_state audio_state_table[] = {
    [0] = {
        .match = audio_is_vol_max,
    },
    [1] = {
        .match = audio_is_vol_min,
    },
    [2] = {
        .match = audio_is_anc_mode_on,
    },
    [3] = {
        .match = audio_is_anc_mode_trans,
    },
    [4] = {
        .match = audio_is_anc_mode_off,
    },
    [5] = {
        .match = audio_is_mic_open,
    },
    [6] = {
        .match = audio_is_mic_close,
    },
};

static u8 volume_flag = 1;
extern u8 user_get_active_device_addr(u8 *bt_addr);
void volume_up(u8 inc)
{
    u8 test_box_vol_up = 0x41;
    s8 cur_vol = 0;
    u8 call_status = bt_get_call_status();
    u8 cur_state;
    s16 max_volume;
	
	//extern u8 user_get_active_device_addr(u8 *bt_addr);
    user_get_active_device_addr(user_bt_dev_addr.active_addr);  
    if ((tone_player_runing() || ring_player_runing()) && volume_flag) {
        if (bt_get_call_status() == BT_CALL_INCOMING) {
            volume_up_down_direct(1);
        }
        return;
    }

    /*打电话出去彩铃要可以调音量大小*/
    if ((call_status == BT_CALL_ACTIVE) || (call_status == BT_CALL_OUTGOING)) {
        cur_state = APP_AUDIO_STATE_CALL;
        max_volume = app_audio_volume_max_query(AppVol_BT_CALL);
    } else {
        cur_state = APP_AUDIO_STATE_MUSIC;
        max_volume = app_audio_volume_max_query(AppVol_BT_MUSIC);
    }
    cur_vol = app_audio_get_volume(cur_state);
    if (bt_get_remote_test_flag()) {
        bt_cmd_prepare(USER_CTRL_TEST_KEY, 1, &test_box_vol_up); //音量加
    }

    /* if (cur_vol >= app_audio_get_max_volume()) { */
    if (cur_vol >= max_volume) {
        audio_event_to_user(AUDIO_EVENT_VOL_MAX);	//触发vol max事件

        if (bt_get_call_status() != BT_CALL_HANGUP) {
            /*本地音量最大，如果手机音量还没最大，继续加，以防显示不同步*/
            if (g_bt_hdl.phone_vol < 15) {
                if (bt_get_curr_channel_state() & HID_CH) {
					bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HID_VOL_UP, 0, NULL);
//                    bt_cmd_prepare(USER_CTRL_HID_VOL_UP, 0, NULL);
                } else {
					bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HID_VOL_UP, 0, NULL);
//                    bt_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_UP, 0, NULL);
                }
            }
            return;
        }
#if TCFG_BT_VOL_SYNC_ENABLE
        opid_play_vol_sync_fun(&app_var.music_volume, 1);
		bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_CMD_SYNC_VOL_INC, 0, NULL);
//        bt_cmd_prepare(USER_CTRL_CMD_SYNC_VOL_INC, 0, NULL);
#endif/*TCFG_BT_VOL_SYNC_ENABLE*/
        return;
    }

#if TCFG_BT_VOL_SYNC_ENABLE
    opid_play_vol_sync_fun(&app_var.music_volume, 1);
    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, app_var.music_volume, 1);
#else
    app_audio_volume_up(inc);
#endif/*TCFG_BT_VOL_SYNC_ENABLE*/
    printf("vol+: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
    if (bt_get_call_status() != BT_CALL_HANGUP) {
        if (bt_get_curr_channel_state() & HID_CH) {
			bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HID_VOL_UP, 0, NULL);
//            bt_cmd_prepare(USER_CTRL_HID_VOL_UP, 0, NULL);
        } else {
			bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HFP_CALL_VOLUME_UP, 0, NULL);
//            bt_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_UP, 0, NULL);
        }
    } else {
#if TCFG_BT_VOL_SYNC_ENABLE
		bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_CMD_SYNC_VOL_INC, 0, NULL);
//        bt_cmd_prepare(USER_CTRL_CMD_SYNC_VOL_INC, 0, NULL);     //使用HID调音量
#endif
    }
}

void volume_down(u8 dec)
{
    u8 test_box_vol_down = 0x42;
    u8 call_status = bt_get_call_status();
    u8 cur_state;
	
    user_get_active_device_addr(user_bt_dev_addr.active_addr);  
    if ((tone_player_runing() || ring_player_runing()) && volume_flag) {
        if (bt_get_call_status() == BT_CALL_INCOMING) {
            volume_up_down_direct(-1);
        }
        return;
    }
    if (bt_get_remote_test_flag()) {
		//bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HID_VOL_DOWN, 0, NULL);
        bt_cmd_prepare(USER_CTRL_TEST_KEY, 1, &test_box_vol_down); //音量减
    }

    if ((call_status == BT_CALL_ACTIVE) || (call_status == BT_CALL_OUTGOING)) {
        cur_state = APP_AUDIO_STATE_CALL;
    } else {
        cur_state = APP_AUDIO_STATE_MUSIC;
    }

    /* if (app_audio_get_volume(APP_AUDIO_CURRENT_STATE) <= 0) { */
    if (app_audio_get_volume(cur_state) <= 0) {
        audio_event_to_user(AUDIO_EVENT_VOL_MIN);	//触发vol mix事件
        if (bt_get_call_status() != BT_CALL_HANGUP) {
            /*
             *本地音量最小，如果手机音量还没最小，继续减
             *注意：有些手机通话最小音量是1(GREE G0245D)
             */
            if (g_bt_hdl.phone_vol > 1) {
                if (bt_get_curr_channel_state() & HID_CH) {
					bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HID_VOL_DOWN, 0, NULL);
                    //bt_cmd_prepare(USER_CTRL_HID_VOL_DOWN, 0, NULL);
                } else {
                    bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HFP_CALL_VOLUME_DOWN, 0, NULL);//bt_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_DOWN, 0, NULL);

                }
            }
            return;
        }
#if TCFG_BT_VOL_SYNC_ENABLE
        opid_play_vol_sync_fun(&app_var.music_volume, 0);
		bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HFP_CALL_VOLUME_DOWN, 0, NULL);//bt_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_DOWN, 0, NULL);
        //bt_cmd_prepare(USER_CTRL_CMD_SYNC_VOL_DEC, 0, NULL);
#endif
        return;
    }

#if TCFG_BT_VOL_SYNC_ENABLE
    opid_play_vol_sync_fun(&app_var.music_volume, 0);
    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, app_var.music_volume, 1);
#else
    app_audio_volume_down(dec);
#endif/*TCFG_BT_VOL_SYNC_ENABLE*/
    printf("vol-: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
    if (bt_get_call_status() != BT_CALL_HANGUP) {
        if (bt_get_curr_channel_state() & HID_CH) {
			bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HID_VOL_DOWN, 0, NULL);//bt_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_DOWN, 0, NULL);
            //bt_cmd_prepare(USER_CTRL_HID_VOL_DOWN, 0, NULL);
        } else {
			bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HID_VOL_DOWN, 0, NULL);//bt_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_DOWN, 0, NULL);
            //bt_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_DOWN, 0, NULL);
        }
    } else {
#if 1//TCFG_BT_VOL_SYNC_ENABLE
        /* opid_play_vol_sync_fun(&app_var.music_volume, 0); */
        if (app_audio_get_volume(APP_AUDIO_CURRENT_STATE) == 0) {
            app_audio_volume_down(0);
        }
		bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_CMD_SYNC_VOL_DEC, 0, NULL);//bt_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_DOWN, 0, NULL);
//        bt_cmd_prepare(USER_CTRL_CMD_SYNC_VOL_DEC, 0, NULL);
#endif
    }
}

static void audio_action_vol_up(struct scene_param *param)
{
    /*按键产测事件*/
#if TCFG_APP_KEY_DUT_ENABLE
    app_key_dut_send((int *)param->msg, "vol_up");
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

    volume_up(1);
}

static void audio_action_vol_down(struct scene_param *param)
{
    /*按键产测事件*/
#if TCFG_APP_KEY_DUT_ENABLE
    app_key_dut_send((int *)param->msg, "vol_down");
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

    volume_down(1);
}

static void audio_action_anc_mode_anc_on(struct scene_param *param)
{
    /*按键产测事件*/
#if TCFG_APP_KEY_DUT_ENABLE
    app_key_dut_send((int *)param->msg, "anc_on");
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

#if TCFG_AUDIO_ANC_ENABLE
    anc_mode_switch(ANC_ON, 1);
#endif/*TCFG_AUDIO_ANC_ENABLE*/
}

static void audio_action_anc_mode_anc_off(struct scene_param *param)
{
    /*按键产测事件*/
#if TCFG_APP_KEY_DUT_ENABLE
    app_key_dut_send((int *)param->msg, "anc_off");
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

#if TCFG_AUDIO_ANC_ENABLE
    anc_mode_switch(ANC_OFF, 1);
#endif/*TCFG_AUDIO_ANC_ENABLE*/
}

static void audio_action_anc_mode_anc_trans(struct scene_param *param)
{
    /*按键产测事件*/
#if TCFG_APP_KEY_DUT_ENABLE
    app_key_dut_send((int *)param->msg, "anc_trans");
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

#if TCFG_AUDIO_ANC_ENABLE
    anc_mode_switch(ANC_TRANSPARENCY, 1);
#endif/*TCFG_AUDIO_ANC_ENABLE*/
}

#if TCFG_AUDIO_ANC_ENABLE
#define TWS_FUNC_ID_ANC_NEXT_SYNC    TWS_FUNC_ID('A', 'N', 'C', 'M')
static void bt_tws_anc_next_sync(void *_data, u16 len, bool rx)
{
    if (rx) {
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
#if RCSP_MODE && RCSP_ADV_KEY_SET_ENABLE
            update_anc_voice_key_opt();
#else
            anc_mode_next();
#endif
        }
    }
}

REGISTER_TWS_FUNC_STUB(app_anc_next_sync_stub) = {
    .func_id = TWS_FUNC_ID_ANC_NEXT_SYNC,
    .func    = bt_tws_anc_next_sync,
};

void bt_tws_sync_anc_next(void)
{
    int err = tws_api_send_data_to_sibling(NULL, 0, TWS_FUNC_ID_ANC_NEXT_SYNC);
    if (err) {
#if !(RCSP_MODE && RCSP_ADV_KEY_SET_ENABLE)
        anc_mode_next();
#endif
    }
}
#endif/* TCFG_AUDIO_ANC_ENABLE*/


static void audio_action_anc_mode_next(struct scene_param *param)
{
    /*按键产测事件*/
#if TCFG_APP_KEY_DUT_ENABLE
    app_key_dut_send((int *)param->msg, "anc_next");
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

#if TCFG_AUDIO_ANC_ENABLE
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        if (priv->local_action) {	//上层限制左右按键
            bt_tws_sync_anc_next();
        }
    } else {
#if RCSP_MODE && RCSP_ADV_KEY_SET_ENABLE
        update_anc_voice_key_opt();
#else
        anc_mode_next();
#endif
    }
#endif/*TCFG_AUDIO_ANC_ENABLE*/
}

static void audio_action_mic_open(struct scene_param *param)
{
    /*按键产测事件*/
#if TCFG_APP_KEY_DUT_ENABLE
    app_key_dut_send((int *)param->msg, "mic_open");
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

    audio_common_mic_mute_en_set(0);
}

static void audio_action_mic_close(struct scene_param *param)
{
    /*按键产测事件*/
#if TCFG_APP_KEY_DUT_ENABLE
    app_key_dut_send((int *)param->msg, "mic_close");
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

    audio_common_mic_mute_en_set(1);
}

#ifdef TCFG_VOICE_CHANGER_NODE_ENABLE

static u16 esco_eff_uncle;
static u16 esco_eff_goddess;
#define TWS_FUNC_ID_ESCO_EFF    TWS_FUNC_ID('A', 'N', 'C', 'M')
static void bt_tws_voice_changer(void *_data, u16 len, bool rx)
{
    if (rx) {
        u16 mode;
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            memcpy(&mode, (const void *)_data, sizeof(u16));
            /* printf("=========mode %d\n", mode); */
            audio_esco_ul_voice_change(mode);
        }
    }
}

REGISTER_TWS_FUNC_STUB(esco_voice_changer) = {
    .func_id = TWS_FUNC_ID_ESCO_EFF,
    .func    = bt_tws_voice_changer,
};

void bt_tws_sync_voice_changer_next(u16 mode)
{
    int err = tws_api_send_data_to_sibling(&mode, sizeof(u16), TWS_FUNC_ID_ESCO_EFF);
    if (err) {
    }
}
#endif


static void audio_esco_ul_voice_changer_uncle(struct scene_param *param)
{
    /*按键产测事件*/
#if TCFG_APP_KEY_DUT_ENABLE
    app_key_dut_send((int *)param->msg, "changer_uncle");
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

#ifdef TCFG_VOICE_CHANGER_NODE_ENABLE
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;
    log_d("============esco_eff_uncle %d\n", esco_eff_uncle);
    if (priv->local_action) {	//上层限制左右按键
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            if (esco_eff_uncle) {
                bt_tws_sync_voice_changer_next(VOICE_CHANGER_NONE);
            } else {
                bt_tws_sync_voice_changer_next(VOICE_CHANGER_UNCLE);
            }
        } else {
            if (esco_eff_uncle) {
                audio_esco_ul_voice_change(VOICE_CHANGER_NONE);
            } else {
                audio_esco_ul_voice_change(VOICE_CHANGER_UNCLE);
            }
        }
        esco_eff_uncle = !esco_eff_uncle;
    }
#endif
}
static void audio_esco_ul_voice_changer_goddess(struct scene_param *param)
{
    /*按键产测事件*/
#if TCFG_APP_KEY_DUT_ENABLE
    app_key_dut_send((int *)param->msg, "changer_goddess");
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

#ifdef TCFG_VOICE_CHANGER_NODE_ENABLE
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;
    log_d("============esco_eff_goddess %d\n", esco_eff_goddess);
    if (priv->local_action) {	//上层限制左右按键
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            if (esco_eff_goddess) {
                bt_tws_sync_voice_changer_next(VOICE_CHANGER_NONE);
            } else {
                bt_tws_sync_voice_changer_next(VOICE_CHANGER_GODDESS);//女声
            }
        } else {
            if (esco_eff_goddess) {
                bt_tws_sync_voice_changer_next(VOICE_CHANGER_NONE);
            } else {
                audio_esco_ul_voice_change(VOICE_CHANGER_GODDESS);//女声
            }
        }
        esco_eff_goddess = !esco_eff_goddess;
    }

#endif
}

static void audio_spatial_effects_mode_set(struct scene_param *param)
{
    /*按键产测事件*/
#if TCFG_APP_KEY_DUT_ENABLE
    app_key_dut_send((int *)param->msg, "spatial_eff");
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

#if TCFG_AUDIO_SPATIAL_EFFECT_ENABLE
    u8 mode = get_a2dp_spatial_audio_mode();
    if (++mode > 2) {
        mode = 0;
    }
    printf("%s : %d", __func__, mode);
    audio_spatial_effects_mode_switch(mode);
#endif
}
static void audio_action_speak_to_chat_switch(struct scene_param *param)
{
    /*按键产测事件*/
#if TCFG_APP_KEY_DUT_ENABLE
    app_key_dut_send((int *)param->msg, "spk_char");
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

#if (defined TCFG_AUDIO_SPEAK_TO_CHAT_ENABLE) && TCFG_AUDIO_SPEAK_TO_CHAT_ENABLE
    audio_speak_to_chat_demo();
#endif /*TCFG_AUDIO_SPEAK_TO_CHAT_ENABLE*/
}

static void audio_action_icsd_wat_click_switch(struct scene_param *param)
{
    /*按键产测事件*/
#if TCFG_APP_KEY_DUT_ENABLE
    app_key_dut_send((int *)param->msg, "wat_click");
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

#if (defined TCFG_AUDIO_WIDE_AREA_TAP_ENABLE) && TCFG_AUDIO_WIDE_AREA_TAP_ENABLE
    audio_wat_click_demo();
#endif /*TCFG_AUDIO_WIDE_AREA_TAP_ENABLE*/
}

static void audio_action_icsd_wind_detect_switch(struct scene_param *param)
{
    /*按键产测事件*/
#if TCFG_APP_KEY_DUT_ENABLE
    app_key_dut_send((int *)param->msg, "wind_det");
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

#if (defined TCFG_AUDIO_ANC_WIND_NOISE_DET_ENABLE) && TCFG_AUDIO_ANC_WIND_NOISE_DET_ENABLE
    audio_icsd_wind_detect_demo();
#endif /*TCFG_AUDIO_ANC_WIND_NOISE_DET_ENABLE*/
}

static void audio_action_anc_ear_adaptive_open(struct scene_param *param)
{
    /*按键产测事件*/
#if TCFG_APP_KEY_DUT_ENABLE
    app_key_dut_send((int *)param->msg, "anc_adap");
#endif /*TCFG_APP_KEY_DUT_ENABLE*/

#if (defined ANC_EAR_ADAPTIVE_EN) && ANC_EAR_ADAPTIVE_EN
    audio_anc_mode_ear_adaptive(1);
#endif /*ANC_EAR_ADAPTIVE_EN*/
}

static const struct scene_action audio_action_table[] = {
    [0] = {
        .action = audio_action_vol_up,
    },
    [1] = {
        .action = audio_action_vol_down,
    },
    [2] = {
        .action = audio_action_anc_mode_anc_on,
    },
    [3] = {
        .action = audio_action_anc_mode_anc_off,
    },
    [4] = {
        .action = audio_action_anc_mode_anc_trans,
    },
    [5] = {
        .action = audio_action_anc_mode_next,
    },
    [6] = {
        .action = audio_action_mic_open,
    },
    [7] = {
        .action = audio_action_mic_close,
    },
    [8] = {
        .action = audio_esco_ul_voice_changer_uncle,
    },
    [9] = {
        .action = audio_esco_ul_voice_changer_goddess,
    },
    [10] = {
        .action = audio_spatial_effects_mode_set,
    },
    [11] = {
        .action = audio_action_speak_to_chat_switch,
    },
    [12] = {
        .action = audio_action_icsd_wat_click_switch,
    },
    [13] = {
        .action = audio_action_icsd_wind_detect_switch,
    },
    [14] = {
        .action = audio_action_anc_ear_adaptive_open,
    },

};

REGISTER_SCENE_ABILITY(audio_ability) = {
    .uuid   = UUID_AUDIO,
    .event  = audio_event_table,
    .state  = audio_state_table,
    .action = audio_action_table,
};

static int audio_msg_handler(int *msg)
{
    u8 *event = (u8 *)msg;
    printf("audio_event_happen: %d\n", event[0]);
    scene_mgr_event_match(UUID_AUDIO, event[0], event);
    return 0;
}

APP_MSG_HANDLER(audio_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_AUDIO,
    .handler    = audio_msg_handler,
};

