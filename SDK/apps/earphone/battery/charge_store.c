#include "init.h"
#include "app_config.h"
#include "system/includes.h"
#include "asm/charge.h"
#include "asm/chargestore.h"
#include "user_cfg.h"
#include "app_chargestore.h"
#include "device/vm.h"
#include "btstack/avctp_user.h"
#include "app_power_manage.h"
#include "app_action.h"
#include "app_main.h"
#include "app_charge.h"
#include "classic/tws_api.h"
#include "update.h"
#include "bt_ble.h"
#include "bt_tws.h"
#include "app_action.h"
#include "bt_common.h"
#include "app_testbox.h"
#include "app_charge.h"
#include "battery_manager.h"

#define LOG_TAG_CONST       APP_CHARGESTORE
#define LOG_TAG             "[APP_CHARGESTORE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#include "led/led_ui_manager.h"

#if 1
#define chargestore_log(x, ...)       g_printf("[USER_CHARGESTORE]" x " ", ## __VA_ARGS__)
#define chargestore_log_hexdump       g_printf("[USER_CHARGESTORE]chargestore_log_hexdump:");\
                                        put_buf
#else
#define chargestore_log(...)
#define chargestore_log_hexdump(...)
#endif
extern USER_VAR user_var;
extern CHARGE_INFO user_charge_info;
extern BT_DEV_ADDR user_bt_dev_addr;
void user_sys_restore(u8 type);

#define USER_NEW_BOX_MODE   1

struct rx_data_st {
    unsigned char *data;
    unsigned int len;
    void (*write)(const unsigned char *buf, unsigned int len);
};
void ldoin_uart_data_deal(u8 *buff, u8 len);
void user_product_test_handle(struct rx_data_st *uart_bus, u8 *buf, u16 len);
u8 user_check_uart_test_cmd(u8 *data, u16 len);
u8 user_check_goto_bt_mode(u8 *data, u16 len);
extern void bt_get_tws_local_addr(u8 *addr);
extern u8 tws_get_sibling_addr(u8 *addr, int *result);
extern void testbox_set_ex_enter_storage_mode_flag(u8 en);
extern u8 set_channel_by_code_or_res(void);

extern void tws_api_set_connect_aa_allways(u32 connect_aa);
extern u32 tws_le_get_pair_aa(void);
extern u32 tws_le_get_connect_aa(void);
extern u32 tws_le_get_search_aa(void);
extern void bt_get_vm_mac_addr(u8 *addr);
extern bool get_tws_sibling_connect_state(void);
extern u8 bt_get_total_connect_dev(void);
extern void cpu_reset();
extern u32 dual_bank_update_exist_flag_get(void);
extern u32 classic_update_task_exist_flag_get(void);
extern char tws_api_get_local_channel();
extern u16 bt_get_tws_device_indicate(u8 *tws_device_indicate);
extern void bt_tws_remove_pairs();
extern const u8 *bt_get_mac_addr();
bool app_have_mode(void);

//-----------------------------------------------------------------------------------
//-----------以下配对相关的接口,提供充电舱和蓝牙测试盒-------------------------------
//-----------------------------------------------------------------------------------
#if (TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE)
void chargestore_set_tws_channel_info(u8 channel)
{
    if ((channel == 'L') || (channel == 'R')) {
        syscfg_write(CFG_CHARGESTORE_TWS_CHANNEL, &channel, 1);
    }
}

u8 chargestore_get_tws_channel_info(void)
{
    u8 channel = 'U';
    syscfg_read(CFG_CHARGESTORE_TWS_CHANNEL, &channel, 1);
    return channel;
}

bool chargestore_set_tws_remote_info(u8 *data, u8 len)
{
    u8 i;
    bool ret = true;
    int w_ret = 0;
    u8 remote_addr[6];
    u8 common_addr[6];
    u8 local_addr[6];
    CHARGE_STORE_INFO *charge_store_info = (CHARGE_STORE_INFO *)data;
    if (len > sizeof(CHARGE_STORE_INFO)) {
        log_error("len err\n");
        return false;
    }
    //set remote addr
    syscfg_read(CFG_TWS_REMOTE_ADDR, remote_addr, sizeof(remote_addr));
    y_printf("===============================");
    put_buf(charge_store_info->tws_local_addr, 6);
    if (memcmp(remote_addr, charge_store_info->tws_local_addr, sizeof(remote_addr))) {
        ret = false;
    }

    if (sizeof(remote_addr) != syscfg_write(CFG_TWS_REMOTE_ADDR, charge_store_info->tws_local_addr, sizeof(remote_addr))) {
        w_ret = -1;
    }

    bt_get_tws_local_addr(local_addr);

#if (CONFIG_TWS_COMMON_ADDR_SELECT == CONFIG_TWS_COMMON_ADDR_AUTO)
    //set common addr
    syscfg_read(CFG_TWS_COMMON_ADDR, common_addr, sizeof(common_addr));

    for (i = 0; i < sizeof(common_addr); i++) {
        if (common_addr[i] != (u8)(local_addr[i] + charge_store_info->tws_local_addr[i])) {
            ret = false;
        }
        common_addr[i] = local_addr[i] + charge_store_info->tws_local_addr[i];
    }

    if (sizeof(common_addr) != syscfg_write(CFG_TWS_COMMON_ADDR, common_addr, sizeof(common_addr))) {
        w_ret = -3;
    }
#endif

#ifndef CONFIG_NEW_BREDR_ENABLE
    if (chargestore_get_tws_channel_info() == 'L') {
        if (tws_le_get_connect_aa() != tws_le_get_pair_aa()) {
            ret = false;
        }
        tws_api_set_connect_aa_allways(tws_le_get_pair_aa());
    } else {
        if (tws_le_get_connect_aa() != charge_store_info->pair_aa) {
            ret = false;
        }
        tws_api_set_connect_aa_allways(charge_store_info->pair_aa);
    }
#endif

#if TCFG_TEST_BOX_ENABLE
    if (testbox_get_status() && (0 == w_ret)) {
        u8 cmd = CMD_BOX_TWS_REMOTE_ADDR;
        chargestore_api_write(&cmd, 1);
        testbox_set_testbox_tws_paired(1);
    }
#endif
    return ret;
}

void chargestore_clean_tws_conn_info(u8 type)
{
    CHARGE_STORE_INFO charge_store_info;
    log_info("chargestore_clean_tws_conn_info=%d\n", type);
    if (type == TWS_DEL_TWS_ADDR) {
        log_info("TWS_DEL_TWS_ADDR\n");
    } else if (type == TWS_DEL_PHONE_ADDR) {
        log_info("TWS_DEL_PHONE_ADDR\n");
    } else if (type == TWS_DEL_ALL_ADDR) {
        log_info("TWS_DEL_ALL_ADDR\n");
    }
    memset(&charge_store_info, 0xff, sizeof(CHARGE_STORE_INFO));
    syscfg_write(CFG_TWS_REMOTE_ADDR, charge_store_info.tws_remote_addr, sizeof(charge_store_info.tws_remote_addr));
}


u16 chargestore_get_tws_remote_info(u8 *data)
{
    u16 ret_len = 0;
#if TCFG_USER_TWS_ENABLE
    u16 device_ind;
    CHARGE_STORE_INFO *charge_store_info = (CHARGE_STORE_INFO *)data;

    bt_get_tws_local_addr(charge_store_info->tws_local_addr);
    bt_get_vm_mac_addr(charge_store_info->tws_mac_addr);
#if TCFG_TEST_BOX_ENABLE
    if (testbox_get_keep_tws_conn_flag()) {
        memcpy(charge_store_info->tws_mac_addr, bt_get_mac_addr(), 6);
    }
#endif
#ifndef CONFIG_NEW_BREDR_ENABLE
    charge_store_info->search_aa = tws_le_get_search_aa();
    charge_store_info->pair_aa = tws_le_get_pair_aa();
#endif

#if (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_AS_LEFT) \
    ||(CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_AS_RIGHT) \
    ||(CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_EXTERN_DOWN_AS_LEFT) \
    ||(CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_EXTERN_UP_AS_LEFT) \
    ||(CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_EXTERN_DOWN_AS_RIGHT) \
    ||(CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_EXTERN_UP_AS_RIGHT)
    charge_store_info->local_channel = tws_api_get_local_channel();
#else
    charge_store_info->local_channel = 'U';
#endif
    charge_store_info->device_ind = bt_get_tws_device_indicate(NULL);
    charge_store_info->reserved_data = chargestore_api_crc8(data, sizeof(CHARGE_STORE_INFO) - 2);
#if TCFG_TEST_BOX_ENABLE
    if (testbox_get_status()) {
        charge_store_info->reserved_data = 0;
    }
#endif
    ret_len = sizeof(CHARGE_STORE_INFO);
#endif
    return ret_len;
}

#endif //TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE







//-----------------------------------------------------------------------------------
//-----------分割线---------以下是充电舱流程实现-------------------------------------
//-----------------------------------------------------------------------------------
#if TCFG_CHARGESTORE_ENABLE

struct chargestore_info {
    int timer;
    int shutdown_timer;
    u8 version;
    u8 chip_type;
    u8 max_packet_size;//充电舱端一包的最大值
    u8 bt_init_ok;//蓝牙协议栈初始化成功
    u8 power_level;//本机记录的充电舱电量百分比
    u8 pre_power_lvl;
    u8 sibling_chg_lev;//对耳同步的充电舱电量
    u8 power_status;//充电舱供电状态 0:断电 5V不在线 1:升压 5V在线
    u8 cover_status;//充电舱盖子状态 0:合盖 1:开盖
    u8 connect_status;//通信成功
    u8 ear_number;//盒盖时耳机在线数
    u8 channel;//左右
    u8 tws_power;//对耳的电量
    u8 power_sync;//第一次获取到充电舱电量时,同步到对耳
    u8 pair_flag;//配对标记
    u8 close_ing;//等待合窗超时
    u8 active_disconnect;//主动断开连接
    u8 switch2bt;
};

static struct chargestore_info info = {
    .power_status = 1,
    .ear_number = 1,
    .tws_power = 0xff,
    .power_level = 0xff,
    .sibling_chg_lev = 0xff,
    .max_packet_size = 32,
    .channel = 'U',
};

#define __this  (&info)
static u8 local_packet[36];
static CHARGE_STORE_INFO read_info, write_info;
static u8 read_index, write_index;

u8 chargestore_get_power_level(void)
{
    if ((__this->power_level == 0xff) ||
        ((!get_charge_online_flag()) &&
         (__this->sibling_chg_lev != 0xff))) {
        return __this->sibling_chg_lev;
    }
    return __this->power_level;
}

u8 chargestore_get_power_status(void)
{
    return __this->power_status;
}

u8 chargestore_get_cover_status(void)
{
    return __this->cover_status;
}

u8 chargestore_get_earphone_online(void)
{
    return __this->ear_number;
}

void chargestore_set_earphone_online(u8 ear_number)
{
    __this->ear_number = ear_number;
}

void chargestore_set_pair_flag(u8 pair_flag)
{
    __this->pair_flag = pair_flag;
}

void chargestore_set_active_disconnect(u8 active_disconnect)
{
    __this->active_disconnect = active_disconnect;
}

u8 chargestore_get_earphone_pos(void)
{
    u8 channel = 'U';
    channel = chargestore_get_tws_channel_info();
    log_info("get_ear_channel = %c\n", channel);
    return channel;
}

u8 chargestore_get_sibling_power_level(void)
{
    return __this->tws_power;
}

static void set_tws_sibling_charge_level(void *_data, u16 len, bool rx)
{
    if (rx) {
        u8 *data = (u8 *)_data;
        chargestore_set_sibling_chg_lev(data[0]);
    }
}

REGISTER_TWS_FUNC_STUB(charge_level_stub) = {
    .func_id = TWS_FUNC_ID_CHARGE_SYNC,
    .func    = set_tws_sibling_charge_level,
};


int chargestore_sync_chg_level(void)
{
    int err = -1;
    if (app_in_mode(APP_MODE_BT) && !app_var.goto_poweroff_flag) {
        err = tws_api_send_data_to_sibling((u8 *)&__this->power_level, 1,
                                           TWS_FUNC_ID_CHARGE_SYNC);
    }
    return err;
}

void chargestore_set_sibling_chg_lev(u8 chg_lev)
{
    __this->sibling_chg_lev = chg_lev;
}

void chargestore_set_power_level(u8 power)
{
    __this->power_level = power;
}

u8 chargestore_check_going_to_poweroff(void)
{
    return __this->close_ing;
}

void chargestore_shutdown_reset(void)
{
    if (__this->shutdown_timer) {
        sys_timer_del(__this->shutdown_timer);
        __this->shutdown_timer = 0;
    }
}

void chargestore_set_bt_init_ok(u8 flag)
{
    __this->bt_init_ok = flag;
}

void chargestore_shutdown_do(void *priv)
{
    log_info("chargestore shutdown!\n");
    power_set_soft_poweroff();
}

void chargestore_event_to_user(u8 *packet, u8 event, u8 size)
{
    struct chargestore_event evt;

    if (packet != NULL) {
        if (size > sizeof(local_packet)) {
            return;
        }
        memcpy(local_packet, packet, size);
    }
    evt.event   = event;
    evt.packet  = local_packet;
    evt.size    = size;

    os_taskq_post_type("app_core", MSG_FROM_CHARGE_STORE,
                       (sizeof(evt) + 3) / 4, (int *)&evt);
}

bool chargestore_check_data_succ(u8 *data, u8 len)
{
    u16 crc;
    u16 device_ind;
    CHARGE_STORE_INFO *charge_store_info = (CHARGE_STORE_INFO *)data;
    if (len > sizeof(CHARGE_STORE_INFO)) {
        log_error("len err\n");
        return false;
    }
    crc = chargestore_api_crc8(data, len - 2);
    if (crc != charge_store_info->reserved_data) {
        log_error("crc err\n");
        return false;
    }
    device_ind = bt_get_tws_device_indicate(NULL);
    if (device_ind != charge_store_info->device_ind) {
        log_error("device_ind err\n");
        return false;
    }
    return true;
}

u16 chargestore_f95_read_tws_remote_info(u8 *data, u8 flag)
{
    u8 read_len;
    u8 *pbuf = (u8 *)&read_info;
    if (flag) {//first packet
        read_index = 0;
        chargestore_get_tws_remote_info((u8 *)&read_info);
    }
    read_len = sizeof(read_info) - read_index;
    read_len = (read_len > (__this->max_packet_size - 1)) ? (__this->max_packet_size - 1) : read_len;
    memcpy(data, pbuf + read_index, read_len);
    read_index += read_len;
    return read_len;
}

u16 chargestore_f95_write_tws_remote_info(u8 *data, u8 len, u8 flag)
{
    u8 write_len;
    u8 *pbuf = (u8 *)&write_info;
    if (flag) {
        write_index = 0;
        memset(&write_info, 0, sizeof(write_info));
    }
    write_len = sizeof(write_info) - write_index;
    write_len = (write_len >= len) ? len : write_len;
    memcpy(pbuf + write_index, data, write_len);
    write_index += write_len;
    return write_len;
}


void chargestore_timeout_deal(void *priv)
{
    __this->timer = 0;
    __this->close_ing = 0;
    if (!__this->cover_status || __this->active_disconnect) {
        //当前为合盖或者主动断开连接
        if (!app_in_mode(APP_MODE_IDLE)) {
            y_printf("chargestore_timeout_deal\n");
            sys_enter_soft_poweroff(POWEROFF_RESET);
        }
    } else {

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)

        /* if ((!bt_get_total_connect_dev()) && (tws_api_get_role() == TWS_ROLE_MASTER) && (get_bt_tws_connect_status())) { */
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            printf("charge_icon_ctl...\n");
            bt_ble_icon_reset();
#if CONFIG_NO_DISPLAY_BUTTON_ICON
            if (bt_get_total_connect_dev()) {
                //蓝牙未真正断开;重新广播
                bt_ble_icon_open(ICON_TYPE_RECONNECT);
            } else {
                //蓝牙未连接，重新开可见性
                bt_ble_icon_open(ICON_TYPE_INQUIRY);
            }
#else
            //不管蓝牙是否连接，默认打开
            bt_ble_icon_open(ICON_TYPE_RECONNECT);
#endif

        }
#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV_RCSP)
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            printf("charge_icon_ctl...\n");
#if CONFIG_NO_DISPLAY_BUTTON_ICON
            if (bt_get_total_connect_dev()) {
                //蓝牙未真正断开;重新广播
                bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_CONNECTED, 1);
            } else {
                //蓝牙未连接，重新开可见性
                bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_UNCONNECTED, 1);
            }
#else
            //不管蓝牙是否连接，默认打开
            bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_CONNECTED, 1);
#endif

        }
#endif

    }
    __this->ear_number = 1;
}

void chargestore_set_phone_disconnect(void)
{
#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
#if (!CONFIG_NO_DISPLAY_BUTTON_ICON)
    if (__this->pair_flag && get_tws_sibling_connect_state()) {
        //check ble inquiry
        //printf("get box log_key...con_dev=%d\n",bt_get_total_connect_dev());
        if ((bt_ble_icon_get_adv_state() == ADV_ST_RECONN)
            || (bt_ble_icon_get_adv_state() == ADV_ST_DISMISS)
            || (bt_ble_icon_get_adv_state() == ADV_ST_END)) {
            bt_ble_icon_reset();
            bt_ble_icon_open(ICON_TYPE_INQUIRY);
        }
    }
#endif
    __this->pair_flag = 0;
#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV_RCSP)
#if (!CONFIG_NO_DISPLAY_BUTTON_ICON)
    if (__this->pair_flag && get_tws_sibling_connect_state()) {
        //check ble inquiry
        bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_UNCONNECTED, 1);
    }
#endif
    __this->pair_flag = 0;
#endif
}

void chargestore_set_phone_connect(void)
{
    __this->active_disconnect = 0;
}

static int app_chargestore_event_handler(int *msg)
{
    int ret = false;
    struct chargestore_event *chargestore_dev = (struct chargestore_event *)msg;

    // if (get_jl_update_flag()){
    if(user_var.is_ota_mode){
        return ret;
    }
#if defined(TCFG_CHARGE_KEEP_UPDATA) && TCFG_CHARGE_KEEP_UPDATA
    //在升级过程中,不响应智能充电舱app层消息
    if (dual_bank_update_exist_flag_get() || classic_update_task_exist_flag_get()) {
        return ret;
    }
#endif
    switch (chargestore_dev->event) {
    case CMD_USER_MODULE:
        chargestore_log("CMD_USER_MODULE");
        // os_time_dly(1);
        if (user_check_goto_bt_mode(chargestore_dev->packet, chargestore_dev->size)) {
            if (!app_in_mode(APP_MODE_BT)) {
                if (app_have_mode() && !app_var.goto_poweroff_flag) {
                    app_var.play_poweron_tone = 0;
                    app_goto_mode(APP_MODE_BT, 0);
                }
            } else {
//                 chargestore_log("enter dut: %d, %d, %d", __this->connect_status, __this->bt_init_ok, testbox_get_keep_tws_conn_flag());
//                 if ((!__this->connect_status) && __this->bt_init_ok) {
//                     __this->connect_status = 1;
// #if	TCFG_USER_TWS_ENABLE
//                     if (0 == testbox_get_keep_tws_conn_flag()) {
//                         chargestore_log("\n\nbt_page_scan_for_test\n\n");
//                         bt_bredr_enter_dut_mode(1, 0);
//                     }
// #endif
//                 }
            }
            if (__this->bt_init_ok) {
                ldoin_uart_data_deal(chargestore_dev->packet, chargestore_dev->size);
            }
        } else {
            ldoin_uart_data_deal(chargestore_dev->packet, chargestore_dev->size);
        }
        break;
    case CMD_RESTORE_SYS:
        chargestore_log("CMD_RESTORE_SYS");
        // bt_tws_remove_pairs();
        // bt_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);
        // os_time_dly(200);
        // cpu_reset();
        if (!get_bt_tws_connect_status()) {
            chargestore_log("tws is no connect, break!!!");
            break;
        }
        user_sys_restore(1);
        break;
    case CMD_TWS_CHANNEL_SET:
        chargestore_log("CMD_TWS_CHANNEL_SET");
        chargestore_set_tws_channel_info(__this->channel);
#if !USER_NEW_BOX_MODE
        if ((user_get_charge_flag() & BIT(0))) {
            chargestore_log("charge now, no opt!!!");
            break;
        }
        chargestore_log("role:%d, tws_status:%d", tws_api_get_role(), get_tws_sibling_connect_state());
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            tws_api_sync_call_by_uuid(0xABCABC03, CMD_BOX_ENTER_PAIR_MODE, 400);
        } else {
            user_tws_sync_call(CMD_BOX_ENTER_PAIR_MODE);
        }
#endif
        break;
#if USER_NEW_BOX_MODE
    case CMD_ENTER_PAIR_MODE:
        chargestore_log("CMD_ENTER_PAIR_MODE");
        chargestore_log("role:%d, tws_status:%d", tws_api_get_role(), get_tws_sibling_connect_state());
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            tws_api_sync_call_by_uuid(0xABCABC03, CMD_BOX_ENTER_PAIR_MODE, 400);
        } else {
            user_tws_sync_call(CMD_BOX_ENTER_PAIR_MODE);
        }
        break;
#endif
    case CMD_TWS_REMOTE_ADDR:
        chargestore_log("CMD_TWS_REMOTE_ADDR\n");
        if (chargestore_set_tws_remote_info(chargestore_dev->packet, chargestore_dev->size) == false) {
            //交换地址后,断开与手机连接,并删除所有连过的手机地址
            // bt_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);
            __this->ear_number = 2;
            // sys_enter_soft_poweroff(POWEROFF_RESET);
        } else {
            __this->pair_flag = 1;
            if (bt_get_total_connect_dev()) {
                __this->active_disconnect = 1;
                // bt_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
            } else {
                // chargestore_set_phone_disconnect();
            }
        }
        break;
    case CMD_TWS_ADDR_DELETE:
        log_info("event_CMD_TWS_ADDR_DELETE\n");
        chargestore_clean_tws_conn_info(chargestore_dev->packet[0]);
        break;
    case CMD_POWER_LEVEL_OPEN:
        log_info("event_CMD_POWER_LEVEL_OPEN\n");

        //电压过低,不进响应开盖命令
#if TCFG_SYS_LVD_EN
        if (get_vbat_need_shutdown() == TRUE) {
            log_info(" lowpower deal!\n");
            break;
        }
#endif

        if (__this->cover_status) {//当前为开盖
            if (__this->power_sync) {
                if (chargestore_sync_chg_level() == 0) {
                    __this->power_sync = 0;
                }
            }
            if (app_have_mode() && !app_in_mode(APP_MODE_BT) && app_var.goto_poweroff_flag == 0) {
                app_var.play_poweron_tone = 0;
                power_set_mode(TCFG_LOWPOWER_POWER_SEL);
                charge_close();//开盖切蓝牙关闭充电
                __this->switch2bt = 1;
                app_goto_mode(APP_MODE_BT, 0);
                __this->switch2bt = 0;
            }
        }
        break;
    case CMD_POWER_LEVEL_CLOSE:
        log_info("event_CMD_POWER_LEVEL_CLOSE\n");
        if (!__this->cover_status) {//当前为合盖
            if (!app_in_mode(APP_MODE_IDLE)) {
                sys_enter_soft_poweroff(POWEROFF_RESET);
            }
        }
        break;
    case CMD_CLOSE_CID:
        log_info("event_CMD_CLOSE_CID\n");
        if (!__this->cover_status) {//当前为合盖
#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
            if ((__this->bt_init_ok) && (tws_api_get_role() == TWS_ROLE_MASTER))  {
                bt_ble_icon_close(1);
            }
#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV_RCSP)
            if ((__this->bt_init_ok) && (tws_api_get_role() == TWS_ROLE_MASTER))  {
                bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_DISMISS, 1);
            }
#endif
            if (!__this->timer) {
                __this->timer = sys_timeout_add(NULL, chargestore_timeout_deal, 2000);
                if (!__this->timer) {
                    log_error("timer alloc err!\n");
                } else {
                    __this->close_ing = 1;
                }
            } else {
                sys_timer_modify(__this->timer, 2000);
                __this->close_ing = 1;
            }
        } else {
            __this->ear_number = 1;
            __this->close_ing = 0;
        }
        break;
    case CMD_SHUT_DOWN:
        log_info("event_CMD_SHUT_DOWN\n");
        if (!__this->shutdown_timer) {
            __this->shutdown_timer = sys_timer_add(NULL, chargestore_shutdown_do, 1000);
        } else {
            sys_timer_modify(__this->shutdown_timer, 1000);
        }
        break;
    default:
        break;
    }

    return ret;
}
APP_MSG_HANDLER(chargestore_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_CHARGE_STORE,
    .handler    = app_chargestore_event_handler,
};

static int chargestore_tws_msg_handler(int *msg)
{
    struct tws_event *evt = (struct tws_event *)msg;
    int role = evt->args[0];
    int phone_link_connection = evt->args[1];
    int reason = evt->args[2];

    switch (evt->event) {
    case TWS_EVENT_CONNECTED:
        chargestore_sync_chg_level();//同步充电舱电量
        break;
    case TWS_EVENT_CONNECTION_DETACH:
    case TWS_EVENT_REMOVE_PAIRS:
        if (app_var.goto_poweroff_flag) {
            break;
        }
        chargestore_set_sibling_chg_lev(0xff);
        break;
    }
    return 0;
}
APP_MSG_HANDLER(chargestore_tws_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = chargestore_tws_msg_handler,
};


u8 chargestore_get_vbat_percent(void)
{
    u8 power;
#if CONFIG_DISPLAY_DETAIL_BAT
    power = get_vbat_percent();//显示个位数的电量
#else
    power = get_self_battery_level() * 10 + 10; //显示10%~100%
#endif

#if TCFG_CHARGE_ENABLE
    if (get_charge_full_flag()) {
        power = 100;
    } else if (power == 100) {
        power = 99;
    }
    if (get_charge_online_flag()) {
        power |= BIT(7);
    }
#endif
    return power;
}

void chargestore_set_power_status(u8 *buf, u8 len)
{
    __this->version = buf[0] & 0x0f;
    __this->chip_type = (buf[0] >> 4) & 0x0f;
    //f95可能传一个大于100的电量
    if ((buf[1] & 0x7f) > 100) {
        __this->power_level = (buf[1] & 0x80) | 100;
    } else {
        __this->power_level = buf[1];
    }
    if (len > 2) {
        __this->max_packet_size = buf[2];
        if (len > 3) {
            __this->tws_power = buf[3];
        }
    }
}

static int app_chargestore_data_handler(u8 *buf, u8 len)
{
    u8 send_buf[36];
     log_info_hexdump(buf, len); 
    chargestore_shutdown_reset();
    send_buf[0] = buf[0];
#ifdef CONFIG_CHARGESTORE_REMAP_ENABLE
    if (remap_app_chargestore_data_deal(buf, len)) {
        return 1;
    }
#endif

    switch (buf[0]) {
    case CMD_USER_MODULE:
        // __this->testbox_status = 1;
        // testbox_set_status(1);
        chargestore_event_to_user(buf+1, buf[0], len-1);
        break;
    case CMD_TWS_CHANNEL_SET:
        __this->channel = (buf[1] == TWS_CHANNEL_LEFT) ? 'L' : 'R';
        chargestore_event_to_user(NULL, buf[0], 0);
        // if (__this->bt_init_ok) {
        if (get_charge_online_flag()) {
            y_printf("charge online!!!");
            len = chargestore_get_tws_remote_info(&send_buf[1]);
            chargestore_api_write(send_buf, len + 1);
        } else {
            y_printf("charge no online!!!");
            send_buf[0] = CMD_UNDEFINE;
            chargestore_api_write(send_buf, 1);
        }
        break;
#if USER_NEW_BOX_MODE
    case CMD_ENTER_PAIR_MODE:
        // chargestore_log("CMD_ENTER_PAIR_MODE");
        chargestore_event_to_user(NULL, buf[0], 0);
        chargestore_api_write(send_buf, 1);
        break;
#endif
    case CMD_TWS_SET_CHANNEL:
        __this->channel = (buf[1] == TWS_CHANNEL_LEFT) ? 'L' : 'R';
        log_info("f95 set channel = %c\n", __this->channel);
        chargestore_event_to_user(NULL, CMD_TWS_CHANNEL_SET, 0);
        // if (__this->bt_init_ok) {
        if (get_charge_online_flag()) {
            y_printf("charge online!!!");
            chargestore_api_write(send_buf, 1);
        } else {
            y_printf("charge no online!!!");
            send_buf[0] = CMD_UNDEFINE;
            chargestore_api_write(send_buf, 1);
        }
        break;

    case CMD_TWS_REMOTE_ADDR:
        __this->close_ing = 0;
        if (chargestore_check_data_succ((u8 *)&buf[1], len - 1) == true) {
            chargestore_event_to_user((u8 *)&buf[1], buf[0], len - 1);
        } else {
            send_buf[0] = CMD_FAIL;
        }
        chargestore_api_write(send_buf, 1);
        break;
    case CMD_EX_FIRST_READ_INFO:
        log_info("read first!\n");
        __this->close_ing = 0;
        len = chargestore_f95_read_tws_remote_info(&send_buf[1], 1);
        chargestore_api_write(send_buf, len + 1);
        break;
    case CMD_EX_CONTINUE_READ_INFO:
        log_info("read continue!\n");
        __this->close_ing = 0;
        len = chargestore_f95_read_tws_remote_info(&send_buf[1], 0);
        chargestore_api_write(send_buf, len + 1);
        break;
    case CMD_EX_FIRST_WRITE_INFO:
        log_info("write first!\n");
        __this->close_ing = 0;
        chargestore_f95_write_tws_remote_info(&buf[1], len - 1, 1);
        chargestore_api_write(send_buf, 1);
        break;
    case CMD_EX_CONTINUE_WRITE_INFO:
        log_info("write continue!\n");
        __this->close_ing = 0;
        chargestore_f95_write_tws_remote_info(&buf[1], len - 1, 0);
        chargestore_api_write(send_buf, 1);
        break;
    case CMD_EX_INFO_COMPLETE:
        log_info("ex complete!\n");
        if (chargestore_check_data_succ((u8 *)&write_info, sizeof(write_info)) == true) {
            chargestore_event_to_user((u8 *)&write_info, CMD_TWS_REMOTE_ADDR, sizeof(write_info));
        } else {
            send_buf[0] = CMD_FAIL;
        }
        chargestore_api_write(send_buf, 1);
        break;

    case CMD_TWS_ADDR_DELETE:
        __this->close_ing = 0;
        chargestore_event_to_user(&buf[1], CMD_TWS_ADDR_DELETE, len - 1);
        chargestore_api_write(send_buf, 1);
        break;
    case CMD_POWER_LEVEL_OPEN:
        // put_buf(buf, len);
        // extern void user_set_box_charge_status(u8 status);
        extern void user_set_local_box_charge_status(u8 status);
        extern void user_sync_box_charge_status(u8 status);
        // user_set_box_charge_status(buf[2]);
        user_set_local_box_charge_status(buf[2]);
        // y_printf("user_charge_info.box_bat_percent_local:%d", user_charge_info.box_bat_percent_local);
        if (get_charge_online_flag()) {
            user_sync_box_charge_status(buf[2]);
        }
        
        __this->power_status = 1;
        __this->cover_status = 1;
        __this->close_ing = 0;
        if (__this->power_level == 0xff) {
            __this->power_sync = 1;
        }
        chargestore_set_power_status(&buf[1], len - 1);
        if (__this->power_level != __this->pre_power_lvl) {
            __this->power_sync = 1;
        }
        __this->pre_power_lvl = __this->power_level;
        send_buf[1] = chargestore_get_vbat_percent();
        send_buf[2] = chargestore_get_det_level(__this->chip_type);
        // chargestore_api_write(send_buf, 3);
        if (get_tws_phone_connect_state()) {
            send_buf[3] = 0xCC;
        } else {
            send_buf[3] = 0x00;
        }
        //充电仓获取耳机的配对状态
        chargestore_api_write(send_buf, 4);
        //切模式过程中不发送消息,防止堆满消息
        if (__this->switch2bt == 0) {
            chargestore_event_to_user(NULL, CMD_POWER_LEVEL_OPEN, 0);
        }
        break;
    case CMD_POWER_LEVEL_CLOSE:
		printf("555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555\n");
        __this->power_status = 1;
        __this->cover_status = 0;
        __this->close_ing = 0;
        chargestore_set_power_status(&buf[1], len - 1);
        send_buf[1] = chargestore_get_vbat_percent();
        send_buf[2] = chargestore_get_det_level(__this->chip_type);
        // chargestore_api_write(send_buf, 3);
        if (get_tws_phone_connect_state()) {
            send_buf[3] = 0xCC;
        } else {
            send_buf[3] = 0x00;
        }
        //充电仓获取耳机的配对状态
        chargestore_api_write(send_buf,4);
		put_buf(send_buf,4);
        //chargestore_api_write(send_buf, 4);
        chargestore_event_to_user(NULL, CMD_POWER_LEVEL_CLOSE, 0);
        break;
    case CMD_SHUT_DOWN:
        log_info("shut down\n");
        __this->power_status = 0;
        __this->cover_status = 0;
        __this->close_ing = 0;
        chargestore_api_write(send_buf, 1);
        chargestore_event_to_user(NULL, CMD_SHUT_DOWN, 0);
        break;
    case CMD_CLOSE_CID:
        log_info("close cid\n");
        __this->power_status = 1;
        __this->cover_status = 0;
        __this->ear_number = buf[1];
        chargestore_api_write(send_buf, 1);
        chargestore_event_to_user(NULL, CMD_CLOSE_CID, 0);
        break;

    case CMD_RESTORE_SYS:
        r_printf("restore sys\n");
        __this->power_status = 1;
        __this->cover_status = 1;
        __this->close_ing = 0;
        chargestore_api_write(send_buf, 1);
        chargestore_event_to_user(NULL, CMD_RESTORE_SYS, 0);
        break;
    //不是充电舱的指令,返回0
    default:
        return 0;
    }
    return 1;
}

CHARGESTORE_HANDLE_REG(chargestore, app_chargestore_data_handler);

#if TCFG_CHARGE_ENABLE
static int app_chargestore_charge_msg_handler(int msg, int type)
{
    switch (msg) {
    case CHARGE_EVENT_LDO5V_KEEP:
        if (!get_charge_poweron_en()) {
            batmgr_send_msg(BAT_MSG_CHARGE_LDO5V_OFF, 0);
            return 1;//拦截关机,智能充电舱不在维持电压消息关机
        }
        break;
    case CHARGE_EVENT_LDO5V_IN:
        chargestore_shutdown_reset();
        if (!get_charge_poweron_en()) {
//开启了弹窗才需要等待弹窗关闭才复位进充电
#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
            if (chargestore_check_going_to_poweroff()) {
                log_info("chargestore do poweroff!\n");
                return 1;//不要关闭蓝牙,让充电舱流程执行关蓝牙
            }
#endif
        }
        break;
    case CHARGE_EVENT_LDO5V_OFF:
#if TCFG_USER_TWS_ENABLE
        if (TWS_ROLE_SLAVE == tws_api_get_role()) {
            chargestore_set_power_level(0xFF);
        }
#endif
        chargestore_shutdown_reset();
        break;
    }
    return 0;
}

APP_CHARGE_HANDLER(chargestore_charge_msg_entry, 0) = {
    .handler = app_chargestore_charge_msg_handler,
};
#endif




/*************************************************************/
//   充电仓触发的自定义操作，在这里进行
/*************************************************************/
void user_sys_restore(u8 type)
{
    chargestore_log("user_sys_restore, flag:%d, type:%d", user_var.restore_flag, type);
    if (user_var.restore_flag == 1) {
        chargestore_log("restore now, return!!!");
        return;
    }
    percent_save_vm();
    user_var.restore_flag = 1;
    app_var.goto_poweroff_flag = 1;
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        if (tws_api_get_tws_state() & TWS_STA_PHONE_CONNECTED) {
            if (user_var.sniff_status != 0) {
                chargestore_log("phone exit sniff mode...\n");
                bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
            }
        } else {
            chargestore_log("tws exit sniff mode...\n");
            if (tws_api_is_sniff_state()) {
                tws_api_tx_unsniff_req();
            }
        }
    }
    user_1t1_mode_reconn_addr_reset();
    user_key_reconn_addr_reset();
    user_app_info_restore();
    // led_ui_manager_display(LED_LOCAL, LED_UUID_WHITE_1S_FLASH_THREE, LED_MODE_C);
    // led_ui_manager_set_wait(1);
    u16 delay_time = 0;
    if (tws_api_is_sniff_state() || (user_var.sniff_status != 0)) {
        delay_time = 120;
    } else {
        delay_time = 40;
    }
    os_time_dly(delay_time);

#if USER_NEW_BOX_MODE
    // bt_tws_remove_pairs();
#endif
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        void *devices[2] = {NULL, NULL};
        int num = btstack_get_conn_devices(devices, 2);
        chargestore_log("conn_devices = %d", num);
        for (int i = 0; i < num; i++) {
            chargestore_log_hexdump(btstack_get_device_mac_addr(devices[i]), 6);
            bt_cmd_prepare_for_addr(btstack_get_device_mac_addr(devices[i]), USER_CTRL_DISCONNECTION_HCI, 0, NULL);
        }
    }
    bt_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);
#if USER_NEW_BOX_MODE
    os_time_dly(70);
#endif
    user_tws_sync_call(CMD_SYNC_CPU_RESET);
}

/*************************************************************/


#endif //TCFG_CHARGESTORE_ENABLE

/*************************************************************/
//   产测用到UART相关接口写在这
/*************************************************************/
#define UART_TEST_BUFF_SIZE   (128)
u8 *uart_txbuf = NULL;
struct rx_data_st uart_bus;
static u8 uart_active = 0;
struct chargestore_platform_data *ldoin_uart;
static int uart_timeout_handler = 0;

static uint16_t USER_CRC16(const uint8_t * buffer, uint32_t size)
{
    uint16_t crc = 0xFFFF;
    if (NULL != buffer && size > 0){
        while (size--) {
            crc = (crc >> 8) | (crc << 8);
            crc ^= *buffer++;
            crc ^= ((unsigned char) crc) >> 4;
            crc ^= crc << 12;
            crc ^= (crc & 0xFF) << 5;
        }
    }
    return crc;
}

static u16 buff_package(u8 *dst, u16 dst_len,u16 offset,  u8 *src, u16 src_len)
{
    if((offset + src_len) > dst_len || !dst || !src)
    {
        return 0;
    }

    if(offset == 0)
    {
        memset(dst, 0x00, dst_len);
    }

    memcpy(dst + offset, src, src_len);

    return src_len;
}

static u16 data_package(u8 *dst, u16 dst_len, u8 src, u16 offset)
{
    if((offset + 1) > dst_len || !dst)
    {
        return 0;
    }

    if(offset == 0)
    {
        memset(dst, 0x00, dst_len);
    }

    dst[offset] = src;

    return 1;
}

static u8 *_txbuf = NULL;
static u8 _txbuf_len = 0;

void tx_buff_register(void *addr, u16 len)
{
    _txbuf = addr;
    _txbuf_len = len;
}

static u16 tx_buff_package(u8 *src, u16 offset, u16 src_len)
{
    return buff_package(_txbuf, _txbuf_len, offset, src, src_len);
}

static u16 tx_data_package(u8 src, u16 offset, u16 src_len)
{
    return data_package(_txbuf, _txbuf_len, src, offset);
}

void uart_no_rx_income_timeout(void *p)
{
    uart_timeout_handler = 0;
    uart_active = 0;
}

u8 chargestore_crc8(u8 *ptr, u8 len)
{
    u8 i, crc;
    crc = 0;
    while (len--) {
        crc ^= *ptr++;
        for (i = 0; i < 8; i++) {
            if (crc & 0x01) {
                crc = (crc >> 1) ^ 0x8c;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void chargestore_data_deal(u8 cmd, u8 *data, u8 len);
#define DMA_BUF_LEN 64
static u8 temp[DMA_BUF_LEN];
#define CMD_USER_MODULE             0x22//自定义一级命令

//走公版SDK的流程
void user_cmd_send_to_sdk(u8 *buf, u16 len)
{
    memset(temp, 0, DMA_BUF_LEN);
    temp[0] = 0x55;
    temp[1] = 0xaa;
    temp[2] = len + 1;
    temp[3] = CMD_USER_MODULE;
    memcpy(temp+4, buf, len);
    temp[len+4] = chargestore_crc8(temp, len+4);
    chargestore_log("user_cmd_send_to_sdk\n");
    chargestore_log_hexdump(temp, len+4+1);
    
    chargestore_data_deal(CMD_RECVDATA, temp, len+4+1);
}

void ldoin_uart_test_write(const unsigned char *buf, unsigned int len)
{
    if(!ldoin_uart){
        return;
    }
    ldoin_uart->open(MODE_SENDDATA);
    ldoin_uart->write(buf, len);
}

void ldoin_uart_test_init(void *uart)
{
    if(uart == NULL){
        return;
    }
    uart_txbuf = malloc(UART_TEST_BUFF_SIZE);
    ASSERT(uart_txbuf);
    tx_buff_register((void *)uart_txbuf, UART_TEST_BUFF_SIZE);
    ldoin_uart = (struct chargestore_platform_data *)uart;
    uart_bus.write = ldoin_uart_test_write;
}

void ldoin_uart_data_deal(u8 *buff, u8 len)
{
    // rx_data_analyze(&uart_bus, buff, len);
    user_product_test_handle(&uart_bus, buff, len);
}

u8 ldoin_uart_data_callback(u8 *buff, u8 len)
{
    // u8 ret = rx_data_analyze(&uart_bus, buff, len);
    u8 ret = user_check_uart_test_cmd(buff, len);
    if(ret){
        //发现是测试命令，删除超时，不允许进低功耗
        if(uart_timeout_handler)
        {
            sys_timeout_del(uart_timeout_handler);
            uart_timeout_handler = 0;
        }
        user_cmd_send_to_sdk(buff, len);
    }
    return ret;
}

u8 uart_idle_query(void)
{
    return uart_active ? 0 : 1;
}

REGISTER_LP_TARGET(uart_lp_target) = {
    .name = "uar",
    .is_idle = uart_idle_query,
};

void uart_test_init(void)
{
    chargestore_log("uart_test_init\n");
    if(ldoin_uart == NULL){
        return;
    }
    set_channel_by_code_or_res();
    uart_active = 1;//不允许进低功耗
    ldoin_uart->open(MODE_RECVDATA);//先把接收打开
    uart_timeout_handler = sys_timeout_add(NULL, uart_no_rx_income_timeout, 20000);//10秒超时允许进低功耗
}

/*************************************************************/

/*************************************************************/
//   产测的流程写在这
/*************************************************************/
static const u8 data_head[] = {0x01, 0xE0, 0xFC};

#define CMD_LID_OPEN                0xE1
#define CMD_LID_CLOSE               0xE2
#define CMD_GET_BAT                 0xF7
#define CMD_RESET                   0xE4
#define CMD_GET_EAR_ADDR            0xF1
#define CMD_RECV_EAR_ADDR           0xF2
#define CMD_GET_COMM_ADDR           0xF4
#define CMD_EXCHANGE_ADDR_OK        0xF5
#define CMD_HEARTBEATS              0xE0
#define CMD_READ_VER                0x04
#define CMD_READ_SN                 0x10
#define CMD_WRITE_SN                0x0A
#define CMD_ENTER_OTA_MODE          0x58

#define CMD_ENTER_DUT_MODE          0x01
#define CMD_ENTER_BT_MODE           0x39
#define CMD_CLEAR_BT_MEMORY         0xE6
#define CMD_CLEAR_TWS_MEMORY        0xE7
#define CMD_UPDATE                  0xF3
#define CMD_ENTER_STORAGE_MODE      0xE3
#define CMD_ENTER_POWEROFF          0xE5
#define CMD_ENTER_TWS_PAIR_MODE     0xE8
#define CMD_SET_TONE_LANGUAGE       0xEA

#define CMD_RESET_VBAT_PERCENT      0xFE
// u8 user_sn_temp[16] = {0x00};   //方便调试，临时储存，后面确定下来再写保留区
u8 testbox_set_ex_enter_dut_flag(u8 en);

u8 user_check_goto_bt_mode(u8 *data, u16 len)
{
    chargestore_log("user_check_goto_bt_mode");
    if (!data) {
        return 0;
    }
    if (len < 5) {
        chargestore_log("data len err, return!!!");
        return 0; 
    }
    if (memcmp(data_head, data, sizeof(data_head)) == 0) {
        chargestore_log("this is test cmd : 0x%x!!!", data[4]);
        switch (data[4]) {
        case CMD_ENTER_TWS_PAIR_MODE:
        case CMD_ENTER_DUT_MODE:
        case CMD_ENTER_BT_MODE:
        case CMD_CLEAR_BT_MEMORY:
        case CMD_CLEAR_TWS_MEMORY:
            testbox_set_ex_enter_dut_flag(1);
            chargestore_log("this cmd need switch to bt mode!!!");
            return 1;
        }
    }
    return 0;
}

u8 user_check_uart_test_cmd(u8 *data, u16 len)
{
    // chargestore_log("user_check_uart_test_cmd");
    if (!data) {
        return 0;
    }
    if (len < 5) {
        // chargestore_log("data len err, return!!!");
        return 0; 
    }
    if (memcmp(data_head, data, sizeof(data_head)) == 0) {
        chargestore_log("this is test cmd!!!");
        return 1;
    }

    return 0;
}


void user_product_test_handle(struct rx_data_st *uart_bus, u8 *buf, u16 len)
{
    chargestore_log("user_product_test_handle");
    chargestore_log_hexdump(buf, len);

    chargestore_log("tws_api_get_local_channel() = %c", tws_api_get_local_channel());
    if ((buf[3]==0x34 && tws_api_get_local_channel()=='L') || (buf[3]==0x35 && tws_api_get_local_channel()=='R')) {
        // chargestore_log("this is local cmd!!!");
    } else {
        // chargestore_log("this is sibling cmd, return!!!");
        return; 
    }

    u32 offset = 0;
    u16 crc = 0;
    u8 cmd = buf[4];

    u8 ear_channel_reply = tws_api_get_local_channel()=='L' ? 0x43 : 0x53;

    switch (cmd) {
    case CMD_LID_OPEN:
        chargestore_log("CMD_LID_OPEN, box bat:%d", buf[6]);
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x01, offset, 1);//len
        offset += tx_data_package(battery_value_to_phone_level(), offset, 1);
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_LID_CLOSE:
        chargestore_log("CMD_LID_CLOSE, box bat:%d", buf[6]);
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x01, offset, 1);//len
        offset += tx_data_package(battery_value_to_phone_level(), offset, 1);
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_GET_BAT:
        chargestore_log("CMD_GET_BAT");
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x02, offset, 1);//len
        offset += tx_data_package(battery_value_to_phone_level(), offset, 1);
        offset += tx_data_package(get_charge_full_flag(), offset, 1);
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_RESET:
        chargestore_log("CMD_RESET");
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x00, offset, 1);//len
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        chargestore_log_hexdump(_txbuf, offset);
        uart_bus->write(_txbuf, offset);
        os_time_dly(50);
        cpu_reset();
        return;
    case CMD_GET_EAR_ADDR:
        chargestore_log("CMD_GET_EAR_ADDR");
        u8 local_addr[6];
        u8 sibling_addr[6];
        u8 result = 0;
        bt_get_tws_local_addr(local_addr);
        if (tws_get_sibling_addr(sibling_addr, result)) {
            chargestore_log("get sibling addr err!!!");
            u8 all_ff[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
            memcpy(sibling_addr, all_ff, 6);
        }
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x01, offset, 1);//len
        offset += tx_buff_package((u8 *)local_addr, offset, 6);
        offset += tx_buff_package((u8 *)sibling_addr, offset, 6);
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_RECV_EAR_ADDR:
        chargestore_log("CMD_RECV_EAR_ADDR");
        chargestore_log_hexdump(buf, len);
        chargestore_log("no save now!!!");
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x00, offset, 1);//len
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_EXCHANGE_ADDR_OK:
        chargestore_log("CMD_EXCHANGE_ADDR_OK");
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x00, offset, 1);//len
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_GET_COMM_ADDR:
        chargestore_log("CMD_GET_COMM_ADDR");
        u8 bt_mac[6];
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x06, offset, 1);//len
        if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
            memcpy(bt_mac, bt_get_mac_addr(), 6);
        } else {
            syscfg_read(CFG_BT_MAC_ADDR, bt_mac, 6);
        }
        put_buf(bt_mac, 6);
        offset += tx_buff_package((u8 *)bt_mac, offset, 6);
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_HEARTBEATS:
        chargestore_log("CMD_HEARTBEATS");
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x01, offset, 1);//len
        offset += tx_data_package(battery_value_to_phone_level(), offset, 1);
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_READ_VER:
        chargestore_log("CMD_READ_VER");
        extern u8 soft_ver_local[3];
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x03, offset, 1);//len
        offset += tx_buff_package((u8 *)soft_ver_local, offset, sizeof(soft_ver_local));
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_READ_SN:
        chargestore_log("CMD_READ_SN");
        chargestore_log("no return sn now!!!");
        // offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        // offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        // offset += tx_data_package(cmd, offset, 1);//cmd
        // offset += tx_data_package(0x10, offset, 1);//len
        // offset += tx_buff_package((u8 *)soft_ver_local, offset, sizeof(soft_ver_local));
        // crc = USER_CRC16(_txbuf, offset);
        // offset += tx_data_package((crc&0xFF), offset, 1);//crc
        // offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_WRITE_SN:
        chargestore_log("CMD_WRITE_SN");
        chargestore_log_hexdump(buf, len);
        // memcpy(user_sn_temp, buf+5, sizeof(user_sn_temp));
        // chargestore_log_hexdump(user_sn_temp, sizeof(user_sn_temp));
        chargestore_log("no save sn now!!!");
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x00, offset, 1);//len
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_ENTER_OTA_MODE:
        chargestore_log("CMD_ENTER_OTA_MODE");
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x00, offset, 1);//len
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_ENTER_DUT_MODE:
        chargestore_log("CMD_ENTER_DUT_MODE");
        user_enter_dut_mode();
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x00, offset, 1);//len
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_ENTER_BT_MODE:
        chargestore_log("CMD_ENTER_BT_MODE");
        user_tws_sync_call(CMD_BOX_ENTER_PAIR_MODE);
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x00, offset, 1);//len
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_CLEAR_BT_MEMORY:
        chargestore_log("CMD_CLEAR_BT_MEMORY");
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x00, offset, 1);//len
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        chargestore_log_hexdump(_txbuf, offset);
        uart_bus->write(_txbuf, offset);
        bt_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);
        user_1t1_mode_reconn_addr_reset();
        user_key_reconn_addr_reset();
        // led_ui_manager_display(LED_LOCAL, LED_UUID_WHITE_1S_FLASH_THREE, LED_MODE_C);
        // led_ui_manager_set_wait(1);
        os_time_dly(200);
        cpu_reset();
        return;
    case CMD_CLEAR_TWS_MEMORY:
        chargestore_log("CMD_CLEAR_TWS_MEMORY");
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x00, offset, 1);//len
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        chargestore_log_hexdump(_txbuf, offset);
        uart_bus->write(_txbuf, offset);
        bt_tws_remove_pairs();
        bt_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);
        user_1t1_mode_reconn_addr_reset();
        user_key_reconn_addr_reset();
        // led_ui_manager_display(LED_LOCAL, LED_UUID_WHITE_1S_FLASH_THREE, LED_MODE_C);
        // led_ui_manager_set_wait(1);
        os_time_dly(200);
        cpu_reset();
        return;
    case CMD_UPDATE:
        chargestore_log("CMD_UPDATE");
        chargestore_log("unknown");
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x00, offset, 1);//len
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_ENTER_STORAGE_MODE:
        chargestore_log("CMD_ENTER_STORAGE_MODE");
        testbox_set_ex_enter_storage_mode_flag(1);
    case CMD_ENTER_POWEROFF:
        chargestore_log("CMD_ENTER_POWEROFF");
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x00, offset, 1);//len
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        chargestore_log_hexdump(_txbuf, offset);
        uart_bus->write(_txbuf, offset);
        user_var.cmd_enter_poweroff = 1;
        // sys_enter_soft_poweroff(POWEROFF_NORMAL);
        return;
    case CMD_ENTER_TWS_PAIR_MODE:
        chargestore_log("CMD_ENTER_TWS_PAIR_MODE");
        void tws_dual_conn_close();
        tws_dual_conn_close();
        tws_api_set_quick_connect_addr(tws_set_auto_pair_code());
        tws_api_auto_pair(0);
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x00, offset, 1);//len
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
    case CMD_RESET_VBAT_PERCENT:
        chargestore_log("CMD_RESET_VBAT_PERCENT");
        set_vm_vbat_percent(user_charge_info.local_detail_percent);
        percent_save_init();
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x01, offset, 1);//len
        offset += tx_data_package(get_vm_vbat_percent(), offset, 1);//data
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
#if USER_PRODUCT_SET_TONE_EN
    case CMD_SET_TONE_LANGUAGE:
        chargestore_log("CMD_SET_TONE_LANGUAGE");
        if (buf[6] < 0 || buf[6] > 2) {
            y_printf("tone type err:%d", buf[6]);
            return;
        }
        extern void user_set_default_tone_language(u8 default_tone);
        user_set_default_tone_language(buf[6]);
		user_app_info_restore();
        offset += tx_buff_package((u8 *)data_head, offset, sizeof(data_head));//头
        offset += tx_data_package(ear_channel_reply, offset, 1);//lr
        offset += tx_data_package(cmd, offset, 1);//cmd
        offset += tx_data_package(0x01, offset, 1);//len
        offset += tx_data_package(buf[6], offset, 1);//data
        crc = USER_CRC16(_txbuf, offset);
        offset += tx_data_package((crc&0xFF), offset, 1);//crc
        offset += tx_data_package((crc>>8)&0xFF, offset, 1);//crc
        break;
#endif
    default:
        break;
    }
    chargestore_log("send data is:");
    chargestore_log_hexdump(_txbuf, offset);
    uart_bus->write(_txbuf, offset);
}

/*************************************************************/
