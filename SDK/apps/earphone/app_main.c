#include "system/includes.h"
#include "btstack/btstack_task.h"
#include "app_config.h"
#include "app_action.h"
#include "gpadc.h"
#include "app_tone.h"
#include "gpio_config.h"
#include "app_main.h"
#include "asm/charge.h"
#include "update.h"
#include "app_power_manage.h"
#include "audio_config.h"
#include "app_charge.h"
#include "bt_profile_cfg.h"
#include "update_loader_download.h"
#include "idle.h"
#include "bt_tws.h"
#include "key_driver.h"
#include "adkey.h"
#include "user_cfg.h"
#include "usb/device/usb_stack.h"
#include "usb/usb_task.h"
#include "dev_manager.h"
#include "vm.h"


#define LOG_TAG             "[APP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#if 1
#define main_log(x, ...)       g_printf("[USER_MAIN]" x " ", ## __VA_ARGS__)
#define main_log_hexdump       g_printf("[USER_MAIN]main_log_hexdump:");\
                                    put_buf
#else
#define main_log(...)
#define main_log_hexdump(...)
#endif

u8 is_power_up = 0; //记录是否上电开机

#include "btstack/avctp_user.h"
#include "led/led_ui_manager.h"

USER_VAR user_var = {0x00};
extern APP_INFO user_app_info;
extern void bt_set_low_latency_mode(int enable, u8 tone_play_enable, int delay_ms);
void user_poweron_check();

extern int cpu_reset_by_soft();

/*任务列表 */
const struct task_info task_info_table[] = {
    {"app_core",            1,     0,   768,   512 },
    {"systimer",		    7,	   0,   256,   0   },
    {"btctrler",            4,     0,   512,   512 },
    {"btencry",             1,     0,   512,   128 },
#if (BT_FOR_APP_EN)
    {"btstack",             3,     0,   1024,  512 },
#else
    {"btstack",             3,     0,   768,   512 },
#endif
    {"jlstream",            5,     0,   512,   128 },
    {"a2dp_dec",            4,     1,   384,   0 },
    {"tone_dec",            4,     0,   512,   0 },
    {"ring_dec",            4,     0,   512,   0 },
    {"key_tone_dec",        4,     0,   512,   0 },
    {"dev_flow",  	        4,     0,   512,   0 },
    {"aud_effect",          5,     1,   512,   128 },
    //音频硬件模块,任务大部分时间在等硬件中断,所以优先级比jlstream高,可以及时输出数据
    {"syncts",              6,     1,   384,   0 },
    {"EQ0",                 6,     0,   384,   0 },
    {"EQ1",                 6,     0,   384,   0 },
    {"DRC0",                6,     0,   384,   0 },
    {"src0",                5,     1,   384,   0 },
    {"src1",                5,     1,   384,   0 },

    {"aec",					2,	   1,   768,   128 },

    {"aec_dbg",				3,	   0,   128,   128 },
    {"update",				1,	   0,   256,   0   },
    {"tws_ota",				2,	   0,   256,   0   },
    {"tws_ota_msg",			2,	   0,   256,   128 },
    {"dw_update",		 	2,	   0,   256,   128 },
    {"aud_capture",         4,     0,   512,   256 },
    {"data_export",         5,     0,   512,   256 },
    {"anc",                 3,     0,   512,   128 },

#ifdef CONFIG_BOARD_AISPEECH_NR
    {"aispeech_enc",		2,	   1,   512,   128 },
#endif
#if RCSP_MODE
    {"rcsp",		    	4,	   0,   768,   128 },
#if RCSP_FILE_OPT
    {"rcsp_file_bs",		    	1,	   0,   768,   128 },
#endif
#endif
#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
    {"kws",                 2,     0,   256,   64  },
#endif
    {"usb_stack",          	1,     0,   512,   128 },
#if (BT_AI_SEL_PROTOCOL & (GFPS_EN | REALME_EN | TME_EN | DMA_EN | GMA_EN))
    {"app_proto",           2,     0,   768,   64  },
#endif
    {"ui",                  3,     0,   384 - 64,  128  },
#if (TCFG_DEV_MANAGER_ENABLE)
    {"dev_mg",           	3,     0,   512,   512 },
#endif
    {"audio_vad",           1,     1,   512,   128 },
#if TCFG_KEY_TONE_EN
    {"key_tone",            5,     0,   256,   32  },
#endif
#if (BT_AI_SEL_PROTOCOL & TUYA_DEMO_EN)
    {"tuya",                2,     0,   640,   256},
#endif
    {"pmu_task",            6,      0,  256,   128  },
    {"imu_trim",            1,     0,   256,   128 },
    // iis 4输入demo使用的任务
    {"iis_play_task",       3,     0,   512,   0 },
    {"periph_demo",       3,     0,   512,   0 },
    {"CVP_RefTask",	        4,	   0,   256,   128	},
#if AUDIO_ENC_MPT_SELF_ENABLE
    {"enc_mpt_sel",		3,	   0,   512,   128 },
#endif/*AUDIO_ENC_MPT_SELF_ENABLE*/
#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
    {"speak_to_chat",       2,     0,   512,   128 },
    {"icsd_adt",            2,     0,   512,   128 },
    {"icsd_src",            2,     0,   512,   128 },
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
    {0, 0},
};


APP_VAR app_var;

__attribute__((weak))
int eSystemConfirmStopStatus(void)
{
    /* 系统进入在未来时间里，无任务超时唤醒，可根据用户选择系统停止，
     * 或者系统定时唤醒(100ms)，或自己指定唤醒时间
     * return:
     *   1:Endless Sleep
     *   0:100 ms wakeup
     *   other: x ms wakeup
     */
    if (get_charge_full_flag()) {
        power_set_soft_poweroff();
        return 1;
    } else {
        return 0;
    }
}

__attribute__((used))
int *__errno()
{
    static int err;
    return &err;
}

void app_var_init(void)
{
    app_var.play_poweron_tone = 1;
}


u8 get_power_on_status(void)
{
#if TCFG_IOKEY_ENABLE
    extern bool is_iokey_press_down();
    return is_iokey_press_down();
#endif

#if TCFG_ADKEY_ENABLE
    return is_adkey_press_down();
#endif

#if TCFG_LP_TOUCH_KEY_ENABLE
    extern u8 lp_touch_key_power_on_status();
    return lp_touch_key_power_on_status();
#endif

    return 0;
}


void check_power_on_key(void)
{
    y_printf("check_power_on_key");
    u32 delay_10ms_cnt = 0;

    while (1) {
        wdt_clear();
        os_time_dly(1);

        if (get_power_on_status()) {
            putchar('+');
            delay_10ms_cnt++;
            if (delay_10ms_cnt > 70) {
                app_var.poweron_reason = SYS_POWERON_BY_KEY;
                return;
            }
        } else {
            log_info("enter softpoweroff\n");
            delay_10ms_cnt = 0;
            app_var.poweroff_reason = SYS_POWEROFF_BY_KEY;
            power_set_soft_poweroff();
        }
    }
}


__attribute__((weak))
u8 get_charge_online_flag(void)
{
    return 0;
}

/*充电拔出,CPU软件复位, 不检测按键，直接开机*/
static void app_poweron_check(int update)
{
    y_printf("app_poweron_check");
#if (CONFIG_BT_MODE == BT_NORMAL)
    y_printf("update: %d, reset_by_soft: %d", update, cpu_reset_by_soft());
    if (!update && cpu_reset_by_soft()) {
        app_var.play_poweron_tone = 0;
        return;
    }

    //OTA完复位，让开机去回连手机
    if (update) {
        y_printf("update before, poweron now!!!");
        return;
    }

#if TCFG_CHARGE_OFF_POWERON_EN
    if (is_ldo5v_wakeup()) {
        app_var.play_poweron_tone = 0;
        app_var.poweron_reason = SYS_POWERON_BY_OUT_BOX;
        return;
    }
#endif

#if TCFG_AUTO_POWERON_ENABLE
    return;
#endif
    check_power_on_key();

#endif
}



__attribute__((weak))
void board_init()
{

}

static void app_version_check()
{
    extern char __VERSION_BEGIN[];
    extern char __VERSION_END[];

    puts("=================Version===============\n");
    for (char *version = __VERSION_BEGIN; version < __VERSION_END;) {
        version += 4;
        printf("%s\n", version);
        version += strlen(version) + 1;
    }
    puts("=======================================\n");
}

static void app_task_init()
{
    y_printf("app_task_init");
    app_var_init();
    app_version_check();

    do_early_initcall();
    board_init();
    do_platform_initcall();
    cfg_file_parse(0);
    key_driver_init();

#if USER_PRODUCT_SET_TONE_EN
    extern void user_default_tone_language_init();
    user_default_tone_language_init();
#endif

    extern void jlstream_create_hook();
    jlstream_create_hook();
    do_initcall();
    do_module_initcall();
    do_late_initcall();

    dev_manager_init();

    is_power_up = (is_reset_source(P33_VDDIO_POR_RST) || (is_reset_source(P33_VDDIO_LVD_RST)));
    y_printf("is_power_up = %d", is_power_up);
    int update = 0;
    if (CONFIG_UPDATE_ENABLE) {
        update = update_result_deal();
    }
#if TCFG_MC_BIAS_AUTO_ADJUST
    mic_capless_trim_init(update);
#endif

    uart_test_init();

    y_printf("get_charge_online_flag() = %d", get_charge_online_flag());
    y_printf("is_ldo5v_wakeup() = %d", is_ldo5v_wakeup());
    if (get_charge_online_flag()) {
#if(TCFG_SYS_LVD_EN == 1)
        vbat_check_init();
#endif

        app_send_message(APP_MSG_GOTO_MODE, APP_MODE_IDLE | (IDLE_MODE_CHARGE << 8));
    } else {
        check_power_on_voltage();
        app_poweron_check(update);
        //开机先切一下本地保存的eq
        eq_file_cfg_switch("MusicEqBt", user_app_info.eq_mode);
        app_send_message(APP_MSG_POWER_ON, 0);

#if TCFG_ENTER_PC_MODE
        app_send_message(APP_MSG_GOTO_MODE, APP_MODE_PC);
#else
        app_send_message(APP_MSG_GOTO_MODE, APP_MODE_BT);
#endif
    }

#if TCFG_CHARGE_ENABLE
    set_charge_event_flag(1);
#endif
}

static int g_mode_switch_msg[2];
static void app_mode_switch_handler(int *msg);

static void retry_goto_mode(void *_arg)
{
    app_mode_switch_handler(g_mode_switch_msg);
}


int app_get_message(int *msg, int max_num)
{
    while (1) {
        int res = os_taskq_pend(NULL, msg, max_num);
        if (res != OS_TASKQ) {
            continue;
        }
        if (msg[0] & Q_MSG) {
            return 1;
        }
    }
    return 0;
}

static void app_mode_switch_handler(int *msg)
{
    u8 mode = msg[1] & 0xff;
    int arg = msg[1] >> 8;
    int err = 0;

    switch (msg[0]) {
    case APP_MSG_GOTO_MODE:
//需要根据具体开发需求选择是否开启切mode也需要缓存flash
//1.需要注意的缓存到flash需要较长一段时间（1~3s），特别是从蓝牙后台模式切换到蓝牙前台模式，会出现用户明显感知卡顿现象，因此需要具体需求选择性开启。
#if 0
        //如果开启了VM配置项暂存RAM功能则在每次切模式的时候保存数据到vm_flash,避免丢失数据
        if (get_vm_ram_storage_enable()) {
            vm_flush2flash();
        }
#endif //#if 0
//

        err = app_goto_mode(mode, arg);
        break;
    default:
        break;
    }

    if (err == -EBUSY) {
        g_mode_switch_msg[0] = msg[0];
        g_mode_switch_msg[1] = msg[1];
        sys_timeout_add(NULL, retry_goto_mode, 100);
    }
}

static void app_task_loop(void *p)
{
    int msg[16];
    struct app_mode *mode;
    const struct app_msg_handler *handler;

    app_task_init();

    while (1) {
        app_get_message(msg, ARRAY_SIZE(msg));

        if (msg[0] == MSG_FROM_APP) {
            app_mode_switch_handler(msg + 1);
        }

        //消息截获,返回1表示中断消息分发
        int abandon = 0;
        for_each_app_msg_prob_handler(handler) {
            if (handler->from == msg[0]) {
                abandon = handler->handler(msg + 1);
                if (abandon) {
                    break;
                }
            }
        }
        if (abandon) {
            continue;
        }

        //当前模式消息处理
        mode = app_get_current_mode();
        if (mode) {
            mode->ops->msg_handler(msg);
        }

        //消息继续分发
        for_each_app_msg_handler(handler) {
            if (handler->from != msg[0]) {
                continue;
            }
            if (mode && mode->name == handler->owner) {
                continue;
            }
            handler->handler(msg + 1);
        }
    }

}
void user_timer(void){
    clock_dump();
}

void app_main()
{
    task_create(app_task_loop, NULL, "app_core");
   // sys_timer_add(NULL,clock_dump,1000);
    os_start(); //no return
    while (1) {
        asm("idle");
    }
}


/*************************************************************/
//   自定义主从同步的，写在这
/*************************************************************/
#define USER_UUID_TWS_SYNC_CALL         0xABCABC03
#define USER_UUID_SEND_TO_BOTH          0xABCABC06
#define USER_UUID_SEND_TO_SIBLING       0xABCABC07

extern void user_save_ver_info(u8 *data, u16 len);
extern void user_recv_phone_info_from_sibling(u8 *data, u16 len);
extern void user_find_ear_deal(u8 en, u8 lr);
extern void user_app_data_notify(u8 cmd);

u16 user_box_enter_pair_mode_cmd_timerid = 0;
void user_box_enter_pair_mode_cmd_timeout()
{
    main_log("user_box_enter_pair_mode_cmd_timeout");
    // main_log("user_box_enter_pair_mode_cmd_timerid = %d", user_box_enter_pair_mode_cmd_timerid);
    if (user_box_enter_pair_mode_cmd_timerid != 0) {
        sys_timeout_del(user_box_enter_pair_mode_cmd_timerid);
        user_box_enter_pair_mode_cmd_timerid = 0;
    }
}

void user_eq_switch()
{
    printf("task:%s", os_current_task());
    printf("irq:%d", cpu_in_irq());
    eq_file_cfg_switch("MusicEqBt", user_app_info.eq_mode);
}

static void user_set_eq_tone_cb(int fname_uuid, enum stream_event event)
{
    main_log("user_set_eq_tone_cb, event:%d", event);
    int argv[2];
    if (event == STREAM_EVENT_START) {
        argv[0] = (int)user_eq_switch;
        argv[1] = 0;
        os_taskq_post_type("app_core", Q_CALLBACK, 2, argv);
    }
}

REGISTER_TWS_TONE_CALLBACK(user_set_eq_entry) = {
    .func_uuid = 0xABCABCFA,
    .callback = user_set_eq_tone_cb,
};

/*************************************************************/
//   需要双方同步执行（同时间）的，写在这
/*************************************************************/
static void user_tws_sync_callback(int priv, int err)
{
    // main_log("user_tws_sync_callback");
    int argv[2];
    switch (priv) {
    case CMD_SYNC_SET_EQ_MODE:
        main_log("tws_sync_call set eq mode!!!");
        // eq_file_cfg_switch("MusicEqBt", user_app_info.eq_mode);
        argv[0] = (int)user_eq_switch;
        argv[1] = 0;
        os_taskq_post_type("app_core", Q_CALLBACK, 2, argv);
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            tws_play_tone_file_alone(get_tone_files()->eq_sw, 400);
            // tws_play_tone_file_alone_callback(get_tone_files()->eq_sw, 400, 0xABCABCFA);
        }
        break;
    case CMD_SYNC_ENTER_PAIR_MODE:
        main_log("tws_sync_call enter pair mode!!!");
        // user_var.tws_poweron_status = 0;
        user_enter_pair_mode();
        break;
    case CMD_SYNC_RECONN_TIMEOUT_MODE:
        main_log("tws_sync_call reconn timeout mode!!!");
        user_1t2_1dev_reconn_timeout_mode();
        break;
    case CMD_BOX_ENTER_PAIR_MODE:
        main_log("tws_sync_call box enter pair mode!!!");
        main_log("user_box_enter_pair_mode_cmd_timerid = %d", user_box_enter_pair_mode_cmd_timerid);
        if (user_box_enter_pair_mode_cmd_timerid != 0) {
            return;
        }
        //1.5秒内重复收到，不重复进配对
        user_box_enter_pair_mode_cmd_timerid = sys_timeout_add(NULL, user_box_enter_pair_mode_cmd_timeout, 1500);
        user_var.box_enter_pair_mode = 1;
        user_dual_conn_disconn_device();
        break;
    case CMD_SYNC_ENTER_LOWPOWER_MODE:
        main_log("tws_sync_call enter lowpower mode!!!");
        user_enter_lowpower_mode();
        break;
    case CMD_SYNC_EXIT_RECONN:
        main_log("tws_sync_call exit reconn!!!");
        extern u16 user_out_of_range_page_timerid;
        if (user_out_of_range_page_timerid != 0) {
            sys_timeout_del(user_out_of_range_page_timerid);
            user_out_of_range_page_timerid = 0;
        }
        break;
    case CMD_SYNC_CPU_RESET:
        main_log("tws_sync_call CPU RESET!!!");
        user_var.restore_flag = 0;
        app_var.goto_poweroff_flag = 0;
        cpu_reset();
        break;
    default:
        break;
    }
}

TWS_SYNC_CALL_REGISTER(user_tws_sync_call) = {
    .uuid = USER_UUID_TWS_SYNC_CALL,
    .task_name = "app_core",
    .func = user_tws_sync_callback,
};

void user_tws_sync_call(int priv)
{
    // main_log("user_tws_sync_call");
    if (get_bt_tws_connect_status()) {
        // main_log("tws is connected!!!");
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            tws_api_sync_call_by_uuid(USER_UUID_TWS_SYNC_CALL, priv, 400);
        }
    } else {
        user_tws_sync_callback(priv, 0);
    }
}
/*************************************************************/

/*************************************************************/
//   需要发给双方都执行的，写在这
/*************************************************************/
static void user_recv_both_handle(u8 *data, int argc, int *argv)
{
    main_log_hexdump(data, argv[0]);
    main_log("this channel is %c", tws_api_get_local_channel());
    switch (data[0]) {
    case CMD_SYNC_SET_EQ_MODE:
        main_log("CMD_SYNC_SET_EQ_MODE");
        user_app_info.eq_mode = data[1];
        main_log("user_eq_mode = %d", user_app_info.eq_mode);
        if (data[2] == 1) {
            user_tws_sync_call(CMD_SYNC_SET_EQ_MODE);
            // if (tws_api_get_role() == TWS_ROLE_MASTER) {
            //     user_tws_sync_call(CMD_SYNC_SET_EQ_MODE);
            //     // tws_play_tone_file_alone(get_tone_files()->eq_sw, 400);
            //     // tws_play_tone_file_alone_callback(get_tone_files()->eq_sw, 400, 0xABCABCFA);
            // }
        } else {
            user_eq_switch();
        }
        // user_tws_sync_call(CMD_SYNC_SET_EQ_MODE);
        break;
    case CMD_SYNC_SET_LANGUAGE:
        main_log("CMD_SYNC_SET_LANGUAGE");
        user_app_info.tone_language = data[1];
        tone_language_set(user_app_info.tone_language);
        break;
    case CMD_SYNC_SEARCH_EAR:
        main_log("CMD_SYNC_SEARCH_EAR");
        user_find_ear_deal(data[1], data[2]);
        break;
    case CMD_SYNC_RESTORE:
        main_log("CMD_SYNC_RESTORE");
        user_sys_restore(data[1]);
        break;
    case CMD_SYNC_STOP_FIND_EAR:
        main_log("CMD_SYNC_STOP_FIND_EAR");
        user_both_stop_find_ear();
        break;
    default:
        break;
    }
    free(data);
}

static void user_recv_both_deal(void *_data, u16 len, bool rx)
{
    u8 *data = malloc(len);
    memcpy(data, _data, len);
    int msg[5] = { (int)user_recv_both_handle, 3, (int)data, len, rx};
    os_taskq_post_type("app_core", Q_CALLBACK, 5, msg);
}

REGISTER_TWS_FUNC_STUB(user_send_to_both_stub) = {
    .func_id = USER_UUID_SEND_TO_BOTH,
    .func    = user_recv_both_deal,
};

void user_send_data_to_both(u8 *data, u16 len)
{
    if (get_bt_tws_connect_status()) {
        tws_api_send_data_to_sibling(data, len, USER_UUID_SEND_TO_BOTH);
    } else {
        u8 *data_temp = malloc(len);
        memcpy(data_temp, data, len);
        int msg[5] = { (int)user_recv_both_handle, 3, (int)data_temp, len, 0};
        os_taskq_post_type("app_core", Q_CALLBACK, 5, msg);
    }
}
/*************************************************************/

/*************************************************************/
//   只发给对方执行，自己不用执行的，写在这
/*************************************************************/
static void user_recv_sibling_handle(u8 *data, int argc, int *argv)
{
    // main_log("user_recv_sibling_handle, len = %d, rx = %d", argv[0], argv[1]);
    main_log_hexdump(data, argv[0]);
    switch (data[0]) {
    case CMD_SYNC_VER_INFO:
        main_log("CMD_SYNC_VER_INFO");
        user_save_ver_info(data, argv[0]);
        break;
	case CMD_SYNC_PHONE_INFO:
		main_log("CMD_SYNC_PHONE_INFO");
        user_recv_phone_info_from_sibling(data, argv[0]);
        break;
	case CMD_SYNC_SET_APP_KEY:
		main_log("CMD_SYNC_SET_APP_KEY");
        memcpy(user_app_info.app_key_lr, data+1, 8);
        put_buf(user_app_info.app_key_lr, 8);
        set_custom_key(user_app_info.app_key_lr, sizeof(user_app_info.app_key_lr));
        break;
	case CMD_SYNC_SET_1T2_MODE:
		main_log("CMD_SYNC_SET_1T2_MODE");
        user_app_info.u1t2_mode = data[1];
        break;
    case CMD_SYNC_SET_GAME_MODE:
        main_log("CMD_SYNC_SET_GAME_MODE");
        user_app_info.game_mode = data[1];
        break;
    case CMD_SYNC_FIND_EAR_STATUS:
        main_log("CMD_SYNC_FIND_EAR_STATUS");
        u8 channel = tws_api_get_local_channel();
        user_var.find_ear_status_tws = data[1];
        if(channel == 'L'){
            user_var.find_ear_status_r = user_var.find_ear_status_tws;
        }else{
            user_var.find_ear_status_l = user_var.find_ear_status_tws;
        }
        main_log("l:%d r:%d tws:%d\n", user_var.find_ear_status_l, user_var.find_ear_status_r, user_var.find_ear_status_tws);
        user_app_data_notify(CMD_NOTIFY_SEARCH_EAR);
        break;
    case CMD_SYNC_SPP_ADDR:
        main_log("CMD_SYNC_SPP_ADDR");
        user_spp_addr_sync_rx(data, argv[0]);
        break;
    default:
        break;
    }
    free(data);
}

static void user_recv_sibling_deal(void *_data, u16 len, bool rx)
{
    // main_log("user_recv_sibling_deal, len = %d, rx = %d", len, rx);
    if (!rx) {
        main_log("there is local data, return!!!");
        return;
    }
    u8 *data = malloc(len);
    memcpy(data, _data, len);
    int msg[5] = { (int)user_recv_sibling_handle, 3, (int)data, len, rx};
    os_taskq_post_type("app_core", Q_CALLBACK, 5, msg);
}

REGISTER_TWS_FUNC_STUB(user_send_to_sibling_stub) = {
    .func_id = USER_UUID_SEND_TO_SIBLING,
    .func    = user_recv_sibling_deal,
};

void user_send_data_to_sibling(u8 *data, u16 len)
{
    // main_log("user_send_data_to_sibling");
    if (get_bt_tws_connect_status()) {
        tws_api_send_data_to_sibling(data, len, USER_UUID_SEND_TO_SIBLING);
    } else {
        main_log("tws is not connected, not send!!!");
    }
}
/*************************************************************/

static u16 user_poweron_detect_cnt = 30;
void user_poweron_check()
{
    main_log("user_poweron_check!!!");
    main_log("is_ldo5v_wakeup() = %d", is_ldo5v_wakeup());
    u16 user_while_cnt = 0;
    u16 user_ldo5v_on_cnt = 0;
    u8 user_poweron_status = 0;
    if (is_ldo5v_wakeup()) {
        while (user_while_cnt < user_poweron_detect_cnt) {
            wdt_clear();
            os_time_dly(1);
            if (LVCMP_DET_GET()) {	//ldoin > vbat
                putchar('X');
                user_ldo5v_on_cnt++;
            }
            user_while_cnt++;
        }
        if (user_ldo5v_on_cnt > user_poweron_detect_cnt - 2) {
            main_log("ldo5v_wakeup!!!");
            return;
        } else {
            main_log("have not 5V, poweroff now!!!");
            charge_check_and_set_pinr(1);
            power_set_soft_poweroff();
        }
    }
    return;
}

/*************************************************************/
// extern const int CONFIG_EXTWS_NACK_LIMIT_INT_CNT;
// u8 get_extws_nack_adjust(u8 per_v, int a2dp_dly_paly_time, int msec)
// {
//     u8 extws_nack_limit = 0;
//     u8 adjust_extws_en = 0;
//     if (CONFIG_EXTWS_NACK_LIMIT_INT_CNT < 2) {
//         return 0;
//     }
// #if 1//check dly_time
//     /* printf("[%d,%d,%d]",per_v,a2dp_dly_paly_time,msec ); */
//     if ((msec >= a2dp_dly_paly_time / 3) && (msec < a2dp_dly_paly_time / 2)) {
//         /*putchar('a');*/
//         extws_nack_limit = CONFIG_EXTWS_NACK_LIMIT_INT_CNT - 1;
//         adjust_extws_en = 1;
//     } else if ((msec >= a2dp_dly_paly_time / 5) && (msec < a2dp_dly_paly_time / 3)) {
//         /*putchar('b');*/
//         extws_nack_limit = CONFIG_EXTWS_NACK_LIMIT_INT_CNT - 2;
//         adjust_extws_en = 1;

//     } else if (msec < a2dp_dly_paly_time / 5) {
//         /*putchar('c');*/
//         extws_nack_limit = 0;
//         adjust_extws_en = 1;
//     }
// #endif
//     if (adjust_extws_en && ((per_v < 40) || (per_v == 0xff))) {
//         if (extws_nack_limit < (CONFIG_EXTWS_NACK_LIMIT_INT_CNT - 1)) {
//             extws_nack_limit = CONFIG_EXTWS_NACK_LIMIT_INT_CNT - 1;
//             /*putchar('L');*/
//         }
//     }
//     if (adjust_extws_en) {

//         extws_nack_limit |= BIT(7);
//     }
//     return 0;
// }

// char user_rssi = 0;
// u8 user_cnt = 0;
u8 check_tws_rssi_extws_nack_adjust(char tws_rssi)
{
    // printf("tws_rssi = %d\n", tws_rssi);
    // user_rssi = tws_rssi;
    // user_cnt++;
    // if (user_cnt == 10) {
    //     user_app_data_notify(CMD_NOTIFY_TWS_RSSI);
    //     user_cnt = 0;
    // }
    if (tws_rssi < -70) {
        return 1;
    }
    return 0;
}