#include "system/includes.h"
#include "battery_manager.h"
#include "app_power_manage.h"
#include "app_main.h"
#include "app_config.h"
#include "app_action.h"
#include "asm/charge.h"
#include "app_tone.h"
#include "gpadc.h"
#include "btstack/avctp_user.h"
#include "user_cfg.h"
#include "asm/charge.h"
#include "bt_tws.h"

#if RCSP_ADV_EN
#include "ble_rcsp_adv.h"
#endif

#define LOG_TAG             "[BATTERY]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if 1
#define bat_log(x, ...)       g_printf("[USER_BAT]" x " ", ## __VA_ARGS__)
#define bat_log_hexdump       g_printf("[USER_BAT]bat_log_hexdump:");\
                                    put_buf
#else
#define bat_log(...)
#define bat_log_hexdump(...)
#endif

extern void user_set_cur_bat_st(u8 value);

#define PLAY_TIME_OFFSET    (3)
#define DEFAULT_PLAY_TIME_97DB   (34)
#define DEFAULT_PLAY_TIME_102DB  (34)
#define CHARGING_POWER_PERCENT_UPDATE_TIME  (27*1000) //充电时，电量更新周期
#define NORMAL_POWER_PERCENT_UPDATE_TIME    (8*1000) //非充电时，电量更新周期
static u32 normal_power_update_time = NORMAL_POWER_PERCENT_UPDATE_TIME;
extern CHARGE_INFO user_charge_info;
// u8 is_power_up = 0; //记录是否上电开机
extern u8 is_power_up;

u16 user_delay_play_lowpower_tone_timerid = 0;
void user_delay_play_lowpower_tone();
void user_play_lowpower_tone_reset();

const struct VBAT_V_TO_LEVEL vbat_value_to_percent[] = {
    {4322, 4262, 90},
//    {4362, 4262, 90},
    {4262, 4145, 80},
    {4145, 4033, 70},
    {4033, 3921, 60},
    {3921, 3849, 50},
    {3849, 3796, 40},
    {3796, 3752, 30},
    {3752, 3706, 20},
    {3706, 3658, 10},
    {3658, 3200, 0},
};


enum {
    VBAT_NORMAL = 0,
    VBAT_WARNING,
    VBAT_LOWPOWER,
} VBAT_STATUS;

#define VBAT_DETECT_CNT         6 //每次更新电池电量采集的次数
#define VBAT_DETECT_TIME        10L //每次采集的时间间隔
#define VBAT_UPDATE_SLOW_TIME   60000L //慢周期更新电池电量
#define VBAT_UPDATE_FAST_TIME   10000L //快周期更新电池电量

struct battery_curve {
    u8 percent;
    u16 voltage;
};

union battery_data {
    u32 raw_data;
    struct {
        u8 reserved;
        u16 voltage;
        u8 percent;
    } __attribute__((packed)) data;
};

static int vbat_slow_timer = 0;
static int vbat_fast_timer = 0;
static int lowpower_timer = 0;
static u8 old_battery_level = 9;
static u16 cur_battery_voltage = 0;
static u8 cur_battery_level = 0;
static u8 cur_battery_percent = 0;
static u8 tws_sibling_bat_level = 0xff;
static u8 tws_sibling_bat_percent_level = 0xff;
static u8 cur_bat_st = VBAT_NORMAL;
static u8 battery_curve_max;
static struct battery_curve *battery_curve_p;

extern void role_switch_check_delay(u16 ms);

static u8 percent_save = 0xff;

void user_set_cur_bat_st(u8 value)
{
    cur_bat_st = value;
}

u8 get_vm_vbat_percent(void)
{
    bat_log("get_vm_vbat_percent");
    u8 percent = 0;
    int len = syscfg_read(CFG_USER_VBAT_PERCENT, &percent, 1);
    if(len == 1){
        return percent;
    }

    return 0xff;
}

u8 set_vm_vbat_percent(u8 per)
{
    bat_log("set_vm_vbat_percent, per:%d", per);
    u8 percent = 0;
    int len = syscfg_read(CFG_USER_VBAT_PERCENT, &percent, 1);
    if(len != 1 || percent != per){
        percent = per;
        len = syscfg_write(CFG_USER_VBAT_PERCENT, &percent, 1);
        return len == 1 ? 1 : 0;
    }

    return percent == per;
}

void percent_save_vm(void)
{
    if (get_charge_full_flag()) {
        percent_save = 100;
        printf("percent_save_vm charge_full\n");
    }
    set_vm_vbat_percent(percent_save);
}

void percent_save_init(void)
{
    bat_log("percent_save_init");
    percent_save = get_vm_vbat_percent();
    y_printf("percent_save:%d", percent_save);
}
__initcall(percent_save_init);



void vbat_check(void *priv);
void clr_wdt(void);

#if TCFG_USER_TWS_ENABLE
u8 get_tws_sibling_bat_level(void)
{
    return tws_sibling_bat_level & 0x7f;
}

u8 get_tws_sibling_bat_persent(void)
{
    return tws_sibling_bat_percent_level;
}

void app_power_set_tws_sibling_bat_level(u8 vbat, u8 percent)
{
    bat_log("app_power_set_tws_sibling_bat_level");
    y_printf("vbat = %d, percent = %d", vbat, percent);
    tws_sibling_bat_level = vbat;
    tws_sibling_bat_percent_level = percent;
    if (tws_sibling_bat_percent_level == 0xFF) {
        if (!get_charge_online_flag()) {
            user_charge_info.box_bat_percent_local = 0xFF;
        }
        user_charge_info.box_bat_percent_sibling = 0xFF;
    }
    y_printf("%d, %d, %d, %d", tws_sibling_bat_level, tws_sibling_bat_percent_level, user_charge_info.remote_detail_percent, user_charge_info.box_bat_percent_sibling);
    //对方更新电量过来，同步一下给APP
    user_app_data_notify(CMD_NOTIFY_BAT_VALUE);
    /*
     ** 发出电量同步事件进行进一步处理
     **/
    batmgr_send_msg(POWER_EVENT_SYNC_TWS_VBAT_LEVEL, 0);

    log_info("set_sibling_bat_level: %d, %d\n", vbat, percent);
}


static void set_tws_sibling_bat_level(void *_data, u16 len, bool rx)
{
    // bat_log("set_tws_sibling_bat_level");
    u8 *data = (u8 *)_data;

    if (rx) {
        user_charge_info.remote_detail_percent = data[2];
        user_charge_info.box_bat_percent_sibling = data[3];
        app_power_set_tws_sibling_bat_level(data[0], data[1]);
    }
}

REGISTER_TWS_FUNC_STUB(vbat_sync_stub) = {
    .func_id = TWS_FUNC_ID_VBAT_SYNC,
    .func    = set_tws_sibling_bat_level,
};

void tws_sync_bat_level(void)
{
    bat_log("tws_sync_bat_level");
#if TCFG_BT_DISPLAY_BAT_ENABLE
    u8 battery_level = cur_battery_level;
#if CONFIG_DISPLAY_DETAIL_BAT
    u8 percent_level = get_vbat_percent();
#else
    u8 percent_level = get_self_battery_level() * 10 + 10;
#endif
    if (get_charge_online_flag()) {
        percent_level |= BIT(7);
    }

    u8 data[4];
    data[0] = battery_level;
    data[1] = percent_level;
    data[2] = user_charge_info.local_detail_percent;
    data[3] = user_charge_info.box_bat_percent_local;
    bat_log("%d, %d, %d, %d", battery_level, percent_level, user_charge_info.local_detail_percent, user_charge_info.box_bat_percent_local);
    tws_api_send_data_to_sibling(data, 4, TWS_FUNC_ID_VBAT_SYNC);

    log_info("tws_sync_bat_level: %d,%d\n", battery_level, percent_level);
#endif
}
#endif

u8 is_power_level_low(void)
{
    y_printf("cur_bat_st = %d", cur_bat_st);
    return cur_bat_st == VBAT_LOWPOWER ? 1 : 0;
}

static void power_warning_timer(void *p)
{
    batmgr_send_msg(POWER_EVENT_POWER_WARNING, 0);
}

static int app_power_event_handler(int *msg)
{
    int ret = false;

#if(TCFG_SYS_LVD_EN == 1)
    switch (msg[0]) {
    case POWER_EVENT_POWER_NORMAL:
        user_app_data_notify(CMD_NOTIFY_BAT_VALUE);
        break;
    case POWER_EVENT_POWER_WARNING:
        r_printf(" POWER_EVENT_POWER_WARNING, %d", tone_player_runing());
        // if (tone_player_runing()) {
        //     user_play_lowpower_tone_reset();
        //     break;
        // }
        play_tone_file(get_tone_files()->low_power);
        if (lowpower_timer == 0) {
            lowpower_timer = sys_timer_add(NULL, power_warning_timer, LOW_POWER_WARN_TIME);
        }
        user_app_data_notify(CMD_NOTIFY_BAT_VALUE);
        break;
    case POWER_EVENT_POWER_LOW:
        r_printf(" POWER_EVENT_POWER_LOW");
        vbat_timer_delete();
        if (lowpower_timer) {
            sys_timer_del(lowpower_timer);
            lowpower_timer = 0 ;
        }
#if TCFG_APP_BT_EN
#if (RCSP_ADV_EN)
        adv_tws_both_in_charge_box(1);
#endif
        if (!app_in_mode(APP_MODE_IDLE)) {
            sys_enter_soft_poweroff(POWEROFF_NORMAL);
        } else {
            power_set_soft_poweroff();
        }
#else
        void app_entry_idle() ;
        app_entry_idle() ;
#endif
        user_app_data_notify(CMD_NOTIFY_BAT_VALUE);
        break;
#if TCFG_APP_BT_EN
    case POWER_EVENT_SYNC_TWS_VBAT_LEVEL:
        tws_inout_box_deal();
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            bt_cmd_prepare(USER_CTRL_HFP_CMD_UPDATE_BATTARY, 0, NULL);
        }
        user_app_data_notify(CMD_NOTIFY_BAT_VALUE);
        break;
    case POWER_EVENT_POWER_CHANGE:
        bat_log("POWER_EVENT_POWER_CHANGE\n");
#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
			// 通话要更新电量
            // if (tws_api_get_tws_state()&TWS_STA_ESCO_OPEN) {
            //     break;
            // }
            tws_sync_bat_level();
        }
#endif
        // bat_log("bt_cmd_prepare(USER_CTRL_HFP_CMD_UPDATE_BATTARY, 0, NULL)");
        bt_cmd_prepare(USER_CTRL_HFP_CMD_UPDATE_BATTARY, 0, NULL);
		
#endif
        user_app_data_notify(CMD_NOTIFY_BAT_VALUE);
        break;
    case POWER_EVENT_POWER_CHARGE:
        bat_log("POWER_EVENT_POWER_CHARGE");
        if (lowpower_timer) {
            sys_timer_del(lowpower_timer);
            lowpower_timer = 0 ;
        }
        user_app_data_notify(CMD_NOTIFY_BAT_VALUE);
        break;
#if TCFG_CHARGE_ENABLE
    case CHARGE_EVENT_LDO5V_OFF:
        //充电拔出时重新初始化检测定时器
        vbat_check_init();
        break;
#endif
    default:
        break;
    }
#endif

    return ret;
}
APP_MSG_HANDLER(bat_level_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BATTERY,
    .handler    = app_power_event_handler,
};

static u16 get_vbat_voltage(void)
{
    return (adc_get_voltage(AD_CH_PMU_VBAT) * 4);
}

u8 a2dp_player_runing(void);
static u8 media_state = 0;
static u8 media_stop_cnt = 0;
static u32 play_start_point_ms = 0;
static u32 prev_play_time = 0;
static u8 power_ctrl_normal_start = 0;
static u8 default_play_time = DEFAULT_PLAY_TIME_97DB;
static u8 default_play_t_db = DEFAULT_PLAY_TIME_97DB;
static u8 play_time_offset = PLAY_TIME_OFFSET;
static u32 total_play_time = DEFAULT_PLAY_TIME_97DB;
static u8 total_play_cnt = 1;

u16 power_ctrl_deal(u16 bat_val, u8 index, u8 level_change)
{
    // y_printf("power_ctrl_deal");
    u16 trim_percent = 0;
    if(percent_save < 20){
        return 0;
    }
    // bat_log("a2dp_player_runing():%d", a2dp_player_runing());
    if(a2dp_player_runing()){
        media_stop_cnt = 0;
        u8 t_db = DEFAULT_PLAY_TIME_97DB;
        if(t_db != default_play_t_db){
            default_play_time = t_db;
            play_time_offset = PLAY_TIME_OFFSET;
            total_play_time = t_db;
            total_play_cnt = 1;
        }
        default_play_t_db = t_db;
        if(media_state == 0){
            media_state = 1;
            default_play_time = default_play_t_db;
            play_time_offset = PLAY_TIME_OFFSET;
            total_play_time = default_play_t_db;
            total_play_cnt = 1;
            play_start_point_ms = sys_timer_get_ms();//开始计时
            if(bat_val >= 4342){//if(bat_val >= 436){
                //电量接近满电
                power_ctrl_normal_start = 1;
            }
            return 0;
        }
    }
    else{
        media_stop_cnt++;
        if(media_stop_cnt > 1){
            media_state = 0;
            media_stop_cnt = 0;
            prev_play_time = 0;
            play_start_point_ms = 0;
            power_ctrl_normal_start = 0;
            total_play_time = total_play_cnt ? total_play_time / total_play_cnt : 0;
            total_play_cnt = total_play_time ? 1 : 0;
            return 0;
        }
    }
    u8 play_time = 0;
    if(media_state){
        play_time = (sys_timer_get_ms() - play_start_point_ms) / 1000 / 60;
        if(level_change){
            //电量掉了10%
            if(power_ctrl_normal_start == 0){
                //开始计时
                power_ctrl_normal_start = 1;
                play_start_point_ms = sys_timer_get_ms();
                goto __power_ctrl_end;
            }
            if(play_time && abs(play_time - default_play_time) <= play_time_offset){
                //播放时间正常
                printf("************************c\n");
                prev_play_time = play_time;//记录时间
                play_start_point_ms = sys_timer_get_ms();//重新计时
            }
            else if(play_time && play_time < (default_play_time - play_time_offset)){
                //播放时间太短，电压到了最后10%要先掉电量，确保最后一级播放时间
                if((percent_save / 10 * 10) <= (10 + vbat_value_to_percent[index].level)
                && bat_val >= (vbat_value_to_percent[index].upper - (vbat_value_to_percent[index].upper - vbat_value_to_percent[index].lower)/2) && bat_val >= vbat_value_to_percent[9].upper)
                {
                    //电压还比较大，不用掉电量
                    printf("************************d\n");
                    u8 temp = default_play_time - play_time;
                    if(temp >= 10){
                        temp = 9;//缺多少补多少，缺太多，先补9
                    }
                    trim_percent = percent_save / 10 * 10 + temp;
                    normal_power_update_time = NORMAL_POWER_PERCENT_UPDATE_TIME*7;
                    goto __power_ctrl_end;
                }
                else{
                    //电压已经很低了，必须掉电量
                    printf("************************e\n");
                    play_start_point_ms = sys_timer_get_ms();//重新计时
                }
            }
            else{
                //播放时间比较长的情况，可能音量比较小，不处理
                printf("************************f\n");
                prev_play_time = play_time;
                play_start_point_ms = sys_timer_get_ms();//重新计时
            }
            normal_power_update_time = NORMAL_POWER_PERCENT_UPDATE_TIME;
            total_play_time += play_time;
            total_play_cnt++;
            if(percent_save == 100 && abs(play_time - default_play_t_db) <= PLAY_TIME_OFFSET){
                printf("************************a\n");
                //100%的播放时间正常
                default_play_time = default_play_t_db;
                play_time_offset = PLAY_TIME_OFFSET;
            }
            else{
                printf("************************b\n");
                //非100%的播放时间或者100%的播放时间不正常
                default_play_time = total_play_time / total_play_cnt;
                play_time_offset = default_play_time / 8;
            }
        }
        else{
            //电量还没掉到10%
            //if(prev_play_time && abs(prev_play_time - default_play_time) <= play_time_offset){
                //上一次时间正常
                if(play_time >= (default_play_time + play_time_offset / 2)){
                    //这一次的时间偏长, 先掉10%电量
                    printf("************************g\n");
                    trim_percent = percent_save / 10 * 10 - 1;
                    prev_play_time = play_time;
                    total_play_time += play_time;
                    total_play_cnt++;
                    //掉电量再更新
                    // default_play_time = total_play_time / total_play_cnt;
                    // play_time_offset = default_play_time / 8;
                    play_start_point_ms = sys_timer_get_ms();//重新计时
                    normal_power_update_time = NORMAL_POWER_PERCENT_UPDATE_TIME;
                }
            //}
        }
    }
__power_ctrl_end:
    if(media_state){
        y_printf("power_ctrl %d %d %d %d %d %d\n", trim_percent, play_time, default_play_t_db, play_start_point_ms, default_play_time, total_play_time);
    }
    return trim_percent;
}

static u16 battery_calc_percent(u16 bat_val)
{
    bat_log("calc bat_val:%d", bat_val);
    u8 i = 0;
    static u8 percent = 0;
    static u32 time_ms = 0;
    u8 temp_percent = percent_save;
    if(bat_val >= vbat_value_to_percent[0].upper){
        percent = 100;
        user_charge_info.local_detail_percent = 100;
        i = 0;
    }
    else if(bat_val < vbat_value_to_percent[ARRAY_SIZE(vbat_value_to_percent) - 1].lower){
        percent = 10;
        user_charge_info.local_detail_percent = 0;
        i = ARRAY_SIZE(vbat_value_to_percent) - 1;
    }
    else{
        for (i = 0; i < ARRAY_SIZE(vbat_value_to_percent); i++)
        {
            if(bat_val >= vbat_value_to_percent[i].lower && bat_val < vbat_value_to_percent[i].upper){
                percent = vbat_value_to_percent[i].level;// + 9*(bat_val - vbat_value_to_percent[i].lower)/(vbat_value_to_percent[i].upper - vbat_value_to_percent[i].lower);
                user_charge_info.local_detail_percent = vbat_value_to_percent[i].level + 9*(bat_val - vbat_value_to_percent[i].lower)/(vbat_value_to_percent[i].upper - vbat_value_to_percent[i].lower);
                break;
            }
        }
    }

    if(percent > 100){
        percent = 100;
    }
    else if(percent < 10){
        percent = 10;
    }

    //首次开机，或者再次上电且非充电状态电量异常
    // bat_log("temp_percent:%d, is_power_up:%d, get_lvcmp_det():%d, sys_timer_get_ms():%d, percent:%d", temp_percent, is_power_up, get_lvcmp_det(), sys_timer_get_ms(), percent);
    if(temp_percent == 0xff 
        || (is_power_up && sys_timer_get_ms() < 2000 && get_lvcmp_det() == 0 
        && percent < temp_percent && abs((int)(temp_percent/10*10) - (int)(percent)) > 20))
    {
    // if(temp_percent == 0xff || (is_power_up && sys_timer_get_ms() < 2000))
    // {
        y_printf("temp_percent abnormal, reset!!!\n");
        temp_percent = percent;//第一次上电开机，重置电量
    }

    // bat_log("sys_timer_get_ms():%d, time_ms:%d", sys_timer_get_ms(), time_ms);
    if(get_lvcmp_det()){ //充电中
        if(sys_timer_get_ms() >= time_ms + CHARGING_POWER_PERCENT_UPDATE_TIME){
            time_ms = sys_timer_get_ms();
            if(temp_percent < percent){
                temp_percent++; //只允许变大
            }
            y_printf("++bat_val: %d --> percent:%d, temp_percent:%d, %d %d\n", bat_val, percent, temp_percent, percent_save, user_charge_info.local_detail_percent);
        }
        if(percent == 100 && temp_percent > 90 && bat_val >= 4342){
            temp_percent = 100;//避免跳100慢
        }
    }
    else{ //非充电中
        if(sys_timer_get_ms() >= time_ms + normal_power_update_time){
            time_ms = sys_timer_get_ms();
            u8 change = 0;
            if(temp_percent > percent){
                temp_percent--; //只允许变小
                change = (temp_percent % 10 == 9);
            }
            u16 temp = power_ctrl_deal(bat_val, i, change);
            switch (temp)
            {
            case 0:
                break;
            case (u16)-1:
                if(change){
                    temp_percent += 1;
                }
                break;
            default:
                temp_percent = temp;
                break;
            }

            bat_log("percent:%d, temp_percent:%d, a2dp_running:%d", percent, temp_percent, a2dp_player_runing());
            //保证10%报低电，以及优先保证10%的播放时间，实际电压到了10%，就直接降下去
            // if (percent == 10 && (temp_percent > 10 && temp_percent < 12) && a2dp_player_runing()) {
            if (percent == 10 && temp_percent == 10 && a2dp_player_runing()) {
                y_printf("--vbat is 10, change!!!");
                temp_percent = percent;
            }
            static u8 percent_t = 0;
            y_printf("--bat_val:%d --> percent:%d, temp_percent:%d, %d %d %d\n", bat_val, percent, temp_percent, percent_save, user_charge_info.local_detail_percent, percent_t);
            if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
                if (user_charge_info.local_detail_percent != percent_t) {
                    percent_t = user_charge_info.local_detail_percent;
                    tws_sync_bat_level();
                }
            }
        }
    }

    percent_save = temp_percent;

    if (percent_save > 100) {
        percent_save = 100;
    }

    return percent_save;
    // u8 i, tmp_percent;
    // u16 max, min, div_percent;
    // if (battery_curve_p == NULL) {
    //     log_error("battery_curve not init!!!\n");
    //     return 0;
    // }

    // for (i = 0; i < (battery_curve_max - 1); i++) {
    //     if (bat_val <= battery_curve_p[i].voltage) {
    //         return battery_curve_p[i].percent;
    //     }
    //     if (bat_val >= battery_curve_p[i + 1].voltage) {
    //         continue;
    //     }
    //     div_percent = battery_curve_p[i + 1].percent - battery_curve_p[i].percent;
    //     min = battery_curve_p[i].voltage;
    //     max = battery_curve_p[i + 1].voltage;
    //     tmp_percent = battery_curve_p[i].percent;
    //     tmp_percent += (bat_val - min) * div_percent / (max - min);
    //     return tmp_percent;
    // }
    // return battery_curve_p[battery_curve_max - 1].percent;
}

u16 get_vbat_value(void)
{
    return cur_battery_voltage;
}

u8 get_vbat_percent(void)
{
    return cur_battery_percent;
}

bool get_vbat_need_shutdown(void)
{   
    // bat_log("get_vbat_need_shutdown, %d, %d, %d", cur_battery_voltage, app_var.poweroff_tone_v, adc_check_vbat_lowpower());
    if ((cur_battery_voltage <= app_var.poweroff_tone_v) || adc_check_vbat_lowpower()) {
    // if ((cur_battery_voltage <= app_var.poweroff_tone_v + 100) || adc_check_vbat_lowpower()) {
        return TRUE;
    }
    return FALSE;
}

/*
 * 将当前电量转换为0~9级发送给手机同步电量
 * 电量:95 - 100 显示 100%
 * 电量:85 - 94  显示 90%
 */
u8  battery_value_to_phone_level(void)
{
    u8  battery_level = 0;
    u8 vbat_percent = get_vbat_percent();
    // if (vbat_percent < 5) { //小于5%电量等级为0，显示10%
    //     return 0;
    // }
    if (vbat_percent <= 10) { //小于10%电量等级为0，显示10%
        return 0;
    } else if (vbat_percent > 90) {   //显示100%
        return 9;
    }
    battery_level = vbat_percent / 10;
    // y_printf("battery_level = %d", battery_level);
    return battery_level;
}

//获取自身的电量
u8  get_self_battery_level(void)
{
    return cur_battery_level;
}

#if TCFG_USER_TWS_ENABLE
u8 get_cur_battery_level(void)
{
    u8 bat_lev = tws_sibling_bat_level & (~BIT(7));
    if (bat_lev == 0x7f) {
        return cur_battery_level;
    }

#if (CONFIG_DISPLAY_TWS_BAT_TYPE == CONFIG_DISPLAY_TWS_BAT_LOWER)
    return cur_battery_level < bat_lev ? cur_battery_level : bat_lev;
#elif (CONFIG_DISPLAY_TWS_BAT_TYPE == CONFIG_DISPLAY_TWS_BAT_HIGHER)
    return cur_battery_level < bat_lev ? bat_lev : cur_battery_level;
#elif (CONFIG_DISPLAY_TWS_BAT_TYPE == CONFIG_DISPLAY_TWS_BAT_LEFT)
    return tws_api_get_local_channel() == 'L' ? cur_battery_level : bat_lev;
#elif (CONFIG_DISPLAY_TWS_BAT_TYPE == CONFIG_DISPLAY_TWS_BAT_RIGHT)
    return tws_api_get_local_channel() == 'R' ? cur_battery_level : bat_lev;
#else
    return cur_battery_level;
#endif
}

#else

u8 get_cur_battery_level(void)
{
    return cur_battery_level;
}
#endif

void vbat_check_slow(void *priv)
{
    // bat_log("vbat_check_slow");
    if (vbat_fast_timer == 0) {
        vbat_fast_timer = usr_timer_add(NULL, vbat_check, VBAT_DETECT_TIME, 1);
    }
    if (get_charge_online_flag()) {
        // sys_timer_modify(vbat_slow_timer, VBAT_UPDATE_SLOW_TIME);
        sys_timer_modify(vbat_slow_timer, CHARGING_POWER_PERCENT_UPDATE_TIME);
    } else {
        sys_timer_modify(vbat_slow_timer, VBAT_UPDATE_FAST_TIME);
    }
}

void vbat_check_init(void)
{
    bat_log("vbat_check_init");
    bat_log("slow_timer:%d, fast_timer:%d", vbat_slow_timer, vbat_fast_timer);
    u8 tmp[128] = {0};
    int ret = 0, i;
    u16 battery_0, battery_100;
    union battery_data battery_data_t;

    //初始化电池曲线
//     if (battery_curve_p == NULL) {
//         memset(tmp, 0x00, sizeof(tmp));
//         ret = syscfg_read(CFG_BATTERY_CURVE_ID, tmp, sizeof(tmp));
//         if (ret > 0) {
//             battery_curve_max = ret / sizeof(battery_data_t);
//         } else {
//             battery_curve_max = 2;
//         }
//         battery_curve_p = malloc(battery_curve_max * sizeof(struct battery_curve));
//         ASSERT(battery_curve_p, "malloc battery_curve err!");
//         if (ret < 0) {
//             log_error("battery curve id, ret: %d\n", ret);
//             battery_0 = app_var.poweroff_tone_v;
// #if TCFG_CHARGE_ENABLE
//             //防止部分电池充不了这么高电量，充满显示未满的情况
//             battery_100 = (get_charge_full_value() - 100);
// #else
//             battery_100 = 4100;
// #endif
//             battery_curve_p[0].percent = 0;
//             battery_curve_p[0].voltage = battery_0;
//             battery_curve_p[1].percent = 100;
//             battery_curve_p[1].voltage = battery_100;
//             log_info("percent: %d, voltage: %d mV", 0, battery_curve_p[0].voltage);
//             log_info("percent: %d, voltage: %d mV", 100, battery_curve_p[1].voltage);
//         } else {
//             for (i = 0; i < battery_curve_max; i++) {
//                 memcpy(&battery_data_t.raw_data,
//                        &tmp[i * sizeof(battery_data_t)], sizeof(battery_data_t));
//                 battery_curve_p[i].percent = battery_data_t.data.percent;
//                 battery_curve_p[i].voltage = battery_data_t.data.voltage;
//                 log_info("percent: %d, voltage: %d mV\n",
//                          battery_curve_p[i].percent, battery_curve_p[i].voltage);
//             }
//         }
//         //初始化相关变量
//         cur_battery_voltage = get_vbat_voltage();
//         cur_battery_percent = battery_calc_percent(cur_battery_voltage);
//         cur_battery_level = battery_value_to_phone_level();
//     }

    if (vbat_slow_timer == 0) {
        vbat_slow_timer = sys_timer_add(NULL, vbat_check_slow, VBAT_UPDATE_FAST_TIME);
    } else {
        sys_timer_modify(vbat_slow_timer, VBAT_UPDATE_FAST_TIME);
    }

    if (vbat_fast_timer == 0) {
        vbat_fast_timer = usr_timer_add(NULL, vbat_check, VBAT_DETECT_TIME, 1);
    }
}

void vbat_timer_delete(void)
{
    if (vbat_slow_timer) {
        sys_timer_del(vbat_slow_timer);
        vbat_slow_timer = 0;
    }
    if (vbat_fast_timer) {
        usr_timer_del(vbat_fast_timer);
        vbat_fast_timer = 0;
    }
}

void vbat_check(void *priv)
{
    // bat_log("vbat_check");
    static u8 unit_cnt = 0;
    static u8 low_voice_cnt = 0;
    static u8 low_power_cnt = 0;
    static u8 power_normal_cnt = 0;
    static u8 charge_online_flag = 0;
    static u8 low_voice_first_flag = 1;//进入低电后先提醒一次
    static u32 bat_voltage = 0;
    u16 tmp_percent;

    bat_voltage += get_vbat_voltage();
    unit_cnt++;
    if (unit_cnt < VBAT_DETECT_CNT) {
        return;
    }
    unit_cnt = 0;

    //更新电池电压,以及电池百分比,还有电池等级
    cur_battery_voltage = bat_voltage / VBAT_DETECT_CNT;
    bat_voltage = 0;
    tmp_percent = battery_calc_percent(cur_battery_voltage);
    // bat_log("tmp_percent = %d", tmp_percent);

    cur_battery_percent = tmp_percent;
    cur_battery_level = battery_value_to_phone_level();

    /*log_info("cur_voltage: %d mV, tmp_percent: %d, cur_percent: %d, cur_level: %d\n",
             cur_battery_voltage, tmp_percent, cur_battery_percent, cur_battery_level);*/
    // bat_log("--%d, %d, %d", tmp_percent, cur_battery_percent, cur_battery_level);
    // bat_log("---%d, %d, %d, %d", cur_battery_voltage, app_var.warning_tone_v, low_voice_first_flag, low_voice_cnt);
    // bat_log("----%d, %d", get_charge_online_flag(), get_lvcmp_det());
    if (get_charge_online_flag() == 0) {
        if (adc_check_vbat_lowpower() ||
            (cur_battery_voltage <= app_var.poweroff_tone_v)) { //低电关机
            low_power_cnt++;
            low_voice_cnt = 0;
            power_normal_cnt = 0;
            cur_bat_st = VBAT_LOWPOWER;
            if (low_power_cnt > 6) {
                bat_log("\n*******Low Power,enter softpoweroff********\n");
                low_power_cnt = 0;
                batmgr_send_msg(POWER_EVENT_POWER_LOW, 0);
                usr_timer_del(vbat_fast_timer);
                vbat_fast_timer = 0;
            }
        } else if (cur_battery_voltage <= app_var.warning_tone_v) { //低电提醒
            low_voice_cnt ++;
            low_power_cnt = 0;
            power_normal_cnt = 0;
            cur_bat_st = VBAT_WARNING;
            if ((low_voice_first_flag && low_voice_cnt > 1) || //第一次进低电10s后报一次
                (!low_voice_first_flag && low_voice_cnt >= 5)) {
                low_voice_first_flag = 0;
                low_voice_cnt = 0;
                if (!lowpower_timer) {
                    bat_log("\n**Low Power,Please Charge Soon!!!**\n");
                    // batmgr_send_msg(POWER_EVENT_POWER_WARNING, 0);
                }
            }
        } else {
            power_normal_cnt++;
            low_voice_cnt = 0;
            low_power_cnt = 0;
            if (power_normal_cnt > 2) {
                if (cur_bat_st != VBAT_NORMAL) {
                    bat_log("[Noraml power]\n");
                    cur_bat_st = VBAT_NORMAL;
                    batmgr_send_msg(POWER_EVENT_POWER_NORMAL, 0);
                }
            }
        }
    } else {
        // batmgr_send_msg(POWER_EVENT_POWER_CHARGE, 0);
        if (get_lvcmp_det()) {
            batmgr_send_msg(POWER_EVENT_POWER_CHARGE, 0);
        } else {
            batmgr_send_msg(POWER_EVENT_POWER_CHANGE, 0);
        }
    }

    // bat_log("-- %d, %d, %d, %d, %d", cur_bat_st, cur_battery_level, old_battery_level, charge_online_flag, get_charge_online_flag());
    if (cur_bat_st != VBAT_LOWPOWER) {
        usr_timer_del(vbat_fast_timer);
        vbat_fast_timer = 0;
        //电量等级变化,或者在仓状态变化,交换电量
        if ((cur_battery_level != old_battery_level) ||
            (charge_online_flag != get_charge_online_flag())) {
                role_switch_check_delay(2000);
            if (cur_battery_level == 0) {
                if (get_charge_online_flag() == 0) {
                    bat_log("lowpower_timer = %d", lowpower_timer);
                    if (!lowpower_timer) {
                        y_printf("10 vbat, play tone delay!!!");
                        user_play_lowpower_tone_reset();
                    }
                }
            }
            batmgr_send_msg(POWER_EVENT_POWER_CHANGE, 0);
        }
        charge_online_flag =  get_charge_online_flag();
        old_battery_level = cur_battery_level;
    }
}

bool vbat_is_low_power(void)
{
    return (cur_bat_st != VBAT_NORMAL);
}

void check_power_on_voltage(void)
{
#if(TCFG_SYS_LVD_EN == 1)

    u16 val = 0;
    u8 normal_power_cnt = 0;
    u8 low_power_cnt = 0;

    while (1) {
        clr_wdt();
        val = get_vbat_voltage();
        printf("vbat: %d\n", val);
        if ((val < app_var.poweroff_tone_v) || adc_check_vbat_lowpower()) {
            low_power_cnt++;
            normal_power_cnt = 0;
            if (low_power_cnt > 10) {
                /* ui_update_status(STATUS_POWERON_LOWPOWER); */
                os_time_dly(100);
                log_info("power on low power , enter softpoweroff!\n");
                cur_bat_st = VBAT_LOWPOWER;
                power_set_soft_poweroff();
            }
        } else {
            normal_power_cnt++;
            low_power_cnt = 0;
            if (normal_power_cnt > 10) {
                vbat_check_init();
                return;
            }
        }
    }
#endif
}

void user_delay_play_lowpower_tone()
{
    if (user_delay_play_lowpower_tone_timerid != 0) {
        sys_timeout_del(user_delay_play_lowpower_tone_timerid);
        user_delay_play_lowpower_tone_timerid = 0;
    }
    batmgr_send_msg(POWER_EVENT_POWER_WARNING, 0);
}

void user_play_lowpower_tone_reset()
{
    if (user_delay_play_lowpower_tone_timerid != 0) {
        sys_timeout_del(user_delay_play_lowpower_tone_timerid);
        user_delay_play_lowpower_tone_timerid = 0;
    }
    bat_log("get_lvcmp_det():%d, cur_battery_level:%d", get_lvcmp_det(), cur_battery_level);
    if (get_lvcmp_det() == 0 && cur_battery_level == 0) {
        //重置一下报低电定时器
        if (lowpower_timer) {
            sys_timer_del(lowpower_timer);
            lowpower_timer = 0 ;
        }

        if (user_delay_play_lowpower_tone_timerid == 0) {
            user_delay_play_lowpower_tone_timerid = sys_timeout_add(NULL, user_delay_play_lowpower_tone, 13000);
        }
    }
}

u8 tws_power_level_cmp(void)
{
#if TCFG_USER_TWS_ENABLE
    u8 slave_lev = tws_sibling_bat_level & (~BIT(7));
    if(cur_battery_level > 4 && slave_lev > 4){ //都大于50%
        // return 0;
    }

    u8 master_level = cur_battery_level;
    // if(tws_api_get_role() == TWS_ROLE_SLAVE){
    //     master_level = slave_lev;
    //     slave_lev = cur_battery_level;
    // }
    if (slave_lev > master_level/* + 1*/){ //主机比从机小20% --> 10%
        printf("tws_power_level_cmp: slave_lev:%d > master_level:%d\n", slave_lev, master_level);
        return 1;
    }
    if (user_charge_info.remote_detail_percent > user_charge_info.local_detail_percent + 5){ //主机比从机小20% --> 10%
        printf("tws_power_level_cmp: remote_detail_percent:%d > local_detail:%d + 5\n", user_charge_info.remote_detail_percent, user_charge_info.local_detail_percent);
        return 1;
    }
#endif
    return 0;
}




