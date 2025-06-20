/*********************************************************************************************
    *   Filename        : ble_rcsp_module.c

    *   Description     :

    *   Author          :

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2022-06-08 11:14

    *   Copyright:(c)JIELI  2011-2022  @ , All Rights Reserved.
*********************************************************************************************/

// *****************************************************************************
// *****************************************************************************
#include "system/includes.h"
#include "rcsp_config.h"

#include "app_action.h"

#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "user_cfg.h"
#include "vm.h"
#include "app_power_manage.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "btstack/avctp_user.h"
#include "app_chargestore.h"

#include "btcrypt.h"
#include "custom_cfg.h"
#include "rcsp_music_info_setting.h"
#include "ble_rcsp_adv.h"
#include "ble_rcsp_server.h"
#include "classic/tws_api.h"
#include "rcsp_update.h"
#include "ble_rcsp_multi_common.h"
#include "btstack/btstack_event.h"
#include "ble_user.h"
#include "classic/tws_api.h"
#include "app_msg.h"
#include "tone_player.h"
#include "app_tone.h"
#include "app_main.h"
#include "multi_protocol_main.h"
#include "JL_rcsp_api.h"
#include "JL_rcsp_protocol.h"
#include "adv_1t2_setting.h"
#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif

#if ASSISTED_HEARING_CUSTOM_TRASNDATA
#include "adv_hearing_aid_setting.h"
#endif

#include "app_config.h"
int app_info_debug_enable = BIT(0) | BIT(2) | BIT(4);
#if 1
#define app_log(x, ...)       g_printf("[USER_APP]" x " ", ## __VA_ARGS__)
#define app_log_hexdump       g_printf("[USER_APP]app_log_hexdump:");\
                                    put_buf
#else
#define app_log(...)
#define app_log_hexdump(...)
#endif
/*************************************************************/
//   APP相关的信息定义
/*************************************************************/
u8 soft_ver_local[3] = {0x07, 0x00, 0x01};  //本地固件版本名改这个数组
u8 soft_ver_l[3] = {0x00, 0x00, 0x00};
u8 soft_ver_r[3] = {0x00, 0x00, 0x00};

u8 *temp_buf1 = NULL;
u8 *temp_buf2 = NULL;
extern u8 addr_all_ff[];
APP_INFO user_app_info = {0x00};
CHARGE_INFO user_charge_info = {0x00};
APP_SPP_ADDR user_app_spp_addr = {0x00};
extern USER_VAR user_var;
extern struct PHONE_INFO user_phone_info1;
extern struct PHONE_INFO user_phone_info2;
extern void user_sys_restore();
extern void get_custom_key(u8 *key_table, u16 len);
extern void user_send_data_to_sibling(u8 *data, u16 len);
extern void user_send_data_to_both(u8 *data, u16 len);
extern u8 *get_spp_working_addr();
extern u8 set_custom_key(u8 *key_table, u16 len);

void user_app_data_deal(u8 *buf, u16 len, u8 type);
void user_app_data_notify(u8 cmd);
static int app_send_user_data_check(u16 len);
static int app_send_user_data_do(void *priv, u8 *data, u16 len);
static int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type);
void user_app_data_transpond(u8 *data, u16 len);
void user_app_dev_data_transpond(u8 *temp_buf1, u16 temp_buf1_len, u8 *temp_buf2, u16 temp_buf2_len, u8 index);
void user_app_dev_data_ble_transpond(u8 *temp_buf, u16 temp_buf_len);
void user_app_dev_data_spp_transpond(u8 *spp_addr, u8 *temp_buf, u16 temp_buf_len);

u16 user_find_ear_cnt = 0;
u16 user_find_ear_timerid = 0;

/**
 *	@brief 用于发送ble/spp自定义数据使用
 *
 *	@param ble_con_hdl ble发送句柄
 *	@param remote_addr spp发送地址
 *	@param buf 发送的数据
 *	@param len 发送的数据长度
 *	@param att_handle ble_con_hdl有值时，可填用户自定义的特征, 为0是rcsp的特征值
 *	@param att_op_type 参考att_op_type_e枚举的排序
 *
 *	注：当ble_con_hdl与remote_addr都不填时，给所有的设备都发数据
 */
extern void bt_rcsp_custom_data_send(u16 ble_con_hdl, u8 *remote_addr, u8 *buf, u16 len, uint16_t att_handle, att_op_type_e att_op_type);

/**
 *	@brief 用于外部接收ble/spp自定义数据使用
 *
 *	@param ble_con_hdl ble发送句柄
 *	@param remote_addr spp发送地址
 *	@param buf 接收数据
 *	@param len 接收数据的长度
 *	@param att_handle ble_con_hdl有值时，ble的特征值，一般是用户自定义的特征
 */
_WEAK_ void bt_rcsp_custom_recieve_callback(u16 ble_con_hdl, void *remote_addr, u8 *buf, u16 len, uint16_t att_handle)
{
    app_log("bt_rcsp_custom_recieve_callback, %d 0x%x", ble_con_hdl, att_handle);
    if (remote_addr == NULL) {
        user_app_data_deal(buf, len, 1);
    } else {
        user_app_data_deal(buf, len, 0);
    }
}


//一些功能用到的UUID
#define USER_UUID_SEND_APP_INFO         0xABCABC01
/*************************************************************/


#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_RCSP_DEMO)

//ANCS profile enable
#define TRANS_ANCS_EN  			  	 0

//AMS profile enable
#define TRANS_AMS_EN  			  	 0

#if (defined(TCFG_UI_ENABLE_NOTICE) && (!TCFG_UI_ENABLE_NOTICE))
#undef TRANS_ANCS_EN
#define TRANS_ANCS_EN  			  	 0

#endif

#if 1
#define log_info(x, ...)       printf("[LE-RCSP]" x " ", ## __VA_ARGS__)
#define log_info_hexdump       put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#define EIR_TAG_STRING   0xd6, 0x05, 0x08, 0x00, 'J', 'L', 'A', 'I', 'S', 'D','K'
static const char user_tag_string[] = {EIR_TAG_STRING};

//------
#define ATT_LOCAL_PAYLOAD_SIZE    (247)//(517)                   //note: need >= 20
#define ATT_SEND_CBUF_SIZE        (512*2)                   //note: need >= 20,缓存大小，可修改
#define ATT_RAM_BUFSIZE           (ATT_CTRL_BLOCK_SIZE + ATT_LOCAL_PAYLOAD_SIZE + ATT_SEND_CBUF_SIZE)                   //note:
static u8 att_ram_buffer[ATT_RAM_BUFSIZE] __attribute__((aligned(4)));

extern void *rcsp_server_ble_hdl;
extern void *rcsp_server_ble_hdl1;
// 获取rcsp已连接设备
extern u8 bt_rcsp_device_conn_num(void);
// 获取当前已连接ble数目
extern u8 bt_rcsp_ble_conn_num(void);
// 获取当前已连接spp数目
extern u8 bt_rcsp_spp_conn_num(void);

//---------------
//---------------
#define ADV_INTERVAL_MIN          (160) //n*0.625ms
/* #define ADV_INTERVAL_MIN          (0x30) */

//加密设置
static const uint8_t sm_min_key_size = 7;

//连接参数设置
static const uint8_t connection_update_enable = 1; ///0--disable, 1--enable
static uint8_t connection_update_cnt = 0; //
static const struct conn_update_param_t connection_param_table[] = {
    {16, 24, 16, 600},
    {12, 28, 14, 600},//11
    {8,  20, 20, 600},//3.7
    /* {12, 28, 4, 600},//3.7 */
    /* {12, 24, 30, 600},//3.05 */
};

static const struct conn_update_param_t connection_param_table_update[] = {
    {96, 120, 0,  600},
    {60,  80, 0,  600},
    {60,  80, 0,  600},
    /* {8,   20, 0,  600}, */
    {6, 12, 0, 400},/*ios 提速*/
};

static u8 conn_update_param_index = 0;
#define CONN_PARAM_TABLE_CNT      (sizeof(connection_param_table)/sizeof(struct conn_update_param_t))
#define CONN_TRY_NUM			  10 // 重复尝试次数

#if (ATT_RAM_BUFSIZE < 64)
#error "adv_data & rsp_data buffer error!!!!!!!!!!!!"
#endif


static char *gap_device_name = "JL_ble_test";
static u8 gap_device_name_len = 0;
volatile static u8 ble_work_state = 0;
static u8 adv_ctrl_en;
static u8 ble_init_flag;

static u8 cur_peer_addr_info[7]; /*当前连接对方地址信息*/
static u8 check_rcsp_auth_flag; /*检查认证是否通过,ios发起回连edr*/
static u8 pair_reconnect_flag;  /*配对回连标记*/

#if TCFG_PAY_ALIOS_ENABLE
static u8 upay_mode_enable;/*upay绑定模式*/
static u8 upay_new_adv_enable;/**/
static void (*upay_recv_callback)(const uint8_t *data, u16 len);
static void upay_ble_new_adv_enable(u8 en);
#endif

static void (*app_recieve_callback)(void *priv, void *buf, u16 len) = NULL;
static void (*ble_resume_send_wakeup)(void) = NULL;
static u32 channel_priv;

extern u8 get_rcsp_connect_status(void);
extern const int config_btctler_le_hw_nums;

/* static const char *const phy_result[] = { */
/*     "None", */
/*     "1M", */
/*     "2M", */
/*     "Coded" */
/* }; */
//------------------------------------------------------
//ANCS
#if TRANS_ANCS_EN
//profile event
#define ANCS_SUBEVENT_CLIENT_CONNECTED                              0xF0
#define ANCS_SUBEVENT_CLIENT_NOTIFICATION                           0xF1
#define ANCS_SUBEVENT_CLIENT_DISCONNECTED                           0xF2

#define ANCS_MESSAGE_MANAGE_EN                                      1
void ancs_client_init(void);
void ancs_client_exit(void);
void ancs_client_register_callback(btstack_packet_handler_t callback);
const char *ancs_client_attribute_name_for_id(int id);
void ancs_set_notification_buffer(u8 *buffer, u16 buffer_size);
u32 get_notification_uid(void);
u16 get_controlpoint_handle(void);
void ancs_set_out_callback(void *cb);

//ancs info buffer
#define ANCS_INFO_BUFFER_SIZE  (1024)
static u8 ancs_info_buffer[ANCS_INFO_BUFFER_SIZE];
#else
#define ANCS_MESSAGE_MANAGE_EN                                      0
#endif

//ams
#if TRANS_AMS_EN
//profile event
#define AMS_SUBEVENT_CLIENT_CONNECTED                               0xF3
#define AMS_SUBEVENT_CLIENT_NOTIFICATION                            0xF4
#define AMS_SUBEVENT_CLIENT_DISCONNECTED                            0xF5

void ams_client_init(void);
void ams_client_exit(void);
void ams_client_register_callback(btstack_packet_handler_t handler);
const char *ams_get_entity_id_name(u8 entity_id);
const char *ams_get_entity_attribute_name(u8 entity_id, u8 attr_id);
#endif
//------------------------------------------------------

//------------------------------------------------------
//广播参数设置
static void advertisements_setup_init();
extern const char *bt_get_local_name();
static int get_buffer_vaild_len(void *priv);
//------------------------------------------------------
static void send_request_connect_parameter(hci_con_handle_t connection_handle, u8 table_index)
{
    struct conn_update_param_t *param = NULL; //static ram
    switch (conn_update_param_index) {
    case 0:
        param = (void *)&connection_param_table[table_index];
        break;
    case 1:
        param = (void *)&connection_param_table_update[table_index];
        break;
    default:
        break;
    }

    log_info("update_request:-%d-%d-%d-%d-\n", param->interval_min, param->interval_max, param->latency, param->timeout);
    if (connection_handle) {
        ble_user_cmd_prepare(BLE_CMD_REQ_CONN_PARAM_UPDATE, 2, connection_handle, param);
    }
}

static void check_connetion_updata_deal(hci_con_handle_t connection_handle)
{
    if (connection_update_enable) {
        if (connection_update_cnt < (CONN_PARAM_TABLE_CNT * CONN_TRY_NUM)) {
            send_request_connect_parameter(connection_handle, connection_update_cnt / CONN_TRY_NUM);
        }
    }
}

/* static void connection_update_complete_success(u8 *packet) */
/* { */
/*     int con_handle, conn_interval, conn_latency, conn_timeout; */
/*  */
/*     con_handle = hci_subevent_le_connection_update_complete_get_connection_handle(packet); */
/*     conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet); */
/*     conn_latency = hci_subevent_le_connection_update_complete_get_conn_latency(packet); */
/*     conn_timeout = hci_subevent_le_connection_update_complete_get_supervision_timeout(packet); */
/*  */
/*     log_info("conn_interval = %d\n", conn_interval); */
/*     log_info("conn_latency = %d\n", conn_latency); */
/*     log_info("conn_timeout = %d\n", conn_timeout); */
/* } */

extern void rcsp_user_event_ble_handler(ble_state_e ble_status, u8 flag);
void set_ble_work_state(ble_state_e state)
{
    log_info("ble_work_st:%x->%x\n", ble_work_state, state);
    ble_work_state = state;
    // 需要放到app_core处理ble状态的函数
    rcsp_user_event_ble_handler(state, 1);
}

static ble_state_e rcsp_get_ble_work_state(void)
{
    return ble_work_state;
}

static void cbk_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    sm_just_event_t *event = (void *)packet;
    u32 tmp32;
    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {

        case SM_EVENT_PAIR_PROCESS:
            log_info("======PAIR_PROCESS: %02x\n", event->data[0]);
            put_buf(event->data, 4);
            switch (event->data[0]) {
            case SM_EVENT_PAIR_SUB_RECONNECT_START:
                pair_reconnect_flag = 1;
                break;

            case SM_EVENT_PAIR_SUB_PIN_KEY_MISS:
            case SM_EVENT_PAIR_SUB_PAIR_FAIL:
            case SM_EVENT_PAIR_SUB_PAIR_TIMEOUT:
            case SM_EVENT_PAIR_SUB_ADD_LIST_SUCCESS:
            case SM_EVENT_PAIR_SUB_ADD_LIST_FAILED:
            default:
                break;
            }
            break;
        }
        break;
    }
}

void rcsp_user_cbk_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    cbk_sm_packet_handler(packet_type, channel, packet, size);
}

static void can_send_now_wakeup(void)
{
    putchar('E');
    if (ble_resume_send_wakeup) {
        ble_resume_send_wakeup();
    }

}

_WEAK_
u8 ble_update_get_ready_jump_flag(void)
{
    return 0;
}

static bool g_close_inquiry_scan = false;
extern void rcsp_clean_update_hdl_for_end_update(u16 ble_con_handle, u8 *spp_remote_addr);
void rcsp_ble_adv_enable_with_con_dev();
/**
 * @brief 一定时间设置是否关闭可发现可连接
 * 			对应TCFG_DUAL_CONN_INQUIRY_SCAN_TIME功能配置
 *
 * @param close_inquiry_scan 是否关闭可发现可连接
 */
void rcsp_close_inquiry_scan(bool close_inquiry_scan)
{
    g_close_inquiry_scan = close_inquiry_scan;
    printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
#if TCFG_USER_TWS_ENABLE
    if ((tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)) {
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            // 根据已连接设备数量判断是否开关蓝牙广播
            rcsp_ble_adv_enable_with_con_dev();
        }
    }
#else
    // 根据已连接设备数量判断是否开关蓝牙广播
    rcsp_ble_adv_enable_with_con_dev();
#endif
}

// 返回当前设备支持的最大连接数
u8 rcsp_max_support_con_dev_num()
{
#if TCFG_RCSP_DUAL_CONN_ENABLE
    // 一拖二
    u8 max_con_dev = 2;
    if (g_close_inquiry_scan) {
        max_con_dev = 1;
    }
    if (!rcsp_get_1t2_switch()) {
        max_con_dev = 1;
    }
#else
    u8 max_con_dev = 1;
#endif
    return max_con_dev;
}

void rcsp_app_ble_set_mac_addr(void *addr)
{
    if (rcsp_server_ble_hdl) {
        app_ble_set_mac_addr(rcsp_server_ble_hdl, addr);
    }
}

// 根据已连接设备数量判断是否开关蓝牙广播
void rcsp_ble_adv_enable_with_con_dev()
{
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    printf("%s, rets=0x%x\n", __FUNCTION__, rets_addr);

    u8 max_con_dev = rcsp_max_support_con_dev_num();
    u8 conn_num = bt_rcsp_device_conn_num();
    y_printf("%s, %s, %d, max:%d, conn_num:%d\n", __FILE__, __FUNCTION__, __LINE__, max_con_dev, conn_num);
#if TCFG_USER_TWS_ENABLE
    // if (get_bt_tws_connect_status() && TWS_ROLE_MASTER == tws_api_get_role()) {
    if (TWS_ROLE_MASTER == tws_api_get_role()) {
        if (conn_num >= max_con_dev) {
            rcsp_bt_ble_adv_enable(0);
        } else {
            ble_module_enable(1);
        }
    } 
#else
    if (conn_num >= max_con_dev) {
        rcsp_bt_ble_adv_enable(0);
    } else {
        ble_module_enable(1);
    }
#endif
}

extern void bt_rcsp_set_conn_info(u16 con_handle, void *remote_addr, bool isconn);
/* LISTING_START(packetHandler): Packet Handler */
static void cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    int mtu;
    u32 tmp;
    const char *attribute_name;

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {

        /* case DAEMON_EVENT_HCI_PACKET_SENT: */
        /* break; */
        case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
            log_info("ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE\n");
        case ATT_EVENT_CAN_SEND_NOW:
            can_send_now_wakeup();
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE: {
                app_log("HCI_SUBEVENT_LE_CONNECTION_COMPLETE");
                if (!hci_subevent_le_enhanced_connection_complete_get_role(packet)) {
                    //is master
                    break;
                }
                hci_con_handle_t con_handle = little_endian_read_16(packet, 4);
                app_log("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x\n", con_handle);
                bt_rcsp_set_conn_info(con_handle, NULL, true);
                log_info_hexdump(packet + 7, 7);
                memcpy(cur_peer_addr_info, packet + 7, 7);
#if RCSP_ADV_EN
                set_connect_flag(SECNE_CONNECTED);
                bt_ble_adv_ioctl(BT_ADV_SET_NOTIFY_EN, 1, 1);
#endif
                set_ble_work_state(BLE_ST_CONNECT);
                // 设置广播需要放在绑定bt_rcsp_set_conn_info之后
                rcsp_ble_adv_enable_with_con_dev();
                pair_reconnect_flag = 0;
            }
            break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            app_log("HCI_EVENT_DISCONNECTION_COMPLETE: %0x\n", packet[5]);
            hci_con_handle_t dis_con_handle = little_endian_read_16(packet, 3);
            printf("%s, %s, %d, dis_con_handle:%d\n", __FILE__, __FUNCTION__, __LINE__, dis_con_handle);
            bt_rcsp_set_conn_info(dis_con_handle, NULL, false);
            rcsp_clean_update_hdl_for_end_update(dis_con_handle, NULL);
            if (packet[5] == 0x13) {
                app_log("disconnect ble by phone!!!");
                u8 data_stop_find[1] = {CMD_SYNC_STOP_FIND_EAR};
                user_send_data_to_both(data_stop_find, sizeof(data_stop_find));
            }
#if TCFG_RCSP_DUAL_CONN_ENABLE
            rcsp_1t2_reset_edr_info_for_ble_disconn(dis_con_handle);
#endif

#if RCSP_ADV_MUSIC_INFO_ENABLE
            extern void stop_get_music_timer(u8 en);
            stop_get_music_timer(1);
#endif
            // 设置广播需要放在解除绑定bt_rcsp_set_conn_info之后

#if TCFG_PAY_ALIOS_ENABLE && UPAY_ONE_PROFILE
            upay_ble_new_adv_enable(0);
#endif

#if RCSP_ADV_EN
            set_connect_flag(SECNE_UNCONNECTED);
#endif
            if (bt_rcsp_ble_conn_num() == 0) {
                set_ble_work_state(BLE_ST_DISCONN);
            }
#if RCSP_UPDATE_EN
            if (get_jl_update_flag()) {
                break;
            } else {
                extern u32 classic_update_task_exist_flag_get(void);
                if (classic_update_task_exist_flag_get()) {
                    break;
                }
            }
#endif
            // 设置广播需要放在解除绑定bt_rcsp_set_conn_info之后
            if (!ble_update_get_ready_jump_flag()) {
#if TCFG_USER_TWS_ENABLE
                if (!(tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)) {
                    log_info(")))))))) 1\n");
                    rcsp_ble_adv_enable_with_con_dev();
                } else {
                    log_info("))))))))>>>> %d\n", tws_api_get_role());
                    if (tws_api_get_role() == TWS_ROLE_MASTER) {
                        rcsp_ble_adv_enable_with_con_dev();
                    }
                }
#else
                rcsp_ble_adv_enable_with_con_dev();
#endif
            }
            connection_update_cnt = 0;
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            log_info("HCI_EVENT_ENCRYPTION_CHANGE\n");
            break;

#if TRANS_ANCS_EN
        case HCI_EVENT_ANCS_META:
            switch (hci_event_ancs_meta_get_subevent_code(packet)) {
            case ANCS_SUBEVENT_CLIENT_NOTIFICATION:
                log_info("ANCS_SUBEVENT_CLIENT_NOTIFICATION\n");
                attribute_name = ancs_client_attribute_name_for_id(ancs_subevent_client_notification_get_attribute_id(packet));
                if (!attribute_name) {
                    log_info("ancs unknow attribute_id :%d", ancs_subevent_client_notification_get_attribute_id(packet));
                    break;
                } else {
                    u16 attribute_strlen = little_endian_read_16(packet, 7);
                    u8 *attribute_str = (void *)little_endian_read_32(packet, 9);
                    log_info("Notification: %s - %s \n", attribute_name, attribute_str);
#if ANCS_MESSAGE_MANAGE_EN
                    extern void notice_set_info_from_ancs(void *name, void *data, u16 len);
                    notice_set_info_from_ancs(attribute_name, (void *)attribute_str, attribute_strlen);
#endif
                }
                break;

            case ANCS_SUBEVENT_CLIENT_CONNECTED:
                log_info("ANCS_SUBEVENT_CLIENT_CONNECTED\n");
                break;

            case ANCS_SUBEVENT_CLIENT_DISCONNECTED:
                log_info("ANCS_SUBEVENT_CLIENT_DISCONNECTED\n");
                break;

            default:
                break;
            }

            break;
#endif

#if TRANS_AMS_EN
        case HCI_EVENT_AMS_META:
            switch (packet[2]) {
            case AMS_SUBEVENT_CLIENT_NOTIFICATION: {
                log_info("AMS_SUBEVENT_CLIENT_NOTIFICATION\n");
                u16 Entity_Update_len = little_endian_read_16(packet, 7);
                u8 *Entity_Update_data = (void *)little_endian_read_32(packet, 9);
                /* log_info("EntityID:%d, AttributeID:%d, Flags:%d, utf8_len(%d):",\ */
                /* Entity_Update_data[0],Entity_Update_data[1],Entity_Update_data[2],Entity_Update_len-3); */
                log_info("%s(%s), Flags:%d, utf8_len(%d)", ams_get_entity_id_name(Entity_Update_data[0]),
                         ams_get_entity_attribute_name(Entity_Update_data[0], Entity_Update_data[1]),
                         Entity_Update_data[2], Entity_Update_len - 3);

#if 1 //for printf debug
                static u8 music_files_buf[128];
                u8 str_len = Entity_Update_len - 3;
                if (str_len > sizeof(music_files_buf)) {
                    str_len = sizeof(music_files_buf) - 1;
                }
                memcpy(music_files_buf, &Entity_Update_data[3], str_len);
                music_files_buf[str_len] = 0;
                printf("string:%s\n", music_files_buf);
#endif

                log_info_hexdump(&Entity_Update_data[3], Entity_Update_len - 3);
                /* if (Entity_Update_data[0] == 1 && Entity_Update_data[1] == 2) { */
                /* log_info("for test: send pp_key"); */
                /* ams_send_request_command(AMS_RemoteCommandIDTogglePlayPause); */
                /* } */
            }
            break;

            case AMS_SUBEVENT_CLIENT_CONNECTED:
                log_info("AMS_SUBEVENT_CLIENT_CONNECTED\n");
                break;

            case AMS_SUBEVENT_CLIENT_DISCONNECTED:
                log_info("AMS_SUBEVENT_CLIENT_DISCONNECTED\n");
                break;


            default:
                break;
            }
            break;
#endif

        }
        break;
    }
}

void rcsp_user_cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    cbk_packet_handler(packet_type, channel, packet, size);
}

void ancs_update_status(u8 status)
{
    switch (status) {
    case 1:
        log_info("ancs trunk start \n");
        break;
    case 2:
        log_info("ancs trunk end \n");
#if ANCS_MESSAGE_MANAGE_EN
        extern void notice_add_info_from_ancs();
        notice_add_info_from_ancs();
#endif
        break;
    default:
        break;
    }
}

static int make_set_adv_data(void)
{
    return rcsp_make_set_adv_data();
}

static int make_set_rsp_data(void)
{
    return rcsp_make_set_rsp_data();
}


u8 *ble_get_gatt_profile_data(u16 *len)
{
    *len = sizeof(profile_data);
    return (u8 *)profile_data;
}

//广播参数设置
static void advertisements_setup_init()
{
    uint8_t adv_type = 0;
    uint8_t adv_channel = 7;
    int   ret = 0;
    u16 adv_interval = ADV_INTERVAL_MIN;//0x30;

    //ble_op_set_adv_param(adv_interval, adv_type, adv_channel);
    app_ble_set_adv_param(rcsp_server_ble_hdl, adv_interval, adv_type, adv_channel);
    /* app_ble_set_adv_param(rcsp_server_ble_hdl1, adv_interval, adv_type, adv_channel); */


#if TCFG_PAY_ALIOS_ENABLE
    if (upay_mode_enable) {
        ret = upay_ble_adv_data_set();
    } else
#endif
    {
        ret |= make_set_adv_data();
        ret |= make_set_rsp_data();
    }

    if (ret) {
        puts("advertisements_setup_init fail !!!!!!\n");
        return;
    }

}

static void ancs_notification_message(u8 *packet, u16 size)
{
#if ANCS_MESSAGE_MANAGE_EN
    u8 *value;
    u32 ancs_notification_uid;
    value = &packet[8];
    ancs_notification_uid = little_endian_read_32(value, 4);
    log_info("Notification: EventID %02x, EventFlags %02x, CategoryID %02x, CategoryCount %u, UID %04x",
             value[0], value[1], value[2], value[3], little_endian_read_32(value, 4));

    if (value[1] & BIT(2)) {
        log_info("is PreExisting Message!!!");
    }

    if (value[0] == 2) { //0:added 1:modifiled 2:removed
        log_info("remove message:ancs_notification_uid %04x", ancs_notification_uid);
        extern void notice_remove_info_from_ancs(u32 uid);
        notice_remove_info_from_ancs(ancs_notification_uid);
    } else if (value[0] == 0) {
        extern void notice_set_info_from_ancs(void *name, void *data, u16 len);
        log_info("add message:ancs_notification_uid %04x", ancs_notification_uid);
        notice_set_info_from_ancs("UID", &ancs_notification_uid, sizeof(ancs_notification_uid));
    }
#endif
}

#define RCSP_TCFG_BLE_SECURITY_EN          0/*是否发请求加密命令*/
extern void le_device_db_init(void);
void rcsp_ble_profile_init(void)
{
    log_info("ble profile init\n");
    /* le_device_db_init(); */

#if TRANS_ANCS_EN || TRANS_AMS_EN
    if ((!config_le_sm_support_enable) || (!config_le_gatt_client_num)) {
        printf("ANCS need sm and client support!!!\n");
        ASSERT(0);
    }
#endif

    if (config_le_gatt_client_num) {
        //setup GATT client
        gatt_client_init();
    }

#if TRANS_ANCS_EN
    log_info("ANCS init...");
    //setup ANCS clent
    ancs_client_init();
    ancs_set_notification_buffer(ancs_info_buffer, sizeof(ancs_info_buffer));
    ancs_client_register_callback(&cbk_packet_handler);
    ancs_set_out_callback(ancs_notification_message);
#endif

#if TRANS_AMS_EN
    log_info("AMS init...");
    ams_client_init();
    ams_client_register_callback(&cbk_packet_handler);
    ams_entity_attribute_config(AMS_IDPlayer_ENABLE | AMS_IDQueue_ENABLE | AMS_IDTrack_ENABLE);
    /* ams_entity_attribute_config(AMS_IDTrack_ENABLE); */
#endif
}

static int set_adv_enable(void *priv, u32 en)
{
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    printf("%s, rets=0x%x\n", __FUNCTION__, rets_addr);
    
    ble_state_e next_state, cur_state;

    if (!adv_ctrl_en && en) {
        printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
        return APP_BLE_OPERATION_ERROR;
    }


#if TCFG_PAY_ALIOS_ENABLE
    if (upay_mode_enable) {
        /*upay模式,跳过spp检测,可以直接开ADV*/
        printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
        goto contrl_adv;
    }
#endif

    if (en) {
        /*控制开adv*/
#if !TCFG_RCSP_DUAL_CONN_ENABLE
        if (bt_rcsp_spp_conn_num() > 0) {
            log_info("spp connect type\n");
            printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
            return APP_BLE_OPERATION_ERROR;
        }

        // 防止ios只连上ble的情况下，android(spp)回连导致ble断开后重新开广播的情况
        if (bt_rcsp_device_conn_num() > 0) {
            log_info("spp is connecting\n");
            printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
            return APP_BLE_OPERATION_ERROR;
        }
#endif
    }

contrl_adv:

    if (en) {
        next_state = BLE_ST_ADV;
    } else {
        next_state = BLE_ST_IDLE;
    }

    log_info("adv_en:%d\n", en);
    set_ble_work_state(next_state);
    if (en) {
        advertisements_setup_init();
    }
    /* ble_user_cmd_prepare(BLE_CMD_ADV_ENABLE, 1, en); */
    app_ble_adv_enable(rcsp_server_ble_hdl, en);
    /* app_ble_adv_enable(rcsp_server_ble_hdl1, en); */
    log_info(">  set_adv_enable  %d\n", en);
    return APP_BLE_NO_ERROR;
}

/**
 * @brief 断开指定的ble
 *
 * @param ble_con_handle ble_con_handle
 */
void rcsp_disconn_designated_ble(u16 ble_con_handle)
{
    printf("%s, %s, %d, dis_con_handle:%d\n", __FILE__, __FUNCTION__, __LINE__, ble_con_handle);
    if (ble_con_handle) {
        u16 ble_con_hdl = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl);
        u16 ble_con_hdl1 = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl1);
        u16 dis_con_handle = (ble_con_handle == ble_con_hdl) ? ble_con_hdl : ble_con_hdl1;
        if (dis_con_handle == ble_con_hdl) {
            app_ble_disconnect(rcsp_server_ble_hdl);
        } else if (dis_con_handle == ble_con_hdl1) {
            app_ble_disconnect(rcsp_server_ble_hdl1);
        }
    }
}

/**
 * @brief 断开另一个ble
 *
 * @param ble_con_handle 保留的ble_con_handle，输入为0的时候，全部断开
 */
void rcsp_disconn_other_ble(u16 ble_con_handle)
{
    if (ble_con_handle) {
        u16 ble_con_hdl = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl);
        u16 ble_con_hdl1 = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl1);
        u16 dis_con_handle = (ble_con_handle == ble_con_hdl) ? ble_con_hdl1 : ble_con_hdl;
        if (dis_con_handle == ble_con_hdl) {
            app_ble_disconnect(rcsp_server_ble_hdl);
        } else if (dis_con_handle == ble_con_hdl1) {
            app_ble_disconnect(rcsp_server_ble_hdl1);
        }
    } else {
        app_ble_disconnect(rcsp_server_ble_hdl);
        app_ble_disconnect(rcsp_server_ble_hdl1);
    }
}

static int ble_disconnect(void *priv)
{
    if (bt_rcsp_ble_conn_num() > 0) {
        /* if (BLE_ST_SEND_DISCONN != rcsp_get_ble_work_state()) { */
        log_info(">>>ble send disconnect\n");
        set_ble_work_state(BLE_ST_SEND_DISCONN);
        app_ble_disconnect(rcsp_server_ble_hdl);
        app_ble_disconnect(rcsp_server_ble_hdl1);
        /* } else { */
        /* log_info(">>>ble wait disconnect...\n"); */
        /* } */
        return APP_BLE_NO_ERROR;
    } else {
        return APP_BLE_OPERATION_ERROR;
    }
}

/**
 * @brief ble主机接收回调
 *
 * @param buf
 * @param len
 */
void rcsp_ble_master_recieve_callback(void *buf, u16 len)
{
    /* printf("ble_rx(%d):\n", len); */
    /* put_buf(buf, len); */
    if (app_recieve_callback) {
        app_recieve_callback(0, buf, len);
    }
}

// rcsp内部调用
void rcsp_bt_ble_adv_enable(u8 enable)
{
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    printf("%s, rets=0x%x\n", __FUNCTION__, rets_addr);
    set_adv_enable(0, enable);
}

extern void ble_client_module_enable(u8 en);
void ble_module_enable(u8 en)
{
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    printf("%s, rets=0x%x\n", __FUNCTION__, rets_addr);
    y_printf("ble_module_enable:%d\n", en);
    adv_ctrl_en = 1;
    if (en) {
        adv_ctrl_en = 1;
        rcsp_bt_ble_adv_enable(1);
    } else {
        rcsp_bt_ble_adv_enable(0);
        adv_ctrl_en = 0;
        if (bt_rcsp_ble_conn_num() > 0) {
            ble_disconnect(NULL);
        }
    }
}


void rcsp_bt_ble_init(void)
{
    log_info("***** rcsp_bt_ble_init *****\n");

    if (ble_init_flag) {
        return;
    }

#if TCFG_PAY_ALIOS_ENABLE
    upay_mode_enable = 0;
    upay_new_adv_enable = 0;

#if UPAY_ONE_PROFILE
    if (config_btctler_le_hw_nums < 2) {
        log_info("error:need add hw to adv new device!!!\n");
        ASSERT(0);
    }
#endif

#endif

    gap_device_name = (char *)bt_get_local_name();
    gap_device_name_len = strlen(gap_device_name);
    if (gap_device_name_len > BT_NAME_LEN_MAX) {
        gap_device_name_len = BT_NAME_LEN_MAX;
    }

    log_info("ble name(%d): %s \n", gap_device_name_len, gap_device_name);

    set_ble_work_state(BLE_ST_INIT_OK);
    bt_ble_init_do();

    ble_vendor_set_default_att_mtu(ATT_LOCAL_PAYLOAD_SIZE);

    const u8 *comm_addr = bt_get_mac_addr();
    u8 tmp_ble_addr[6] = {0};
#if DOUBLE_BT_SAME_MAC || TCFG_BT_BLE_BREDR_SAME_ADDR
    memcpy(tmp_ble_addr, comm_addr, 6);
#else
    bt_make_ble_address(tmp_ble_addr, comm_addr);
#endif

    /* printf("init tmp_ble_addr:\n"); */
    /* put_buf(tmp_ble_addr, 6); */
    app_ble_set_mac_addr(rcsp_server_ble_hdl, tmp_ble_addr);
    app_ble_set_mac_addr(rcsp_server_ble_hdl1, tmp_ble_addr);

    ble_module_enable(1);
    bt_ble_rcsp_adv_enable();

    ble_init_flag = 1;
}

void rcsp_bt_ble_exit(void)
{
    log_info("***** ble_exit******\n");
    if (!ble_init_flag) {
        return;
    }
    y_printf("===========5")
    ble_module_enable(0);
#if RCSP_MODE
    extern void rcsp_exit(void);
    rcsp_exit();
#endif

    ble_init_flag = 0;

}


void ble_app_disconnect(void)
{
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    printf("%s, rets=0x%x\n", __FUNCTION__, rets_addr);
    ble_disconnect(NULL);
}

#if TRANS_ANCS_EN
void hangup_ans_call_handle(u8 en)
{

    u32 notification_id;
    u16 control_point_handle;

    log_info("hang_up or answer\n");
    notification_id = get_notification_uid();
    control_point_handle = get_controlpoint_handle();
    u8 ble_hangup[] = {0x02, 0, 0, 0, 0, en};
    memcpy(&ble_hangup[1], &notification_id, 4);
    log_info_hexdump(ble_hangup, 6);
    u8 ble_hangup_size = 6;
    ble_op_att_send_data(control_point_handle, (void *)&ble_hangup, ble_hangup_size, ATT_OP_WRITE);

}
#endif


#if TCFG_PAY_ALIOS_ENABLE

#if UPAY_ONE_PROFILE
static void upay_ble_new_adv_enable(u8 en)
{
    if (upay_new_adv_enable == en) {
        return;
    }
    upay_new_adv_enable = en;
    if (en) {
        /* ble_op_set_adv_param(ADV_INTERVAL_MIN, 0, 7); */
        ble_op_set_adv_param(ADV_INTERVAL_MIN, ADV_SCAN_IND, 7);/*just adv*/
        upay_ble_adv_data_set();
    }
    ble_user_cmd_prepare(BLE_CMD_ADV_ENABLE, 1, en);
    log_info(">>>new_adv_enable  %d\n", en);
}
#endif

/*upay recieve data regies*/
void upay_ble_regiest_recv_handle(void (*handle)(const uint8_t *data, u16 len))
{
    upay_recv_callback = handle;
}

/*upay send data api*/
int upay_ble_send_data(const uint8_t *data, u16 len)
{
    log_info("upay_ble_send(%d):", len);
    log_info_hexdump(data, len);
    if (att_get_ccc_config(ATT_CHARACTERISTIC_4a02_01_CLIENT_CONFIGURATION_HANDLE)) {
        return ble_op_att_send_data(ATT_CHARACTERISTIC_4a02_01_VALUE_HANDLE, data, len, ATT_OP_NOTIFY);
    }
    return BLE_CMD_CCC_FAIL;
}

/*open or close upay*/
void upay_ble_mode_enable(u8 enable)
{
    if (enable == upay_mode_enable) {
        return;
    }

    upay_mode_enable = enable;
    log_info("upay_mode_enable= %d\n", upay_mode_enable);

#if UPAY_ONE_PROFILE
    if (upay_mode_enable) {
        if (bt_rcsp_ble_conn_num() > 0) {
            /*已连接,要开新设备广播*/
            u16 ble_con_handle = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl);
            u16 ble_con_handle1 = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl1);
            if (ble_con_handle) {
                ble_op_latency_skip(ble_con_handle, 0xffff);
            }
            if (ble_con_handle1) {
                ble_op_latency_skip(ble_con_handle1, 0xffff);
            }
            upay_ble_new_adv_enable(1);
        } else {
            /*未连接，只切换原设备广播*/
            ble_module_enable(0);
            ble_module_enable(1);
        }
    } else {
        upay_ble_new_adv_enable(0);
        if (!bt_rcsp_ble_conn_num()) {
            /*未连接，只切换广播*/
            ble_module_enable(0);
            ble_module_enable(1);
        } else {
            ble_op_latency_skip(con_handle, 0);
            u16 ble_con_handle = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl);
            u16 ble_con_handle1 = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl1);
            if (ble_con_handle) {
                ble_op_latency_skip(ble_con_handle, 0);
            }
            if (ble_con_handle1) {
                ble_op_latency_skip(ble_con_handle1, 0);
            }
        }
    }
#else
    ble_module_enable(0);
    if (upay_mode_enable) {
        att_server_change_profile(profile_data_upay);
#if TRANS_ANCS_EN
        ancs_client_exit();
#endif
#if TRANS_AMS_EN
        ams_client_exit();
#endif

    } else {
        att_server_change_profile(profile_data);
#if TRANS_ANCS_EN
        ancs_client_init();
#endif
#if TRANS_AMS_EN
        ams_client_init();
#endif

    }
    ble_module_enable(1);

#endif

}
#endif

#endif


/*************************************************************/
//   一些与APP信息有关的功能写在这
/*************************************************************/
void user_app_disconn_device(u8 *addr)
{
    app_log("user_app_disconn_device");
    put_buf(addr, 6);
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        // u16 ble_con_hdl = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl);
        // u16 ble_con_hdl1 = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl1);
        // put_buf(rcsp_get_ble_hdl_remote_mac_addr(ble_con_hdl), 6);
        // put_buf(rcsp_get_ble_hdl_remote_mac_addr(ble_con_hdl1), 6);
        // if (memcmp(addr, rcsp_get_ble_hdl_remote_mac_addr(ble_con_hdl), 6)) {
        //     app_ble_disconnect(rcsp_server_ble_hdl);
        // }
        // if (memcmp(addr, rcsp_get_ble_hdl_remote_mac_addr(ble_con_hdl1), 6)) {
        //     app_ble_disconnect(rcsp_server_ble_hdl1);
        // }
        bt_cmd_prepare_for_addr(addr, USER_CTRL_DISCONNECTION_HCI, 0, NULL);
    }
}

void user_set_local_box_charge_status(u8 status)
{
    user_charge_info.box_charge_online = (status & BIT(7)) ? 1 : 0;
    user_charge_info.box_bat_percent_local = status;
}

void user_set_sibling_box_charge_status(u8 status)
{
    user_charge_info.box_charge_online = (status & BIT(7)) ? 1 : 0;
    user_charge_info.box_bat_percent_sibling = status;
}

void user_sync_box_charge_status(u8 status)
{
    static u8 last_box_bat = 0; 
    static u8 same_bat_send_cnt = 0; 
    // app_log("user_sync_box_charge_status, last_box_bat:%d, status:%d", last_box_bat, status);
    if (last_box_bat == status) {
        // app_log("box vbat same, return!!!");
        return;
    } 
    if (get_tws_sibling_connect_state()) {
        app_log("tws is connect, sync bat!!!");
        tws_sync_bat_level();
    } else {
        //对耳未连接，直接发自己的电量
        user_app_data_notify(CMD_NOTIFY_BAT_VALUE);
    }

    last_box_bat = status;
}

void user_update_bat_value()
{
    // app_log("user_update_bat_value");
    char tws_channel = tws_api_get_local_channel();
    //客户说不需要耳机充电标志
    user_charge_info.ear_bat_percent_l = tws_channel == 'L' ? (get_self_battery_level()+1)*10 : (get_tws_sibling_bat_level()+1)*10;
    user_charge_info.ear_bat_percent_r = tws_channel == 'R' ? (get_self_battery_level()+1)*10 : (get_tws_sibling_bat_level()+1)*10;

	if (user_charge_info.ear_bat_percent_l == 0) {
        user_charge_info.ear_bat_percent_l = 0xFF;
    }
    if (user_charge_info.ear_bat_percent_r == 0) {
        user_charge_info.ear_bat_percent_r = 0xFF;
    }

    u8 sibling_in_box_flag = get_tws_sibling_bat_persent() >> 7;
    // app_log("user_update_bat_value: %d, %d, %d", tws_api_get_role(), get_charge_online_flag(), sibling_in_box_flag);
    // app_log("%d, %d", user_charge_info.box_bat_percent_local, user_charge_info.box_bat_percent_sibling);
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        if (get_charge_online_flag()) {
            user_charge_info.box_bat_percent = user_charge_info.box_bat_percent_local;
        } else if (sibling_in_box_flag) {
            user_charge_info.box_bat_percent = user_charge_info.box_bat_percent_sibling;
        } else {
            user_charge_info.box_bat_percent = user_charge_info.box_bat_percent_local;
        }
        if (user_charge_info.box_bat_percent == 0) {
            user_charge_info.box_bat_percent = 0xFF;
        }
    }
    app_log("update vbat: %d %d %d", user_charge_info.ear_bat_percent_l, user_charge_info.ear_bat_percent_r, user_charge_info.box_bat_percent);
}

u8 user_app_set_key(u8 *buf, u16 len)
{
    // app_log("user_app_set_key");
    if (len < 13) {
        // app_log("len err, len = %d", len);
    }
    memcpy(user_app_info.app_key_lr, buf+5, 8);
    // put_buf(user_app_info.app_key_lr, 8);
    return set_custom_key(user_app_info.app_key_lr, sizeof(user_app_info.app_key_lr));
}

u8 is_searching_ear(void)
{
    if(tws_api_get_local_channel() == 'L' && user_var.find_ear_status_l){
        return 1;
    }
    if(tws_api_get_local_channel() == 'R' && user_var.find_ear_status_r){
        return 1;
    }

    return 0;
}

void user_both_stop_find_ear()
{
    app_log("user_stop_find_ear");
    user_var.find_ear_status_l = 0;
    user_var.find_ear_status_r = 0;
    user_var.find_ear_status_tws = 0;
    if (user_find_ear_timerid != 0) {
        sys_timer_del(user_find_ear_timerid);
        user_find_ear_timerid = 0;
        tone_player_stop(); //立即停止
        user_set_tone_volume(1, user_var.tone_volume_fix);
        user_find_ear_cnt = 0;
    }
}

void user_find_ear_timer()
{
    // app_log("user_find_ear_timer");
    if (user_find_ear_cnt < 1) {
        user_set_tone_volume(1, 9);
    } else if (user_find_ear_cnt < 2) {
        user_set_tone_volume(1, 13);
    } else {
        user_find_ear_cnt = 15;
        user_set_tone_volume(1, 16);
    }
    user_find_ear_cnt++;
    play_tone_file(get_tone_files()->buzzer);
}

void user_find_ear_deal(u8 en, u8 lr)
{
    app_log("user_find_ear_deal");
    u8 channel = tws_api_get_local_channel();
    app_log("ch: %c, en: %d, lr: %d", channel, en, lr);
    if (en) {
        if (lr == 1) {
            if (channel == 'L') {
                if (user_find_ear_timerid == 0) {
                    user_var.find_ear_status_l = 1;
                    user_find_ear_cnt = 0;
                    user_find_ear_timer(); //立即开始
                    user_find_ear_timerid = sys_timer_add(NULL, user_find_ear_timer, 4000);
                }
            } else {
                user_var.find_ear_status_l = 1;
                user_var.find_ear_status_tws = 1;
            }
        } else if (lr == 2) {
            if (channel == 'R') {
                if (user_find_ear_timerid == 0) {
                    user_var.find_ear_status_r = 1;
                    user_find_ear_cnt = 0;
                    user_find_ear_timer(); //立即开始
                    user_find_ear_timerid = sys_timer_add(NULL, user_find_ear_timer, 4000);
                }
            } else {
                user_var.find_ear_status_r = 1;
                user_var.find_ear_status_tws = 1; 
            }
        }
    } else {
	        if (lr == 1) {
	            if (channel == 'L') {
	                if (user_find_ear_timerid != 0) {
	                    user_var.find_ear_status_l = 0;
	                    tone_player_stop(); //立即停止
	                    user_set_tone_volume(1, user_var.tone_volume_fix);
	                    user_find_ear_cnt = 0;
	                    sys_timer_del(user_find_ear_timerid);
	                    user_find_ear_timerid = 0;
	                }
            } else {
                user_var.find_ear_status_l = 0;
                user_var.find_ear_status_tws = 0;
            }
        } else if (lr == 2) {
            if (channel == 'R') {
                if (user_find_ear_timerid != 0) {
                    user_var.find_ear_status_r = 0;
                    tone_player_stop(); //立即停止
                    user_set_tone_volume(1, user_var.tone_volume_fix);
                    user_find_ear_cnt = 0;
                    sys_timer_del(user_find_ear_timerid);
                    user_find_ear_timerid = 0;
                }
            } else {
                user_var.find_ear_status_r = 0;
                user_var.find_ear_status_tws = 0;
            }
        }
    }
    app_log("l:%d r:%d tws:%d id:%d\n",user_var.find_ear_status_l,user_var.find_ear_status_r,user_var.find_ear_status_tws,user_find_ear_timerid);
}
/*************************************************************/

/*************************************************************/
//   这里初始化和设置版本号
/*************************************************************/
void user_ver_info_init()
{
    // app_log("user_ver_info_init");
    if (tws_api_get_local_channel() == 'L') {
        memcpy(soft_ver_l, soft_ver_local, sizeof(soft_ver_l));
    } else if (tws_api_get_local_channel() == 'R') {
        memcpy(soft_ver_r, soft_ver_local, sizeof(soft_ver_r));
    } 
}
// __initcall(user_ver_info_init);

void user_save_ver_info(u8 *data, u16 len)
{
    // app_log("user_save_ver_info");
    if (len < 4) {
        app_log("len err: %d, return!!!", len);
        return;
    }
    if (tws_api_get_local_channel() == 'L') {
        memcpy(soft_ver_r, data+1, sizeof(soft_ver_r));
    } else if (tws_api_get_local_channel() == 'R') {
        memcpy(soft_ver_l, data+1, sizeof(soft_ver_l));
    } 
}
/*************************************************************/

/*************************************************************/
//   这里只同步和设置APP结构体里的数据
/*************************************************************/
// 恢复出厂设置，app信息要怎么存，修改这里
void user_app_info_restore()
{
    app_log("user_app_info_restore");
    user_app_info.ear_id = 0;
    user_app_info.ear_color = 0;
    user_app_info.find_ear_en = 0;
    user_app_info.find_ear_lr = 0;
    user_app_info.anc_mode = 0;
    user_app_info.eq_mode = 0;
    user_app_info.u1t2_mode = 0;
    // user_app_info.u1t2_mode = 1;
    user_app_info.game_mode = 0;
    user_app_info.hifi_mode = 0;
#if USER_PRODUCT_SET_TONE_EN
    extern u8 user_default_tone_language;
    user_app_info.tone_language = user_default_tone_language;
#else
    user_app_info.tone_language = 0;
#endif
    user_app_info.volume_protect = 0;
    user_app_info.app_key_lr[0] = 0;
    user_app_info.app_key_lr[1] = 1;
    user_app_info.app_key_lr[2] = 6;
    user_app_info.app_key_lr[3] = 5;
    user_app_info.app_key_lr[4] = 0;
    user_app_info.app_key_lr[5] = 1;
    user_app_info.app_key_lr[6] = 7;
    user_app_info.app_key_lr[7] = 4;
    app_log_hexdump(&user_app_info, sizeof(user_app_info));
    syscfg_write(CFG_USER_APP_INFO, (u8 *)&user_app_info, sizeof(user_app_info));
}

//开机初始化APP数据
void user_app_info_init()
{
    app_log("user_app_info_init");
	app_log_hexdump(&user_app_info, sizeof(user_app_info));
    int ret = syscfg_read(CFG_USER_APP_INFO, (u8 *)&user_app_info, sizeof(user_app_info));
    if (ret <= 0) {
        //没写过VM就用出厂设置
        user_app_info_restore();
    } else {
        app_log_hexdump(&user_app_info, sizeof(user_app_info));
    }	
	set_custom_key(user_app_info.app_key_lr, sizeof(user_app_info.app_key_lr));
    user_charge_info.ear_bat_percent_l = 0xFF;
    user_charge_info.ear_bat_percent_r = 0xFF;
    user_charge_info.local_detail_percent = 0xFF;
    user_charge_info.remote_detail_percent = 0xFF;
    user_charge_info.box_bat_percent = 0xFF;
    user_charge_info.box_bat_percent_local = 0xFF;
    user_charge_info.box_bat_percent_sibling = 0xFF;
}
__initcall(user_app_info_init);

void user_local_app_info_save()
{
    app_log("user_local_app_info_save");
    //关机不保存游戏模式
    user_app_info.game_mode = 0;
    app_log_hexdump(&user_app_info, sizeof(user_app_info));
    syscfg_write(CFG_USER_APP_INFO, (u8 *)&user_app_info, sizeof(user_app_info));
}

//APP数据写VM
void user_set_app_info_vm(u8 *data, u16 len)
{
    // app_log("user_set_app_info_vm");
    memcpy(&user_app_info, data, sizeof(user_app_info));
    // app_log_hexdump(&user_app_info, sizeof(user_app_info));
    syscfg_write(CFG_USER_APP_INFO, (u8 *)&user_app_info, sizeof(user_app_info));
    free(data);
}

static void user_app_info_sync(void *_data, u16 len, bool rx)
{
    app_log("user_app_info_sync, len = %d, rx = %d", len, rx);
    if (sizeof(user_app_info) != len) {
        app_log("len err!!!")
    }
    u8 *data = malloc(len);
    memcpy(data, _data, len);
    int msg[4] = { (int)user_set_app_info_vm, 2, (int)data, len};
    os_taskq_post_type("app_core", Q_CALLBACK, 4, msg);
}

REGISTER_TWS_FUNC_STUB(user_send_app_info_stub) = {
    .func_id = USER_UUID_SEND_APP_INFO,
    .func    = user_app_info_sync,
};

void user_send_app_info_to_sibling()
{
    // app_log("user_send_app_info_to_sibling");
    if (get_bt_tws_connect_status()) {
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            // app_log_hexdump(&user_app_info, sizeof(user_app_info));
            tws_api_send_data_to_sibling(&user_app_info, sizeof(user_app_info), USER_UUID_SEND_APP_INFO);
        }
    } else {
        // u16 len = sizeof(user_app_info);
        // u8 *data = malloc(len);
        // memcpy(data, &user_app_info, len);
        // int msg[4] = { (int)user_set_app_info_vm, 2, (int)data, len};
        // os_taskq_post_type("app_core", Q_CALLBACK, 4, msg);
    }
}
/*************************************************************/

/*************************************************************/
//   拼包和校验接口写在这
/*************************************************************/
#define TX_BUFFER_LEN   128
static u8 _txbuf[TX_BUFFER_LEN];
static u8 _txbuf_len = TX_BUFFER_LEN;

static uint16_t USER_CRC16(const uint8_t * buffer, uint32_t size)
{
    uint16_t crc = 0xFFFF;
    if (NULL != buffer && size > 0) {
        // while (size--) {
        //     crc = (crc >> 8) | (crc << 8);
        //     crc ^= *buffer++;
        //     crc ^= ((unsigned char) crc) >> 4;
        //     crc ^= crc << 12;
        //     crc ^= (crc & 0xFF) << 5;
        // }

        for (int i = 3; i<size ; i++) {  //去掉数据头
            crc = (crc >> 8) | (crc << 8);
            crc ^= buffer[i];
            crc ^= ((unsigned char) crc) >> 4;
            crc ^= crc << 12;
            crc ^= (crc & 0xFF) << 5;
        }
    }
    return crc;
}

static u16 buff_package(u8 *dst, u16 dst_len,u16 offset,  u8 *src, u16 src_len)
{
    if ((offset + src_len) > dst_len || !dst || !src) {
        return 0;
    }
    if (offset == 0) {
        memset(dst, 0x00, dst_len);
    }
    memcpy(dst + offset, src, src_len);
    return src_len;
}

static u16 data_package(u8 *dst, u16 dst_len, u8 src, u16 offset)
{
    if ((offset + 1) > dst_len || !dst) {
        return 0;
    }
    if (offset == 0) {
        memset(dst, 0x00, dst_len);
    }
    dst[offset] = src;
    return 1;
}

static u16 tx_buff_package(u8 *src, u16 offset, u16 src_len)
{
    return buff_package(_txbuf, _txbuf_len, offset, src, src_len);
}

static u16 tx_data_package(u8 src, u16 offset, u16 src_len)
{
    return data_package(_txbuf, _txbuf_len, src, offset);
}
/*************************************************************/

/*************************************************************/
//   拼包和收发包写在这
/*************************************************************/
static const u8 data_head_rx[] = {0xAA, 0xBB, 0xCC};
static const u8 data_head_tx[] = {0xDD, 0xEE, 0xFF};
static const u8 data_head_notify[] = {0x85, 0x86, 0x87};
#define RX_PARAM_LEN   (data[sizeof(data_head_rx)])
#define CMD_OFFSET     (sizeof(data_head_rx) + 1)
#define DATA_OFFSET    (sizeof(data_head_rx) + 2)
#define CMD_GET_VER                 0x01    //获取版本号
#define CMD_GET_ID                  0x02    //获取ID号
#define CMD_GET_COLOR               0x03    //货物耳机颜色
#define CMD_GET_INFO                0x04    //获取当前状态
#define CMD_SET_EQ_MODE             0x05    //EQ切换
#define CMD_SET_1T2_MODE            0x06    //1t2开关状态
#define CMD_SEARCH_EAR              0x07    //查找耳机
#define CMD_GAME_MODE_SET           0x08    //游戏模式
#define CMD_SET_KEY                 0x0A    //按键设置
#define CMD_SET_LANGUAGE            0x0C    //按键设置
#define CMD_GET_MANUFACTURE         0x0D    //获取手机厂商
#define CMD_RESTORE                 0x0E    //恢复出厂设置
#define CMD_GET_INOUTCHARGESTORE    0x10    //获取出入仓状态

//APP来获取的写在这
void user_app_data_deal(u8 *buf, u16 len, u8 type)
{
    app_log("user_app_data_deal type:%d", type);
    put_buf(buf, len);
    if (len < 4) {
        app_log("data len err, return!!!");
        return; 
    }
    if (buf[0]!=0xAA || buf[1]!=0xBB || buf[2]!=0xCC) {
        app_log("cmd head err, return!!!");
        return;
    }

    u32 offset = 0;
    u16 crc = 0;
    u8 cmd = buf[3];
    switch (cmd) {
    case CMD_GET_VER:
        app_log("CMD_GET_VER");
        offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
        offset += tx_data_package(cmd, offset, 1);
        offset += tx_data_package(0x01, offset, 1);
        offset += tx_data_package(0x06, offset, 1);
        offset += tx_buff_package((u8 *)soft_ver_l, offset, sizeof(soft_ver_l));
        offset += tx_buff_package((u8 *)soft_ver_r, offset, sizeof(soft_ver_r));
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);
        break;
    case CMD_GET_ID:
        app_log("CMD_GET_ID");
        offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
        offset += tx_data_package(cmd, offset, 1);
        offset += tx_data_package(0x01, offset, 1);
        offset += tx_data_package(0x01, offset, 1);
        offset += tx_data_package(user_app_info.ear_id, offset, 1);
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);
        break;
	case CMD_GET_COLOR:
        app_log("CMD_GET_COLOR");
        offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
        offset += tx_data_package(cmd, offset, 1);
        offset += tx_data_package(0x01, offset, 1);
        offset += tx_data_package(0x01, offset, 1);
        offset += tx_data_package(user_app_info.ear_color, offset, 1);
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);
        break;
    case CMD_GET_INFO:
        app_log("CMD_GET_INFO");
        user_update_bat_value();
        set_custom_key(user_app_info.app_key_lr, sizeof(user_app_info.app_key_lr));
        offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
        offset += tx_data_package(cmd, offset, 1);
        offset += tx_data_package(0x01, offset, 1);
        offset += tx_data_package(0x13, offset, 1);
        offset += tx_data_package(user_charge_info.ear_bat_percent_l, offset, 1);
        offset += tx_data_package(user_charge_info.ear_bat_percent_r, offset, 1);
        offset += tx_data_package(user_charge_info.box_bat_percent, offset, 1);
        offset += tx_data_package(user_app_info.anc_mode, offset, 1);
        offset += tx_data_package(user_app_info.eq_mode, offset, 1);
        offset += tx_data_package(user_app_info.u1t2_mode, offset, 1);
        offset += tx_data_package(user_app_info.game_mode, offset, 1);
        offset += tx_data_package(user_app_info.hifi_mode, offset, 1);
        offset += tx_buff_package((u8 *)user_app_info.app_key_lr, offset, sizeof(user_app_info.app_key_lr));
        offset += tx_data_package(user_app_info.tone_language, offset, 1);
        offset += tx_data_package(user_app_info.find_ear_en, offset, 1);
        offset += tx_data_package(user_app_info.find_ear_lr, offset, 1);
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);
        break;
    case CMD_SET_EQ_MODE:
        app_log("CMD_SET_EQ_MODE");
        user_app_info.eq_mode = buf[DATA_OFFSET];
        offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
        offset += tx_data_package(cmd, offset, 1);
        offset += tx_data_package(0x01, offset, 1);
        offset += tx_data_package(0x01, offset, 1);
        offset += tx_data_package(user_app_info.eq_mode, offset, 1);
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);
        u8 data_set_eq[3] = {CMD_SYNC_SET_EQ_MODE, user_app_info.eq_mode, 1};
        user_send_data_to_both(data_set_eq, sizeof(data_set_eq));
        break;
   	case CMD_SET_1T2_MODE:
		app_log("CMD_SET_1T2_MODE");
        user_app_info.u1t2_mode = buf[DATA_OFFSET];
        if (user_app_info.u1t2_mode == 0 && buf[4] == 0x08) {
            app_log("cmd have disconn addr!!!");
            u8 disconn_addr[6];
            memcpy(disconn_addr, buf+7, 6);
            user_app_disconn_device(disconn_addr);
        }
		offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
		offset += tx_data_package(cmd, offset, 1);
		offset += tx_data_package(0x01, offset, 1);
		offset += tx_data_package(0x01, offset, 1);
        offset += tx_data_package(user_app_info.u1t2_mode, offset, 1);
		crc = USER_CRC16(_txbuf, offset);
		offset += tx_data_package((crc&0xFF), offset, 1);
		offset += tx_data_package((crc>>8)&0xFF, offset, 1);
        u8 data_set_1t2_mode[2] = {CMD_SYNC_SET_1T2_MODE, user_app_info.u1t2_mode};
        user_send_data_to_sibling(data_set_1t2_mode, sizeof(data_set_1t2_mode));
		break;
	case CMD_SEARCH_EAR:
		app_log("CMD_SEARCH_EAR");
        user_app_info.find_ear_en = buf[DATA_OFFSET];
        user_app_info.find_ear_lr = buf[DATA_OFFSET+1];
		offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
		offset += tx_data_package(cmd, offset, 1);
		offset += tx_data_package(0x01, offset, 1);
        offset += tx_data_package(0x02, offset, 1);
		offset += tx_data_package(user_app_info.find_ear_en, offset, 1);
        offset += tx_data_package(user_app_info.find_ear_lr, offset, 1);
		crc = USER_CRC16(_txbuf, offset);
		offset += tx_data_package((crc&0xFF), offset, 1);
		offset += tx_data_package((crc>>8)&0xFF, offset, 1);
        u8 data_search_ear[3] = {CMD_SYNC_SEARCH_EAR, user_app_info.find_ear_en, user_app_info.find_ear_lr};
        user_send_data_to_both(data_search_ear, sizeof(data_search_ear));
		break;
	case CMD_GAME_MODE_SET:
		app_log("CMD_GAME_MODE_SET");
        user_app_info.game_mode = buf[DATA_OFFSET];
        bt_set_low_latency_mode(user_app_info.game_mode, 1, 300);
		offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
		offset += tx_data_package(cmd, offset, 1);
		offset += tx_data_package(0x01, offset, 1);
		offset += tx_data_package(0x01, offset, 1);
        offset += tx_data_package(user_app_info.game_mode, offset, 1);
		crc = USER_CRC16(_txbuf, offset);
		offset += tx_data_package((crc&0xFF), offset, 1);
		offset += tx_data_package((crc>>8)&0xFF, offset, 1);
        u8 data_set_game[2] = {CMD_SYNC_SET_GAME_MODE, user_app_info.game_mode};
        user_send_data_to_sibling(data_set_game, sizeof(data_set_game));
		break;
    case CMD_SET_KEY:
        app_log("CMD_SET_KEY");
        offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
        offset += tx_data_package(cmd, offset, 1);
        offset += tx_data_package(user_app_set_key(buf, len), offset, 1);
        offset += tx_data_package(0x08, offset, 1);
        offset += tx_buff_package((u8 *)user_app_info.app_key_lr, offset, sizeof(user_app_info.app_key_lr));
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);
        u8 data_set_key[9];
        data_set_key[0] = CMD_SYNC_SET_APP_KEY;
        memcpy(data_set_key+1, user_app_info.app_key_lr, 8);
        user_send_data_to_sibling(data_set_key, sizeof(data_set_key));
        break;
	case CMD_SET_LANGUAGE:
		app_log("CMD_SET_LANGUAGE");
        user_app_info.tone_language = buf[DATA_OFFSET];
		offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
		offset += tx_data_package(cmd, offset, 1);
		offset += tx_data_package(0x01, offset, 1);
		offset += tx_data_package(0x01, offset, 1);
		offset += tx_data_package(user_app_info.tone_language, offset, 1);
		crc = USER_CRC16(_txbuf, offset);
		offset += tx_data_package((crc&0xFF), offset, 1);
		offset += tx_data_package((crc>>8)&0xFF, offset, 1);
        u8 data_set_language[2] = {CMD_SYNC_SET_LANGUAGE, user_app_info.tone_language};
        user_send_data_to_both(data_set_language, sizeof(data_set_language));
		break;
	case CMD_GET_MANUFACTURE:
		app_log("CMD_GET_MANUFACTURE");
        u8 user_conn_dev_num = bt_get_total_connect_dev();
        app_log("dev_num:%d, 1t2_mode:%d", user_conn_dev_num, user_app_info.u1t2_mode);
        u16 temp_buf1_len = 0;
        u16 temp_buf2_len = 0;
        if (user_phone_info1.remote_name != NULL) {
            app_log("info1: %s\n", user_phone_info1.remote_name);
            offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
            offset += tx_data_package(cmd, offset, 1);
            offset += tx_data_package(0x01, offset, 1);
            offset += tx_data_package((user_phone_info1.name_len+8), offset, 1);
            offset += tx_data_package(user_conn_dev_num, offset, 1);
            offset += tx_data_package(0x01, offset, 1);
            offset += tx_buff_package((u8 *)user_phone_info1.remote_name, offset, user_phone_info1.name_len);
            offset += tx_buff_package((u8 *)user_phone_info1.addr, offset, 6);
            crc = USER_CRC16(_txbuf, offset);
            offset += tx_data_package((crc&0xFF), offset, 1);
            offset += tx_data_package((crc>>8)&0xFF, offset, 1);
            temp_buf1_len = offset;
            temp_buf1 = (u8 *)malloc(temp_buf1_len);
            memcpy(temp_buf1, _txbuf, temp_buf1_len);
        }
        offset = 0;
        if (user_phone_info2.remote_name != NULL && user_app_info.u1t2_mode == 1) {
            app_log("info2: %s\n", user_phone_info2.remote_name);
            offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
            offset += tx_data_package(cmd, offset, 1);
            offset += tx_data_package(0x01, offset, 1);
            offset += tx_data_package((user_phone_info2.name_len+8), offset, 1);
            offset += tx_data_package(user_conn_dev_num, offset, 1);
            offset += tx_data_package(0x02, offset, 1);
            offset += tx_buff_package((u8 *)user_phone_info2.remote_name, offset, user_phone_info2.name_len);
            offset += tx_buff_package((u8 *)user_phone_info2.addr, offset, 6);
            crc = USER_CRC16(_txbuf, offset);
            offset += tx_data_package((crc&0xFF), offset, 1);
            offset += tx_data_package((crc>>8)&0xFF, offset, 1);
            temp_buf2_len = offset;
            temp_buf2 = (u8 *)malloc(temp_buf2_len);
            memcpy(temp_buf2, _txbuf, temp_buf2_len);
        }
        if (user_app_info.u1t2_mode == 1 && temp_buf2 != NULL) {
            user_app_dev_data_ble_transpond(temp_buf1, temp_buf1_len);
            user_app_dev_data_ble_transpond(temp_buf2, temp_buf2_len);
            if (memcmp(user_app_spp_addr.spp_addr1, addr_all_ff, 6)) {
                if (memcmp(user_app_spp_addr.spp_addr1, user_phone_info1.addr, 6) == 0) {
                    temp_buf1[7] = 0x01;
                    user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr1, temp_buf1, temp_buf1_len);
                    temp_buf2[7] = 0x02;
                    user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr1, temp_buf2, temp_buf2_len);
                    if (memcmp(user_app_spp_addr.spp_addr2, addr_all_ff, 6)) {
                        temp_buf2[7] = 0x01;
                        user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr2, temp_buf2, temp_buf2_len);
                        temp_buf1[7] = 0x02;
                        user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr2, temp_buf1, temp_buf1_len);
                    }
                } else if (memcmp(user_app_spp_addr.spp_addr1, user_phone_info2.addr, 6) == 0) {
                    temp_buf2[7] = 0x01;
                    user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr1, temp_buf2, temp_buf2_len);
                    temp_buf1[7] = 0x02;
                    user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr1, temp_buf1, temp_buf1_len);
                    if (memcmp(user_app_spp_addr.spp_addr2, addr_all_ff, 6)) {
                        temp_buf1[7] = 0x01;
                        user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr2, temp_buf1, temp_buf1_len);
                        temp_buf2[7] = 0x02;
                        user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr2, temp_buf2, temp_buf2_len);
                    }
                }
            }
        } else {
            user_app_dev_data_ble_transpond(temp_buf1, temp_buf1_len);
            if (memcmp(user_app_spp_addr.spp_addr1, addr_all_ff, 6)) {
                if (memcmp(user_app_spp_addr.spp_addr1, user_phone_info1.addr, 6) == 0) {
                    temp_buf1[7] = 0x01;
                    user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr1, temp_buf1, temp_buf1_len);
                } 
            } else if (memcmp(user_app_spp_addr.spp_addr2, addr_all_ff, 6)) {
                if (memcmp(user_app_spp_addr.spp_addr2, user_phone_info1.addr, 6) == 0) {
                    temp_buf1[7] = 0x01;
                    user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr2, temp_buf1, temp_buf1_len);
                } 
            }
        }
        if (temp_buf1 != NULL) {
            free(temp_buf1);
            temp_buf1 = NULL;
            temp_buf1_len = 0;
        }
        if (temp_buf2 != NULL) {
            free(temp_buf2);
            temp_buf2 = NULL;
            temp_buf2_len = 0;
        }
        return;
    case CMD_RESTORE:
        app_log("CMD_RESTORE");
        if (!get_bt_tws_connect_status()) {
            app_log("tws is no connect, return!!!");
            return;
        }
		offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
		offset += tx_data_package(cmd, offset, 1);
		offset += tx_data_package(0x01, offset, 1);
		offset += tx_data_package(0x01, offset, 1);
		offset += tx_data_package(0x00, offset, 1);
		crc = USER_CRC16(_txbuf, offset);
		offset += tx_data_package((crc&0xFF), offset, 1);
		offset += tx_data_package((crc>>8)&0xFF, offset, 1);
        user_app_data_transpond(_txbuf, offset);
        u8 data_restore[2] = {CMD_SYNC_RESTORE, 0};
        user_send_data_to_both(data_restore, sizeof(data_restore));
        return;
	case CMD_GET_INOUTCHARGESTORE:
		app_log("CMD_GET_INOUTCHARGESTORE");
        u8 ear_in_box_status = 0;
        u8 local_in_box_flag = get_charge_online_flag();
        u8 sibling_in_box_flag = get_tws_sibling_bat_persent() >> 7;
        app_log("local_in_box_flag:%d, sibling_in_box_flag:%d", local_in_box_flag, sibling_in_box_flag);
        if (!local_in_box_flag || !sibling_in_box_flag) {
            // app_log("ear no in the box, return err!!!\n");
            ear_in_box_status = 0;
        } else {
            // app_log("two ear in the box, return ok!!!\n");
            ear_in_box_status = 1;
        }
		offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
		offset += tx_data_package(cmd, offset, 1);
		offset += tx_data_package(0x01, offset, 1);
		offset += tx_data_package(0x01, offset, 1);
		offset += tx_data_package(ear_in_box_status, offset, 1);
		crc = USER_CRC16(_txbuf, offset);
		offset += tx_data_package((crc&0xFF), offset, 1);
		offset += tx_data_package((crc>>8)&0xFF, offset, 1);
		break; 
    default:
        break;
    }
    user_app_data_transpond(_txbuf, offset);
    //APP获取完信息后，回一包铃声状态更新APP
    if (cmd == CMD_GET_INFO) {
        user_app_data_notify(CMD_NOTIFY_SEARCH_EAR);
    }
}

//耳机主动通知APP的写在这
void user_app_data_notify(u8 cmd)
{
    if (app_var.goto_poweroff_flag) {
        app_log("goto poweroff, not notify, return!!!");
        return;
    }
    u32 offset = 0;
    u16 crc = 0;
    switch (cmd) {
    case CMD_NOTIFY_BAT_VALUE:
        app_log("CMD_NOTIFY_BAT_VALUE");
        user_update_bat_value();
        offset += tx_buff_package((u8 *)data_head_notify, offset, sizeof(data_head_notify));
        offset += tx_data_package(0x04, offset, 1);
        offset += tx_data_package(cmd, offset, 1);
        offset += tx_data_package(user_charge_info.ear_bat_percent_l, offset, 1);
        offset += tx_data_package(user_charge_info.ear_bat_percent_r, offset, 1);
        offset += tx_data_package(user_charge_info.box_bat_percent, offset, 1);
        break;
    case CMD_NOTIFY_GAME_MODE:
        app_log("CMD_NOTIFY_GAME_MODE");
        offset += tx_buff_package((u8 *)data_head_notify, offset, sizeof(data_head_notify));
        offset += tx_data_package(0x02, offset, 1);
        offset += tx_data_package(cmd, offset, 1);
        offset += tx_data_package(user_app_info.game_mode, offset, 1);
        break;
    case CMD_NOTIFY_EQ_MODE:
        app_log("CMD_NOTIFY_EQ_MODE");
        offset += tx_buff_package((u8 *)data_head_notify, offset, sizeof(data_head_notify));
        offset += tx_data_package(0x02, offset, 1);
        offset += tx_data_package(cmd, offset, 1);
        offset += tx_data_package(user_app_info.eq_mode, offset, 1);
        break;
    case CMD_NOTIFY_ROLE_SWITCH:
        app_log("CMD_NOTIFY_ROLE_SWITCH");
        user_update_bat_value();
        offset += tx_buff_package((u8 *)data_head_notify, offset, sizeof(data_head_notify));
        offset += tx_data_package(0x04, offset, 1);
        offset += tx_data_package(cmd, offset, 1);
        offset += tx_data_package(user_charge_info.ear_bat_percent_l, offset, 1);
        offset += tx_data_package(user_charge_info.ear_bat_percent_r, offset, 1);
        offset += tx_data_package(user_charge_info.box_bat_percent, offset, 1);
        break;
    case CMD_NOTIFY_SEARCH_EAR:
        app_log("CMD_NOTIFY_SEARCH_EAR");
        u8 channel = tws_api_get_local_channel(); 
        app_log("l status: %d, r status: %d, tws: %d, ch: %c", user_var.find_ear_status_l, user_var.find_ear_status_r, user_var.find_ear_status_tws, channel);
        offset += tx_buff_package((u8 *)data_head_notify, offset, sizeof(data_head_notify));
        offset += tx_data_package(0x03, offset, 1);
        offset += tx_data_package(cmd, offset, 1);
        offset += tx_data_package(user_var.find_ear_status_l, offset, 1);
        offset += tx_data_package(user_var.find_ear_status_r, offset, 1);
        break;
    case CMD_NOTIFY_DEVICE_CHANGE:
        app_log("CMD_NOTIFY_DEVICE_CHANGE");
        u8 user_conn_dev_num = bt_get_total_connect_dev();
        app_log("dev_num:%d, 1t2_mode:%d", user_conn_dev_num, user_app_info.u1t2_mode);
        u16 temp_buf1_len = 0;
        u16 temp_buf2_len = 0;
        if (user_phone_info1.remote_name != NULL) {
            app_log("info1: %s\n", user_phone_info1.remote_name);
            offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
            offset += tx_data_package(cmd, offset, 1);
            offset += tx_data_package(0x01, offset, 1);
            offset += tx_data_package((user_phone_info1.name_len+8), offset, 1);
            offset += tx_data_package(user_conn_dev_num, offset, 1);
            offset += tx_data_package(0x01, offset, 1);
            offset += tx_buff_package((u8 *)user_phone_info1.remote_name, offset, user_phone_info1.name_len);
            offset += tx_buff_package((u8 *)user_phone_info1.addr, offset, 6);
            crc = USER_CRC16(_txbuf, offset);
            offset += tx_data_package((crc&0xFF), offset, 1);
            offset += tx_data_package((crc>>8)&0xFF, offset, 1);
            temp_buf1_len = offset;
            temp_buf1 = (u8 *)malloc(temp_buf1_len);
            memcpy(temp_buf1, _txbuf, temp_buf1_len);
        }
        offset = 0;
        if (user_phone_info2.remote_name != NULL && user_app_info.u1t2_mode == 1) {
            app_log("info2: %s\n", user_phone_info2.remote_name);
            offset += tx_buff_package((u8 *)data_head_tx, offset, sizeof(data_head_tx));
            offset += tx_data_package(cmd, offset, 1);
            offset += tx_data_package(0x01, offset, 1);
            offset += tx_data_package((user_phone_info2.name_len+8), offset, 1);
            offset += tx_data_package(user_conn_dev_num, offset, 1);
            offset += tx_data_package(0x02, offset, 1);
            offset += tx_buff_package((u8 *)user_phone_info2.remote_name, offset, user_phone_info2.name_len);
            offset += tx_buff_package((u8 *)user_phone_info2.addr, offset, 6);
            crc = USER_CRC16(_txbuf, offset);
            offset += tx_data_package((crc&0xFF), offset, 1);
            offset += tx_data_package((crc>>8)&0xFF, offset, 1);
            temp_buf2_len = offset;
            temp_buf2 = (u8 *)malloc(temp_buf2_len);
            memcpy(temp_buf2, _txbuf, temp_buf2_len);
        }
        if (user_app_info.u1t2_mode == 1 && temp_buf2 != NULL) {
            user_app_dev_data_ble_transpond(temp_buf1, temp_buf1_len);
            user_app_dev_data_ble_transpond(temp_buf2, temp_buf2_len);
            if (memcmp(user_app_spp_addr.spp_addr1, addr_all_ff, 6)) {
                if (memcmp(user_app_spp_addr.spp_addr1, user_phone_info1.addr, 6) == 0) {
                    temp_buf1[7] = 0x01;
                    user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr1, temp_buf1, temp_buf1_len);
                    temp_buf2[7] = 0x02;
                    user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr1, temp_buf2, temp_buf2_len);
                    if (memcmp(user_app_spp_addr.spp_addr2, addr_all_ff, 6)) {
                        temp_buf2[7] = 0x01;
                        user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr2, temp_buf2, temp_buf2_len);
                        temp_buf1[7] = 0x02;
                        user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr2, temp_buf1, temp_buf1_len);
                    }
                } else if (memcmp(user_app_spp_addr.spp_addr1, user_phone_info2.addr, 6) == 0) {
                    temp_buf2[7] = 0x01;
                    user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr1, temp_buf2, temp_buf2_len);
                    temp_buf1[7] = 0x02;
                    user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr1, temp_buf1, temp_buf1_len);
                    if (memcmp(user_app_spp_addr.spp_addr2, addr_all_ff, 6)) {
                        temp_buf1[7] = 0x01;
                        user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr2, temp_buf1, temp_buf1_len);
                        temp_buf2[7] = 0x02;
                        user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr2, temp_buf2, temp_buf2_len);
                    }
                }
            }
        } else {
            user_app_dev_data_ble_transpond(temp_buf1, temp_buf1_len);
            if (memcmp(user_app_spp_addr.spp_addr1, addr_all_ff, 6)) {
                if (memcmp(user_app_spp_addr.spp_addr1, user_phone_info1.addr, 6) == 0) {
                    temp_buf1[7] = 0x01;
                    user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr1, temp_buf1, temp_buf1_len);
                } 
            } else if (memcmp(user_app_spp_addr.spp_addr2, addr_all_ff, 6)) {
                if (memcmp(user_app_spp_addr.spp_addr2, user_phone_info1.addr, 6) == 0) {
                    temp_buf1[7] = 0x01;
                    user_app_dev_data_spp_transpond(user_app_spp_addr.spp_addr2, temp_buf1, temp_buf1_len);
                } 
            }
        }
        if (temp_buf1 != NULL) {
            free(temp_buf1);
            temp_buf1 = NULL;
            temp_buf1_len = 0;
        }
        if (temp_buf2 != NULL) {
            free(temp_buf2);
            temp_buf2 = NULL;
            temp_buf2_len = 0;
        }
        return;
    default:
        break;
    }
    user_app_data_transpond(_txbuf, offset);
}


// APP消息转发函数，给连接上的设备都回复
#if 0
void user_app_data_transpond(u8 *data, u16 len)
{
    app_log("user_app_data_transpond");
    put_buf(data, len);
    u32 ret1 = BLE_CMD_RET_SUCESS;
    u32 ret2 = BLE_CMD_RET_SUCESS;
    u16 ble_con_hdl = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl);
    u16 ble_con_hdl1 = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl1);
    if (ble_con_hdl) {
        ret1 = app_ble_att_send_data(rcsp_server_ble_hdl, ATT_CHARACTERISTIC_77777777_7777_7777_7777_777777777777_01_VALUE_HANDLE, data, len, ATT_OP_NOTIFY);
        app_log("ble1 send_data ret1:%d", ret1);
    }
    if (ble_con_hdl1) {
        ret2 = app_ble_att_send_data(rcsp_server_ble_hdl1, ATT_CHARACTERISTIC_77777777_7777_7777_7777_777777777777_01_VALUE_HANDLE, data, len, ATT_OP_NOTIFY);
        app_log("ble2 send_data ret2:%d", ret2);
    }
    if (memcmp(user_app_spp_addr.spp_addr1, addr_all_ff, 6)) {
        app_log("spp1 send_data");
        bt_cmd_prepare_for_addr(user_app_spp_addr.spp_addr1, USER_CTRL_SPP_SEND_DATA, len, data);
    }
    if (memcmp(user_app_spp_addr.spp_addr2, addr_all_ff, 6)) {
        app_log("spp2 send_data");
        bt_cmd_prepare_for_addr(user_app_spp_addr.spp_addr2, USER_CTRL_SPP_SEND_DATA, len, data);
    }
}
#else
void user_app_data_transpond(u8 *data, u16 len)
{
    app_log("user_app_data_transpond");
    bt_rcsp_custom_data_send(0, NULL, data, len, ATT_CHARACTERISTIC_77777777_7777_7777_7777_777777777777_01_VALUE_HANDLE, ATT_OP_NOTIFY);
}
#endif

void user_app_dev_data_ble_transpond(u8 *temp_buf, u16 temp_buf_len)
{
    u32 ret1 = BLE_CMD_RET_SUCESS;
    u32 ret2 = BLE_CMD_RET_SUCESS;
    u16 ble_con_hdl = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl);
    u16 ble_con_hdl1 = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl1);
    if (ble_con_hdl) {
        ret1 = app_ble_att_send_data(rcsp_server_ble_hdl, ATT_CHARACTERISTIC_77777777_7777_7777_7777_777777777777_01_VALUE_HANDLE, temp_buf, temp_buf_len, ATT_OP_NOTIFY);
        app_log("ble1 dev send_data ret1:%d", ret1);
    }
    if (ble_con_hdl1) {
        ret2 = app_ble_att_send_data(rcsp_server_ble_hdl1, ATT_CHARACTERISTIC_77777777_7777_7777_7777_777777777777_01_VALUE_HANDLE, temp_buf, temp_buf_len, ATT_OP_NOTIFY);
        app_log("ble2 dev send_data ret2:%d", ret2);
    }
}

void user_app_dev_data_spp_transpond(u8 *spp_addr, u8 *temp_buf, u16 temp_buf_len)
{
    u32 ret1 = BLE_CMD_RET_SUCESS;
    u32 ret2 = BLE_CMD_RET_SUCESS;
    app_log("spp dev send_data");
    bt_cmd_prepare_for_addr(spp_addr, USER_CTRL_SPP_SEND_DATA, temp_buf_len, temp_buf);
}

/*************************************************************/
