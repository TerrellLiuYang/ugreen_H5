#include "key_driver.h"
#include "app_main.h"
#include "init.h"
#include "btstack/avctp_user.h"
#include "classic/tws_api.h"
#include "led/led_ui_manager.h"
#include "bt_tws.h"

#if 1
#define key_log(x, ...)       g_printf("[USER_KEY]" x " ", ## __VA_ARGS__)
#define key_log_hexdump       g_printf("[USER_KEY]key_log_hexdump:");\
                                put_buf
#else
#define key_log(...)
#define key_log_hexdump(...)
#endif

#define SECOND_CALL_CCWA_COME     BIT(0)
#define SECOND_CALL_IN            BIT(1)//第三方打入
#define SECOND_CALL_OUT           BIT(2)//第三方打出
#define SECOND_CALL_HOLD          BIT(3)//第三方保持

extern APP_INFO user_app_info;
extern USER_VAR user_var;
extern BT_DEV_ADDR user_bt_dev_addr;

#define CUSTOM_KEY_NUM      4   //按键可以自定义的数量，单击/双击/三击/长按
#define CUSTOM_KEY_CH       2   //自定义按键的声道数

enum {
    ONE_KEY_CTL_NEXT_PREV = 1,
    ONE_KEY_CTL_VOL_UP_DOWN,
    ONE_KEY_CTL_SIRI_GAME,
};

enum {
    FUNCTION_CLOSE          = 0,
    FUNCTION_PLAY_PAUSE     = 1,
    FUNCTION_VOLUME_UP      = 2,
    FUNCTION_VOLUME_DOWN    = 3,
    FUNCTION_FORWARD        = 4,
    FUNCTION_BACKWARD       = 5,
    FUNCTION_SIRI           = 6,
    FUNCTION_GAME_MODE      = 7,
    FUNCTION_ANC_MODE       = 8,
    FUNCTION_ANSWER_CALL    = 9,
    FUNCTION_HANGUP_CALL    = 10,
};

static u8 custom_key[CUSTOM_KEY_NUM * CUSTOM_KEY_CH] = {
    //单击                  双击                        三击                    长按
    FUNCTION_CLOSE,     FUNCTION_PLAY_PAUSE,        FUNCTION_SIRI,              FUNCTION_BACKWARD,      //左耳
    FUNCTION_CLOSE,     FUNCTION_PLAY_PAUSE,        FUNCTION_GAME_MODE,         FUNCTION_FORWARD,      //右耳
};
extern int app_default_msg_handler(int *msg);


const int key_power_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_POWER_KEY_ONCE_CLICK,           //短按
    APP_MSG_POWER_KEY_LONG,                 //长按
    APP_MSG_POWER_KEY_HOLD,                 //hold
    APP_MSG_NULL,                           //长按抬起
    APP_MSG_POWER_KEY_DOUBLE_CLICK,         //双击
    APP_MSG_POWER_KEY_THIRD_CLICK,          //三击
    APP_MSG_POWER_KEY_FOURTH_CLICK, 
    APP_MSG_POWER_KEY_FIVETH_CLICK,
};

const int key_next_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_NEXT,     //短按
    APP_MSG_VOL_UP,         //长按
    APP_MSG_VOL_UP,         //hold
    APP_MSG_NULL,           //长按抬起
    APP_MSG_OPEN_SIRI,      //双击
    APP_MSG_NULL,           //三击
};

const int key_prev_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_PREV,     //短按
    APP_MSG_VOL_DOWN,       //长按
    APP_MSG_VOL_DOWN,       //hold
    APP_MSG_NULL,           //长按抬起
    APP_MSG_NULL,           //双击
    APP_MSG_NULL,           //三击
};

const int key_mode_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL,           //短按
    APP_MSG_NULL,           //长按
    APP_MSG_NULL,           //hold
    APP_MSG_NULL,           //长按抬起
    APP_MSG_NULL,           //双击
    APP_MSG_NULL,           //三击
};

const int key_play_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_PP,       //短按
    APP_MSG_NULL,           //长按
    APP_MSG_NULL,           //hold
    APP_MSG_NULL,           //长按抬起
    APP_MSG_NULL,           //双击
    APP_MSG_NULL,           //三击
};


const int key_slider_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_VOL_UP,         //上滑
    APP_MSG_VOL_DOWN,       //下滑
};

#if TCFG_ADKEY_ENABLE
const int adkey_msg_table[10][KEY_ACTION_MAX] = {
    //短按, 长按, hold, 长按抬起,
    //双击, 3击, 4击, 5击,
    //按住3s, 按住5s
    [0] = {
        APP_MSG_MUSIC_PP,   APP_MSG_CALL_HANGUP,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_POWER_OFF,
    },
    [1] = {
        APP_MSG_MUSIC_PREV, APP_MSG_VOL_DOWN,   APP_MSG_VOL_DOWN,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,
    },
    [2] = {
        APP_MSG_MUSIC_NEXT, APP_MSG_VOL_UP,   APP_MSG_VOL_UP,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,
    },
    [3] = {
        APP_MSG_LOW_LANTECY,   APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,
    },
    [4] = {
        APP_MSG_ANC_SWITCH, APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,
    },
    [5] = {
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,
    },
    [6] = {
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,
    },
    [7] = {
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,
    },
    [8] = {
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,
    },
    [9] = {
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
        APP_MSG_NULL,       APP_MSG_NULL,
    },
};
#endif

static const struct key_remap_table bt_mode_key_table[] = {
    { KEY_POWER, key_power_msg_table },
    { KEY_NEXT,  key_next_msg_table  },
    { KEY_PREV,  key_prev_msg_table  },
    { KEY_MODE,  key_mode_msg_table  },
    { KEY_PLAY,  key_play_msg_table  },
    { KEY_SLIDER, key_slider_msg_table },

#if TCFG_ADKEY_ENABLE
    { KEY_AD_NUM0, adkey_msg_table[0] },
    { KEY_AD_NUM1, adkey_msg_table[1] },
    { KEY_AD_NUM2, adkey_msg_table[2] },
    { KEY_AD_NUM3, adkey_msg_table[3] },
    { KEY_AD_NUM4, adkey_msg_table[4] },
    { KEY_AD_NUM5, adkey_msg_table[5] },
    { KEY_AD_NUM6, adkey_msg_table[6] },
    { KEY_AD_NUM7, adkey_msg_table[7] },
    { KEY_AD_NUM8, adkey_msg_table[8] },
    { KEY_AD_NUM9, adkey_msg_table[9] },
#endif

    { .key_value = 0xff }
};

int key_event_remap_to_app_msg(const struct key_remap_table *table, int key_event)
{
    int key_value   = APP_MSG_KEY_VALUE(key_event);
    int key_action  = APP_MSG_KEY_ACTION(key_event);

    for (int i = 0; table[i].key_value != 0xff; i++) {
        if (key_value == table[i].key_value) {
            return table[i].remap_table[key_action];
        }
    }
    return APP_MSG_NULL;
}

static void lr_diff_otp_deal(u8 *addr, u8 opt, char channel)
{
    switch (opt) {
    case ONE_KEY_CTL_NEXT_PREV:
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            if (channel == 'L') {
                bt_cmd_prepare_for_addr(addr, USER_CTRL_AVCTP_OPID_PREV, 0, NULL);
            } else if (channel == 'R') {
                bt_cmd_prepare_for_addr(addr, USER_CTRL_AVCTP_OPID_NEXT, 0, NULL);
            } else {
                key_log("this channel is unknown");
            }
        }
        break;
    case ONE_KEY_CTL_SIRI_GAME:
        if (channel == 'L') {
            if (bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) != BT_CALL_HANGUP) {
                if (bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) != BT_SIRI_STATE) {
                    break;
                } 
            }
            bt_sniff_disable();
            user_exit_sniff();
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                if (app_var.siri_stu) {
                    key_log("siri close!!!");
                    bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HFP_GET_SIRI_CLOSE, 0, NULL);
                } else {
                    key_log("siri open!!!\n");
                    bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HFP_GET_SIRI_OPEN, 0, NULL);
                }
            }
            bt_sniff_enable();
        } else if (channel == 'R') {
            if (bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) == BT_CALL_HANGUP) {
                bt_set_low_latency_mode((!bt_get_low_latency_mode()), 1, 300);
            }
        } else {
            key_log("this channel is unknown");
        }
        break;
    default:
        break;
    }
}

void key_tws_lr_diff_deal(u8 *addr, int *msg, u8 opt)
{
    u8 channel = 'U';
    channel = tws_api_get_local_channel();
    if (get_bt_tws_connect_status()) {
        if ('L' == channel) {
            channel = msg[1] == APP_KEY_MSG_FROM_TWS ? 'R' : 'L';
        } else {
            channel = msg[1] == APP_KEY_MSG_FROM_TWS ? 'L' : 'R';
        }
    }
    lr_diff_otp_deal(addr, opt, channel);
}

u8 set_custom_key(u8 *key_table, u16 len)
{
    if (sizeof(custom_key) != len) {
        key_log("table len err, len:%d!!!", len);
        return 0;
    }
    memcpy(custom_key, key_table, sizeof(custom_key));
    return 1;
}

void get_custom_key(u8 *key_table, u16 len)
{
    if (sizeof(custom_key) != len) {
        key_log("table len err, len:%d!!!", len);
        return;
    }
    memcpy(key_table, custom_key, sizeof(custom_key));
}

//自定义按键优先处理。
static int custom_key_msg_handler(int *msg)
{
    int ret = 0; //如果不想要进行后续处理返回1，要进行后续处理返回0
    u8 key_value = APP_MSG_KEY_VALUE(msg[0]);
    u8 key_action = 0;
    switch(APP_MSG_KEY_ACTION(msg[0])) {
    case KEY_ACTION_CLICK:
        key_log("KEY_ACTION_CLICK");
        key_action = 0;
        if (bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) == BT_CALL_HANGUP) {
            if (bt_get_total_connect_dev() < 2) {
                user_key_reconn_start();
            }
        }
        break;
    case KEY_ACTION_DOUBLE_CLICK:
        key_log("KEY_ACTION_DOUBLE_CLICK");
        key_action = 1;
        break;
    case KEY_ACTION_TRIPLE_CLICK:
        key_log("KEY_ACTION_TRIPLE_CLICK");
        key_action = 2;
        break;
    case KEY_ACTION_LONG:
        key_log("KEY_ACTION_LONG");
        key_action = 3;
        break;
    default: //非单击双击三击长按，不处理
        return 0;
    }

    u8 channel = tws_api_get_local_channel() == 'R' ? 1 : 0; //右耳1，左耳0
    if (msg[1] == APP_KEY_MSG_FROM_TWS) {
        channel = !channel; //对方产生的按键
    }
    u8 key_function = custom_key[CUSTOM_KEY_NUM * channel + key_action];
    key_log("app key, key_action:%d, channel:%d, key_function:%d", key_action, channel, key_function);
    put_buf(custom_key, 8);

    ret = 1;
    if ((bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) == BT_CALL_INCOMING) || (bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) == BT_CALL_OUTGOING) || (bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) == BT_CALL_ACTIVE) || (bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) == BT_CALL_ALERT)) { //通话中按键按键处理
        key_log("app key, have phone call now, return!!!");
        return 0;
    }

    switch (key_function) {
    case FUNCTION_CLOSE:
        key_log("FUNCTION_CLOSE");
        break;
    case FUNCTION_PLAY_PAUSE:
        key_log("FUNCTION_PLAY_PAUSE");
        bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
        break;
    case FUNCTION_VOLUME_UP:
        key_log("FUNCTION_VOLUME_UP");
        volume_up(1);
        break;
    case FUNCTION_VOLUME_DOWN:
        key_log("FUNCTION_VOLUME_DOWN");
        volume_down(1);
        break;
    case FUNCTION_FORWARD:
        key_log("FUNCTION_FORWARD");
        bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_AVCTP_OPID_NEXT, 0, NULL);
        break;
    case FUNCTION_BACKWARD:
        key_log("FUNCTION_BACKWARD");
        bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_AVCTP_OPID_PREV, 0, NULL);
        break;
    case FUNCTION_SIRI:
        key_log("FUNCTION_SIRI");
        bt_sniff_disable();
        user_exit_sniff();
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            if (app_var.siri_stu) {
                key_log("siri close!!!");
                bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HFP_GET_SIRI_CLOSE, 0, NULL);
            } else {
                key_log("siri open!!!\n");
                bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HFP_GET_SIRI_OPEN, 0, NULL);
            }
        }
        bt_sniff_enable();
        break;
    case FUNCTION_GAME_MODE:
        key_log("FUNCTION_GAME_MODE");
        key_log("bt_get_low_latency_mode():%d", bt_get_low_latency_mode());
        bt_set_low_latency_mode((!bt_get_low_latency_mode()), 1, 300);
        break;
    case FUNCTION_ANC_MODE:
        key_log("FUNCTION_ANC_MODE");
        break;
    default:
        ret = 0;
        break;
    }

    return ret;
};

extern u8 user_get_active_device_addr(u8 *bt_addr);
static int app_key_msg_handler(int *msg)
{
#if 0
    int remap_msg[2];
    const struct key_remap_table *table;

    if (!APP_MSG_FROM_KEY(msg[0])) {
        return 0;
    }

    if (app_in_mode(APP_MODE_BT)) {
        table = bt_mode_key_table;
    } else {
        return 0;
    }
    remap_msg[0] = key_event_remap_to_app_msg(table, msg[0]);
    remap_msg[1] = msg[1];

#if CONFIG_KEY_SCENE_ENABLE == 0
    return app_default_msg_handler(remap_msg);
#endif
#else
    const struct key_remap_table *table;
    if (!APP_MSG_FROM_KEY(msg[0])) {
        return 0;
    }
    if (app_in_mode(APP_MODE_BT)) {
        table = bt_mode_key_table;
    } else {
        return 0;
    }

    user_get_active_device_addr(user_bt_dev_addr.active_addr);  
    if (custom_key_msg_handler(msg)) {
        key_log("app define, break sdk default func");
        return 0;
    }

    u8 *bt_other_addr = NULL;
    int from_tws = msg[1];
    int key_msg = key_event_remap_to_app_msg(table, msg[0]);
    u8 second_call_status = get_second_call_status();
    switch (key_msg) {
    case APP_MSG_POWER_KEY_ONCE_CLICK:
        key_log("APP_MSG_POWER_KEY_ONCE_CLICK");
        break;
    case APP_MSG_POWER_KEY_DOUBLE_CLICK:
        key_log("APP_MSG_POWER_KEY_DOUBLE_CLICK");
        if ((bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) == BT_CALL_OUTGOING) || (bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) == BT_CALL_ALERT)) {
            key_log("phone call outgoing/alert, hangup!!!");
            bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        } else if (bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) == BT_CALL_INCOMING) {
            key_log("phone incoming, answer!!!");
            bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
        } else if (bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) == BT_CALL_ACTIVE) {
            bt_other_addr = btstack_get_other_dev_addr(user_bt_dev_addr.active_addr);
            if (bt_get_call_status_for_addr(bt_other_addr) == BT_CALL_INCOMING) {
                key_log("A phone active and B phone incomeing answer B!!!");
                bt_cmd_prepare_for_addr(bt_other_addr, USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
            } else if (bt_get_call_status_for_addr(bt_other_addr) == BT_CALL_OUTGOING || bt_get_call_status_for_addr(bt_other_addr) == BT_CALL_ALERT) {
                key_log("A phone active and B phone outgoing/call alert hangup B!!!");
                bt_cmd_prepare_for_addr(bt_other_addr, USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
            } else if (second_call_status == SECOND_CALL_IN) {
                key_log("one phone active, answer the other!!!");
                bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HFP_THREE_WAY_ANSWER2, 0, NULL);
            } else {
                key_log("phone call avtive, hangup!!!");
                bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
            }
        } else if (bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) == BT_CALL_HANGUP) {
            key_log("no phone call now, music play/pause!!!");
            bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
        } else {
            key_log("unknown, bt_get_call_status_for_addr():%d", bt_get_call_status_for_addr(user_bt_dev_addr.active_addr));
        }
        break;
    case APP_MSG_POWER_KEY_THIRD_CLICK:
        key_log("APP_MSG_POWER_KEY_THIRD_CLICK");
        if (bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) == BT_CALL_ACTIVE) {
            if (second_call_status == SECOND_CALL_HOLD) {
                key_log("two phone hold, switch active phone!!!");
                bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HFP_THREE_WAY_ANSWER2, 0, NULL);
                break;
            }
        }
        key_tws_lr_diff_deal(user_bt_dev_addr.active_addr, msg, ONE_KEY_CTL_SIRI_GAME);
        break;
    case APP_MSG_POWER_KEY_LONG:
        key_log("APP_MSG_POWER_KEY_LONG");
        if (bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) == BT_CALL_INCOMING) {
            key_log("phone incoming, reject!!!");
            bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        } else if (bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) == BT_CALL_ACTIVE) {
            key_log("phone active!!!");
            bt_other_addr = btstack_get_other_dev_addr(user_bt_dev_addr.active_addr);
            if (bt_get_call_status_for_addr(bt_other_addr) == BT_CALL_INCOMING) {
                key_log("A phone active and B phone incomeing reject!!!");
                bt_cmd_prepare_for_addr(bt_other_addr, USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
            } else if (second_call_status == SECOND_CALL_IN) {
                key_log("one phone active, reject the other!!!");
                bt_cmd_prepare_for_addr(user_bt_dev_addr.active_addr, USER_CTRL_HFP_THREE_WAY_REJECT, 0, NULL);
            } 
        } else if (bt_get_call_status_for_addr(user_bt_dev_addr.active_addr) == BT_CALL_HANGUP) {
            key_log("no phone call now, music prev/next!!!");
            key_tws_lr_diff_deal(user_bt_dev_addr.active_addr, msg, ONE_KEY_CTL_NEXT_PREV);
        } else {
            key_log("unknown, bt_get_call_status_for_addr():%d", bt_get_call_status_for_addr(user_bt_dev_addr.active_addr));
        }
        break;
    case APP_MSG_POWER_KEY_HOLD:
        key_log("APP_MSG_POWER_KEY_HOLD");
        break;
    case APP_MSG_POWER_KEY_FOURTH_CLICK:
        key_log("APP_MSG_POWER_KEY_FOURTH_CLICK");
        break;
    case APP_MSG_POWER_KEY_FIVETH_CLICK:
        key_log("APP_MSG_POWER_KEY_FIVETH_CLICK");
        //客户取消5击dut
        // user_enter_dut_mode();
        break;
    case APP_MSG_MUSIC_PP:
        break;
    case APP_MSG_MUSIC_NEXT:
        break;
    case APP_MSG_MUSIC_PREV:
        break;
    case APP_MSG_VOL_UP:
        break;
    case APP_MSG_VOL_DOWN:
        break;
    default:
        break;
    }
#endif
    return 0;
}

APP_MSG_HANDLER(app_key_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = app_key_msg_handler,
};

