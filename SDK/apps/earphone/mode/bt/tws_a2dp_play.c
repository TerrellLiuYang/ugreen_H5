#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "classic/tws_api.h"
#include "os/os_api.h"
#include "bt_slience_detect.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "app_tone.h"
#include "app_main.h"
#include "vol_sync.h"
#include "audio_config.h"
#include "btstack/a2dp_media_codec.h"
#include "earphone.h"
#include "app_msg.h"
#include "audio_manager.h"

extern APP_INFO user_app_info;
extern USER_VAR user_var;

enum {
    CMD_A2DP_PLAY = 1,
    CMD_A2DP_SLIENCE_DETECT,
    CMD_A2DP_CLOSE,
    CMD_SET_A2DP_VOL,
    CMD_A2DP_SWITCH,
};

static u8 g_play_addr[6];
static u8 g_slience_addr[6];

int g_avrcp_vol_chance_timer = 0;
u8 g_avrcp_vol_chance_data[8];


void tws_a2dp_play_send_cmd(u8 cmd, u8 *data, u8 len, u8 tx_do_action);
extern void tws_dual_conn_state_handler();

static void tws_a2dp_player_close(u8 *bt_addr)
{
    puts("tws_a2dp_player_close\n");
    put_buf(bt_addr, 6);
    a2dp_player_close(bt_addr);
    bt_stop_a2dp_slience_detect(bt_addr);
    a2dp_media_close(bt_addr);
}

static void tws_a2dp_play_in_task(u8 *data)
{
    u8 btaddr[6];
    u8 dev_vol;
    u8 *bt_addr = data + 2;

    switch (data[0]) {
    case CMD_A2DP_SLIENCE_DETECT:
        puts("CMD_A2DP_SLIENCT_DETECE\n");
        put_buf(bt_addr, 6);
        a2dp_player_close(bt_addr);
        bt_start_a2dp_slience_detect(bt_addr, 50);
        memset(g_slience_addr, 0xff, 6);
        break;
    case CMD_A2DP_PLAY:
        puts("app_msg_bt_a2dp_play\n");
        int err;
        put_buf(bt_addr, 6);
#if (TCFG_BT_A2DP_PLAYER_ENABLE == 0)
        break;
#endif
        dev_vol = data[8];
        //更新一下音量再开始播放
        u8 a2dp_player_addr[6];
        u8 a2dp_player_busy = 0;
        if (a2dp_player_get_btaddr(a2dp_player_addr) && memcmp(a2dp_player_addr, bt_addr, 6)) {
            a2dp_player_busy = 1;
            g_printf("a2dp_player_busy");
        }
        if (!a2dp_player_busy) {
            app_audio_state_switch(APP_AUDIO_STATE_MUSIC, app_audio_volume_max_query(AppVol_BT_MUSIC), NULL);
            set_music_device_volume(dev_vol);
        }
        bt_stop_a2dp_slience_detect(bt_addr);
        a2dp_player_low_latency_enable(tws_api_get_low_latency_state());

        err = a2dp_player_open(bt_addr);
        if (err == -EBUSY) {
            bt_start_a2dp_slience_detect(bt_addr, 50);
        }
        memset(g_play_addr, 0xff, 6);
        break;
    case CMD_A2DP_CLOSE:
        tws_a2dp_player_close(bt_addr);
        /*
         * 如果后台有A2DP数据,关闭检测和MEDIA_START状态,
         * 等待协议栈重新发送MEDIA_START消息
         */
        if (bt_slience_get_detect_addr(btaddr)) {
            bt_stop_a2dp_slience_detect(btaddr);
            a2dp_media_close(btaddr);
        }
        break;
    case CMD_A2DP_SWITCH:
        puts("CMD_A2DP_SWITCH\n");
        if (!bt_slience_get_detect_addr(btaddr)) {
            break;
        }
        a2dp_player_close(bt_addr);
        bt_start_a2dp_slience_detect(bt_addr, 0);

        bt_stop_a2dp_slience_detect(btaddr);
        a2dp_media_close(btaddr);
        break;
    case CMD_SET_A2DP_VOL:
        dev_vol = data[8];
        set_music_device_volume(dev_vol);
        break;
    }
    if (data[1] != 2) {
        free(data);
    }
}

static void tws_a2dp_play_callback(u8 *data, u8 len)
{
    int msg[4];

    u8 *buf = malloc(len);
    if (!buf) {
        return;
    }
    memcpy(buf, data, len);

    msg[0] = (int)tws_a2dp_play_in_task;
    msg[1] = 1;
    msg[2] = (int)buf;

    os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
}

static void tws_a2dp_player_data_in_irq(void *_data, u16 len, bool rx)
{
    u8 *data = (u8 *)_data;

    if (data[1] == 0 && !rx) {
        return;
    }
    tws_a2dp_play_callback(data, len);
}
REGISTER_TWS_FUNC_STUB(tws_a2dp_player_stub) = {
    .func_id = 0x076AFE82,
    .func = tws_a2dp_player_data_in_irq,
};

void tws_a2dp_play_send_cmd(u8 cmd, u8 *_data, u8 len, u8 tx_do_action)
{
    u8 data[16];
    data[0] = cmd;
    data[1] = tx_do_action;
    memcpy(data + 2, _data, len);
    int err = tws_api_send_data_to_sibling(data, len + 2, 0x076AFE82);
    if (err) {
        data[1] = 2;
        tws_a2dp_play_in_task(data);
    } else {
        if (cmd == CMD_A2DP_PLAY) {
            memcpy(g_play_addr, _data, 6);
        } else if (cmd == CMD_A2DP_SLIENCE_DETECT) {
            memcpy(g_slience_addr, _data, 6);
        }
    }
}

void tws_a2dp_sync_play(u8 *bt_addr, bool tx_do_action)
{
    u8 data[8];
    memcpy(data, bt_addr, 6);
    data[6] = bt_get_music_volume(bt_addr);
    if (data[6] > 127) {
        data[6] = app_audio_bt_volume_update(bt_addr, APP_AUDIO_STATE_MUSIC);
    }
    tws_a2dp_play_send_cmd(CMD_A2DP_PLAY, data, 7, tx_do_action);
}

void tws_a2dp_slience_detect(u8 *bt_addr, bool tx_do_action)
{
    tws_a2dp_play_send_cmd(CMD_A2DP_SLIENCE_DETECT, bt_addr, 6, tx_do_action);
}
static void avrcp_vol_chance_timeout(void *priv)
{
    g_avrcp_vol_chance_timer = 0;
    tws_a2dp_play_send_cmd(CMD_SET_A2DP_VOL, g_avrcp_vol_chance_data, 7, 1);
}

static int a2dp_bt_status_event_handler(int *event)
{
    int ret;
    u8 data[8];
    u8 btaddr[6];
    struct bt_event *bt = (struct bt_event *)event;

    switch (bt->event) {
    case BT_STATUS_A2DP_MEDIA_START:
        g_printf("BT_STATUS_A2DP_MEDIA_START\n");
        put_buf(bt->args, 6);
        if (app_var.goto_poweroff_flag) {
            break;
        }
        if (tws_api_get_role() == TWS_ROLE_MASTER &&
            bt_get_call_status_for_addr(bt->args) == BT_CALL_INCOMING) {
            //小米11来电挂断偶现没有hungup过来，hfp链路异常，重新断开hfp再连接
            puts("<<<<<<<<waring a2dp start hfp_incoming\n");
            bt_cmd_prepare_for_addr(bt->args, USER_CTRL_HFP_DISCONNECT, 0, NULL);
            bt_cmd_prepare_for_addr(bt->args, USER_CTRL_HFP_CMD_CONN, 0, NULL);
        }
        if (esco_player_runing()) {
            r_printf("esco_player_runing");
            a2dp_media_close(bt->args);
            break;
        }
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            break;
        }

        if (a2dp_player_get_btaddr(btaddr)) {
            if (memcmp(btaddr, bt->args, 6) == 0) {
                tws_a2dp_sync_play(bt->args, 1);
            } else {
                tws_a2dp_slience_detect(bt->args, 1);
            }
        } else {
            tws_a2dp_sync_play(bt->args, 1);
        }
        break;
    case BT_STATUS_A2DP_MEDIA_STOP:
        g_printf("BT_STATUS_A2DP_MEDIA_STOP\n");
        put_buf(bt->args, 6);
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            break;
        }
        tws_a2dp_play_send_cmd(CMD_A2DP_CLOSE, bt->args, 6, 1);
        break;
    case BT_STATUS_AVRCP_VOL_CHANGE:
        puts("BT_STATUS_AVRCP_VOL_CHANGE\n");
        //判断是当前地址的音量值才更新
        extern void clock_refurbish(void);
        clock_refurbish();
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            break;
        }
        ret = a2dp_player_get_btaddr(data);
        if (ret && memcmp(data, bt->args, 6) == 0) {
            data[6] = bt->value;

            memcpy(g_avrcp_vol_chance_data, data, 7);
            if (g_avrcp_vol_chance_timer) {
                sys_timer_modify(g_avrcp_vol_chance_timer, 100);
            } else {
                g_avrcp_vol_chance_timer = sys_timeout_add(NULL, avrcp_vol_chance_timeout, 100);
            }
        }
        break;
    case BT_STATUS_AVDTP_START:
        g_printf("BT_STATUS_AVDTP_START\n");
        break;
    }
    return 0;
}
APP_MSG_HANDLER(a2dp_stack_msg_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = a2dp_bt_status_event_handler,
};


static int a2dp_bt_hci_event_handler(int *event)
{
    struct bt_event *bt = (struct bt_event *)event;

    switch (bt->event) {
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        tws_a2dp_player_close(bt->args);
        break;
    }

    return 0;
}
APP_MSG_HANDLER(a2dp_hci_msg_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_HCI,
    .handler    = a2dp_bt_hci_event_handler,
};

static int a2dp_app_msg_handler(int *msg)
{
    u8 *bt_addr = (u8 *)(msg +  1);

    switch (msg[0]) {
    case APP_MSG_BT_A2DP_PAUSE:
        puts("app_msg_bt_a2dp_pause\n");
        if (a2dp_player_is_playing(bt_addr)) {
            tws_a2dp_slience_detect(bt_addr, 1);
        }
        break;
    case APP_MSG_BT_A2DP_STOP:
        puts("APP_MSG_BT_A2DP_STOP\n");
        tws_a2dp_play_send_cmd(CMD_A2DP_CLOSE, bt_addr, 6, 1);
        break;
    case APP_MSG_BT_A2DP_PLAY:
        puts("APP_MSG_BT_A2DP_PLAY\n");
        tws_a2dp_sync_play(bt_addr, 1);
        break;
    }
    return 0;
}
APP_MSG_HANDLER(a2dp_app_msg_handler_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = a2dp_app_msg_handler,
};

extern u8 tws_monitor_start;
static int a2dp_tws_msg_handler(int *msg)
{
    struct tws_event *evt = (struct tws_event *)msg;
    u8 role = evt->args[0];
    u8 state = evt->args[1];
    u8 *bt_addr = evt->args + 3;
    u8 addr[6];

    switch (evt->event) {
    case TWS_EVENT_MONITOR_START:
        y_printf("TWS_EVENT_MONITOR_START");
        tws_monitor_start = 1;
        if (role == TWS_ROLE_SLAVE) {
            break;
        }
        if (a2dp_player_is_playing(bt_addr)) {
            if (memcmp(g_slience_addr, bt_addr, 6)) {
                puts("--monitor_start_play\n");
                tws_a2dp_sync_play(bt_addr, 0);
            }
        } else {
            int result = bt_slience_detect_get_result(bt_addr);
            if (result != BT_SLIENCE_NO_DETECTING) {
                if (memcmp(g_play_addr, bt_addr, 6)) {
                    puts("--monitor_start_slience_detect\n");
                    tws_a2dp_slience_detect(bt_addr,  0);
                }
            }
        }
        break;
    case TWS_EVENT_PHONE_LINK_DETACH:
        tws_a2dp_player_close(evt->args + 3);
        break;
    case TWS_EVENT_ROLE_SWITCH:
        while (bt_slience_get_detect_addr(addr)) {
            bt_stop_a2dp_slience_detect(addr);
            a2dp_media_close(addr);
        }
        break;
    }
    return 0;
}
APP_MSG_HANDLER(a2dp_tws_msg_handler_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = a2dp_tws_msg_handler,
};

int a2dp_audio_msg_handler(int *msg)
{
    u8 addr[6];

    switch (msg[0]) {
    case AUDIO_EVENT_A2DP_NO_ENERGY:
        g_printf("AUDIO_EVENT_A2DP_NO_ENERGY\n");
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            break;
        }
        if (a2dp_player_get_btaddr(addr)) {
            tws_a2dp_play_send_cmd(CMD_A2DP_SWITCH, addr, 6, 1);
        }
        break;
    }

    return 0;
}

APP_MSG_HANDLER(a2dp_audio_msg_handler_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_AUDIO,
    .handler    = a2dp_audio_msg_handler,
};


#if 1
static void tws_low_latency_tone_cb(int fname_uuid, enum stream_event event)
{
    y_printf("tws_low_latency_tone_cb, event:%d", event);
    if (event == STREAM_EVENT_START) {
        int uuid = tone_player_get_fname_uuid(get_tone_files()->low_latency_in);
        y_printf("uuid: %d, fname_uuid: %d", uuid, fname_uuid);
        if (uuid == fname_uuid) {
            tws_api_low_latency_enable(1);
            a2dp_player_low_latency_enable(1);
            g_printf("tws_api_low_latency_enable 1");
        } else {
            tws_api_low_latency_enable(0);
            a2dp_player_low_latency_enable(0);
            g_printf("tws_api_low_latency_enable 0");
        }
    }
};
REGISTER_TWS_TONE_CALLBACK(tws_low_latency_entry) = {
    .func_uuid = 0x6F90E37B,
    .callback = tws_low_latency_tone_cb,
};

void bt_set_low_latency_mode(int enable, u8 tone_play_enable, int delay_ms)
{
    /*
     * 未连接手机,操作无效
     */
    int state = tws_api_get_tws_state();
    if (state & TWS_STA_PHONE_DISCONNECTED) {
        return;
    }

    const char *fname = enable ? get_tone_files()->low_latency_in :
                        get_tone_files()->low_latency_out;
    g_printf("bt_set_low_latency_mode=%d\n", enable);
    if (state & TWS_STA_SIBLING_CONNECTED) {
        if (delay_ms < 100) {
            delay_ms = 100;
        }
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            tws_play_tone_file_alone_callback(fname, delay_ms, 0x6F90E37B);
        }
    } else  {
        play_tone_file_alone(fname);
        tws_api_low_latency_enable(enable);
        a2dp_player_low_latency_enable(enable);
    }
    if (enable) {
        if (bt_get_total_connect_dev()) {
            lmp_hci_write_scan_enable(0);
        }

    }
    user_app_info.game_mode = enable;
    user_app_data_notify(CMD_NOTIFY_GAME_MODE);

}

int bt_get_low_latency_mode()
{
    return tws_api_get_low_latency_state();
}

#endif

