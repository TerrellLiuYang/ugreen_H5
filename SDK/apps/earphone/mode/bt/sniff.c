#include "btstack/avctp_user.h"

#include "app_config.h"
#include "app_main.h"
#include "earphone.h"
#include "bt_tws.h"
#include "audio_anc.h"
#if (RCSP_ADV_EN)
#include "ble_rcsp_adv.h"
#endif

#define LOG_TAG             "[SNIFF]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

extern USER_VAR user_var;


/*
 *以下情况，关闭sniff
 *(1)通过spp在线调试eq
 *(2)通过spp导出数据
 */
#if APP_ONLINE_DEBUG
#if ((TCFG_AUDIO_DATA_EXPORT_DEFINE == AUDIO_DATA_EXPORT_VIA_SPP) || \
    TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE || USE_DMA_UART_TEST)
/*!!! 不要使用MY_SNIFF_EN宏去控制sniff开关，这样会导致协议栈状态错误!!!*/
static u8 sniff_enable = 0;
#else
static u8 sniff_enable = 1;
#endif
#else
static u8 sniff_enable = 1;
#endif

//#define SNIFF_CNT_TIME                  TCFG_SNIFF_CHECK_TIME * 60  //空闲6S之后进入sniff模式
#define SNIFF_CNT_TIME                  300  //空闲5min之后进入sniff模式
#define SNIFF_MAX_INTERVALSLOT          800
#define SNIFF_MIN_INTERVALSLOT          100
#define SNIFF_ATTEMPT_SLOT              4
#define SNIFF_TIMEOUT_SLOT              1


u8 sniff_ready_status = 0; //0:sniff_ready 1:sniff_not_ready


bool bt_is_sniff_close(void)
{
    return (g_bt_hdl.sniff_timer == 0);
}

void bt_check_exit_sniff()
{
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }
    bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
}

void bt_sniff_ready_clean(void)
{
    sniff_ready_status = 1;
}

void bt_check_enter_sniff()
{
    // return;
    //  y_printf("bt_check_enter_sniff");
    u8 addr[12];
    // put_buf(addr, 12);
    struct sniff_ctrl_config_t config;
#if TCFG_BT_SNIFF_ENABLE

#if TCFG_AUDIO_ANC_ENABLE
    if (anc_train_open_query() || anc_online_busy_get()) { //如果ANC训练则不进入SNIFF
        return;
    }
#endif

#if 0//(RCSP_ADV_EN)
    y_printf("get_ble_adv_modify() = %d", get_ble_adv_modify());
    y_printf("get_ble_adv_notify() = %d", get_ble_adv_notify());
    if (get_ble_adv_modify() || get_ble_adv_notify()) {
        return;
    }
#endif
    //  y_printf("sniff_ready_status = %d", sniff_ready_status);
    if (sniff_ready_status) {
        sniff_ready_status = 0;
        return;
    }

    if (bt_get_esco_coder_busy_flag()) {
        return;
    }

    int conn_cnt = bt_api_enter_sniff_status_check(SNIFF_CNT_TIME, addr);
    ASSERT(conn_cnt <= 2);
    for (int i = 0; i < conn_cnt; i++) {
        log_info("-----USER SEND SNIFF IN %d %d\n", i, conn_cnt);
        config.sniff_max_interval = SNIFF_MAX_INTERVALSLOT;
        config.sniff_mix_interval = SNIFF_MIN_INTERVALSLOT;
        config.sniff_attemp = SNIFF_ATTEMPT_SLOT;
        config.sniff_timeout  = SNIFF_TIMEOUT_SLOT;
        memcpy(config.sniff_addr, addr + i * 6, 6);
        bt_cmd_prepare(USER_CTRL_SNIFF_IN, sizeof(config), (u8 *)&config);
    }
#endif
}

void sys_auto_sniff_controle(u8 enable, u8 *addr)
{
    if (addr) {
        if (bt_api_conn_mode_check(enable, addr) == 0) {
            log_info("sniff ctr not change\n");
            return;
        }
    }

    if (enable) {

        if (!sniff_enable) {
            //sniff_enable为0时不启动定时器去检测进入sniff
            return;
        }

        if (tws_api_get_role_async() == TWS_ROLE_SLAVE) {
            return;
        }
        if (g_bt_hdl.sniff_timer == 0) {
            log_info("check_sniff_enable\n");
            g_bt_hdl.sniff_timer = sys_timer_add(NULL, bt_check_enter_sniff, 1000);
        }
    } else {
        if (g_bt_hdl.sniff_timer) {
            log_info("check_sniff_disable\n");
            sys_timeout_del(g_bt_hdl.sniff_timer);
            g_bt_hdl.sniff_timer = 0;
        }
    }
}

void bt_sniff_enable()
{
    sniff_enable = 1;
    sys_auto_sniff_controle(1, NULL);
}

void bt_sniff_disable()
{
    sys_auto_sniff_controle(0, NULL);
    bt_check_exit_sniff();
    sniff_enable = 0;
}

void bt_sniff_feature_init()
{
#if TCFG_BT_SNIFF_ENABLE == 0
    u8 feature = lmp_hci_read_local_supported_features(0);
    feature &= ~BIT(7); //BIT_SNIFF_MODE;
    lmp_hci_write_local_supported_features(feature, 0);
#endif
}

static int sniff_btstack_event_handler(int *_event)
{
    struct bt_event *bt = (struct bt_event *)_event;

    switch (bt->event) {
    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        sys_auto_sniff_controle(1, bt->args);
        break;
    case BT_STATUS_SNIFF_STATE_UPDATE:
        log_info(" BT_STATUS_SNIFF_STATE_UPDATE %d\n", bt->value);    //0退出SNIFF
        user_var.sniff_status = bt->value;
        extern u8 role_switch_after_exit_sniff;
        if (bt->value == 0) {
            sys_auto_sniff_controle(1, bt->args);
            app_send_message(APP_MSG_BT_EXIT_SNIFF, 0);
        } else {
            sys_auto_sniff_controle(0, bt->args);
            app_send_message(APP_MSG_BT_ENTER_SNIFF, 0);
        }
        if (bt->value == 0 && role_switch_after_exit_sniff) {
            tws_api_role_switch();
        } 
        break;
    }
    return 0;
}
APP_MSG_HANDLER(sniff_btstack_msg_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = sniff_btstack_event_handler,
};



static int sniff_tws_event_handler(int *_event)
{
    struct tws_event *event = (struct tws_event *)_event;
    int role = event->args[0];
    int reason = event->args[2];

    if (app_var.goto_poweroff_flag) {
        return 0;
    }

    switch (event->event) {
    case TWS_EVENT_CONNECTION_DETACH:
        break;
    case TWS_EVENT_ROLE_SWITCH:
        if (role == TWS_ROLE_MASTER) {
            sys_auto_sniff_controle(1, NULL);
        } else {
            sys_auto_sniff_controle(0, NULL);
        }
        break;
    }

    return 0;
}

APP_MSG_HANDLER(sniff_tws_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = sniff_tws_event_handler,
};
