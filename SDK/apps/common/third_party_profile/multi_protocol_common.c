#include "system/includes.h"
#include "multi_protocol_main.h"
#include "classic/tws_api.h"
#include "ble_user.h"
#include "app_msg.h"

#if (BT_AI_SEL_PROTOCOL & (RCSP_MODE_EN | GFPS_EN))

extern u8 addr_all_ff[];
extern APP_SPP_ADDR user_app_spp_addr;
static void check_connetion_updata_deal(u16 handle);
static void multi_protocol_spp_connect_callback(void *hdl, u8 state);
static void multi_protocol_spp_recieve_callback(void *hdl, void *remote_addr, u8 *buf, u16 len);
static void multi_protocol_spp_state_callback(void *hdl, void *remote_addr, u8 state);
static void multi_protocol_spp_wakeup_callback(void *hdl);
void user_spp_addr_init();
void user_spp_addr_add(u8 *addr);
void user_spp_addr_del(u8 *addr);
void user_spp_addr_sync_tx();

#if 1//0
#define log_info(x, ...)       printf("[MULTI_PROTOCOL]" x " ", ## __VA_ARGS__)
#define log_info_hexdump       put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

void *multi_protocol_ble_hdl = NULL;
void *multi_protocol_spp_hdl = NULL;
extern void *rcsp_server_ble_hdl;
extern void *gfps_app_ble_hdl;


static hci_con_handle_t con_handle;
static char gap_device_name[BT_NAME_LEN_MAX] = "jl_ble_test";
static u8 gap_device_name_len = 0;

static const char *const phy_result[] = {
    "None",
    "1M",
    "2M",
    "Coded"
};

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

static uint16_t multi_protocol_att_read_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{

    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;
    log_info("read_callback, handle= 0x%04x,buffer= %08x\n", handle, (u32)buffer);

    switch (handle) {
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        att_value_len = strlen(gap_device_name);

        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }

        if (buffer) {
            memcpy(buffer, &gap_device_name[offset], buffer_size);
            att_value_len = buffer_size;
            log_info("\n------read gap_name: %s \n", gap_device_name);
        }

        break;
    default:
        break;
    }

    log_info("att_value_len= %d\n", att_value_len);
    return att_value_len;

}

static int multi_protocol_att_write_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;
    u16 handle = att_handle;

    switch (handle) {
    // case ATT_CHARACTERISTIC_77777777_7777_7777_7777_777777777777_01_VALUE_HANDLE:
    //     log_info("ATT_CHARACTERISTIC_77777777_7777_7777_7777_777777777777_01_VALUE_HANDLE, connection_handle= 0x%d", connection_handle);
    //     put_buf(buffer, buffer_size);
    //     user_app_data_deal(buffer, buffer_size, 0);
    //     break;

    // case ATT_CHARACTERISTIC_77777777_7777_7777_7777_777777777777_01_CLIENT_CONFIGURATION_HANDLE:
    //     log_info("ATT_CHARACTERISTIC_77777777_7777_7777_7777_777777777777_01_CLIENT_CONFIGURATION_HANDLE");
    //     set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
    //     check_connetion_updata_deal(handle);
    //     log_info("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
    //     att_set_ccc_config(handle, buffer[0]);
    //     break;
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        log_info("ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE");
        tmp16 = BT_NAME_LEN_MAX;
        if ((offset >= tmp16) || (offset + buffer_size) > tmp16) {
            break;
        }

        if (offset == 0) {
            memset(gap_device_name, 0x00, BT_NAME_LEN_MAX);
        }
        memcpy(&gap_device_name[offset], buffer, buffer_size);
        log_info("\n------write gap_name:");
        break;
    default:
        break;
    }

    return 0;
}

static void send_request_connect_parameter(u8 table_index)
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
    if (con_handle) {
        ble_user_cmd_prepare(BLE_CMD_REQ_CONN_PARAM_UPDATE, 2, con_handle, param);
    }
}

static void check_connetion_updata_deal(u16 handle)
{
    if (connection_update_enable) {
        con_handle = handle;
        if (connection_update_cnt < (CONN_PARAM_TABLE_CNT * CONN_TRY_NUM)) {
            send_request_connect_parameter(connection_update_cnt / CONN_TRY_NUM);
        }
    }
}

void notify_update_connect_parameter(u8 table_index)
{
    u8 conn_update_param_index_record = conn_update_param_index;
    if ((u8) - 1 != table_index) {
        conn_update_param_index = 1;
        send_request_connect_parameter(table_index);
    } else {
        if (connection_update_cnt >= (CONN_PARAM_TABLE_CNT * CONN_TRY_NUM)) {
            log_info("connection_update_cnt >= CONN_PARAM_TABLE_CNT");
            connection_update_cnt = 0;
        }
        send_request_connect_parameter(connection_update_cnt / CONN_TRY_NUM);
    }
    conn_update_param_index = conn_update_param_index_record;
}

static void connection_update_complete_success(u8 *packet)
{
    int conn_handle, conn_interval, conn_latency, conn_timeout;

    conn_handle = hci_subevent_le_connection_update_complete_get_connection_handle(packet);
    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
    conn_latency = hci_subevent_le_connection_update_complete_get_conn_latency(packet);
    conn_timeout = hci_subevent_le_connection_update_complete_get_supervision_timeout(packet);

    log_info("conn_handle = %d\n", conn_handle);
    log_info("conn_interval = %d\n", conn_interval);
    log_info("conn_latency = %d\n", conn_latency);
    log_info("conn_timeout = %d\n", conn_timeout);
}

static void multi_protocol_cbk_packet_handler(void *hdl, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    int mtu;
    u32 tmp;
    const char *attribute_name;
    u16 temp_con_handle;
    int temp_index;

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
            log_info("ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE\n");
        case ATT_EVENT_CAN_SEND_NOW:
            /* can_send_now_wakeup(); */
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE: {
                if (!hci_subevent_le_connection_complete_get_role(packet)) {
                    // GATT_ROLE_CLIENT
                    break;
                } else {
                    // GATT_ROLE_SERVER;
                }
                temp_con_handle = little_endian_read_16(packet, 4);
                log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x\n", temp_con_handle);
                log_info_hexdump(packet + 7, 7);
                connection_update_complete_success(packet + 8);

                temp_index = get_att_handle_idle();
                if (temp_index == -1) {
                    printf("BLE connect handle list is FULL !!!\n");
                    ble_op_disconnect_ext(temp_con_handle, 0x13);
                }
                ble_op_multi_att_send_conn_handle(temp_con_handle, temp_index, 0);
            }

            break;

            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                temp_con_handle = little_endian_read_16(packet, 4);
                connection_update_complete_success(packet);
                break;

            case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                temp_con_handle = little_endian_read_16(packet, 3);
                log_info("HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE\n");
                break;

            case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
                temp_con_handle = little_endian_read_16(packet, 4);
                if (temp_con_handle) {
                    log_info("HCI_SUBEVENT_LE_PHY_UPDATE %s\n", hci_event_le_meta_get_phy_update_complete_status(packet) ? "Fail" : "Succ");
                    log_info("Tx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_tx_phy(packet)]);
                    log_info("Rx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_rx_phy(packet)]);
                }
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            temp_con_handle = little_endian_read_16(packet, 3);

            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: %0x\n", packet[5]);
            multi_att_clear_ccc_config(temp_con_handle);

            temp_index = get_att_handle_index(temp_con_handle);
            ble_op_multi_att_send_conn_handle(0, temp_index, 0);

            con_handle = 0;
            connection_update_cnt = 0;
            break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            temp_con_handle = little_endian_read_16(packet, 2);

            mtu = att_event_mtu_exchange_complete_get_MTU(packet) - 3;
            log_info("ATT MTU = %u\n", mtu);
            ble_user_cmd_prepare(BLE_CMD_MULTI_ATT_MTU_SIZE, 2, temp_con_handle, mtu);
            break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("--- HCI_EVENT_VENDOR_REMOTE_TEST\n");
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            temp_con_handle = little_endian_read_16(packet, 2);
            tmp = little_endian_read_16(packet, 4);
            log_info("-update_rsp: %02x\n", tmp);
            if (tmp) {
                connection_update_cnt++;
                log_info("remoter reject!!!\n");
                check_connetion_updata_deal(temp_con_handle);
            } else {
                connection_update_cnt = (CONN_PARAM_TABLE_CNT * CONN_TRY_NUM);
            }
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            temp_con_handle = little_endian_read_16(packet, 3);

            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d\n", packet[2]);
            if (packet[2] == 0) {
                /*ENCRYPTION is ok */
                check_connetion_updata_deal(temp_con_handle);
                /*要配对加密后才能检测对方系统类型*/
                //att_server_set_check_remote(con_handle, rcsp_att_check_remote_result);
            }
            break;
        }
        break;
    }
}

static void multi_protocol_cbk_sm_packet_handler(void *hdl, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    sm_just_event_t *event = (void *)packet;
    u32 tmp32;
    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            log_info("Just Works Confirmed.\n");
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            log_info_hexdump(packet, size);
            memcpy(&tmp32, event->data, 4);
            log_info("Passkey display: %06u.\n", tmp32);
            break;

        case SM_EVENT_PAIR_PROCESS:
            /* log_info("======PAIR_PROCESS: %02x\n", event->data[0]); */
            /* put_buf(event->data, 4); */
            switch (event->data[0]) {
            case SM_EVENT_PAIR_SUB_RECONNECT_START:
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


#if (BT_AI_SEL_PROTOCOL & RCSP_MODE_EN)
extern void *bt_rcsp_spp_hdl;
extern void *rcsp_server_ble_hdl;
static u8 rcsp_spp_connected = 0;
static u8 rcsp_ble_connected = 0;
#endif

#if (BT_AI_SEL_PROTOCOL & GFPS_EN)
extern void *gfps_app_spp_hdl;
extern void *gfps_app_ble_hdl;
static u8 gfps_spp_connected = 0;
static u8 gfps_ble_connected = 0;
#endif

// type   0:ble   1:spp
// state  0:disconnected    1:connected
static u8 connected_state[2] = {0};
static void multi_protocol_connect_callback(void *hdl, u8 state, u8 type)
{
    log_info("multi_protocol_connect_callback hdl:0x%x state:%d type:%d\n", (u32)hdl, state, type);
    switch (state) {
    case APP_PROTOCOL_CONNECTED:
        log_info("APP_PROTOCOL_CONNECTED %x\n", (u32)hdl);
        // close all other hdl
#if (BT_AI_SEL_PROTOCOL & RCSP_MODE_EN)
        log_info("rcsp_spp_hdl:%x\n", (u32)bt_rcsp_spp_hdl);
        log_info("rcsp_ble_hdl:%x\n", (u32)rcsp_server_ble_hdl);
        if (hdl == rcsp_server_ble_hdl) {
            rcsp_ble_connected = 1;
            /* app_ble_adv_enable(hdl, 0); */
        }

        if (hdl == bt_rcsp_spp_hdl) {
            rcsp_spp_connected = 1;
        }

        /* if ((hdl != bt_rcsp_spp_hdl) && (hdl != rcsp_server_ble_hdl)) { */
        /* log_info("close spp hdl rcsp\n"); */
        /* app_ble_sleep_hdl(rcsp_server_ble_hdl, 1); */
        /* app_spp_sleep_hdl(bt_rcsp_spp_hdl, 1); */
        /* } */
#endif
#if (BT_AI_SEL_PROTOCOL & GFPS_EN)
        log_info("gfps_spp_hdl:%x\n", (u32)gfps_app_spp_hdl);
        log_info("gfps_ble_hdl:%x\n", (u32)gfps_app_ble_hdl);
        if (hdl == gfps_app_ble_hdl) {
            gfps_ble_connected = 1;
        }

        if (hdl == gfps_app_spp_hdl) {
            gfps_spp_connected = 1;
        }

        if ((hdl != gfps_app_ble_hdl) \
            && (hdl != gfps_app_spp_hdl) \
            && (gfps_spp_connected == 0) \
            && (gfps_ble_connected == 0) \
           ) {
            log_info("close ble hdl gfps\n");
            app_ble_sleep_hdl(gfps_app_ble_hdl, 1);
            app_spp_sleep_hdl(gfps_app_spp_hdl, 1);
        }
#endif
        connected_state[type]++;
        break;
    case APP_PROTOCOL_DISCONNECTED:
        log_info("APP_PROTOCOL_DISCONNECTED %x\n", (u32)hdl);

#if (BT_AI_SEL_PROTOCOL & RCSP_MODE_EN)
        if (hdl == rcsp_server_ble_hdl) {
            rcsp_ble_connected = 0;
        }

        if (hdl == bt_rcsp_spp_hdl) {
            rcsp_spp_connected = 0;
        }
#endif

#if (BT_AI_SEL_PROTOCOL & GFPS_EN)
        if (hdl == gfps_app_ble_hdl) {
            gfps_ble_connected = 0;
        }

        if (hdl == gfps_app_spp_hdl) {
            gfps_spp_connected = 0;
        }
#endif

        connected_state[type]--;
        if ((connected_state[0] == 0) && (connected_state[1] == 0)) {
            log_info("wakeup all protocol !\n");
#if (BT_AI_SEL_PROTOCOL & RCSP_MODE_EN)
            app_ble_sleep_hdl(rcsp_server_ble_hdl, 0);
            app_spp_sleep_hdl(bt_rcsp_spp_hdl, 0);
#endif
#if (BT_AI_SEL_PROTOCOL & GFPS_EN)
            app_ble_sleep_hdl(gfps_app_ble_hdl, 0);
            app_spp_sleep_hdl(gfps_app_spp_hdl, 0);
#endif
        }
        break;
    }
}

static void multi_protocol_ble_connect_callback(void *hdl, u8 state)
{
    multi_protocol_connect_callback(hdl, state, 0);
}

void multi_protocol_common_callback_init(void)
{
    user_spp_addr_init();
    if (multi_protocol_ble_hdl == NULL) {
        multi_protocol_ble_hdl = app_ble_hdl_alloc();
        if (multi_protocol_ble_hdl == NULL) {
            log_info("multi_protocol_ble_hdl alloc err !!\n");
            return;
        }
    }

    const char *name_p;
    name_p = bt_get_local_name();
    gap_device_name_len = strlen(name_p);
    if (gap_device_name_len > BT_NAME_LEN_MAX - 1) {
        gap_device_name_len = BT_NAME_LEN_MAX - 1;//1是结束符
    }
    memcpy(gap_device_name, name_p, gap_device_name_len);

    app_ble_att_read_callback_register(multi_protocol_ble_hdl, multi_protocol_att_read_callback);
    app_ble_att_write_callback_register(multi_protocol_ble_hdl, multi_protocol_att_write_callback);
    app_ble_att_server_packet_handler_register(multi_protocol_ble_hdl, multi_protocol_cbk_packet_handler);
    app_ble_hci_event_callback_register(multi_protocol_ble_hdl, multi_protocol_cbk_packet_handler);
    app_ble_l2cap_packet_handler_register(multi_protocol_ble_hdl, multi_protocol_cbk_packet_handler);
    app_ble_sm_event_callback_register(multi_protocol_ble_hdl, multi_protocol_cbk_sm_packet_handler);
    app_ble_protocol_connect_callback_register(multi_protocol_ble_hdl, multi_protocol_ble_connect_callback);

    if (multi_protocol_spp_hdl == NULL) {
        multi_protocol_spp_hdl = app_spp_hdl_alloc(0);
        if (multi_protocol_spp_hdl == NULL) {
            log_info("multi_protocol_spp_hdl alloc err !!\n");
            return;
        }
    }

    app_spp_protocol_connect_callback_register(multi_protocol_spp_hdl, multi_protocol_spp_connect_callback);
    app_spp_recieve_callback_register(multi_protocol_spp_hdl, multi_protocol_spp_recieve_callback);
    app_spp_state_callback_register(multi_protocol_spp_hdl, multi_protocol_spp_state_callback);
    app_spp_wakeup_callback_register(multi_protocol_spp_hdl, multi_protocol_spp_wakeup_callback);
}

#endif


/*************************************************************/
//   双连SPP流程写在这
/*************************************************************/
static void multi_protocol_spp_connect_callback(void *hdl, u8 state)
{
    multi_protocol_connect_callback(hdl, state, 1);
}

static void multi_protocol_spp_recieve_callback(void *hdl, void *remote_addr, u8 *buf, u16 len)
{
    log_info("spp_recieve hdl:0x%x len:%d\n", hdl, len);
    put_buf(buf, len);
    // user_app_data_deal(buf, len, 1);
}

static void multi_protocol_spp_state_callback(void *hdl, void *remote_addr, u8 state)
{
    log_info("spp_state hdl:0x%x state:%d\n", hdl, state);
    put_buf(remote_addr, 6);
    if (state == 1) {
        user_spp_addr_add(remote_addr);
        user_spp_addr_sync_tx();
    } else if (state == 2) {
        user_spp_addr_del(remote_addr);
        user_spp_addr_sync_tx();
    }
}

static void multi_protocol_spp_wakeup_callback(void *hdl)
{
    log_info("spp_wakeup hdl:0x%x\n", hdl);
}

void user_spp_addr_init()
{
    log_info("user_spp_addr_init");
    memcpy(user_app_spp_addr.spp_addr1, addr_all_ff, 6);
    memcpy(user_app_spp_addr.spp_addr2, addr_all_ff, 6);
}

void user_spp_addr_add(u8 *addr)
{
    log_info("user_spp_addr_add");
    if (addr) {
        if (memcmp(user_app_spp_addr.spp_addr1, addr_all_ff, 6) == 0) {
            memcpy(user_app_spp_addr.spp_addr1, addr, 6);
        } else if (memcmp(user_app_spp_addr.spp_addr2, addr_all_ff, 6) == 0) {
            memcpy(user_app_spp_addr.spp_addr2, addr, 6);
        } else {
            memcpy(user_app_spp_addr.spp_addr1, addr, 6);
        }
    }
}

void user_spp_addr_del(u8 *addr)
{
    log_info("user_spp_addr_del");
    if (addr) {
        if (memcmp(user_app_spp_addr.spp_addr1, addr, 6) == 0) {
            memcpy(user_app_spp_addr.spp_addr1, addr_all_ff, 6);
        } else if (memcmp(user_app_spp_addr.spp_addr2, addr, 6) == 0) {
            memcpy(user_app_spp_addr.spp_addr2, addr_all_ff, 6);
        } else {
            log_info("spp addr del err!");
        }
    }
}

void user_spp_addr_sync_tx()
{
    log_info("user_spp_addr_sync_tx role:%d", tws_api_get_role());
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        u8 spp_addr_buf[13];
        spp_addr_buf[0] = CMD_SYNC_SPP_ADDR;
        memcpy(spp_addr_buf+1, user_app_spp_addr.spp_addr1, 6);
        memcpy(spp_addr_buf+7, user_app_spp_addr.spp_addr2, 6);
        user_send_data_to_sibling(spp_addr_buf, sizeof(spp_addr_buf));
    }
}

void user_spp_addr_sync_rx(u8 *data, u16 len)
{
    memcpy(user_app_spp_addr.spp_addr1, data+1, 6);
    memcpy(user_app_spp_addr.spp_addr2, data+7, 6);
}

/*************************************************************/


