#include "btstack/avctp_user.h"
#include "btstack/bluetooth.h"
#include "classic/tws_api.h"
#include "app_main.h"
#include "earphone.h"
#include "app_config.h"
#include "bt_tws.h"
#include "led/led_ui_manager.h"
#include "app_tone.h"
#include "ble_rcsp_server.h"

/********edr蓝牙相关状态debug打印说明***********************
  *** tws快连page:             打印 'c'(小写)
  *** tws快连page_scan:        打印 'p'(小写)
  *** 等待被发现inquiry_scan   打印 'I'(大写)
  *** 回连手机page:            打印 'C'(大写)
  *** 等待手机连接page_scan:   打印 'P'(大写)
 *********************************************************/

#if TCFG_USER_TWS_ENABLE
#define MAX_PAGE_DEVICE_NUM 2

#define TIMEOUT_CONN_DEVICE_OPEN_PAGE  0//1 //第二台设备超时断开回连一直开启page

#define TCFG_TWS_CONN_DISABLE    0//不配对成tws,作为单耳使用,关闭tws相关配对接口


#if 1
#define tws_dual_log(x, ...)       g_printf("[USER_TWS_DUAL]" x " ", ## __VA_ARGS__)
#define tws_dual_log_hexdump       g_printf("[USER_TWS_DUAL]tws_dual_log_hexdump:");\
                                    put_buf
#else
#define tws_dual_log(...)
#define tws_dual_log_hexdump(...)
#endif

u8 user_1t1_mode_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
u8 addr_all_ff[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
BT_DEV_ADDR user_bt_dev_addr = {0x00};
extern APP_INFO user_app_info;
extern USER_VAR user_var;
extern struct PHONE_INFO user_phone_info1;
extern struct PHONE_INFO user_phone_info2;
extern void user_phone_info_del(u8 *addr);
extern void set_unconn_tws_phone_enter_idle(u8 en);

extern void user_send_data_to_both(u8 *data, u16 len);
void user_enter_pair_mode();
void user_poweron_page_deal();
void user_enter_lowpower_mode(void);

u16 user_reconn_page_device_timer = 0;
u16 user_reconn_tws_conn_timer = 0;
u16 user_lowpower_scan_conn_timer = 0;
u16 user_key_reconn_timeid = 0;

struct page_device_info {
    struct list_head entry;
    u32 timeout;
    u16 timer;
    u8 mac_addr[6];
};

struct dual_conn_handle {
    u16 timer;
    u16 inquiry_scan_time;
    u16 page_scan_timer;
    u8 device_num_recorded;
    u8 remote_addr[3][6];
    u8 page_head_inited;
    u8 page_scan_auto_disable;
    u8 inquiry_scan_disable;
    u8 need_keep_scan;
    struct list_head page_head;
};

static struct dual_conn_handle g_dual_conn;

extern const char *bt_get_local_name();
extern u16 bt_get_tws_device_indicate(u8 *tws_device_indicate);
void dual_conn_page_device();
extern void user_tws_sync_call(int priv);

void bt_set_need_keep_scan(u8 en)
{
    g_dual_conn.need_keep_scan = en;
}

bool page_list_empty()
{
    return list_empty(&g_dual_conn.page_head);
}

static void auto_close_page_scan(void *p)
{
    puts("auto_close_page_scan\n");
    g_dual_conn.page_scan_timer = 0;
    g_dual_conn.page_scan_auto_disable = 1;
    lmp_hci_write_scan_enable((0 << 1) | 0);

    u8 data[2] = {0, 0};
    data[0] = g_dual_conn.page_scan_auto_disable |
              (g_dual_conn.inquiry_scan_disable << 1) |
              (g_dual_conn.device_num_recorded << 2);
    data[1] = g_dual_conn.device_num_recorded << 1;
    tws_api_send_data_to_slave(data, 2, 0xF730EBC7);
}

void write_scan_conn_enable(bool scan_enable, bool conn_enable)
{
    if (g_dual_conn.page_scan_auto_disable) {
        if (!scan_enable && conn_enable) {
            return;
        }
    }
    r_printf("write_scan_conn_enable=%d,%d\n", scan_enable, conn_enable);

    lmp_hci_write_scan_enable((conn_enable << 1) | scan_enable);

    if ((scan_enable || conn_enable) && page_list_empty()) {
        int connect_device = bt_get_total_connect_dev();
        app_send_message(APP_MSG_BT_IN_PAIRING_MODE, connect_device);
    }

#if TCFG_DUAL_CONN_PAGE_SCAN_TIME
    if (tws_api_get_role() == TWS_ROLE_SLAVE || (conn_enable && !scan_enable)) {
        if (g_dual_conn.page_scan_timer) {
            sys_timer_modify(g_dual_conn.page_scan_timer,
                             TCFG_DUAL_CONN_PAGE_SCAN_TIME * 1000);
        } else {
            g_dual_conn.page_scan_timer = sys_timeout_add(NULL, auto_close_page_scan,
                                          TCFG_DUAL_CONN_PAGE_SCAN_TIME * 1000);
        }
    } else {
        if (g_dual_conn.page_scan_timer) {
            sys_timeout_del(g_dual_conn.page_scan_timer);
            g_dual_conn.page_scan_timer = 0;
        }
    }
#endif
}

static void close_inquiry_scan(void *p)
{
    puts("check close_inquiry_scan\n");
    g_dual_conn.inquiry_scan_time = 0;
    g_dual_conn.inquiry_scan_disable = 1;
    if (g_dual_conn.device_num_recorded == 1 && bt_get_total_connect_dev() == 1) {
        write_scan_conn_enable(0, 0);
#if RCSP_MODE && TCFG_RCSP_DUAL_CONN_ENABLE
        rcsp_close_inquiry_scan(true);
#endif
    }
    u8 data[2] = {0, 0};
    data[0] = g_dual_conn.page_scan_auto_disable |
              (g_dual_conn.inquiry_scan_disable << 1) |
              (g_dual_conn.device_num_recorded << 2);
    data[1] = g_dual_conn.device_num_recorded << 1;
    tws_api_send_data_to_slave(data, 2, 0xF730EBC7);
}

// static int dual_conn_try_open_inquiry_scan()
// {
//     tws_dual_log("dual_conn_try_open_inquiry_scan");
// #if TCFG_DUAL_CONN_INQUIRY_SCAN_TIME
//     if (g_dual_conn.inquiry_scan_disable) {
//         return 0;
//     }
//     if (tws_api_get_role() == TWS_ROLE_MASTER) {
//         write_scan_conn_enable(1, 1);
//         return 1;
//     }
// #endif
//     return 0;
// }

static int dual_conn_try_open_inquiry_scan()
{
    tws_dual_log("dual_conn_try_open_inquiry_scan:%d, %d", tws_api_get_role(), g_dual_conn.inquiry_scan_disable);
#if TCFG_DUAL_CONN_INQUIRY_SCAN_TIME
    if (g_dual_conn.inquiry_scan_disable) {
        return 0;
    }
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        write_scan_conn_enable(1, 1);
        return 1;
    }
#endif
    return 0;
}

static int dual_conn_try_open_inquiry_page_scan()
{
    tws_dual_log("dual_conn_try_open_inquiry_page_scan:%d, %d", tws_api_get_role(), bt_get_total_connect_dev());
#if TCFG_DUAL_CONN_INQUIRY_SCAN_TIME
    dual_conn_try_open_inquiry_scan();
#else
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return 0;
    }
    write_scan_conn_enable(0, 1);
#endif
    return 1;
}

int add_device_2_page_list(u8 *mac_addr, u32 timeout)
{
    struct page_device_info *info;

    printf("add_device_2_page_list: %d\n", timeout);
    put_buf(mac_addr, 6);

    if (!g_dual_conn.page_head_inited) {
        return 0;
    }

    list_for_each_entry(info, &g_dual_conn.page_head, entry) {
        if (memcmp(info->mac_addr, mac_addr, 6) == 0) {
            if (info->timer) {
                sys_timeout_del(info->timer);
                info->timer = 0;
            }
            info->timeout = jiffies + msecs_to_jiffies(timeout);

            __list_del_entry(&info->entry);
            list_add_tail(&info->entry, &g_dual_conn.page_head);
            return 1;
        }
    }

    info = malloc(sizeof(*info));
    info->timer = 0;
    info->timeout = jiffies + msecs_to_jiffies(timeout);
    memcpy(info->mac_addr, mac_addr, 6);
    list_add_tail(&info->entry, &g_dual_conn.page_head);
    return 0;
}

static void del_device_from_page_list(u8 *mac_addr)
{
    // tws_dual_log("del_device_from_page_list");
    struct page_device_info *info;

    if (!g_dual_conn.page_head_inited) {
        return;
    }

    list_for_each_entry(info, &g_dual_conn.page_head, entry) {
        if (memcmp(info->mac_addr, mac_addr, 6) == 0) {
            puts("del_device\n");
            put_buf(mac_addr, 6);
            __list_del_entry(&info->entry);
            if (info->timer) {
                sys_timeout_del(info->timer);
            }
            free(info);
            return;
        }
    }

}

void clr_device_in_page_list()
{
    // tws_dual_log("clr_device_in_page_list");
    struct page_device_info *info, *n;

    if (!g_dual_conn.page_head_inited) {
        return;
    }

    list_for_each_entry_safe(info, n, &g_dual_conn.page_head, entry) {
        __list_del_entry(&info->entry);
        if (info->timer) {
            sys_timeout_del(info->timer);
        }
        free(info);
    }
}

static u8 *get_device_addr_in_page_list()
{
    struct page_device_info *info, *n;
    list_for_each_entry_safe(info, n, &g_dual_conn.page_head, entry) {
        return info->mac_addr;
    }
    return NULL;
}


static void tws_wait_pair_timeout(void *p)
{
    g_dual_conn.timer = 0;
    tws_api_cancle_wait_pair();
    dual_conn_page_device();
}

static void tws_auto_pair_timeout(void *p)
{
    tws_dual_log("tws_auto_pair_timeout");
    g_dual_conn.timer = 0;
    tws_api_cancle_wait_pair();
    tws_api_cancle_create_connection();
    write_scan_conn_enable(0, 0);
    dual_conn_page_device();
}

static void tws_pair_new_tws(void *p)
{
    tws_api_cancle_create_connection();
    tws_api_auto_pair(0);
    g_dual_conn.timer = sys_timeout_add(NULL, tws_auto_pair_timeout, 3000);
}

static void tws_wait_conn_timeout(void *p)
{
    tws_dual_log("tws_wait_conn_timeout");
    write_scan_conn_enable(0, 0);
    dual_conn_page_device();
}


void tws_dual_conn_state_handler()
{
    int state               = tws_api_get_tws_state();
    int connect_device      = bt_get_total_connect_dev();
    int have_page_device    = page_list_empty() ? false : true;

    tws_dual_log("page_state: %d, %x, %d, %d, %d, %d, %d\n", connect_device, state, have_page_device, user_var.tws_poweron_status, user_var.out_of_range, g_dual_conn.device_num_recorded, user_var.enter_pairing_mode);

    if (g_dual_conn.timer) {
        sys_timeout_del(g_dual_conn.timer);
        g_dual_conn.timer = 0;
    }
    if (app_var.goto_poweroff_flag) {
        return;
    }

    if (state & TWS_STA_SIBLING_CONNECTED) {
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            write_scan_conn_enable(0, 0);
            return;
        }
        if (connect_device == 0) {
            if (user_var.out_of_range) {
                write_scan_conn_enable(0, 1);
            } else {
                write_scan_conn_enable(1, 1);
            }
        } else if (connect_device == 1) {
            if (g_dual_conn.device_num_recorded > 1) {
#if TIMEOUT_CONN_DEVICE_OPEN_PAGE
                if (have_page_device) {
                    r_printf("page_and_page_sacn");
                    if (get_device_addr_in_page_list()) {
                        bt_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6,
                                       get_device_addr_in_page_list());
                    }
                }
#endif
                write_scan_conn_enable(0, 1);
            }
        }
        if (user_var.enter_pairing_mode) {
            if (connect_device < 2) {
                write_scan_conn_enable(1, 1);
            }  
        }

        if (have_page_device) {
            //开机回连，直接再page手机
            if (user_var.tws_poweron_status) {
                write_scan_conn_enable(0, 0);
                dual_conn_page_device();
            } else {
                g_dual_conn.timer = sys_timeout_add(NULL, tws_wait_conn_timeout, 5000);
            }
            // g_dual_conn.timer = sys_timeout_add(NULL, tws_wait_conn_timeout, 6000);
            tws_api_auto_role_switch_disable();
        } else {
            if (g_dual_conn.device_num_recorded == 1) {
                if (connect_device == 1) {
                    if (user_var.enter_pairing_mode) {
                        write_scan_conn_enable(1, 1);
                    } else {
                        dual_conn_try_open_inquiry_page_scan();
                    }
                } else if (connect_device == 0) {
                    write_scan_conn_enable(1, 1);
                }
                // dual_conn_try_open_inquiry_page_scan();
            } else if (connect_device == 2) {
#if TCFG_PREEMPT_CONNECTION_ENABLE
                write_scan_conn_enable(0, 1);
#endif
            }
            if (user_var.role_switch_disable == 0) {
                tws_api_auto_role_switch_enable();
            }
            // tws_api_auto_role_switch_enable();
        }
    } else if (state & TWS_STA_TWS_PAIRED) {
        if (connect_device == 0) {
            tws_api_create_connection(0);
            if (user_var.out_of_range) {
                write_scan_conn_enable(0, 1);
            } else {
                write_scan_conn_enable(1, 1);
            }
#if CONFIG_TWS_AUTO_PAIR_WITHOUT_UNPAIR
            g_dual_conn.timer = sys_timeout_add(NULL, tws_pair_new_tws,
                                                TCFG_TWS_CONN_TIMEOUT * 1000);
            return;
#endif
        } else if (connect_device == 1) {
            if (bt_get_remote_test_flag()) {
                return;
            }
            tws_api_wait_connection(0);
            if (g_dual_conn.device_num_recorded > 1) {
                write_scan_conn_enable(0, 1);
            } else {
                dual_conn_try_open_inquiry_page_scan();
            }
        } else {
            tws_api_wait_connection(0);
#if TCFG_PREEMPT_CONNECTION_ENABLE
            write_scan_conn_enable(0, 1);
#endif
        }
        if (user_var.enter_pairing_mode) {
            if (connect_device < 2) {
                write_scan_conn_enable(1, 1);
            }  
        }
        if (have_page_device) {
            //开机回连，直接再page手机
            if (user_var.tws_poweron_status) {
                write_scan_conn_enable(0, 0);
                dual_conn_page_device();
            } else {
                g_dual_conn.timer = sys_timeout_add(NULL, tws_auto_pair_timeout,
                                                    TCFG_TWS_CONN_TIMEOUT * 1000);
            }
        }
    } else {
        /* TWS未配对 */
        if (connect_device == 2) {
            return;
        }
        if (connect_device == 1 && bt_get_remote_test_flag()) {
            return;
        }
#if TCFG_TWS_CONN_DISABLE
        write_scan_conn_enable(1, 1);
        if (have_page_device) {
            g_dual_conn.timer = sys_timeout_add(NULL, tws_auto_pair_timeout,
                                                TCFG_TWS_CONN_TIMEOUT * 1000);
        }
        return;

#endif
#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_CLICK
        // const char *bt_name;
        // bt_name = connect_device ? NULL : bt_get_local_name();      
        // tws_api_wait_pair_by_code(0, bt_name, 0);
        tws_api_wait_connection(0);
        if (connect_device == 0) {
            if (user_var.out_of_range) {
                write_scan_conn_enable(0, 1);
            } else {
                write_scan_conn_enable(1, 1);
            }
        } else if (g_dual_conn.device_num_recorded > 1) {
            /*write_scan_conn_enable(0, 1);*/
        }
        if (user_var.enter_pairing_mode) {
            if (connect_device < 2) {
                write_scan_conn_enable(1, 1);
            }  
        }
        if (have_page_device) {
            g_dual_conn.timer = sys_timeout_add(NULL, tws_wait_pair_timeout,
                                                TCFG_TWS_PAIR_TIMEOUT * 1000);
        }
#elif CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_AUTO
        if (connect_device == 0) {
            tws_api_auto_pair(0);
            write_scan_conn_enable(1, 1);
        } else if (connect_device == 1) {
            if (g_dual_conn.device_num_recorded > 1) {
                write_scan_conn_enable(0, 1);
            }
        }
        if (have_page_device) {
            // tws_dual_log("2333 - have_page_device, user_var.tws_poweron_status = %d", user_var.tws_poweron_status);
            //开机回连，直接再page手机
            if (user_var.tws_poweron_status) {
                write_scan_conn_enable(0, 0);
                dual_conn_page_device();
            } else {
                g_dual_conn.timer = sys_timeout_add(NULL, tws_auto_pair_timeout,
                                                    TCFG_TWS_PAIR_TIMEOUT * 1000);
            }
            // g_dual_conn.timer = sys_timeout_add(NULL, tws_auto_pair_timeout,
            //                                     TCFG_TWS_PAIR_TIMEOUT * 1000);
        }
#else
        if (connect_device == 0) {
            write_scan_conn_enable(1, 1);
        } else if (connect_device == 1) {
            if (g_dual_conn.device_num_recorded > 1) {
                write_scan_conn_enable(0, 1);
            }
        }
#endif
    }

}

static void dual_conn_page_device_timeout(void *p)
{
    tws_dual_log("dual_conn_page_device_timeout");
    struct page_device_info *info;

    if (!g_dual_conn.page_head_inited) {
        return;
    }

    /* 参数有效性检查 */
    list_for_each_entry(info, &g_dual_conn.page_head, entry) {
        if (info == p) {
            tws_dual_log("page_device_timeout: %lu, %d\n", jiffies, info->timeout);
            info->timer = 0;
            list_del(&info->entry);
            if (time_after(jiffies, info->timeout)) {
                free(info);
                tws_dual_log("one dev dual_conn_page_device_timeout end!!!");
                tws_dual_log("page_list_empty:%d, total_connect:%d, tws_status:%d, out_of_range:%d", page_list_empty(), bt_get_total_connect_dev(), get_tws_sibling_connect_state(), user_var.out_of_range);
                if (page_list_empty()) {
                    tws_dual_log("all dev page timeout end!!!");
                    user_var.tws_poweron_status = 0;
                    //page结束时如果已经连接上至少一台设备，就不要进入配对模式了
                    if (user_var.out_of_range && !get_tws_sibling_connect_state() && (bt_get_total_connect_dev() == 0)) {
                        //单耳超距超时进入休眠模式
                        user_enter_lowpower_mode();
                    } else {
                        if (bt_get_total_connect_dev() == 0) {
                            user_tws_sync_call(CMD_SYNC_ENTER_PAIR_MODE);
                            break;
                        }
                    }
                }
            } else {
                list_add_tail(&info->entry, &g_dual_conn.page_head);
            }
            bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
            tws_dual_conn_state_handler();
            break;
        }
    }
}

void dual_conn_page_device()
{
    tws_dual_log("dual_conn_page_device");
    struct page_device_info *info, *n;

    if (!g_dual_conn.page_head_inited) {
        return;
    }
    tws_dual_log("page_list_empty:%d, tws_poweron_status:%d", page_list_empty(), user_var.tws_poweron_status);
    list_for_each_entry_safe(info, n, &g_dual_conn.page_head, entry) {
        if (info->timer) {
            return;
        }
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            break;
        }
        tws_dual_log("start_page_device: %lu, %d, %d\n", jiffies, info->timeout);
        put_buf(info->mac_addr, 6);

        int page_timeout = TCFG_BT_PAGE_TIMEOUT * 1000 + 100;    //预防卡临界
        if (user_var.tws_poweron_status == 1) {
            int num = btstack_get_num_of_remote_device_recorded();
            if (num >= 2) {
                page_timeout = page_timeout / 2;
            }
        }

        info->timer = sys_timeout_add(info, dual_conn_page_device_timeout,
                            page_timeout);
        bt_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, info->mac_addr);
        return;
    }
    tws_dual_conn_state_handler();
}




void dual_conn_page_devices_init()
{
    // tws_dual_log("dual_conn_page_devices_init");
    u8 mac_addr[6];

    INIT_LIST_HEAD(&g_dual_conn.page_head);
    g_dual_conn.page_head_inited = 1;
    g_dual_conn.page_scan_auto_disable = 0;

    int num = btstack_get_num_of_remote_device_recorded();
    tws_dual_log("recorded_num:%d, 1t2_mode:%d", num, user_app_info.u1t2_mode);
    for (int i = num - 1; i >= 0 && i + 2 >= num ; i--) {
        if (user_app_info.u1t2_mode == 0) {
            tws_dual_log("1T1 mode, break!!!");
            tws_dual_log_hexdump(user_1t1_mode_addr, 6);
            if (memcmp(user_1t1_mode_addr, addr_all_ff, 6)) {
                add_device_2_page_list(user_1t1_mode_addr, TCFG_BT_POWERON_PAGE_TIME * 1000);
            }
            // add_device_2_page_list(user_1t1_mode_addr, TCFG_BT_POWERON_PAGE_TIME * 1000);
            break;
        }
        btstack_get_remote_addr(mac_addr, i);
        // tws_dual_log_hexdump(mac_addr, 6);
        // add_device_2_page_list(mac_addr, TCFG_BT_POWERON_PAGE_TIME * 1000);
        if (num >= 2) {
            add_device_2_page_list(mac_addr, TCFG_BT_POWERON_PAGE_TIME * 1000 / 2);
        } else {
            add_device_2_page_list(mac_addr, TCFG_BT_POWERON_PAGE_TIME * 1000);
        }
    }
    g_dual_conn.device_num_recorded = num;
    y_printf("g_dual_conn.device_num_recorded = num:%d", g_dual_conn.device_num_recorded);

    if (num == 1) {
        memcpy(g_dual_conn.remote_addr[2], mac_addr, 6);
    }
#if TCFG_DUAL_CONN_INQUIRY_SCAN_TIME
    g_dual_conn.inquiry_scan_disable = 0;
    g_dual_conn.inquiry_scan_time = sys_timeout_add(NULL, close_inquiry_scan, TCFG_DUAL_CONN_INQUIRY_SCAN_TIME * 1000);
#else
    g_dual_conn.inquiry_scan_disable = 1;
#endif
}

static void page_next_device(void *p)
{
    g_dual_conn.timer = 0;
    dual_conn_page_device();
}



static int dual_conn_btstack_event_handler(int *_event)
{
    struct bt_event *event = (struct bt_event *)_event;
    int state = tws_api_get_tws_state();

    switch (event->event) {
    case BT_STATUS_INIT_OK:
        // dual_conn_page_devices_init();
        return 0;
    case BT_STATUS_FIRST_CONNECTED:
        tws_dual_log("BT_STATUS_FIRST_CONNECTED");
        if (g_dual_conn.timer) {
            sys_timeout_del(g_dual_conn.timer);
            g_dual_conn.timer = 0;
        }
        del_device_from_page_list(event->args);

        if (bt_get_remote_test_flag()) {
            return 0;
        }
        memcpy(g_dual_conn.remote_addr[0], event->args, 6);
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            if (!page_list_empty()) {
                if (g_dual_conn.device_num_recorded == 1) {
                    if (memcmp(event->args, g_dual_conn.remote_addr[2], 6)) {
                        g_dual_conn.device_num_recorded++;
                    }
                }
                g_dual_conn.timer = sys_timeout_add(NULL, page_next_device, 500);
                return 0;
            }
        }
        if ((state & TWS_STA_TWS_PAIRED) && (state & TWS_STA_SIBLING_DISCONNECTED)) {
            tws_api_wait_connection(0);
        } else if (state & TWS_STA_SIBLING_CONNECTED) {
            tws_api_auto_role_switch_enable();
        }

        if (g_dual_conn.device_num_recorded == 0) {
            g_dual_conn.device_num_recorded++;
            memcpy(g_dual_conn.remote_addr[2], event->args, 6);
            if (state & TWS_STA_SIBLING_CONNECTED) {
                dual_conn_try_open_inquiry_scan();
            }
            break;
        }
        if (g_dual_conn.device_num_recorded == 1) {
            if (memcmp(event->args, g_dual_conn.remote_addr[2], 6) == 0) {
                if (state & TWS_STA_SIBLING_CONNECTED) {
                    dual_conn_try_open_inquiry_scan();
                }
                break;
            }
            g_dual_conn.device_num_recorded++;
        }
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
#if TCFG_BT_DUAL_CONN_ENABLE
            write_scan_conn_enable(0, 1);
#else
            write_scan_conn_enable(0, 0);
#endif
        }
        break;
    case BT_STATUS_SECOND_CONNECTED:
        tws_dual_log("BT_STATUS_SECOND_CONNECTED");
        if (g_dual_conn.device_num_recorded == 1) {
            g_dual_conn.device_num_recorded++;
        }
        if (g_dual_conn.timer) {
            sys_timeout_del(g_dual_conn.timer);
            g_dual_conn.timer = 0;
        }
        clr_device_in_page_list();
        memcpy(g_dual_conn.remote_addr[1], event->args, 6);
        if ((state & TWS_STA_TWS_PAIRED) && (state & TWS_STA_SIBLING_DISCONNECTED)) {
            tws_api_wait_connection(0);
        } else {
            tws_api_auto_role_switch_enable();
        }
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            break;
        }
#if (TCFG_PREEMPT_CONNECTION_ENABLE)
        write_scan_conn_enable(0, 1);
#else
        write_scan_conn_enable(0, 0);
#endif
        break;
    }

    return 0;
}

APP_MSG_HANDLER(dual_conn_stack_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = dual_conn_btstack_event_handler,
};


static int dual_conn_hci_event_handler(int *_event)
{
    struct bt_event *event = (struct bt_event *)_event;

    if (app_var.goto_poweroff_flag) {
        return 0;
    }

    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return 0;
    }
    int is_remote_test = bt_get_remote_test_flag();

    switch (event->event) {
    case HCI_EVENT_VENDOR_NO_RECONN_ADDR:
        break;
    case HCI_EVENT_VENDOR_REMOTE_TEST:
        if (event->value != VENDOR_TEST_DISCONNECTED) {
            if (g_dual_conn.timer) {
                sys_timeout_del(g_dual_conn.timer);
                g_dual_conn.timer = 0;
            }
            tws_api_cancle_wait_pair();
            clr_device_in_page_list();
            bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
        }
        return 0;
    case HCI_EVENT_DISCONNECTION_COMPLETE :
        tws_dual_log("HCI_EVENT_DISCONNECTION_COMPLETE, event->value:0x%x", event->value);
        if (event->value == ERROR_CODE_CONNECTION_TIMEOUT || event->value == ERROR_CODE_ROLE_CHANGE_NOT_ALLOWED || event->value == 0x16) {
            if (is_remote_test == 0) {

            }
        }
        break;
    case HCI_EVENT_CONNECTION_COMPLETE:
        switch (event->value) {
        case ERROR_CODE_SUCCESS :
            if (g_dual_conn.timer) {
                sys_timeout_del(g_dual_conn.timer);
                g_dual_conn.timer = 0;
            }
            del_device_from_page_list(event->args);
            return 0;
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR:
        case ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES:
            if (!list_empty(&g_dual_conn.page_head)) {
                struct page_device_info *info;
                info = list_first_entry(&g_dual_conn.page_head,
                                        struct page_device_info, entry);
                list_del(&info->entry);
                list_add_tail(&info->entry, &g_dual_conn.page_head);
            }
            break;
        case ERROR_CODE_PIN_OR_KEY_MISSING:
        case ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED :
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST :
        case ERROR_CODE_AUTHENTICATION_FAILURE :
            break;
        case ERROR_CODE_PAGE_TIMEOUT:
            break;
        case ERROR_CODE_ROLE_CHANGE_NOT_ALLOWED:
        case ERROR_CODE_CONNECTION_TIMEOUT:
        case ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS  :
            if (is_remote_test == 0) {
                // add_device_2_page_list(event->args, TCFG_BT_TIMEOUT_PAGE_TIME * 1000);
            }
            break;
        default:
            return 0;
        }
        break;
    default:
        return 0;
    }
    g_dual_conn.page_scan_auto_disable = 0;
    for (int i = 0; i < 2; i++) {
        if (memcmp(event->args, g_dual_conn.remote_addr[i], 6) == 0) {
            tws_dual_log("g_dual_conn.remote_addr[%d] set 0xff", i);
            memset(g_dual_conn.remote_addr[i], 0xff, 6);
        }
    }

    // tws_dual_conn_state_handler();
    tws_dual_log("---------out_of_range:%d", user_var.out_of_range);
    //如果是超距了，这里先不切换回连TWS
    if (!user_var.out_of_range) {
        tws_dual_conn_state_handler();
    }

    return 0;
}
APP_MSG_HANDLER(dual_conn_hci_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_HCI,
    .handler    = dual_conn_hci_event_handler,
};

static void rx_device_info(u8 *data, int len)
{
    if (g_dual_conn.inquiry_scan_disable == 0) {
        g_dual_conn.inquiry_scan_disable = data[0] & 0x02;
    }
    g_dual_conn.page_scan_auto_disable = data[0] & 0x01;
    g_dual_conn.device_num_recorded = data[0] >> 2;

    if (g_dual_conn.page_scan_auto_disable) {
        if (g_dual_conn.page_scan_timer) {
            sys_timeout_del(g_dual_conn.page_scan_timer);
            g_dual_conn.page_scan_timer = 0;
        }
    }
    if ((data[1] & 0x01) == 1) {
        if (tws_api_get_role() == TWS_ROLE_SLAVE || len > 2) {
            clr_device_in_page_list();
            for (int i = 2; i < len; i += 6) {
                add_device_2_page_list(data + i, TCFG_BT_POWERON_PAGE_TIME * 1000);
            }
        }
    }
    free(data);
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        tws_dual_conn_state_handler();
    }
}

static void tws_page_device_info_sync(void *_data, u16 len, bool rx)
{
    if (!rx) {
        return;
    }
    u8 *data = malloc(len);
    memcpy(data, _data, len);
    int msg[4] = { (int)rx_device_info, 2, (int)data, len};
    os_taskq_post_type("app_core", Q_CALLBACK, 4, msg);
}
REGISTER_TWS_FUNC_STUB(tws_dual_conn_stub) = {
    .func_id = 0xF730EBC7,
    .func   = tws_page_device_info_sync,
};

void send_page_device_addr_2_sibling()
{
    u8 data[14];
    int offset = 2;
    struct page_device_info *info, *n;

    if (!g_dual_conn.page_head_inited) {
        return;
    }
    data[0] = g_dual_conn.page_scan_auto_disable |
              (g_dual_conn.inquiry_scan_disable << 1) |
              (g_dual_conn.device_num_recorded << 2);
    data[1] = 1;
    list_for_each_entry_safe(info, n, &g_dual_conn.page_head, entry) {
        memcpy(data + offset, info->mac_addr, 6);
        offset += 6;
    }
    tws_api_send_data_to_sibling(data, offset, 0xF730EBC7);
}


static int dual_conn_tws_event_handler(int *_event)
{
    struct tws_event *event = (struct tws_event *)_event;
    int role = event->args[0];
    int phone_link_connection = event->args[1];
    int reason = event->args[2];

    int is_remote_test = bt_get_remote_test_flag();
    switch (event->event) {
    case TWS_EVENT_CONNECTED:
        tws_dual_log("TWS_EVENT_CONNECTED");
        tws_dual_log("page_list_empty:%d, role:%d, total_connect:%d", page_list_empty(), role, bt_get_total_connect_dev());
        if (g_dual_conn.timer) {
            sys_timeout_del(g_dual_conn.timer);
            g_dual_conn.timer = 0;
        }
        //TWS连接上，UI先关掉，后面开什么灯效看下面主机状态决定
        // led_ui_manager_display(LED_LOCAL, LED_UUID_WHITE_OFF, LED_MODE_C);
        if (app_var.goto_poweroff_flag) {
            break;
        }

        tws_dual_log("tws_poweron_status:%d, out_of_range:%d", user_var.tws_poweron_status, user_var.out_of_range);
        if (role == TWS_ROLE_MASTER) {
            tws_api_auto_role_switch_disable();
            if (bt_get_total_connect_dev() > 0) {
                //TWS连接上，有连着手机，就把回连的定时器删掉
                user_tws_sync_call(CMD_SYNC_EXIT_RECONN);
                //TWS连接上，有连着手机，两边的灯效关掉
                // led_ui_manager_display(LED_TWS_SYNC, LED_UUID_WHITE_OFF, LED_MODE_C);
                if (!page_list_empty()) {
                    dual_conn_page_device();
                } else {
                    tws_dual_conn_state_handler();
                }
            } else if (bt_get_total_connect_dev() == 0) {
                //开机回连才走这个流程，其他情况不走
                if (user_var.tws_poweron_status == 1) {
                    if (!page_list_empty()) {
                        //TWS连接成功，有BT记录就回连手机
                        dual_conn_page_device();
                    } else {
                        //TWS连接成功，无BT记录就进配对模式
                        user_tws_sync_call(CMD_SYNC_ENTER_PAIR_MODE);
                    }
                } else {
                    if (!page_list_empty()) {
                        //TWS连接成功，有BT记录就回连手机
                        dual_conn_page_device();
                    } else {
                        write_scan_conn_enable(1, 1);
                    }
                }
            }
            send_page_device_addr_2_sibling();
        } else {
            write_scan_conn_enable(0, 0);
            bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
        }
        break;
    case TWS_EVENT_CONNECTION_DETACH:
        tws_dual_log("TWS_EVENT_CONNECTION_DETACH, reason:0x%x", reason);
        if (g_dual_conn.timer) {
            sys_timeout_del(g_dual_conn.timer);
            g_dual_conn.timer = 0;
        }
        bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);

        if (app_var.goto_poweroff_flag) {
            break;
        }

        if (reason & TWS_DETACH_BY_REMOVE_PAIRS) {
            if (role == TWS_ROLE_MASTER) {
                if (bt_get_total_connect_dev()) {
                    break;
                }
            }
            write_scan_conn_enable(1, 1);
            break;
        }
        if (reason & TWS_DETACH_BY_TESTBOX_CON) {
            write_scan_conn_enable(1, 1);
            break;
        }
        if (reason & TWS_DETACH_BY_POWEROFF) {
            tws_dual_conn_state_handler();
            break;
        }
        if (reason & TWS_DETACH_BY_SUPER_TIMEOUT) {
            if (role == TWS_ROLE_SLAVE) {
                // clr_device_in_page_list();
                if (is_remote_test == 0) {
                    tws_dual_log("disconn by timeout, role is slave!!!");
                    tws_dual_log("phone_link_connection:%d, out_of_range:%d", event->args[1], user_var.out_of_range);
                    if (phone_link_connection) {
                        play_tone_file_alone(get_tone_files()->bt_disconnect);
                    }
                    // play_tone_file_alone(get_tone_files()->bt_disconnect);
                    
                    //如果TWS断开前，已经跟手机断开了，那么就先回连TWS，再去page手机（避免大多数情况下回连TWS慢）
                    //如果TWS先断开，再跟手机断开，那就先page手机，主要是在主耳超距，优化从机回连手机的场景
                    if (user_var.out_of_range) {
                        tws_dual_log("out_of_range now, reconn tws first!!!");
                    } else {
                        tws_dual_log("tws disconn now, reconn phone first!!!");
                        user_var.slave_reconn = 1;
                        break;
                    }
                }
            }
            tws_dual_conn_state_handler();
            break;
        }
        if (!page_list_empty()) {
            dual_conn_page_device();
        } else {
            tws_api_wait_connection(0);
            int connect_device = bt_get_total_connect_dev();
            if (connect_device == 0) {
                write_scan_conn_enable(1, 1);
            } else if (connect_device == 1) {
                if (g_dual_conn.device_num_recorded > 1) {
                    write_scan_conn_enable(0, 1);
                } else {
                    write_scan_conn_enable(0, 0);
                }
            }
        }
        break;
    case TWS_EVENT_ROLE_SWITCH:
        tws_dual_log("TWS_EVENT_ROLE_SWITCH, role:0x%x", role);
        if (role == TWS_ROLE_SLAVE) {
            write_scan_conn_enable(0, 0);
            bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
            if (app_var.goto_poweroff_flag == 0) {
                send_page_device_addr_2_sibling();
            }
        }

        break;
    }

    return 0;
}


APP_MSG_HANDLER(dual_conn_tws_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = dual_conn_tws_event_handler,
};



static void page_device_msg_handler()
{
    y_printf("page_device_msg_handler");
    u8 mac_addr[6];
    struct page_device_info *info;
    int device_num = bt_get_total_connect_dev();

    if (!g_dual_conn.page_head_inited) {
        return;
    }

    list_for_each_entry(info, &g_dual_conn.page_head, entry) {
        device_num++;
    }

    if (device_num >= 2) {
        return;
    }

    int num = btstack_get_num_of_remote_device_recorded();
    for (int i = num - 1; i >= 0; i--) {
        btstack_get_remote_addr(mac_addr, i);
        if (memcmp(mac_addr, g_dual_conn.remote_addr[0], 6) == 0) {
            continue;
        }
        if (memcmp(mac_addr, g_dual_conn.remote_addr[1], 6) == 0) {
            continue;
        }
        int ret = add_device_2_page_list(mac_addr, TCFG_BT_POWERON_PAGE_TIME * 1000);
        if (ret == 0) {
            if (++device_num >= 2) {
                break;
            }
        }
    }
    if (g_dual_conn.timer) {
        sys_timeout_del(g_dual_conn.timer);
        g_dual_conn.timer = 0;
    }
    tws_api_cancle_create_connection();
    tws_api_cancle_wait_pair();
    write_scan_conn_enable(0, 0);
    dual_conn_page_device();
}



/*
 * 设置自动回连的识别码 6个byte
 * */
static u8 auto_pair_code[6] = {0x34, 0x66, 0x33, 0x87, 0x09, 0x42};

static u8 *tws_set_auto_pair_code(void)
{
    u16 code = bt_get_tws_device_indicate(NULL);
    auto_pair_code[0] = code >> 8;
    auto_pair_code[1] = code & 0xff;
    return auto_pair_code;
}

static void tws_pair_timeout(void *p)
{
    tws_dual_log("tws_pair_timeout\n");
    g_dual_conn.timer = 0;

    tws_api_cancle_create_connection();
    tws_dual_log("page_list_empty():%d", page_list_empty());
    if (!page_list_empty()) {
        dual_conn_page_device();
    } else {
#if TCFG_TWS_CONN_DISABLE
        write_scan_conn_enable(1, 1);
#else
        user_enter_pair_mode();
        // tws_api_wait_pair_by_code(0, bt_get_local_name(), 0);
#endif
    }
    app_send_message(APP_MSG_TWS_POWERON_PAIR_TIMEOUT, 0);
}


static void tws_create_conn_timeout(void *p)
{
    tws_dual_log("tws_create_conn:%d", page_list_empty());
    g_dual_conn.timer = 0;
    tws_api_cancle_create_connection();

    if (!page_list_empty()) {
        dual_conn_page_device();
    } else {
        // tws_dual_conn_state_handler();
        user_enter_pair_mode();
    }
    app_send_message(APP_MSG_TWS_POWERON_CONN_TIMEOUT, 0);
}

static void msg_tws_wait_pair_timeout(void *p)
{
    g_dual_conn.timer = 0;
    tws_api_cancle_wait_pair();
}

static void msg_tws_start_pair_timeout(void *p)
{
    g_dual_conn.timer = 0;
    tws_api_wait_pair_by_code(0, bt_get_local_name(), 0);
    app_send_message(APP_MSG_TWS_START_PAIR_TIMEOUT, 0);
}

static void msg_tws_start_conn_timeout(void *p)
{
    g_dual_conn.timer = 0;
    app_send_message(APP_MSG_TWS_START_CONN_TIMEOUT, 0);
}

static int dual_conn_app_event_handler(int *msg)
{
    // tws_dual_log("dual_conn_app_event_handler, msg:%d", msg[0]);
    switch (msg[0]) {
    case APP_MSG_TWS_PAIRED:
        tws_dual_log("APP_MSG_TWS_PAIRED");
        user_var.tws_poweron_status = 1;
#if TCFG_TWS_CONN_DISABLE
        tws_pair_timeout(NULL);
        break;
#endif
        tws_api_create_connection(0);
        g_dual_conn.timer = sys_timeout_add(NULL, tws_create_conn_timeout,
                                            TCFG_TWS_CONN_TIMEOUT * 1000);
        break;
    case APP_MSG_TWS_UNPAIRED:
        tws_dual_log("APP_MSG_TWS_UNPAIRED");
        user_var.tws_poweron_status = 1;
        // led_ui_manager_display(LED_LOCAL, LED_UUID_WHITE_FAST_FLASH, LED_MODE_C);
#if TCFG_TWS_CONN_DISABLE
        tws_pair_timeout(NULL);
        break;
#endif
#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_AUTO
        /* 未配对, 开始自动配对 */
        tws_api_set_quick_connect_addr(tws_set_auto_pair_code());
        tws_api_auto_pair(0);
        g_dual_conn.timer = sys_timeout_add(NULL, tws_pair_timeout,
                                            TCFG_TWS_PAIR_TIMEOUT  * 1000);
#else
        /* 未配对, 等待发起配对 */
        if (!list_empty(&g_dual_conn.page_head)) {
            dual_conn_page_device();
        } else {
            tws_api_wait_pair_by_code(0, bt_get_local_name(), 0);
        }
#endif
        break;
    case APP_MSG_BT_PAGE_DEVICE:
        tws_dual_log("APP_MSG_BT_PAGE_DEVICE:%d", page_list_empty());
        user_var.tws_poweron_status = 1;
        // page_device_msg_handler();
        // dual_conn_page_device();
        if (!page_list_empty()) {
            dual_conn_page_device();
        } else {
            //开机无TWS记录，无BT记录，直接进配对模式
            user_enter_pair_mode();
        }
        break;
    case APP_MSG_TWS_START_PAIR_AND_CONN_PROCESS:
        tws_dual_log("APP_MSG_TWS_START_PAIR_AND_CONN_PROCESS");
        if (bt_tws_is_paired()) {
            app_send_message(APP_MSG_TWS_PAIRED, 0);
        } else {
            app_send_message(APP_MSG_TWS_UNPAIRED, 0);
        }
        break;
    case APP_MSG_TWS_WAIT_PAIR:
        tws_dual_log("APP_MSG_TWS_WAIT_PAIR");
        tws_api_wait_pair_by_code(0, bt_get_local_name(), 0);
        g_dual_conn.timer = sys_timeout_add(NULL, msg_tws_wait_pair_timeout,
                                            TCFG_TWS_PAIR_TIMEOUT * 1000);
        break;
    case APP_MSG_TWS_START_PAIR:
        tws_dual_log("APP_MSG_TWS_START_PAIR");
        tws_api_search_sibling_by_code();
        if (g_dual_conn.timer) {
            sys_timeout_del(g_dual_conn.timer);
        }
        g_dual_conn.timer = sys_timeout_add(NULL, msg_tws_start_pair_timeout,
                                            TCFG_TWS_PAIR_TIMEOUT * 1000);
        break;
    case APP_MSG_TWS_WAIT_CONN:
        tws_dual_log("APP_MSG_TWS_WAIT_CONN");
        tws_api_wait_connection(0);
        break;
    case APP_MSG_TWS_START_CONN:
        tws_dual_log("APP_MSG_TWS_START_CONN");
        tws_api_create_connection(0);
        if (g_dual_conn.timer) {
            sys_timeout_del(g_dual_conn.timer);
        }
        g_dual_conn.timer = sys_timeout_add(NULL, msg_tws_start_conn_timeout,
                                            TCFG_TWS_CONN_TIMEOUT * 1000);
        break;
    }

    return 0;
}

APP_MSG_HANDLER(dual_conn_app_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = dual_conn_app_event_handler,
};



void tws_dual_conn_close()
{
    // tws_dual_log("tws_dual_conn_close");
    if (g_dual_conn.timer) {
        sys_timeout_del(g_dual_conn.timer);
        g_dual_conn.timer = 0;
    }
    //clr_device_in_page_list();
    tws_api_cancle_create_connection();
    tws_api_cancle_wait_pair();
    write_scan_conn_enable(0, 0);
    bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
}
#endif



/*************************************************************/
//   获取活跃设备
/*************************************************************/
int custom_get_call_addr(u8 *btaddr)
{
    void *devices[2];
    int num = btstack_get_conn_devices(devices, 2);
    for (u8 i = 0; i < num; i++) {
        tws_dual_log("btstack_bt_get_call_status(devices[i]):%d", btstack_bt_get_call_status(devices[i]));
        if (btstack_bt_get_call_status(devices[i]) != BT_CALL_HANGUP) { //这里可以进一步判断两个设备的通话状态，判断用哪一个设备
            memcpy(btaddr, btstack_get_device_mac_addr(devices[i]), 6);
            return 1;
        }
    }
    return 0;
}

u8 user_get_active_device_addr(u8 *bt_addr)
{
    if (esco_player_get_btaddr(bt_addr)) {  //有ESCO状态
        tws_dual_log("esco player now!!!");
        tws_dual_log_hexdump(bt_addr, 6);
        return 1;
    } else if (bt_get_call_status() != BT_CALL_HANGUP) {     //有HFP状态
        if (custom_get_call_addr(bt_addr)) {
            tws_dual_log("hfp player now!!!");
            tws_dual_log_hexdump(bt_addr, 6);
            return 1;
        }
    } else if (a2dp_player_get_btaddr(bt_addr)) {       //有A2DP状态
        tws_dual_log("a2dp player now!!!");
        tws_dual_log_hexdump(bt_addr, 6);
        return 1;
    } else {
        tws_dual_log("no active device now!!!");
        memcpy(bt_addr, addr_all_ff, 6);
        tws_dual_log_hexdump(bt_addr, 6);
    }
    return 0;
}
/*************************************************************/

/*************************************************************/
//   一拖一模式开机回连信息处理，一拖一只存一个，最后连接上的哪个
/*************************************************************/
void user_1t1_mode_reconn_addr_init()
{
    // tws_dual_log("user_1t1_mode_reconn_addr_init");
    syscfg_read(CFG_USER_1T1MODE_ADDR, user_1t1_mode_addr, sizeof(user_1t1_mode_addr));
}
__initcall(user_1t1_mode_reconn_addr_init);

void user_1t1_mode_reconn_addr_save()
{
    // tws_dual_log("user_1t1_mode_reconn_addr_save");
    syscfg_write(CFG_USER_1T1MODE_ADDR, user_1t1_mode_addr, sizeof(user_1t1_mode_addr));
}

void user_1t1_mode_reconn_addr_reset()
{
    // tws_dual_log("user_1t1_mode_reconn_addr_reset");
    memcpy(user_1t1_mode_addr, addr_all_ff, 6);
    syscfg_write(CFG_USER_1T1MODE_ADDR, user_1t1_mode_addr, sizeof(user_1t1_mode_addr));
}

void user_1t1_mode_reconn_add_device(u8 *mac_addr)
{
    tws_dual_log("user_1t1_mode_reconn_add_device");
    tws_dual_log_hexdump(mac_addr, 6);
    memcpy(user_1t1_mode_addr, mac_addr, 6);
    user_1t1_mode_reconn_addr_save();
}


/*************************************************************/

/*************************************************************/
//   按键回连的操作，在这里进行
/*************************************************************/
u8 user_add_device_page_list(u8 *reconn_addr)
{
    // tws_dual_log_hexdump(reconn_addr, 6);
    // y_printf("====%d", memcmp(reconn_addr, addr_all_ff, 6));
    if (memcmp(reconn_addr, addr_all_ff, 6)) {
        add_device_2_page_list(reconn_addr, TCFG_BT_PAGE_TIMEOUT * 1000);
        return 1;
    }
    return 0;
}

void user_key_reconn_addr_init()
{
    // tws_dual_log("user_key_reconn_addr_init");
    memcpy(user_bt_dev_addr.reconn_addr1, addr_all_ff, 6);
    memcpy(user_bt_dev_addr.reconn_addr2, addr_all_ff, 6);
    syscfg_read(CFG_USER_RECONN_ADDR, (u8 *)&user_bt_dev_addr, sizeof(user_bt_dev_addr));
}
__initcall(user_key_reconn_addr_init);

void user_key_reconn_addr_save()
{
    // tws_dual_log("user_key_reconn_addr_save");
    syscfg_write(CFG_USER_RECONN_ADDR, (u8 *)&user_bt_dev_addr, sizeof(user_bt_dev_addr));
}

void user_key_reconn_addr_reset()
{
    // tws_dual_log("user_key_reconn_addr_reset");
    memcpy(user_bt_dev_addr.reconn_addr1, addr_all_ff, 6);
    memcpy(user_bt_dev_addr.reconn_addr2, addr_all_ff, 6);
    syscfg_write(CFG_USER_RECONN_ADDR, (u8 *)&user_bt_dev_addr, sizeof(user_bt_dev_addr));
}

void user_key_reconn_add_device(u8 *mac_addr)
{
    tws_dual_log("user_key_reconn_add_device");
    tws_dual_log_hexdump(mac_addr, 6);
    if (memcmp(mac_addr, user_bt_dev_addr.reconn_addr1, 6) == 0) {
        tws_dual_log("this addr is same as addr1, return!!!");
        return;
    }
    memcpy(user_bt_dev_addr.reconn_addr2, user_bt_dev_addr.reconn_addr1, 6);
    memcpy(user_bt_dev_addr.reconn_addr1, mac_addr, 6);
    // tws_dual_log_hexdump(user_bt_dev_addr.reconn_addr1, 6);
    // tws_dual_log_hexdump(user_bt_dev_addr.reconn_addr2, 6);
    user_key_reconn_addr_save();
}

// void user_key_reconn_timeout()
// {
//     tws_dual_log("user_key_reconn_timeout");
//     tws_api_cancle_create_connection();
//     if (user_key_reconn_timeid != 0) {
//         sys_timeout_del(user_key_reconn_timeid);
//         user_key_reconn_timeid = 0;
//     }
// }

void user_key_reconn_start()
{
    tws_dual_log("user_key_reconn_start");
    int state = tws_api_get_tws_state();
    tws_dual_log("1t2_mode:%d, total_connect:%d, tws_status:%d, state:%d, tws_poweron:%d", user_app_info.u1t2_mode, bt_get_total_connect_dev(), get_tws_sibling_connect_state(), state, user_var.tws_poweron_status);
    if (user_var.tws_poweron_status) {
        return;
    }
    // set_unconn_tws_phone_enter_idle(0);
    // if (g_dual_conn.timer) {
    //     sys_timeout_del(g_dual_conn.timer);
    //     g_dual_conn.timer = 0;
    // }
    int ret = 0;
    if (user_app_info.u1t2_mode) {
        if (bt_get_total_connect_dev() == 1) {
            ret = user_add_device_page_list(user_bt_dev_addr.reconn_addr1);
        } else if (bt_get_total_connect_dev() == 0){
            ret = user_add_device_page_list(user_bt_dev_addr.reconn_addr1);
            ret = user_add_device_page_list(user_bt_dev_addr.reconn_addr2);
        } 
    } else {
        if (bt_get_total_connect_dev() == 0) {
            ret = user_add_device_page_list(user_bt_dev_addr.reconn_addr1);
        } 
    }
    tws_dual_log("-----------------ret:%d", ret);
    set_unconn_tws_phone_enter_idle(0);
    if (g_dual_conn.timer) {
        sys_timeout_del(g_dual_conn.timer);
        g_dual_conn.timer = 0;
    }
    if ((state & TWS_STA_TWS_PAIRED) && !get_tws_sibling_connect_state()) {
        tws_api_create_connection(0);
    }
    dual_conn_page_device();
}
/*************************************************************/

/*************************************************************/
//   一拖二的一些断开操作
/*************************************************************/
static u8 scan_conn_cnt = 0;
void user_lowpower_scan_conn_deal()
{
    // tws_dual_log("user_lowpower_scan_conn_deal");
    scan_conn_cnt++;
    if (scan_conn_cnt == 1) {
        if (!get_tws_sibling_connect_state()) {
            tws_api_wait_connection(0);
        }
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            // write_scan_conn_enable(1, 1);
            write_scan_conn_enable(0, 1);
        } else {
            write_scan_conn_enable(0, 0);
        }
    } else if (scan_conn_cnt == 3) {
        if (!get_tws_sibling_connect_state()) {
            tws_api_cancle_create_connection();
            tws_api_cancle_wait_pair();
        }
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            tws_dual_log("lmp_standard_connect_check():%d", lmp_standard_connect_check());
            if (lmp_standard_connect_check()) {
                //如果当前连接着，先不关可发现可连接
                // write_scan_conn_enable(1, 1);
                write_scan_conn_enable(0, 1);
            } else {
                write_scan_conn_enable(0, 0);
            }
        } else {
            write_scan_conn_enable(0, 0);
        }

    } else if (scan_conn_cnt == 8) {
        scan_conn_cnt = 0;
    } 
}

void user_enter_lowpower_mode(void)
{
    tws_dual_log("user_enter_lowpower_mode");
    tws_dual_log("role:%d, tws_status:%d", tws_api_get_role(), get_tws_sibling_connect_state());
    user_var.tws_poweron_status = 0;
    user_var.enter_pairing_mode = 0;
    user_var.box_enter_pair_mode = 0;
    user_var.slave_reconn = 0;
    user_var.out_of_range = 0;
    // led_ui_manager_display(LED_LOCAL, LED_UUID_WHITE_OFF, LED_MODE_C);
    clr_device_in_page_list();
    tws_dual_conn_close();
    bt_ble_rcsp_adv_disable();
    percent_save_vm();

    if (!get_tws_sibling_connect_state()) {
        set_unconn_tws_phone_enter_idle(1);
        tws_api_wait_connection(0);
    } else {
        set_unconn_tws_phone_enter_idle(0);
        tws_api_cancle_create_connection();
        tws_api_cancle_wait_pair();
    }

    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        // write_scan_conn_enable(1, 1);
        write_scan_conn_enable(0, 1);
    } else {
        write_scan_conn_enable(0, 0);
    }

    if (user_lowpower_scan_conn_timer == 0) {
        scan_conn_cnt = 0;
        user_lowpower_scan_conn_timer = sys_timer_add(NULL, user_lowpower_scan_conn_deal, 1000);
    }
    bt_sniff_enable();
}

//进入配对模式5分钟，先关掉所有tws回连和page，报提示音和闪灯
void user_enter_pair_mode()
{
    tws_dual_log("user_enter_pair_mode");
    tws_dual_log("role:%d, tws_status:%d", tws_api_get_role(), get_tws_sibling_connect_state());
    user_var.tws_poweron_status = 0;
    user_var.enter_pairing_mode = 1;
    user_var.box_enter_pair_mode = 0;
    user_var.slave_reconn = 0;
    user_var.out_of_range = 0;
    if (user_lowpower_scan_conn_timer != 0) {
        sys_timer_del(user_lowpower_scan_conn_timer);
        user_lowpower_scan_conn_timer = 0;
    }
    clr_device_in_page_list();
    tws_dual_conn_close();
    set_unconn_tws_phone_enter_idle(0);
    bt_ble_rcsp_adv_enable();
    sys_auto_shut_down_disable();
    sys_auto_shut_down_enable();

    if (!get_tws_sibling_connect_state()) {
        tws_api_wait_connection(0);
    }

    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        // led_ui_manager_display(LED_LOCAL, LED_UUID_WHITE_SLOW_FLASH, LED_MODE_C);
        tws_play_tone_file_alone(get_tone_files()->pairing, 400);
        write_scan_conn_enable(1, 1);
    } else {
        // led_ui_manager_display(LED_LOCAL, LED_UUID_WHITE_OFF, LED_MODE_C);
        write_scan_conn_enable(0, 0);
    }
}

void user_1t2_1dev_reconn_timeout_mode()
{
    tws_dual_log("user_1t2_1dev_reconn_timeout_mode(");
    user_var.tws_poweron_status = 0;
    user_var.box_enter_pair_mode = 0;
    user_var.slave_reconn = 0;
    user_var.out_of_range = 0;
    if (user_lowpower_scan_conn_timer != 0) {
        sys_timer_del(user_lowpower_scan_conn_timer);
        user_lowpower_scan_conn_timer = 0;
    }
    clr_device_in_page_list();
    write_scan_conn_enable(0, 1);
}

void user_dual_conn_disconn_device()
{
    tws_dual_log("user_dual_conn_disconn_device");
    tws_dual_log("role:%d, tws_status:%d, total_dev:%d, 1t2_mode:%d", tws_api_get_role(), get_tws_sibling_connect_state(), bt_get_total_connect_dev(), user_app_info.u1t2_mode);
    u8 bt_active_addr[6];
    u8 *bt_idle_addr = NULL;
    if (user_app_info.u1t2_mode) {
        if (bt_get_total_connect_dev() == 2) {
            if (user_get_active_device_addr(user_bt_dev_addr.active_addr)) {
                bt_idle_addr = btstack_get_other_dev_addr(user_bt_dev_addr.active_addr);
                if (bt_idle_addr != NULL) {
                    tws_dual_log_hexdump(bt_idle_addr, 6);
                    if (tws_api_get_role() == TWS_ROLE_MASTER) {
                        bt_cmd_prepare_for_addr(bt_idle_addr, USER_CTRL_DISCONNECTION_HCI, 0, NULL);
                    }
                }
            } else {
                tws_dual_log_hexdump(user_phone_info2.addr, 6);
                if (tws_api_get_role() == TWS_ROLE_MASTER) {
                    bt_cmd_prepare_for_addr(user_phone_info2.addr, USER_CTRL_DISCONNECTION_HCI, 0, NULL);
                }
            }
        } else if (bt_get_total_connect_dev() == 1) {
            user_tws_sync_call(CMD_SYNC_ENTER_PAIR_MODE);
        } else if (bt_get_total_connect_dev() == 0) {
            user_tws_sync_call(CMD_SYNC_ENTER_PAIR_MODE);
        }
    } else {
        if (bt_get_total_connect_dev() == 1) {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                bt_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
            }
        } else if (bt_get_total_connect_dev() == 0) {
            user_tws_sync_call(CMD_SYNC_ENTER_PAIR_MODE);
        }
    }
}

void user_1t1_and_1t2_mode_connect_deal()
{
    tws_dual_log("user_1t1_and_1t2_mode_connect_deal");
    tws_dual_log("1t2_mode:%d, total_connect:%d", user_app_info.u1t2_mode, bt_get_total_connect_dev());
    if (!user_app_info.u1t2_mode) {
        //1T1模式下，第二台连接上来后，断开前面连接的那台
        if (bt_get_total_connect_dev() == 2) {
            tws_dual_log_hexdump(user_bt_dev_addr.reconn_addr2, 6);
            bt_cmd_prepare_for_addr(user_bt_dev_addr.reconn_addr2, USER_CTRL_DISCONNECTION_HCI, 0, NULL);
        }
    }
}

/*************************************************************/
