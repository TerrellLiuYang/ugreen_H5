#include "key_driver.h"
#include "app_msg.h"
#include "btstack/avctp_user.h"


extern void volume_up(u8 inc);
extern void volume_down(u8 inc);

void bt_key_call_last_on()
{
    if (bt_get_call_status() == BT_CALL_INCOMING) {
        bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        return;
    }

    if ((bt_get_call_status() == BT_CALL_ACTIVE) ||
        (bt_get_call_status() == BT_CALL_OUTGOING) ||
        (bt_get_call_status() == BT_CALL_ALERT) ||
        (bt_get_call_status() == BT_CALL_INCOMING)) {
        return;//通话过程不允许回拨
    }

    bt_cmd_prepare(USER_CTRL_HFP_CALL_LAST_NO, 0, NULL);
}

int app_default_msg_handler(int *msg)
{
    int ret = false;  //默认不拦截消息

    switch (msg[0]) {
    case APP_MSG_VOL_UP:
        printf("%s APP_MSG_VOL_UP\n", __FUNCTION__);
        volume_up(1);
        return 0;
    case APP_MSG_VOL_DOWN:
        printf("%s APP_MSG_VOL_DOWN\n", __FUNCTION__);
        volume_down(1);
        return 0;
    }


    /* 下面是蓝牙相关消息,从机不用处理  */
#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return 0;
    }
#endif
    switch (msg[0]) {
    case APP_MSG_MUSIC_PP:
        printf("%s APP_MSG_MUSIC_PP\n", __FUNCTION__);
        bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
        break;
    case APP_MSG_MUSIC_NEXT:
        printf("%s APP_MSG_MUSIC_NEXT\n", __FUNCTION__);
        bt_cmd_prepare(USER_CTRL_AVCTP_OPID_NEXT, 0, NULL);
        break;
    case APP_MSG_MUSIC_PREV:
        printf("%s APP_MSG_MUSIC_PREV\n", __FUNCTION__);
        bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PREV, 0, NULL);
        break;
    case APP_MSG_CALL_ANSWER:
        printf("%s APP_MSG_CALL_ANSWER\n", __FUNCTION__);
        if (bt_get_call_status() == BT_CALL_INCOMING) {
            printf("send call answer cmd\n");
            bt_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
        }
        break;
    case APP_MSG_CALL_HANGUP:
        printf("%s APP_MSG_CALL_HANGUP\n", __FUNCTION__);
        if ((bt_get_call_status() >= BT_CALL_INCOMING) &&
            (bt_get_call_status() <= BT_CALL_ALERT)) {
            printf("send call hangup cmd\n");
            bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        }
        break;
    case APP_MSG_CALL_LAST_NO:
        printf("%s APP_MSG_CALL_LAST_NO\n", __FUNCTION__);
        bt_key_call_last_on();
        break;
    case APP_MSG_HID_CONTROL:
        // 拍照
        printf("%s APP_MSG_HID_CONTROL\n", __FUNCTION__);
        if (bt_get_curr_channel_state() & HID_CH) {
            printf("send USER_CTRL_HID_IOS\n");
            bt_cmd_prepare(USER_CTRL_HID_IOS, 0, NULL);
        }
        break;
    default:
        break;
    }

    return ret;
}

// APP_MSG_HANDLER(app_default_msg_entry) = {
//     .owner      = 0xff,
//     .from       = MSG_FROM_APP,
//     .handler    = app_default_msg_handler,
// };


