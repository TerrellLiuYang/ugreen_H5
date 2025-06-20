
#include "app_config.h"
#include "app_action.h"

#include "system/includes.h"
#include "spp_user.h"
#include "string.h"
#include "circular_buf.h"
#include "bt_common.h"
#include "online_db_deal.h"
#include "spp_online_db.h"
#include "classic/tws_api.h"
#include "cfg_tool.h"
#include "btstack/avctp_user.h"
#include "app_msg.h"

#if (TCFG_ANC_TOOL_DEBUG_ONLINE && TCFG_AUDIO_ANC_ENABLE)
#include "app_anctool.h"
#endif


#if 1
extern void printf_buf(u8 *buf, u32 len);
// #define log_info          printf
// #define log_info_hexdump  printf_buf
#define log_info(x, ...)       g_printf("[USER_SPP]" x " ", ## __VA_ARGS__)
#define log_info_hexdump       g_printf("[USER_SPP]spp_log_hexdump:");\
                                put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

// void online_api_spp_recieve_cbk(void *priv, u8 *buf, u16 len)
// {
//     online_spp_recieve_cbk(priv, buf, len);
// }
// void online_api_spp_state_cbk(u8 state)
// {
//     online_spp_state_cbk(state);
// }
// void online_api_spp_send_wakeup(void)
// {
//     online_spp_send_wakeup();
// }

#if APP_ONLINE_DEBUG

static struct db_online_api_t *db_api;
static struct spp_operation_t *spp_api = NULL;
static u8 spp_state;

#define ONLINE_SPP_CONNECT      0x0A
#define ONLINE_SPP_DISCONNECT   0x0B
#define ONLINE_SPP_DATA         0x0C
void tws_online_spp_send(u8 cmd, u8 *_data, u16 len, u8 tx_do_action);
int online_spp_send_data(u8 *data, u16 len)
{
    if (spp_api) {
        /* log_info("spp_api_tx(%d) \n", len); */
        /* log_info_hexdump(data, len); */
        return spp_api->send_data(NULL, data, len);
    }
    return SPP_USER_ERR_SEND_FAIL;
}

int online_spp_send_data_check(u16 len)
{
    if (spp_api) {
        if (spp_api->busy_state()) {
            return 0;
        }
    }
    return 1;
}

static void online_spp_state_cbk(u8 state)
{
    spp_state = state;
    switch (state) {
    case SPP_USER_ST_CONNECT:
        log_info("SPP_USER_ST_CONNECT ~~~\n");
        tws_online_spp_send(ONLINE_SPP_CONNECT, &state, 1, 1);
        break;

    case SPP_USER_ST_DISCONN:
        log_info("SPP_USER_ST_DISCONN ~~~\n");
        tws_online_spp_send(ONLINE_SPP_DISCONNECT, &state, 1, 1);
        break;

    default:
        break;
    }

}

static void online_spp_send_wakeup(void)
{
    /* putchar('W'); */
    db_api->send_wake_up();
}

static void online_spp_recieve_cbk(void *priv, u8 *buf, u16 len)
{
    log_info("spp_api_rx(%d) \n", len);
    /* log_info_hexdump(buf, len); */
    tws_online_spp_send(ONLINE_SPP_DATA, buf, len, 1);
}

void online_spp_init(void)
{
    y_printf("online_spp_init");
    log_info("spp_file: %s", __FILE__);
    spp_state = 0;
    spp_get_operation_table(&spp_api);
    spp_api->regist_recieve_cbk(0, online_spp_recieve_cbk);
    spp_api->regist_state_cbk(0, online_spp_state_cbk);
    spp_api->regist_wakeup_send(NULL, online_spp_send_wakeup);
    db_api = app_online_get_api_table();
}

extern u8 soft_ver_local[3];
extern u8 soft_ver_l[3];
extern u8 soft_ver_r[3];

#define CHECK_AT_CMD(data, STR)   (memcmp(data, STR, strlen(STR)) == 0)
void testbox_set_ex_enter_storage_mode_flag(u8 en);
void sys_enter_soft_poweroff(int reason);
extern CHARGE_INFO user_charge_info;
extern void user_set_default_tone_language(u8 default_tone);
extern u8 user_default_tone_language;
void at_cmd_deal(u8 *data, u16 len)
{
    if(!data){
        return;
    }
    u8 tx_data[32] = {0};
    u8 tx_len;
    if(CHECK_AT_CMD(data, "AT+GETSWVER")){
        tx_len = sprintf(tx_data, "AT+SW_VER,L:%d.%d.%d,R:%d.%d.%d", 
            soft_ver_l[0], soft_ver_l[1], soft_ver_l[2],
            soft_ver_r[0], soft_ver_r[1], soft_ver_r[2]);
        online_spp_send_data(tx_data, tx_len);
        return;
    }

    if(CHECK_AT_CMD(data, "AT+SHIP_MODE")){
        tx_len = sprintf(tx_data, "AT+OK");
        online_spp_send_data(tx_data, tx_len);
        testbox_set_ex_enter_storage_mode_flag(1);
        sys_timeout_add(NULL, sys_enter_soft_poweroff, 1000);
        return;
    }

    if(CHECK_AT_CMD(data, "AT+BATTERY")){
        u8 chargestore_get_vbat_percent(void);
        tx_len = sprintf(tx_data, "AT+BAT=%c,%d",tws_api_get_local_channel(), chargestore_get_vbat_percent());
        online_spp_send_data(tx_data, tx_len);
        return;
    }

    if(CHECK_AT_CMD(data, "AT+POWER_OFF")){
        tx_len = sprintf(tx_data, "AT+OK");
        online_spp_send_data(tx_data, tx_len);
        sys_timeout_add(NULL, sys_enter_soft_poweroff, 1000);
        return;
    }

    if(CHECK_AT_CMD(data, "AT+CLR_BT_TWS")){
        tx_len = sprintf(tx_data, "AT+OK");
        bt_tws_remove_pairs();
        bt_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);
        user_1t1_mode_reconn_addr_reset();
        user_key_reconn_addr_reset();
        online_spp_send_data(tx_data, tx_len);
        return;
    }

    if(CHECK_AT_CMD(data, "AT+CLR_BT")){
        tx_len = sprintf(tx_data, "AT+OK");
        bt_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);
        user_1t1_mode_reconn_addr_reset();
        user_key_reconn_addr_reset();
        online_spp_send_data(tx_data, tx_len);
        return;
    }

    if(CHECK_AT_CMD(data, "AT+CLR_TWS")){
        tx_len = sprintf(tx_data, "AT+OK");
        bt_tws_remove_pairs();
        online_spp_send_data(tx_data, tx_len);
        return;
    }

    if(CHECK_AT_CMD(data, "AT+BTNAME")){
        tx_len = sprintf(tx_data, "AT+BTNM,%s", bt_get_local_name());
        online_spp_send_data(tx_data, tx_len);
        return;
    }

    if(CHECK_AT_CMD(data, "AT+RESET_VBAT")){
        tx_len = sprintf(tx_data, "AT+VBAT,%d", user_charge_info.local_detail_percent);
        set_vm_vbat_percent(user_charge_info.local_detail_percent);
        percent_save_init();
        online_spp_send_data(tx_data, tx_len);
        return;
    }

    if(CHECK_AT_CMD(data, "AT+SET_TONE_0")){
        user_set_default_tone_language(0);
		user_app_info_restore();
        tx_len = sprintf(tx_data, "AT+TONE,%d", user_default_tone_language);
        online_spp_send_data(tx_data, tx_len);
        return;
    }

    if(CHECK_AT_CMD(data, "AT+SET_TONE_1")){
        user_set_default_tone_language(1);
		user_app_info_restore();
        tx_len = sprintf(tx_data, "AT+TONE,%d", user_default_tone_language);
        online_spp_send_data(tx_data, tx_len);
        return;
    }

    if(CHECK_AT_CMD(data, "AT+SET_TONE_2")){
        user_set_default_tone_language(2);
		user_app_info_restore();
        tx_len = sprintf(tx_data, "AT+TONE,%d", user_default_tone_language);
        online_spp_send_data(tx_data, tx_len);
        return;
    }
}

static void tws_online_spp_in_task(u8 *data)
{
    printf("tws_online_spp_in_task");
    u16 data_len = little_endian_read_16(data, 2);
    switch (data[0]) {
    case ONLINE_SPP_CONNECT:
        puts("ONLINE_SPP_CONNECT000\n");
        db_api->init(DB_COM_TYPE_SPP);
        db_api->register_send_data(online_spp_send_data);
#if (TCFG_ANC_TOOL_DEBUG_ONLINE && TCFG_AUDIO_ANC_ENABLE)
        app_anctool_spp_connect();
#endif
        break;
    case ONLINE_SPP_DISCONNECT:
        puts("ONLINE_SPP_DISCONNECT000\n");
        db_api->exit();
#if (TCFG_ANC_TOOL_DEBUG_ONLINE && TCFG_AUDIO_ANC_ENABLE)
        app_anctool_spp_disconnect();
#endif
        break;
    case ONLINE_SPP_DATA:
        puts("ONLINE_SPP_DATA0000\n");
        log_info_hexdump(&data[4], data_len);
        if(memcmp(&data[4], "AT+", 3) == 0){
            at_cmd_deal(&data[4], data_len);
            break;
        }
#if (TCFG_ANC_TOOL_DEBUG_ONLINE && TCFG_AUDIO_ANC_ENABLE)
        if (app_anctool_spp_rx_data(&data[4], data_len)) {
            free(data);
            return;
        }
#endif
#if TCFG_CFG_TOOL_ENABLE
        if (!online_cfg_tool_data_deal(&data[4], data_len)) {
            free(data);
            return;
        }
#endif
        db_api->packet_handle(&data[4], data_len);

        //loop send data for test
        /* if (online_spp_send_data_check(data_len)) { */
        /*online_spp_send_data(&data[4], data_len);*/
        /* } */
        break;
    }
    free(data);
}

static void tws_online_spp_callback(u8 *data, u16 len)
{
    int msg[4];

    u8 *buf = malloc(len);
    if (!buf) {
        return;
    }
    memcpy(buf, data, len);

    msg[0] = (int)tws_online_spp_in_task;
    msg[1] = 1;
    msg[2] = (int)buf;
    os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
}

static void tws_online_spp_data_in_irq(void *_data, u16 len, bool rx)
{
    u8 *data = (u8 *)_data;

    if (data[1] == 0 && !rx) {
        return;
    }
    tws_online_spp_callback(data, len);
}
REGISTER_TWS_FUNC_STUB(tws_online_spp_player_stub) = {
    .func_id = 0x096A5E82,
    .func = tws_online_spp_data_in_irq,
};

void tws_online_spp_send(u8 cmd, u8 *_data, u16 len, u8 tx_do_action)
{
    u8 *data = malloc(len + 4 + 4);
    data[0] = cmd;
    data[1] = tx_do_action;
    little_endian_store_16(data, 2, len);
    memcpy(data + 4, _data, len);
#if TCFG_USER_TWS_ENABLE
    int err = tws_api_send_data_to_sibling(data, len + 4, 0x096A5E82);
    if (err) {
        tws_online_spp_in_task(data);
    } else {
        free(data);
    }
#else
    tws_online_spp_in_task(data);
#endif
}
#endif

