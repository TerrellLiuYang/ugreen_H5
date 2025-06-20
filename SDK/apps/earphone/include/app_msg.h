#ifndef APP_MSG_H
#define APP_MSG_H


#include "os/os_api.h"

enum {
    MSG_FROM_KEY =  Q_MSG + 1,

    MSG_FROM_TWS,
    MSG_FROM_BT_STACK,
    MSG_FROM_BT_HCI,

    MSG_FROM_EARTCH,

    MSG_FROM_BATTERY,
    MSG_FROM_CHARGE_STORE,
    MSG_FROM_TESTBOX,
    MSG_FROM_ANCBOX,
    MSG_FROM_CHARGE_CLIBRATION,

    MSG_FROM_TONE,

    MSG_FROM_APP,

    MSG_FROM_AUDIO,

    MSG_FROM_OTA,

    MSG_FROM_CI_UART,
    MSG_FROM_CDC,
    MSG_FROM_CFGTOOL_TWS_SYNC,

    MSG_FROM_DEVICE,

    MSG_FROM_RCSP,
    MSG_FROM_RCSP_BT,
    MSG_FROM_TWS_UPDATE_NEW,

    MSG_FROM_IOT,
};


struct app_msg_handler {
    int owner;
    int from;
    int (*handler)(int *msg);
};


#define APP_MSG_PROB_HANDLER(msg_handler) \
    const struct app_msg_handler msg_handler sec(.app_msg_prob_handler)

extern const struct app_msg_handler app_msg_prob_handler_begin[];
extern const struct app_msg_handler app_msg_prob_handler_end[];

#define for_each_app_msg_prob_handler(p) \
    for (p = app_msg_prob_handler_begin; p < app_msg_prob_handler_end; p++)


#define APP_MSG_HANDLER(msg_handler) \
    const struct app_msg_handler msg_handler sec(.app_msg_handler)

extern const struct app_msg_handler app_msg_handler_begin[];
extern const struct app_msg_handler app_msg_handler_end[];

#define for_each_app_msg_handler(p) \
    for (p = app_msg_handler_begin; p < app_msg_handler_end; p++)



#define APP_KEY_MSG_FROM_TWS    1

#define APP_MSG_KEY    0x010000

#define APP_MSG_FROM_KEY(msg)   (msg & APP_MSG_KEY)

#define APP_MSG_KEY_VALUE(msg)  ((msg >> 8) & 0xff)

#define APP_MSG_KEY_ACTION(msg)  (msg & 0xff)


#define APP_KEY_MSG_REMAP(key_value, key_action) \
            (APP_MSG_KEY | (key_value << 8) | key_action)


enum {
    APP_MSG_NULL = 0,

    APP_MSG_WRITE_RESFILE_START,
    APP_MSG_WRITE_RESFILE_STOP,

    APP_MSG_MUSIC_PP,
    APP_MSG_MUSIC_PREV,
    APP_MSG_MUSIC_NEXT,
    APP_MSG_CALL_HANGUP,
    APP_MSG_CALL_LAST_NO,
    APP_MSG_OPEN_SIRI,
    APP_MSG_VOL_UP,
    APP_MSG_VOL_DOWN,
    APP_MSG_MAX_VOL,
    APP_MSG_MIN_VOL,
    APP_MSG_LOW_LANTECY,
    APP_MSG_POWER_KEY_LONG,
    APP_MSG_POWER_KEY_HOLD,
    APP_MSG_ANC_SWITCH,

    APP_MSG_SYS_TIMER,

    APP_MSG_POWER_ON,
    APP_MSG_POWER_OFF,
    APP_MSG_GOTO_MODE,
    APP_MSG_GOTO_NEXT_MODE,
    APP_MSG_ENTER_MODE,
    APP_MSG_EXIT_MODE,

    APP_MSG_BT_GET_CONNECT_ADDR,
    APP_MSG_BT_OPEN_PAGE_SCAN,
    APP_MSG_BT_CLOSE_PAGE_SCAN,
    APP_MSG_BT_ENTER_SNIFF,
    APP_MSG_BT_EXIT_SNIFF,
    APP_MSG_BT_A2DP_PAUSE,
    APP_MSG_BT_A2DP_STOP,
    APP_MSG_BT_A2DP_PLAY,
    APP_MSG_BT_A2DP_START,
    APP_MSG_BT_PAGE_DEVICE,
    APP_MSG_BT_IN_PAIRING_MODE,

    APP_MSG_TWS_PAIRED,
    APP_MSG_TWS_UNPAIRED,
    APP_MSG_TWS_PAIR_SUSS,
    APP_MSG_TWS_CONNECTED,
    APP_MSG_TWS_WAIT_PAIR,                      // 等待配对
    APP_MSG_TWS_START_PAIR,                     // 手动发起配对
    APP_MSG_TWS_START_CONN,                     // 开始回连TWS
    APP_MSG_TWS_WAIT_CONN,                      // 等待TWS连接
    APP_MSG_TWS_START_PAIR_AND_CONN_PROCESS,    // 执行配对和连接的默认流程
    APP_MSG_TWS_POWERON_PAIR_TIMEOUT,           // TWS开机配对超时
    APP_MSG_TWS_POWERON_CONN_TIMEOUT,           // TWS开机回连超时
    APP_MSG_TWS_START_PAIR_TIMEOUT,
    APP_MSG_TWS_START_CONN_TIMEOUT,

    APP_MSG_IMU_TRIM_START,                     //开始陀螺仪校准
    APP_MSG_IMU_TRIM_STOP,                      //关闭陀螺仪校准

    APP_MSG_SMART_VOICE_EVENT,

    APP_MSG_KEY_TONE,                           //播放按键音

    APP_MSG_POWER_KEY_ONCE_CLICK,
    APP_MSG_POWER_KEY_DOUBLE_CLICK,
    APP_MSG_POWER_KEY_THIRD_CLICK,
    APP_MSG_POWER_KEY_FOURTH_CLICK,
    APP_MSG_POWER_KEY_FIVETH_CLICK,
    APP_MSG_CALL_ANSWER,						// 接听电话
    APP_MSG_HID_CONTROL,						// 拍照

};


struct key_remap_table {
    u8 key_value;
    const int *remap_table;
    int (*remap_func)(int *event);
};

struct VBAT_V_TO_LEVEL {
    u16 upper;
    u16 lower;
    u8 level;
};


typedef struct {
    u8 ear_id;
    u8 ear_color;
    u8 find_ear_en;
    u8 find_ear_lr;
    u8 anc_mode;
    u8 eq_mode;
    u8 u1t2_mode;
    u8 game_mode;
    u8 hifi_mode;
    u8 tone_language;
    u8 volume_protect;
    u8 app_key_lr[8];
} APP_INFO;

typedef struct {
    u8 ear_charge_online_l;
    u8 ear_charge_online_r;
    u8 box_charge_online;
    u8 ear_bat_percent_l;
    u8 ear_bat_percent_r;
    u8 box_bat_percent;
    u8 box_bat_percent_local;
    u8 box_bat_percent_sibling;

    u8 local_detail_percent;
    u8 remote_detail_percent;
} CHARGE_INFO;


typedef struct {
    u8 voice_mute_en;
    u8 tone_volume_fix;
    u8 spp_conn_status;
    u8 ble_conn_status;
    u8 out_of_range;
    u8 enter_pairing_mode;
    u8 is_ota_mode;
    u8 cmd_enter_poweroff;
    u8 slave_reconn;
    u8 tws_poweron_status;
    u8 box_enter_pair_mode;
    u8 sniff_status;
    u8 restore_flag;
    u8 role_switch_disable;
    u8 find_ear_status_l;
    u8 find_ear_status_r;
    u8 find_ear_status_tws;
    u8 dut_mode;
} USER_VAR;

typedef struct {
    u8 active_addr[6];
    u8 idle_addr[6];
    u8 reconn_addr1[6];
    u8 reconn_addr2[6];
}BT_DEV_ADDR;

typedef struct {
    u8 spp_addr1[6];
    u8 spp_addr2[6];
}APP_SPP_ADDR;

struct PHONE_INFO {
    u8 name_len;
    u8 addr[6];
    u8 *remote_name
} __attribute__((packed));

#define CMD_NOTIFY_BAT_VALUE         0x01
#define CMD_NOTIFY_GAME_MODE         0x04
#define CMD_NOTIFY_EQ_MODE           0x05
#define CMD_NOTIFY_ROLE_SWITCH       0x08
#define CMD_NOTIFY_SEARCH_EAR        0x09
#define CMD_NOTIFY_DEVICE_CHANGE     0x0D
#define CMD_NOTIFY_TWS_RSSI          0x0F



#define CMD_SYNC_VER_INFO               0xF1
#define CMD_SYNC_PHONE_INFO             0xF2

#define CMD_SYNC_SET_APP_KEY            0xF3
#define CMD_SYNC_SET_1T2_MODE           0xF4
#define CMD_SYNC_SET_GAME_MODE          0xF5
#define CMD_SYNC_SET_EQ_MODE            0xF6
#define CMD_SYNC_SET_LANGUAGE           0xF7
#define CMD_SYNC_SEARCH_EAR             0xF8
#define CMD_SYNC_RESTORE                0xF9

// #define CMD_SYNC_PAGE_TIMEOUT           0xFA
#define CMD_BOX_ENTER_PAIR_MODE         0xFA
#define CMD_SYNC_ENTER_LOWPOWER_MODE    0xFB
#define CMD_SYNC_ENTER_PAIR_MODE        0xFC
#define CMD_SYNC_BOX_BAT                0xFD
#define CMD_SYNC_STOP_FIND_EAR          0xFE
#define CMD_SYNC_FIND_EAR_STATUS        0xFF

#define CMD_SYNC_EXIT_RECONN            0xE1
#define CMD_SYNC_CPU_RESET              0xE2
#define CMD_SYNC_RECONN_TIMEOUT_MODE    0xE3
#define CMD_SYNC_SPP_ADDR               0xE4

// #define CMD_SYNC_USER_VAR            0xF8
// #define CMD_SYNC_SEARCH_EAR_STATUS   0xF9
// #define CMD_SYNC_ENTER_PAIRING_MODE  0xFA
// #define CMD_SYNC_BOX_BAT             0xF6
// #define CMD_SYNC_SET_KEY_FLAG        0xF7


void app_send_message(int msg, int arg);

void app_send_message_from(int from, int argc, int *msg);

int app_key_event_remap(int *_event);



#endif

