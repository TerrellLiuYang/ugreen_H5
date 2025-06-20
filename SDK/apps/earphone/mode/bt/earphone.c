#include "system/includes.h"
#include "media/includes.h"
#include "app_tone.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "esco_recoder.h"
#include "earphone.h"
#include "idle.h"
#include "bt_slience_detect.h"

#include "app_config.h"
#include "app_action.h"

#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "btstack/a2dp_media_codec.h"
#include "btctrler/btctrler_task.h"
#include "btctrler/btcontroller_modules.h"
#include "user_cfg.h"
#include "audio_cvp.h"
#include "classic/hci_lmp.h"
#include "bt_common.h"
#include "bt_ble.h"
#include "pbg_user.h"
#include "btstack/bluetooth.h"
#if RCSP_MODE && TCFG_RCSP_DUAL_CONN_ENABLE
#include "adv_1t2_setting.h"
#endif

#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif/*TCFG_AUDIO_ANC_ENABLE*/

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
#include "icsd_adt_app.h"
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

#if TCFG_ANC_BOX_ENABLE
#include "app_ancbox.h"
#endif

#include "bt_tws.h"


#include "asm/charge.h"
#include "app_charge.h"

#include "fs/resfile.h"
#include "app_chargestore.h"
#include "app_testbox.h"
#include "app_online_cfg.h"
#include "app_main.h"
#include "app_power_manage.h"
#include "gSensor/gSensor_manage.h"
#include "classic/tws_api.h"
#include "asm/pwm_led_hw.h"
#include "ir_sensor/ir_manage.h"
#include "in_ear_detect/in_ear_manage.h"
#include "vol_sync.h"
#include "audio_config.h"
#include "clock_manager/clock_manager.h"
#include "app_tone.h"
#include "led/led_ui_manager.h"


#define LOG_TAG             "[EARPHONE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if 1
#define bt_log(x, ...)       g_printf("[USER_BT]" x " ", ## __VA_ARGS__)
#define bt_log_hexdump       g_printf("[USER_BT]bt_log_hexdump:");\
                                put_buf
#else
#define bt_log(...)
#define bt_log_hexdump(...)
#endif

u8 need_default_volume = 127; 
u8 sync_default_volume_every_time = 0;   //0是首次连接时使用默认音量即（need_default_volume）   1是每次都使用默认音量

extern APP_INFO user_app_info;
extern USER_VAR user_var;

extern void user_1t1_and_1t2_mode_connect_deal();
extern int add_device_2_page_list(u8 *mac_addr, u32 timeout);
extern void dual_conn_page_device();
extern void dual_conn_page_devices_init();
extern void user_play_lowpower_tone_reset();

void user_out_of_range_page_deal();
void tws_dual_conn_close();
void user_phone_info_del(u8 *addr);
void user_send_phone_info_to_sibling();
void user_record_remote_name_addr(u8 status, u8 *addr, u8 *name);
void user_exit_sniff();


extern u16 user_lowpower_scan_conn_timer;
u16 user_out_of_range_page_timerid = 0;



#define AVC_VOLUME_UP			0x41
#define AVC_VOLUME_DOWN			0x42

u16 disturb_scan_timer = 0xff;
int link_disturb_scan_enable();
void disturb_scan_timeout_cb_api()  ;
extern void set_lmp_support_dual_con(u8 en);
extern void set_tws_task_interval(int tws_task_interval);

void bredr_link_disturb_scan_timeout(void *priv)
{
    puts("\n++++++++disturb_scan_timeout\n");
    disturb_scan_timeout_cb_api();
#if TCFG_USER_TWS_ENABLE
    bt_tws_poweron();
#else
    lmp_hci_write_scan_enable((1 << 1) | 1);

#endif

}

void bredr_link_disturb_scan()
{
    puts("\n++++++++disturb_scan\n");
    link_disturb_scan_enable();
    disturb_scan_timer =  sys_timeout_add(NULL, bredr_link_disturb_scan_timeout, 5000);
}

struct bt_mode_var g_bt_hdl;
static u8 sniff_out = 0;

u8 get_sniff_out_status()
{
    return sniff_out;
}
void clear_sniff_out_status()
{
    sniff_out = 0;
}

/**********进入蓝牙dut模式
*  mode=0:使能可以进入dut，原本流程不变。
*  mode=1:删除一些其它切换状态，产线中通过工具调用此接口进入dut模式，提高测试效率
 *********************/
void bt_bredr_enter_dut_mode(u8 mode, u8 inquiry_scan_en)
{
    puts("<<<<<<<<<<<<<bt_bredr_enter_dut_mode>>>>>>>>>>>>>>\n");
    clock_alloc("DUT", SYS_48M);
    bredr_set_dut_enble(1, 1);
    if (mode) {
        g_bt_hdl.auto_connection_counter = 0;
#if TCFG_USER_TWS_ENABLE
        extern void bt_page_scan_for_test(u8 inquiry_en);
        bt_page_scan_for_test(inquiry_scan_en);

#else
#endif

    }
}
void bt_bredr_exit_dut_mode()
{
    bredr_set_dut_enble(0, 1);
    clock_free("DUT");
}

u16 user_enter_dut_poweroff_timerid = 0;
void user_enter_dut_poweroff_timer_deal()
{
    bt_log("user_enter_dut_poweroff_timer_deal");
    sys_enter_soft_poweroff(POWEROFF_NORMAL);
}

void user_enter_dut_mode()
{
    bt_log("user_enter_dut_mode");
    tws_dual_conn_close();
    tws_cancel_state();
    bt_sniff_disable();
    sys_auto_shut_down_disable();
    tws_cancle_all_noconn();
    bt_ble_adv_enable(0);
    user_var.dut_mode = 1;
    bt_bredr_enter_dut_mode(1, 1);
    led_ui_manager_display(LED_LOCAL, LED_UUID_WHITE_ON, LED_MODE_C);
    if (user_enter_dut_poweroff_timerid == 0) {
        user_enter_dut_poweroff_timerid = sys_timeout_add(NULL, user_enter_dut_poweroff_timer_deal, 20 * 60 * 1000);
    }
}


void bt_send_keypress(u8 key)
{
    bt_cmd_prepare(USER_CTRL_KEYPRESS, 1, &key);
}

void bt_send_pair(u8 en)
{
    bt_cmd_prepare(USER_CTRL_PAIR, 1, &en);
}

void bt_init_ok_search_index(void)
{
    if (bt_get_current_poweron_memory_search_index(g_bt_hdl.auto_connection_addr)) {
        log_info("bt_wait_connect_and_phone_connect_switch\n");
        bt_clear_current_poweron_memory_search_index(1);
        app_send_message(APP_MSG_BT_GET_CONNECT_ADDR, 0);
    }
}




static int bt_get_battery_value()
{
    // y_printf("bt_get_battery_value");
    extern u8  get_cur_battery_level(void);
    //取消默认蓝牙定时发送电量给手机，需要更新电量给手机使用USER_CTRL_HFP_CMD_UPDATE_BATTARY命令
    /*电量协议的是0-9个等级，请比例换算*/
    return get_cur_battery_level();
}

extern void audio_fast_mode_test();

void bt_fast_test_api(void)
{
    /*
     * 进入快速测试模式，用户根据此标志判断测试，
     * 如测试按键-开按键音  、测试mic-开扩音 、
     * 根据fast_test_mode根据改变led闪烁方式、关闭可发现可连接
     */
    log_info("------------bt_fast_test_api---------\n");
    if (g_bt_hdl.fast_test_mode == 0) {
        g_bt_hdl.fast_test_mode = 1;
        audio_fast_mode_test();
    }
}

static void bt_dut_api(u8 param)
{
    log_info("bt in dut \n");
    sys_auto_shut_down_disable();
#if TCFG_USER_TWS_ENABLE
    extern 	void tws_cancle_all_noconn();
    tws_cancle_all_noconn() ;
#endif

    if (g_bt_hdl.auto_connection_timer) {
        sys_timeout_del(g_bt_hdl.auto_connection_timer);
        g_bt_hdl.auto_connection_timer = 0;
    }

#if TCFG_BT_BLE_ADV_ENABLE
#if (CONFIG_BT_MODE == BT_NORMAL)
    bt_ble_adv_enable(0);
#endif
#endif
}

void bit_clr_ie(unsigned char index);
void bt_fix_fre_api(u8 fre)
{
    bt_dut_api(0);

    bit_clr_ie(IRQ_BREDR_IDX);
    bit_clr_ie(IRQ_BT_CLKN_IDX);

    bredr_fcc_init(BT_FRE, fre);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式进入定频测试接收发射
   @param      mode ：0 测试发射    1：测试接收
   			  mac_addr:测试设置的地址
			  fre：定频的频点   0=2402  1=2403
			  packet_type：数据包类型

				#define DH1_1        0
				#define DH3_1        1
				#define DH5_1        2
				#define DH1_2        3
				#define DH3_2        4
				#define DH5_2        5

			  payload：数据包内容  0x0000  0x0055 0x00aa 0x00ff
						0xffff:prbs9
   @return
   @note     量产的时候通过串口，发送设置的参数，设置发送接收的参数

   关闭定频接收发射测试
	void link_fix_txrx_disable();

	更新接收结果
	void bt_updata_fix_rx_result()

struct link_fix_rx_result {
    u32 rx_err_b;  //接收到err bit
    u32 rx_sum_b;  //接收到正确bit
    u32 rx_perr_p;  //接收到crc 错误 包数
    u32 rx_herr_p;  //接收到crc 以外其他错误包数
    u32 rx_invail_p; //接收到crc错误bit太多的包数，丢弃不统计到err bit中
};

*/

/*----------------------------------------------------------------------------*/
void bt_fix_txrx_api(u8 mode, u8 *mac_addr, u8 fre, u8 packet_type, u16 payload)
{
    bt_dut_api(0);
    local_irq_disable();
    link_fix_txrx_disable();
    if (mode) {
        link_fix_rx_enable(mac_addr, fre, packet_type, 0xffff);
    } else {
        link_fix_tx_enable(mac_addr, fre, packet_type, 0xffff);
    }
    local_irq_enable();
}


void bt_updata_fix_rx_result()
{
    struct link_fix_rx_result fix_rx_result;
    link_fix_rx_update_result(&fix_rx_result);
    printf("err_b:%d sum_b:%d perr:%d herr_b:%d invaile:%d  \n",
           fix_rx_result.rx_err_b,
           fix_rx_result.rx_sum_b,
           fix_rx_result.rx_perr_p,
           fix_rx_result.rx_herr_p,
           fix_rx_result.rx_invail_p
          );
}


void spp_data_handler(u8 packet_type, u16 ch, u8 *packet, u16 size)
{
    switch (packet_type) {
    case 1:
        bt_log("spp connect\n");
        user_var.spp_conn_status = 1;
        break;
    case 2:
        bt_log("spp disconnect\n");
        user_var.spp_conn_status = 0;
        bt_log("disconnect ble by phone!!!");
        u8 data_stop_find[1] = {CMD_SYNC_STOP_FIND_EAR};
        user_send_data_to_both(data_stop_find, sizeof(data_stop_find));
        break;
    case 7:
        bt_log("spp_rx:");
        put_buf(packet, size);
        // user_app_data_deal(packet, size, 1);
#if AEC_DEBUG_ONLINE
        aec_debug_online(packet, size);
#endif

#if TCFG_USER_RSSI_TEST_EN
        int spp_get_rssi_handler(u8 * packet, u16 size);
        spp_get_rssi_handler(packet, size);
#endif
        break;
    }
}

extern const char *sdk_version_info_get(void);
extern void online_spp_init(void);
extern u8 get_vbat_percent(void);
extern u8 *sdfile_get_burn_code(u8 *len);
extern void bt_make_ble_address(u8 *ble_address, u8 *edr_address);
extern int le_controller_set_mac(void *addr);

static u8 *bt_get_sdk_ver_info(u8 *len)
{
    const char *p = sdk_version_info_get();
    if (len) {
        *len = strlen(p);
    }

    log_info("sdk_ver:%s %x\n", p, *len);
    return (u8 *)p;
}

void bredr_handle_register()
{
    y_printf("bredr_handle_register");
#if (TCFG_BT_SUPPORT_SPP==1)
#if APP_ONLINE_DEBUG
    online_spp_init();
#endif
    bt_spp_data_deal_handle_register(spp_data_handler);
#endif
    bt_fast_test_handle_register(bt_fast_test_api);//测试盒快速测试接口
#if TCFG_BT_VOL_SYNC_ENABLE
    bt_music_vol_change_handle_register(set_music_device_volume, phone_get_device_vol);
#endif
#if TCFG_BT_DISPLAY_BAT_ENABLE
    bt_get_battery_value_handle_register(bt_get_battery_value);   /*电量显示获取电量的接口*/
#endif

    bt_read_remote_name_handle_register(user_record_remote_name_addr);

    bt_dut_test_handle_register(bt_dut_api);
}

#if TCFG_USER_TWS_ENABLE
static void rx_dual_conn_info(u8 *data, int len)
{
    r_printf("tws_sync_dual_conn_info_func: %d, %d\n", data[0], data[1]);
    if (data[0]) {
        g_bt_hdl.bt_dual_conn_config = DUAL_CONN_SET_TWO;
    } else {
        g_bt_hdl.bt_dual_conn_config = DUAL_CONN_SET_ONE;
    }
    syscfg_write(CFG_TWS_DUAL_CONFIG, &(g_bt_hdl.bt_dual_conn_config), 1);

}
static void tws_sync_dual_conn_info_func(void *_data, u16 len, bool rx)
{
    if (rx) {
        u8 *data = malloc(len);
        memcpy(data, _data, len);
        int msg[4] = { (int)rx_dual_conn_info, 2, (int)data, len};
        os_taskq_post_type("app_core", Q_CALLBACK, 4, msg);
    }
}
REGISTER_TWS_FUNC_STUB(app_vol_sync_stub) = {
    .func_id = 0x1A782C1B,
    .func    = tws_sync_dual_conn_info_func,
};
void tws_sync_dual_conn_info()
{
    u8 data[2];
    data[0] = g_bt_hdl.bt_dual_conn_config;
    tws_api_send_data_to_slave(data, 2, 0x1A782C1B);

}
#endif

u8 get_bt_dual_config()
{
    return g_bt_hdl.bt_dual_conn_config;
}
void set_dual_conn_config(u8 *addr, u8 dual_conn_en)
{
#if TCFG_BT_DUAL_CONN_ENABLE
    if (dual_conn_en) {
        g_bt_hdl.bt_dual_conn_config = DUAL_CONN_SET_TWO;
    } else {
        g_bt_hdl.bt_dual_conn_config = DUAL_CONN_SET_ONE;
        u8 *other_conn_addr;
        other_conn_addr = btstack_get_other_dev_addr(addr);
        if (other_conn_addr) {
            btstack_device_detach(btstack_get_conn_device(other_conn_addr));
        }
        extern void updata_last_link_key(bd_addr_t bd_addr, u8 id);
        extern u8 get_remote_dev_info_index();
        if (addr) {
            updata_last_link_key(addr, get_remote_dev_info_index());
        }
    }
#if TCFG_USER_TWS_ENABLE
    tws_sync_dual_conn_info();
#endif
    bt_set_auto_conn_device_num((get_bt_dual_config() == DUAL_CONN_SET_TWO) ? 2 : 1);
    syscfg_write(CFG_TWS_DUAL_CONFIG, &(g_bt_hdl.bt_dual_conn_config), 1);
    r_printf("set_dual_conn_config=%d\n", g_bt_hdl.bt_dual_conn_config);
#endif

}
void test_set_dual_config()
{
    /* extern u8 *bt_get_current_remote_addr(void); */
    u8 *addr = bt_get_current_remote_addr();
    set_dual_conn_config(addr, (get_bt_dual_config() == DUAL_CONN_SET_TWO ? 0 : 1));
}

void user_read_remote_name_handle(u8 status, u8 *addr, u8 *name)
{
    r_printf("\nuser_read_remote_name_handle:\n");
    put_buf(addr, 6);
    printf("name=%s\n", name);
#if RCSP_MODE && TCFG_RCSP_DUAL_CONN_ENABLE
    rcsp_1t2_set_edr_info(addr, name);
#endif
}
void bt_function_select_init()
{
    extern void set_qos_value(u8 qos_on_value, u8 qos_off_value);
    set_qos_value(5, 16);
    /* set_bt_data_rate_acl_3mbs_mode(1); */
#if TCFG_BT_DUAL_CONN_ENABLE
    g_bt_hdl.bt_dual_conn_config = DUAL_CONN_SET_TWO;//DUAL_CONN_SET_TWO:默认可以连接1t2  DUAL_CONN_SET_ONE:默认只支持一个连接
    syscfg_read(CFG_TWS_DUAL_CONFIG, &(g_bt_hdl.bt_dual_conn_config), 1);
#else
    g_bt_hdl.bt_dual_conn_config = DUAL_CONN_CLOSE;
#endif
    g_printf("<<<<<<<<<<<<<<bt_dual_conn_config=%d>>>>>>>>>>\n", g_bt_hdl.bt_dual_conn_config);
    if (g_bt_hdl.bt_dual_conn_config != DUAL_CONN_SET_TWO) {
        set_tws_task_interval(120);
    }
    bt_set_user_ctrl_conn_num((get_bt_dual_config() == DUAL_CONN_CLOSE) ? 1 : 2);
    set_lmp_support_dual_con((get_bt_dual_config() == DUAL_CONN_CLOSE) ? 1 : 2);
    bt_set_auto_conn_device_num((get_bt_dual_config() == DUAL_CONN_SET_TWO) ? 2 : 1);

    bt_set_support_msbc_flag(TCFG_BT_MSBC_EN);

#if (!CONFIG_A2DP_GAME_MODE_ENABLE)
    bt_set_support_aac_flag(TCFG_BT_SUPPORT_AAC);
    bt_set_aac_bitrate(0x20000);
#endif

#if TCFG_BT_DISPLAY_BAT_ENABLE
    bt_set_update_battery_time(60);
#else
    bt_set_update_battery_time(0);
#endif
    /*回连搜索时间长度设置,可使用该函数注册使用，ms单位,u16*/
    bt_set_page_timeout_value(0);

    /*回连时超时参数设置。ms单位。做主机有效*/
    bt_set_super_timeout_value(8000);


#if TCFG_BT_VOL_SYNC_ENABLE
    vol_sys_tab_init();
#endif
    /* io_capabilities
     * 0: Display only 1: Display YesNo 2: KeyboardOnly 3: NoInputNoOutput
     *  authentication_requirements: 0:not protect  1 :protect
    */
    bt_set_simple_pair_param(3, 0, 2);
    bt_testbox_ex_info_get_handle_register(TESTBOX_INFO_VBAT_VALUE, get_vbat_value);
    bt_testbox_ex_info_get_handle_register(TESTBOX_INFO_VBAT_PERCENT, get_vbat_percent);
    bt_testbox_ex_info_get_handle_register(TESTBOX_INFO_BURN_CODE, sdfile_get_burn_code);
    bt_testbox_ex_info_get_handle_register(TESTBOX_INFO_SDK_VERSION, bt_get_sdk_ver_info);
    bt_read_remote_name_handle_register(user_read_remote_name_handle);

    bt_set_sbc_cap_bitpool(TCFG_BT_SBC_BITPOOL);
#if TCFG_USER_BLE_ENABLE
    u8 tmp_ble_addr[6];
#if TCFG_BT_BLE_BREDR_SAME_ADDR
    memcpy(tmp_ble_addr, (void *)bt_get_mac_addr(), 6);
#else
    bt_make_ble_address(tmp_ble_addr, (void *)bt_get_mac_addr());
#endif
    le_controller_set_mac((void *)tmp_ble_addr);
    puts("-----edr + ble 's address-----\n");
    printf_buf((void *)bt_get_mac_addr(), 6);
    printf_buf((void *)tmp_ble_addr, 6);
#endif

#if (CONFIG_BT_MODE != BT_NORMAL)
    set_bt_enhanced_power_control(1);
#endif

}

// extern void mem_unfree_dump();
void user_info_ptintf()
{
    // g_printf("tws_api_get_role():%d", tws_api_get_role());
    g_printf("tws_api_get_local_channel():%c", tws_api_get_local_channel());
    g_printf("this soft var: v7.0.1");
    y_printf("get_charge_online_flag():%d", get_charge_online_flag());
    // void *devices[2] = {NULL, NULL};
    // int num = btstack_get_conn_devices(devices, 2);
    // for (int i = 0; i < num; i++) {
    //     g_printf("bt_get_curr_channel_state_for_addr(%d):0x%x", i, bt_get_curr_channel_state_for_addr(btstack_get_device_mac_addr(devices[i])));
    // }
    // mem_unfree_dump();
    mem_stats();
    // g_printf("get_vbat_value() = %d", get_vbat_value());
    // g_printf("clk_get(sys) = %d", clk_get("sys"));
    // g_printf("bt_get_curr_channel_state() = 0x%x", bt_get_curr_channel_state());
    // g_printf("tws_in_sniff_state() = %d", tws_in_sniff_state());
    // audio_gain_dump();
    // extern u8 key_idle_query(void);
    // extern u8 pll_trim_idle_query(void);
    // extern u8 ota_idle_query(void);
    // extern u8 uart_idle_query(void);
    // extern u8 audio_mic_capless_idle_query(void);
    // extern u8 audio_cvp_idle_query();
    // extern u8 effect_tool_idle_query(void);
    // extern u8 lpkey_idle_query(void);
    // g_printf("key_idle_query() = %d", key_idle_query());
    // g_printf("pll_trim_idle_query() = %d", pll_trim_idle_query());
    // g_printf("ota_idle_query() = %d", ota_idle_query());
    // g_printf("uart_idle_query() = %d", uart_idle_query());
    // g_printf("audio_mic_capless_idle_query() = %d", audio_mic_capless_idle_query());
    // g_printf("audio_cvp_idle_query() = %d", audio_cvp_idle_query());
    // g_printf("effect_tool_idle_query() = %d", effect_tool_idle_query());
    // g_printf("lpkey_idle_query() = %d", lpkey_idle_query());
}
void audio_dump2(void) //打印所有的dac,adc寄存器
{
    printf("-bt_tws_volume--tx-[%d] \n",app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
	audio_dump();
}

/*
 * 对应原来的状态处理函数，连接，电话状态等
 */

static int bt_connction_status_event_handler(struct bt_event *bt)
{

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        g_bt_hdl.init_ok = 1;
        bt_log("BT_STATUS_INIT_OK\n");
		extern void clock_manager_test3(void);
		clock_manager_test3();
        dual_conn_page_devices_init();
        sys_timer_add(NULL, user_info_ptintf, 5000);
		//sys_timer_add(NULL, audio_dump2, 1000);
#if TCFG_NORMAL_SET_DUT_MODE
        log_info("edr set dut mode\n");
        bredr_set_dut_enble(1, 1);
        log_info("ble set dut mode\n");
        ble_bqb_test_thread_init();
#endif

#if TCFG_BLE_AUDIO_TEST_EN
        extern void big_tx_test_open(void);
        extern void big_rx_test_open(void);
        extern void cig_perip_test_open(void);
        extern void cig_central_test_open(void);
        extern void df_aoa_broadcast_test_open(void);
        extern void df_aoa_tx_connected_test_open(void);

        /* df_aoa_tx_connected_test_open(); */
        /* df_aoa_broadcast_test_open(); */
        /* big_tx_test_open(); */
        /* big_rx_test_open(); */
        cig_perip_test_open();
        /* cig_central_test_open(); */
        break;
#endif

#if TCFG_AUDIO_DATA_EXPORT_DEFINE
#if (TCFG_AUDIO_DATA_EXPORT_DEFINE == AUDIO_DATA_EXPORT_VIA_SPP)
        lmp_hci_set_role_switch_supported(0);
#endif /*TCFG_AUDIO_DATA_EXPORT_DEFINE == AUDIO_DATA_EXPORT_VIA_SPP*/
#if ((TCFG_AUDIO_DATA_EXPORT_DEFINE == AUDIO_DATA_EXPORT_VIA_SPP) \
    || (TCFG_AUDIO_DATA_EXPORT_DEFINE == AUDIO_DATA_EXPORT_VIA_SD))
        extern int audio_capture_init();
        audio_capture_init();
#endif
#endif /*TCFG_AUDIO_DATA_EXPORT_DEFINE*/

#if TCFG_AUDIO_ANC_ENABLE
        anc_poweron();
#endif

#if (TCFG_USER_BLE_ENABLE || TCFG_BT_BLE_ADV_ENABLE || (BT_AI_SEL_PROTOCOL & TUYA_DEMO_EN))
        if (BT_MODE_IS(BT_BQB)) {
            ble_bqb_test_thread_init();
        } else {
            bt_ble_init();
        }
#endif

        bt_init_ok_search_index();
		//led_ui_manager_display(LED_LOCAL, LED_UUID_WHITE_ON, LED_MODE_C);
#if TCFG_CHARGESTORE_ENABLE
        chargestore_set_bt_init_ok(1);
#endif

#if TCFG_TEST_BOX_ENABLE
        testbox_set_bt_init_ok(1);
#endif

#if ((CONFIG_BT_MODE == BT_BQB)||(CONFIG_BT_MODE == BT_PER))
        break;
#endif
		bt_log("tone_player_runing():%d", tone_player_runing());
        if (app_var.play_poweron_tone) {
            if (tone_player_runing()) {
                break;
            }
        }
#if TCFG_USER_TWS_ENABLE
        bt_tws_poweron();
#else
        lmp_hci_write_scan_enable((1 << 1) | 1);
#endif
        sys_auto_shut_down_enable();
        break;

    case BT_STATUS_SECOND_CONNECTED:
        bt_log("BT_STATUS_CONNECTED 2\n");
        bt_clear_current_poweron_memory_search_index(0);
    case BT_STATUS_FIRST_CONNECTED:
        bt_log("BT_STATUS_CONNECTED 1\n");
        user_var.out_of_range = 0;
        user_var.enter_pairing_mode = 0;
        user_var.box_enter_pair_mode = 0;
        user_var.slave_reconn = 0;
        user_var.tws_poweron_status = 0;
		
        set_unconn_tws_phone_enter_idle(0);
        bt_ble_rcsp_adv_enable();
        // led_ui_manager_display(LED_LOCAL, LED_UUID_WHITE_OFF, LED_MODE_C);
        user_key_reconn_add_device(bt->args);
        user_1t1_mode_reconn_add_device(bt->args);
        if (user_out_of_range_page_timerid != 0) {
            sys_timeout_del(user_out_of_range_page_timerid);
            user_out_of_range_page_timerid = 0;
        }
        if (user_lowpower_scan_conn_timer != 0) {
            sys_timer_del(user_lowpower_scan_conn_timer);
            user_lowpower_scan_conn_timer = 0;
        }

        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            extern u8 *get_last_device_connect_linkkey(u16 *len);
            u8 *test_linkkey;
            u16 test_linkkey_len = 16;
            test_linkkey = get_last_device_connect_linkkey(&test_linkkey_len);
            if (test_linkkey) {
                y_printf("===============test_linkkey================\n");
                put_buf(test_linkkey, test_linkkey_len);
                y_printf("===========================================\n");
            }
            if (get_tws_sibling_connect_state()) {
                user_send_phone_info_to_sibling();
            }
        }
        user_app_data_notify(CMD_NOTIFY_DEVICE_CHANGE);
#if(JL_EARPHONE_APP_EN && RCSP_UPDATE_EN)
        extern u8 rcsp_update_get_role_switch(void);
        if (rcsp_update_get_role_switch()) {
            tws_api_role_switch();
            tws_api_auto_role_switch_disable();
        }
#endif

#if TCFG_CHARGESTORE_ENABLE
        chargestore_set_phone_connect();
#endif
#if TCFG_USER_TWS_ENABLE
        bt_tws_phone_connected();
#endif
        bt_tws_sync_volume();
        user_1t1_and_1t2_mode_connect_deal();
        break;
    case BT_STATUS_FIRST_DISCONNECT:
        bt_log("BT_STATUS_DISCONNECT 1\n");
        bt_log("poweroff:%d, total_connect:%d\n", app_var.goto_poweroff_flag, bt_get_total_connect_dev());
        if (app_var.goto_poweroff_flag) {
            break;
        }
        int delay_msec = 400;
        if (tws_in_sniff_state()) {
            delay_msec = 800;
        }
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            tws_play_tone_file_alone(get_tone_files()->bt_disconnect, delay_msec);
        }
    case BT_STATUS_SECOND_DISCONNECT:
        bt_log("BT_STATUS_DISCONNECT 2\n");
        bt_log("poweroff:%d, total_connect:%d\n", app_var.goto_poweroff_flag, bt_get_total_connect_dev());
        if (app_var.goto_poweroff_flag) {
            break;
        }

        user_key_reconn_add_device(bt->args);
        user_phone_info_del(bt->args);

        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            if (get_tws_sibling_connect_state()) {
                user_send_phone_info_to_sibling();
            }
        }
#if TCFG_CHARGESTORE_ENABLE
        chargestore_set_phone_disconnect();
#endif
        user_app_data_notify(CMD_NOTIFY_DEVICE_CHANGE);
        break;

    case BT_STATUS_CONN_A2DP_CH:
        bt_log("++++++++ BT_STATUS_CONN_A2DP_CH +++++++++  \n");
        user_app_data_notify(CMD_NOTIFY_DEVICE_CHANGE);
        break;
    case BT_STATUS_DISCON_A2DP_CH:
        bt_log("++++++++ BT_STATUS_DISCON_A2DP_CH +++++++++  \n");
        user_app_data_notify(CMD_NOTIFY_DEVICE_CHANGE);
        break;
    case BT_STATUS_AVRCP_INCOME_OPID:
        bt_log("BT_STATUS_AVRCP_INCOME_OPID:%d\n", bt->value);
        if (bt->value == AVC_VOLUME_UP) {

        } else if (bt->value == AVC_VOLUME_DOWN) {

        }
        break;
    default:
        bt_log(" BT STATUS DEFAULT\n");
        break;
    }
    return 0;
}



static void bt_hci_event_inquiry(struct bt_event *bt)
{
#if TCFG_USER_EMITTER_ENABLE
    if (g_bt_hdl.emitter_or_receiver == BT_EMITTER_EN) {
        extern void emitter_search_stop();
        emitter_search_stop();
    }
#endif
}

static void bt_hci_event_connection(struct bt_event *bt)
{
    //clk_set_sys_lock(BT_CONNECT_HZ, 0);
    log_info("tws_conn_state=%d\n", g_bt_hdl.tws_conn_state);
}

extern void bt_get_vm_mac_addr(u8 *addr);
static void bt_discovery_and_connectable_using_loca_mac_addr(u8 inquiry_scan_en,
        u8 page_scan_en)
{
    u8 local_addr[6];

    bt_get_vm_mac_addr(local_addr);
    lmp_hci_write_local_address(local_addr);
    if (page_scan_en) {
        bt_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
    }
    if (inquiry_scan_en) {
        bt_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
    }
}

static void bt_hci_event_disconnect(struct bt_event *bt)
{
    if (app_var.goto_poweroff_flag) {
        return;
    }

    log_info("<<<<<<<<<<<<<<total_dev: %d>>>>>>>>>>>>>\n", bt_get_total_connect_dev());

#if TCFG_TEST_BOX_ENABLE
    if (testbox_get_ex_enter_dut_flag()) {
        bt_discovery_and_connectable_using_loca_mac_addr(1, 1);
        return;
    }

    if (testbox_get_status()) {
        bt_discovery_and_connectable_using_loca_mac_addr(0, 1);
        return;
    }
#endif
}

static void bt_hci_event_linkkey_missing(struct bt_event *bt)
{

}

static void bt_hci_event_page_timeout()
{
    log_info("HCI_EVENT_PAGE_TIMEOUT conuter %d\n",
             g_bt_hdl.auto_connection_counter);
}

static void bt_hci_event_connection_timeout(struct bt_event *bt)
{
#if TCFG_TEST_BOX_ENABLE
    if (testbox_get_ex_enter_dut_flag()) {
        bt_discovery_and_connectable_using_loca_mac_addr(1, 1);
        return;
    }
#endif
}

static void bt_hci_event_connection_exist(struct bt_event *bt)
{
}


void __attribute__((weak)) lp_touch_key_testbox_inear_trim(u8 flag)
{
}

enum {
    TEST_STATE_INIT = 1,
    TEST_STATE_EXIT = 3,
};

enum {
    ITEM_KEY_STATE_DETECT = 0,
    ITEM_IN_EAR_DETECT,
};

static u8 in_ear_detect_test_flag = 0;
static void testbox_in_ear_detect_test_flag_set(u8 flag)
{
    in_ear_detect_test_flag = flag;
}

u8 testbox_in_ear_detect_test_flag_get(void)
{
    return in_ear_detect_test_flag;
}

static void bt_in_ear_detection_test_state_handle(u8 state, u8 *value)
{
    switch (state) {
    case TEST_STATE_INIT:
        testbox_in_ear_detect_test_flag_set(1);

#if TCFG_LP_EARTCH_KEY_ENABLE
        lp_touch_key_testbox_inear_trim(1);
#endif
        //start trim
        break;
    case TEST_STATE_EXIT:
        testbox_in_ear_detect_test_flag_set(0);
#if TCFG_LP_EARTCH_KEY_ENABLE
        lp_touch_key_testbox_inear_trim(0);
#endif
        break;
    }
}

static void bt_vendor_meta_event_handle(u8 sub_evt, u8 *arg, u8 len)
{
    log_info("vendor event:%x\n", sub_evt);
    log_info_hexdump(arg, 6);

    if (sub_evt != HCI_SUBEVENT_VENDOR_TEST_MODE_CFG) {
        log_info("unknow_sub_evt:%x\n", sub_evt);
        return;
    }

    u8 test_item = arg[0];
    u8 state = arg[1];

    if (ITEM_IN_EAR_DETECT == test_item) {
        bt_in_ear_detection_test_state_handle(state, NULL);
    }
}

extern void bt_set_remote_test_flag(u8 own_remote_test);

static int bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("-----------bt_hci_event_handler reason %x %x", bt->event, bt->value);

    if (bt->event == HCI_EVENT_VENDOR_REMOTE_TEST) {
        if (bt->value == VENDOR_TEST_DISCONNECTED) {
#if TCFG_TEST_BOX_ENABLE
            if (testbox_get_status()) {
                if (bt_get_remote_test_flag()) {
                    testbox_clear_connect_status();
                }
            }
#endif
            bt_set_remote_test_flag(0);
            log_info("clear_test_box_flag");
            cpu_reset();
            return 0;
        } else {

#if TCFG_BT_BLE_ADV_ENABLE
#if (CONFIG_BT_MODE == BT_NORMAL)
            //1:edr con;2:ble con;
            if (VENDOR_TEST_LEGACY_CONNECTED_BY_BT_CLASSIC == bt->value) {
                extern void bt_ble_adv_enable(u8 enable);
                bt_ble_adv_enable(0);
            }
#endif
#endif
#if TCFG_USER_TWS_ENABLE
            if (VENDOR_TEST_CONNECTED_WITH_TWS != bt->value) {
                bt_tws_poweroff();
            }
#endif
        }
    }


    switch (bt->event) {
    case HCI_EVENT_VENDOR_META:
        bt_vendor_meta_event_handle(bt->value, bt->args, 6);
        break;
    case HCI_EVENT_INQUIRY_COMPLETE:
        log_info(" HCI_EVENT_INQUIRY_COMPLETE \n");
        bt_hci_event_inquiry(bt);
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        log_info(" HCI_EVENT_USER_CONFIRMATION_REQUEST \n");
        ///<可通过按键来确认是否配对 1：配对   0：取消
        bt_send_pair(1);
        break;
    case HCI_EVENT_USER_PASSKEY_REQUEST:
        log_info(" HCI_EVENT_USER_PASSKEY_REQUEST \n");
        ///<可以开始输入6位passkey
        break;
    case HCI_EVENT_USER_PRESSKEY_NOTIFICATION:
        log_info(" HCI_EVENT_USER_PRESSKEY_NOTIFICATION %x\n", bt->value);
        ///<可用于显示输入passkey位置 value 0:start  1:enrer  2:earse   3:clear  4:complete
        break;
    case HCI_EVENT_PIN_CODE_REQUEST :
        log_info("HCI_EVENT_PIN_CODE_REQUEST  \n");
        break;

    case HCI_EVENT_VENDOR_NO_RECONN_ADDR :
        log_info("HCI_EVENT_VENDOR_NO_RECONN_ADDR \n");
        bt_hci_event_disconnect(bt) ;
        break;

    case HCI_EVENT_DISCONNECTION_COMPLETE :
        bt_log("HCI_EVENT_DISCONNECTION_COMPLETE bt->value:0x%x\n", bt->value);
        bt_log("poweroff:%d, role:%d, tws_status:%d, total_connect:%d", app_var.goto_poweroff_flag, tws_api_get_role(), get_bt_tws_connect_status(), bt_get_total_connect_dev());
        if (bt->value ==  ERROR_CODE_CONNECTION_TIMEOUT) {
            bt_hci_event_connection_timeout(bt);
        }
        if (app_var.goto_poweroff_flag) {
            return 0;
        }
        user_phone_info_del(bt->args);
        bt_hci_event_disconnect(bt);
        if (bt->value == ERROR_CODE_CONNECTION_TIMEOUT || bt->value == ERROR_CODE_ROLE_CHANGE_NOT_ALLOWED) {
            user_var.out_of_range = 1;
            add_device_2_page_list(bt->args, TCFG_BT_TIMEOUT_PAGE_TIME * 1000);
            user_out_of_range_page_deal();
        } else if (bt->value == ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION || bt->value == 0x16) {
            bt_log("user_var.slave_reconn:%d", user_var.slave_reconn);
            if (bt->value == 0x16 && user_var.slave_reconn) {
                user_var.out_of_range = 1;
                add_device_2_page_list(bt->args, TCFG_BT_TIMEOUT_PAGE_TIME * 1000);
                user_out_of_range_page_deal();
            } else {
                bt_log("user_var.box_enter_pair_mode:%d", user_var.box_enter_pair_mode);
                if (bt_get_total_connect_dev() == 0 || user_var.box_enter_pair_mode) {
                    user_tws_sync_call(CMD_SYNC_ENTER_PAIR_MODE);
                }
            }
        } 

        if (bt_get_total_connect_dev() == 0) {
            bt_log("bt_get_low_latency_mode():%d", bt_get_low_latency_mode());
            if (bt_get_low_latency_mode()) {
                tws_api_low_latency_enable(0);
                a2dp_player_low_latency_enable(0);
                user_app_info.game_mode = 0;
                // user_app_data_notify(CMD_NOTIFY_GAME_MODE);
            }
        }
        break;

    case BTSTACK_EVENT_HCI_CONNECTIONS_DELETE:
    case HCI_EVENT_CONNECTION_COMPLETE:
        log_info(" HCI_EVENT_CONNECTION_COMPLETE bt->value = 0x%x\n", bt->value);
        switch (bt->value) {
        case ERROR_CODE_SUCCESS :
            log_info("ERROR_CODE_SUCCESS  \n");
            testbox_in_ear_detect_test_flag_set(0);
            bt_hci_event_connection(bt);
            break;
        case ERROR_CODE_PIN_OR_KEY_MISSING:
            log_info(" ERROR_CODE_PIN_OR_KEY_MISSING \n");
            bt_hci_event_linkkey_missing(bt);

        case ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED :
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR:
        case ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED  :
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION   :
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST :
        case ERROR_CODE_AUTHENTICATION_FAILURE :
            //case CUSTOM_BB_AUTO_CANCEL_PAGE:
            bt_hci_event_disconnect(bt) ;
            break;

        case ERROR_CODE_PAGE_TIMEOUT:
            log_info(" ERROR_CODE_PAGE_TIMEOUT \n");
            bt_hci_event_page_timeout();
            break;

        case ERROR_CODE_CONNECTION_TIMEOUT:
            log_info(" ERROR_CODE_CONNECTION_TIMEOUT \n");
            bt_hci_event_connection_timeout(bt);
            break;

        case ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS  :
            log_info("ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS   \n");
            bt_hci_event_connection_exist(bt);
            break;
        default:
            break;

        }
        break;
    default:
        break;

    }
    return 0;
}



int bt_in_background_event_handler(struct sys_event *event)
{
    return 0;
}

u8 bt_app_exit_check(void)
{
    int esco_state;
    if (app_var.siri_stu && app_var.siri_stu != 3) {
        // siri不退出
        return 0;
    }
    esco_state = bt_get_call_status();
    if (esco_state == BT_CALL_OUTGOING  ||
        esco_state == BT_CALL_ALERT     ||
        esco_state == BT_CALL_INCOMING  ||
        esco_state == BT_CALL_ACTIVE) {
        // 通话不退出
        return 0;
    }

    return 1;
}


int bt_mode_btstack_event_handler(int *event)
{
    bt_connction_status_event_handler((struct bt_event *)event);
    return 0;
}
APP_MSG_HANDLER(btmode_stack_msg_handler) = {
    .owner      = APP_MODE_BT,
    .from       = MSG_FROM_BT_STACK,
    .handler    = bt_mode_btstack_event_handler,
};

int bt_mode_hci_event_handler(int *event)
{
    bt_hci_event_handler((struct bt_event *)event);
    return 0;
}
APP_MSG_HANDLER(btmode_hci_msg_handler) = {
    .owner      = APP_MODE_BT,
    .from       = MSG_FROM_BT_HCI,
    .handler    = bt_mode_hci_event_handler,
};

int bt_mode_msg_handler(int *msg)
{
    int key_msg;
    struct bt_event *event = (struct bt_event *)(msg + 1);

    switch (msg[0]) {
#if TCFG_USER_TWS_ENABLE
    case MSG_FROM_TWS:
        bt_tws_connction_status_event_handler(msg + 1);
        break;
#endif
    case MSG_FROM_BT_STACK:
        bt_connction_status_event_handler(event);
        break;
    case MSG_FROM_BT_HCI:
        bt_hci_event_handler(event);
        break;
    case MSG_FROM_APP:
        break;
    }

    return 0;
}

static int tone_poweron_callback(void *priv, enum stream_event event)
{
#if TCFG_USER_TWS_ENABLE
    if (event == STREAM_EVENT_STOP) {
        bt_tws_poweron();
    }
#else
    lmp_hci_write_scan_enable((1 << 1) | 1);

#endif
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙非后台模式退出蓝牙等待蓝牙状态可以退出
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_no_background_exit_check(void *priv)
{
    if (g_bt_hdl.init_ok == 0) {
        return;
    }
    if (esco_player_runing() || a2dp_player_runing()) {
        return ;
    }
#if TCFG_USER_BLE_ENABLE
    bt_ble_exit();
#endif
    btstack_exit();
    sys_timer_del(g_bt_hdl.exit_check_timer);
    g_bt_hdl.init_ok = 0;
    g_bt_hdl.init_start = 0;
    g_bt_hdl.exit_check_timer = 0;
    bt_set_stack_exiting(0);
    g_bt_hdl.exiting = 0;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙非后台模式退出模式
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static u8 bt_nobackground_exit()
{
    if (!g_bt_hdl.init_start) {
        g_bt_hdl.exiting = 0;
        return 0;
    }
    bt_set_stack_exiting(1);
    //extern void __app_protocol_speech_stop(void);
    //__app_protocol_speech_stop();
#if TCFG_USER_TWS_ENABLE
    // void tws_dual_conn_close();
    tws_dual_conn_close();
    bt_tws_poweroff();
#endif
    //app_protocol_exit(0);
    bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    bt_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
    bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    if (g_bt_hdl.auto_connection_timer) {
        sys_timeout_del(g_bt_hdl.auto_connection_timer);
        g_bt_hdl.auto_connection_timer = 0;
    }

    if (g_bt_hdl.exit_check_timer == 0) {
        g_bt_hdl.exit_check_timer = sys_timer_add(NULL, bt_no_background_exit_check, 10);
        printf("set exit timer\n");
    }

    return 0;
}

int bt_mode_init()
{
    bt_log("bt_mode_init, tone:%d", app_var.play_poweron_tone);
    int tone_player_err = 0;

    if (app_var.play_poweron_tone) {
        play_tone_file_callback(get_tone_files()->power_on, NULL, tone_poweron_callback);
    }

    g_bt_hdl.init_start = 1;//蓝牙协议栈已经开始初始化标志位
    g_bt_hdl.init_ok = 0;
    g_bt_hdl.exiting = 0;
    g_bt_hdl.wait_exit = 0;
    g_bt_hdl.ignore_discon_tone = 0;
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

    bt_function_select_init();
    bredr_handle_register();
    EARPHONE_STATE_INIT();
    btstack_init();
#if TCFG_USER_TWS_ENABLE

#if TCFG_TWS_AUTO_ROLE_SWITCH_ENABLE
    tws_api_esco_rssi_role_switch(1);//通话根据信号强度主从切换使能
#endif
    tws_profile_init();
#endif

    void bt_sniff_feature_init();
    bt_sniff_feature_init();
    app_var.dev_volume = -1;

    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_BT);

    return 0;
}

int bt_mode_try_exit()
{
    return 0;

    if (g_bt_hdl.wait_exit) {
        //等待蓝牙断开或者音频资源释放或者电话资源释放
        if (!g_bt_hdl.exiting) {
            g_bt_hdl.wait_exit++;
            if (g_bt_hdl.wait_exit > 3) {
                //wait two round to do some hci event or other stack event
                return 0;
            }
        }
        return -EBUSY;
    }
    g_bt_hdl.wait_exit = 1;
    g_bt_hdl.exiting = 1;
#if(TCFG_BT_BACKGROUND_ENABLE == 0) //非后台退出不播放提示音
    g_bt_hdl.ignore_discon_tone = 1;
#endif
    //only need to do once
#if (TCFG_BT_BACKGROUND_ENABLE)
    bt_background_suspend();
#else
    bt_nobackground_exit();
#endif
    return -EBUSY;
}

int bt_mode_release()
{
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_BT);
    return 0;
}

static const struct app_mode_ops bt_mode_ops = {
    .enter          = bt_mode_init,
    .try_exit       = bt_mode_try_exit,
    .exit           = bt_mode_release,
    .msg_handler    = bt_mode_msg_handler,
};

REGISTER_APP_MODE(bt_mode) = {
    .name   = APP_MODE_BT,
    .index  = 1,
    .ops    = &bt_mode_ops,
};

/*************************************************************/
//   记录连接的设备地址和设备名的，在这里进行
/*************************************************************/
extern APP_INFO user_app_info;
struct PHONE_INFO user_phone_info1;
struct PHONE_INFO user_phone_info2;
extern u8 addr_all_ff[];

void user_phone_info_init()
{
    bt_log("user_phone_info_init");
    memcpy(user_phone_info1.addr, addr_all_ff, 6);
    user_phone_info1.name_len = 0;
    user_phone_info1.remote_name = NULL;
    memcpy(user_phone_info2.addr, addr_all_ff, 6);
    user_phone_info2.name_len = 0;
    user_phone_info2.remote_name = NULL;
}
__initcall(user_phone_info_init);

void user_phone_info_del(u8 *addr)
{
    bt_log("user_phone_info_del");
    if (!memcmp(user_phone_info1.addr, addr, 6)) {
        bt_log("this addr is same at info1!!!");
        bt_log_hexdump(user_phone_info1.addr, 6);
        bt_log("%s\n", user_phone_info1.remote_name);
        if (user_phone_info1.remote_name != NULL) {
            free(user_phone_info1.remote_name);
        }
        user_phone_info1.name_len = 0;
        user_phone_info1.remote_name = NULL;
        if (user_phone_info2.remote_name != NULL) {
            bt_log("info2 have iphone info!!!");
            memcpy(user_phone_info1.addr, user_phone_info2.addr, 6);
            user_phone_info1.name_len = user_phone_info2.name_len;
            user_phone_info1.remote_name = malloc(user_phone_info1.name_len);
            memcpy(user_phone_info1.remote_name, user_phone_info2.remote_name, user_phone_info1.name_len);
            free(user_phone_info2.remote_name);
            user_phone_info2.remote_name = NULL;
            memcpy(user_phone_info2.addr, addr_all_ff, 6);
            user_phone_info2.name_len = 0;
        }
    } else if (!memcmp(user_phone_info2.addr, addr, 6)) {
        bt_log("this addr is same at info2!!!");
        bt_log_hexdump(user_phone_info2.addr, 6);
        bt_log("%s\n", user_phone_info2.remote_name);
        if (user_phone_info2.remote_name != NULL) {
            free(user_phone_info2.remote_name);
        }
        user_phone_info2.remote_name = NULL;
        memcpy(user_phone_info2.addr, addr_all_ff, 6);
        user_phone_info2.name_len = 0;
    } else {
        bt_log("this addr is unknow!!!");
    }
}

u8 *p = NULL;
void user_record_remote_name_addr(u8 status, u8 *addr, u8 *name)
{
    // bt_log("user_record_remote_name_addr, status:%d", status);
    if (user_phone_info1.remote_name == NULL) {
        bt_log("phone info1 have not info!!!");
    } else {
        if (memcmp(user_phone_info1.addr, addr, 6) == 0) {
            bt_log("info1 have info and this addr same as info1, not record repeat!!!");
            return;
        }
        bt_log("phone info1 copy to phone info2!!!");
        if (user_phone_info2.remote_name != NULL) {
            free(user_phone_info2.remote_name);
        }
        memcpy(user_phone_info2.addr, user_phone_info1.addr, 6);
        user_phone_info2.name_len = user_phone_info1.name_len;
        bt_log("malloc info2.name_len:%d!!!", user_phone_info2.name_len);
        user_phone_info2.remote_name = malloc(user_phone_info2.name_len);
        memcpy(user_phone_info2.remote_name, user_phone_info1.remote_name, user_phone_info2.name_len);
        free(user_phone_info1.remote_name);
        user_phone_info1.remote_name = NULL;
        bt_log_hexdump(user_phone_info2.addr, 6);
        bt_log("%s\n", user_phone_info2.remote_name);
    }

    memcpy(user_phone_info1.addr, addr, 6);
    p = name;
    user_phone_info1.name_len = 0;
    while (*p != '\0') {
       p++;
       user_phone_info1.name_len++;
    }
    p = NULL;
    bt_log("malloc info1.name_len:%d!!!", user_phone_info1.name_len);
    user_phone_info1.remote_name = malloc(user_phone_info1.name_len);
    memcpy(user_phone_info1.remote_name, name, user_phone_info1.name_len);
    bt_log_hexdump(user_phone_info1.addr, 6);
    bt_log("%s\n", user_phone_info1.remote_name);

}

//发手机地址和蓝牙名到对方
void user_send_phone_info_to_sibling()
{
    // bt_log("user_send_phone_info_to_sibling");
    u8 phone_info_buf1[user_phone_info1.name_len + 9];
    phone_info_buf1[0] = CMD_SYNC_PHONE_INFO;
    phone_info_buf1[1] = 1;
    phone_info_buf1[2] = user_phone_info1.name_len;
    // bt_log("send phone info, info1.name_len:%d", user_phone_info1.name_len);
    if (user_phone_info1.name_len != 0) {
        // bt_log_hexdump(user_phone_info1.addr, 6);
        // bt_log("%s\n", user_phone_info1.remote_name);
        memcpy(phone_info_buf1+3, user_phone_info1.addr, 6);
        memcpy(phone_info_buf1+9, user_phone_info1.remote_name, user_phone_info1.name_len);
    }
    user_send_data_to_sibling(phone_info_buf1, sizeof(phone_info_buf1));

    u8 phone_info_buf2[user_phone_info2.name_len + 9];
    phone_info_buf2[0] = CMD_SYNC_PHONE_INFO;
    phone_info_buf2[1] = 2;
    phone_info_buf2[2] = user_phone_info2.name_len;
    // bt_log("send phone info, info2.name_len:%d", user_phone_info2.name_len);
    if (user_phone_info2.name_len != 0) {
        // bt_log_hexdump(user_phone_info2.addr, 6);
        // bt_log("%s\n", user_phone_info2.remote_name);
        memcpy(phone_info_buf2+3, user_phone_info2.addr, 6);
        memcpy(phone_info_buf2+9, user_phone_info2.remote_name, user_phone_info2.name_len);
    }
    user_send_data_to_sibling(phone_info_buf2, sizeof(phone_info_buf2));
}

void user_recv_phone_info_from_sibling(u8 *data, u16 len)
{
    // bt_log("user_recv_phone_info_from_sibling");
    if (data[1] == 1) {
        // bt_log("sync phone info1, %d!!!", data[2]);
        free(user_phone_info1.remote_name);
        user_phone_info1.remote_name = NULL;
        user_phone_info1.name_len = 0;
        if (data[2] == 0) {
            memcpy(user_phone_info1.addr, addr_all_ff, 6);
        } else {
            user_phone_info1.name_len = data[2];
            memcpy(user_phone_info1.addr, data+3, 6);
            user_phone_info1.remote_name = malloc(user_phone_info1.name_len);
            memcpy(user_phone_info1.remote_name, data+9, user_phone_info1.name_len);
            // bt_log_hexdump(user_phone_info1.addr, 6);
            // bt_log("%s\n", user_phone_info1.remote_name);
        }
    } else if (data[1] == 2) {
        // bt_log("sync phone info2, %d!!!", data[2]);
        free(user_phone_info2.remote_name);
        user_phone_info2.remote_name = NULL;
        user_phone_info2.name_len = 0;
        if (data[2] == 0) {
            memcpy(user_phone_info2.addr, addr_all_ff, 6);
        } else {
            user_phone_info2.name_len = data[2];
            memcpy(user_phone_info2.addr, data+3, 6);
            user_phone_info2.remote_name = malloc(user_phone_info2.name_len);
            memcpy(user_phone_info2.remote_name, data+9, user_phone_info2.name_len);
            // bt_log_hexdump(user_phone_info2.addr, 6);
            // bt_log("%s\n", user_phone_info2.remote_name);
        }
    }
}

/*************************************************************/


/*************************************************************/
//   设备超距回连的一些操作在这里
/*************************************************************/
void user_out_of_range_page_timeout()
{
    bt_log("user_out_of_range_page_timeout");
    bt_log("tws_status:%d, role:%d, total_connect:%d", get_tws_sibling_connect_state(), tws_api_get_role(), bt_get_total_connect_dev());
    if (user_out_of_range_page_timerid != 0) {
        sys_timeout_del(user_out_of_range_page_timerid);
        user_out_of_range_page_timerid = 0;
    }
    clr_device_in_page_list();
    if (get_tws_sibling_connect_state()) {
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            if (bt_get_total_connect_dev() > 0) {
                user_tws_sync_call(CMD_SYNC_RECONN_TIMEOUT_MODE);
            } else {
                user_tws_sync_call(CMD_SYNC_ENTER_PAIR_MODE);
            }
        }
    } else {
        if (bt_get_total_connect_dev() > 0) {
            user_1t2_1dev_reconn_timeout_mode();
        } else {
            user_enter_lowpower_mode();
        }
    }
}

void user_out_of_range_page_deal()
{
    bt_log("user_out_of_range_page_deal");
    u32 page_time = TCFG_BT_TIMEOUT_PAGE_TIME * 1000 - 2000;//500
    bt_log("timerid:%d, time_ms:%d", user_out_of_range_page_timerid, page_time);
    user_var.tws_poweron_status = 0;
    if (user_out_of_range_page_timerid == 0) {
        user_out_of_range_page_timerid = sys_timeout_add(NULL, user_out_of_range_page_timeout, page_time);
        dual_conn_page_device();
    }
}

void user_exit_sniff()
{
    bt_log("user_exit_sniff, role:%d", tws_api_get_role());
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        if (tws_api_get_tws_state() & TWS_STA_PHONE_CONNECTED) {
            if (user_var.sniff_status != 0) {
                bt_log("phone exit sniff mode...\n");
                bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
                for (int i = 0; i < 20; i++) {
                    if (user_var.sniff_status == 0) {
                        bt_log("********************************exit sniff ok1\n");
                        break;
                    }
                    os_time_dly(4);
                } 
            }
        } else {
            if (tws_api_is_sniff_state()) {
                bt_log("tws exit sniff mode...\n");
                tws_api_tx_unsniff_req();
                for (int i = 0; i < 20; i++) {
                    if (!tws_in_sniff_state()) {
                        bt_log("********************************exit sniff ok2\n");
                        break;
                    }
                    os_time_dly(4);
                }
            }
        }
    }
}
/*************************************************************/
