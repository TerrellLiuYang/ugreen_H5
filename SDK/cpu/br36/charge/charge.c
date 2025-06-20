#include "timer.h"
#include "asm/power/p33.h"
#include "asm/charge.h"
#include "gpadc.h"
#include "asm/voltage.h"
#include "uart.h"
#include "device/device.h"
#include "asm/power_interface.h"
#include "system/event.h"
#include "asm/efuse.h"
#include "gpio.h"
#include "driver/clock.h"
#include "clock_manager/clock_manager.h"
#include "syscfg_id.h"
#include "app_config.h"
#include "app_msg.h"

#if TCFG_CHARGE_CALIBRATION_ENABLE
#include "asm/charge_calibration.h"
#endif

#define LOG_TAG_CONST   CHARGE
#define LOG_TAG         "[CHARGE]"
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"

extern USER_VAR user_var;

typedef struct _CHARGE_VAR {
    struct charge_platform_data *data;
    volatile u8 charge_online_flag;
    volatile u8 charge_event_flag;
    volatile u8 init_ok;
    volatile u8 detect_stop;
#if TCFG_DEBUG_UART_TX_PIN == IO_PORTP_00
    volatile u8 log_board_flag;
#endif
    volatile int ldo5v_timer;   //检测LDOIN状态变化的usr timer
    volatile int charge_timer;  //检测充电是否充满的usr timer
    volatile int cc_timer;      //涓流切恒流的sys timer
#if TCFG_CHARGE_CALIBRATION_ENABLE
    calibration_result result;
    s8 vbg_lvl;     //记录原始的VBG档位
    u8 result_flag; //校准数据有效标记
    u8 enter_flag;  //进入过模式标记
    u8 full_time;   //标记第几次触发充满
    u8 full_lev;    //满电档位记录
    u8 hv_mode;     //HV模式记录
#endif
} CHARGE_VAR;

#define __this 	(&charge_var)
static CHARGE_VAR charge_var;
static u8 charge_flag;

#define BIT_LDO5V_IN		BIT(0)
#define BIT_LDO5V_OFF		BIT(1)
#define BIT_LDO5V_KEEP		BIT(2)

static void charge_config(void);
extern struct charge_platform_data charge_data;
u8 get_charge_state(void)
{
    g_printf("get_charge_state: %d %d\n", LVCMP_DET_GET(), LDO5V_DET_GET());
    gpio_set_mode(IO_PORT_SPILT(IO_PORTB_09), PORT_OUTPUT_LOW);
    if (LVCMP_DET_GET()) {
        __this->data = &charge_data;
        charge_config();
        set_charge_mA(CHARGE_mA_50);
        CHARGE_EN(1);
        CHGGO_EN(1);
    }
    return LVCMP_DET_GET() || LDO5V_DET_GET();
}

u8 user_get_charge_flag()
{
    y_printf("user_get_charge_flag, %d", charge_flag);
    return charge_flag;
}

u8 get_charge_poweron_en(void)
{
    return __this->data->charge_poweron_en;
}

void set_charge_poweron_en(u32 onOff)
{
    __this->data->charge_poweron_en = onOff;
}

void charge_check_and_set_pinr(u8 level)
{
    u8 pinr_io, reg;
    reg = P33_CON_GET(P3_PINR_CON1);
    //开启LDO5V_DET长按复位
    if ((reg & BIT(0)) && ((reg & BIT(3)) == 0)) {
        if (level == 0) {
            P33_CON_SET(P3_PINR_CON1, 2, 1, 0);
        } else {
            P33_CON_SET(P3_PINR_CON1, 2, 1, 1);
        }
    }
}

static u8 check_charge_state(void)
{
    u8 online_cnt = 0;
    u8 i = 0;

    __this->charge_online_flag = 0;

    for (i = 0; i < 20; i++) {
        if (LVCMP_DET_GET() || LDO5V_DET_GET()) {
            online_cnt++;
        }
        udelay(1000);
    }
    log_info("online_cnt = %d\n", online_cnt);
    if (online_cnt > 5) {
        __this->charge_online_flag = 1;
    }

    return __this->charge_online_flag;
}

#define LOG_BOARD_DET_VOLT_MIN  300 //检测日志板在线的最小电压
#define LOG_BOARD_DET_VOLT_MAX  1500//检测日志板在线的最大电压
#define LOG_BOARD_VOLT_DIFF     100 //检测日志板过程中电压波动最大值
static void check_log_board_status(void)
{
#if TCFG_DEBUG_UART_TX_PIN == IO_PORTP_00
    u32 voltage_min, voltage_max, voltage_tmp;

    //开50K下拉电阻
    L5V_RES_DET_S_SEL(CHARGE_PULLDOWN_100K);
    L5V_LOAD_EN(1);

    //关闭PP0的串口输出功能,并把IO设置为输入
    gpio_disable_fun_output_port(IO_PORTP_00);
    gpio_direction_input(IO_PORTP_00);

    adc_add_sample_ch(AD_CH_LDO5V);

    mdelay(50);

    voltage_min = (u32) - 1;
    voltage_max = 0;
    for (int i = 0; i < 10; i++) {
        voltage_tmp = adc_get_voltage(AD_CH_LDO5V) * 4;
        if ((voltage_tmp < LOG_BOARD_DET_VOLT_MIN) || (voltage_tmp > LOG_BOARD_DET_VOLT_MAX)) {
            //电压不在范围,退出
            goto __det_exit;
        }
        voltage_min = (voltage_min > voltage_tmp) ? voltage_tmp : voltage_min;
        voltage_max = (voltage_max < voltage_tmp) ? voltage_tmp : voltage_max;
        if ((voltage_max - voltage_min) > LOG_BOARD_VOLT_DIFF) {
            //波动超出范围,退出
            goto __det_exit;
        }
        mdelay(100);
    }
    //检测到日志板,关闭LDOIN_DET以及LDOIN长按复位功能
    __this->log_board_flag = 1;
    power_awakeup_gpio_enable(IO_LDOIN_DET, 0);
    power_awakeup_gpio_enable(IO_VBTCH_DET, 0);
    P33_CON_SET(P3_PINR_CON1, 0, 8, 0);

    //恢复LDOIN打印信息
    gpio_direction_output(IO_PORTP_00, 1);
    gpio_set_fun_output_port(IO_PORTP_00, FO_UART0_TX, 1, 1);
__det_exit:
    adc_remove_sample_ch(AD_CH_LDO5V);
    L5V_LOAD_EN(0);
#endif
}

void set_charge_online_flag(u8 flag)
{
    __this->charge_online_flag = flag;
}

u8 get_charge_online_flag(void)
{
    return __this->charge_online_flag;
}

void set_charge_event_flag(u8 flag)
{
    __this->charge_event_flag = flag;
}

u8 charge_get_log_board_flag(void)
{
#if TCFG_DEBUG_UART_TX_PIN == IO_PORTP_00
    return __this->log_board_flag;
#else
    return 0;
#endif
}

u8 get_ldo5v_online_hw(void)
{
    return LDO5V_DET_GET();
}

u8 get_lvcmp_det(void)
{
    return LVCMP_DET_GET();
}

u8 get_ldo5v_pulldown_en(void)
{
    if (!__this->data) {
        return 0;
    }

#if TCFG_DEBUG_UART_TX_PIN == IO_PORTP_00
    //在日志板关机,使能唤醒
    if (__this->log_board_flag) {
        power_awakeup_gpio_enable(IO_LDOIN_DET, 1);
        power_awakeup_gpio_enable(IO_VBTCH_DET, 1);
        return 0;
    }
#endif

    if (get_ldo5v_online_hw()) {
        if (__this->data->ldo5v_pulldown_keep == 0) {
            return 0;
        }
    }
    return __this->data->ldo5v_pulldown_en;
}

u8 get_ldo5v_pulldown_res(void)
{
    if (__this->data) {
        return __this->data->ldo5v_pulldown_lvl;
    }
    return CHARGE_PULLDOWN_200K;
}

extern void charge_event_to_user(u8 event);
/*void charge_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_CHARGE;
    e.u.dev.event = event;
    e.u.dev.value = 0;
    sys_event_notify(&e);
}*/

#if TCFG_CHARGE_CALIBRATION_ENABLE

//恒流模式才需要校准电流
static void charge_enter_calibration_mode(void *priv)
{
    u8 is_charging;
    if ((!__this->result_flag) || __this->enter_flag) {
        return;
    }

    //切系统时钟24M并锁时钟
    clock_lock("charge", 24000000L);

    __this->enter_flag = 1;
    is_charging = IS_CHARGE_EN();
    //先关闭充电
    CHARGE_EN(0);
    CHGGO_EN(0);

    __this->full_lev = GET_CHARGE_FULL_SET();
    __this->hv_mode = IS_CHG_HV_MODE();

    adc_suspend();
    __this->vbg_lvl = voltage_api_set_mvbg(__this->result.vbg_lev);
    adc_resuem();

    //根据充电电流设置对应第一次判满的档位
    if (__this->result.curr_lev == CHARGE_mA_20) {
        CHARGE_FULL_mA_SEL(CHARGE_FULL_mA_15);
    } else if (__this->result.curr_lev == CHARGE_mA_30) {
        CHARGE_FULL_mA_SEL(CHARGE_FULL_mA_25);
    } else {
        CHARGE_FULL_mA_SEL(CHARGE_FULL_mA_30);
    }
    set_charge_mA(__this->result.curr_lev);
    CHG_HV_MODE(__this->result.hv_mode);
    CHARGE_FULL_V_SEL(__this->result.vbat_lev);

    __this->full_time = 0;
    //再打开充电
    if (is_charging) {
        CHARGE_EN(1);
        CHGGO_EN(1);
    }
}

static void charge_exit_calibration_mode(void *priv)
{
    u8 is_charging;
    if ((!__this->result_flag) || (!__this->enter_flag)) {
        return;
    }

    is_charging = IS_CHARGE_EN();
    //先关闭充电
    CHARGE_EN(0);
    CHGGO_EN(0);

    adc_suspend();
    voltage_api_set_mvbg(__this->vbg_lvl);
    adc_resuem();

    //1、减小了一个充电档位,充电电流应该设置为(配置值-1档)
    //2、增大了一个充电档位,充电电流应该设置为(配置档)
    //3、只是增大了VBG,充电电流应该设置为(配置档)
    //4、只是减小了VBG,充电电流应该设置为(配置档-1)
    CHARGE_FULL_mA_SEL(__this->data->charge_full_mA);
    CHG_HV_MODE(__this->hv_mode);
    CHARGE_FULL_V_SEL(__this->full_lev);
    if (__this->data->charge_mA > 0) {
        if (__this->result.curr_lev < __this->data->charge_mA) {
            set_charge_mA(__this->data->charge_mA - 1);
        } else if (__this->result.curr_lev > __this->data->charge_mA) {
            set_charge_mA(__this->data->charge_mA);
        } else {
            if (__this->result.vbg_lev > __this->vbg_lvl) {
                set_charge_mA(__this->data->charge_mA);
            } else {
                set_charge_mA(__this->data->charge_mA - 1);
            }
        }
    } else {
        set_charge_mA(__this->data->charge_mA);
    }

    //再打开充电
    if (is_charging) {
        CHARGE_EN(1);
        CHGGO_EN(1);
        if ((int)priv == 1) {
            power_awakeup_gpio_enable(IO_CHGFL_DET, 1);
            charge_wakeup_isr();
        }
    }

    __this->enter_flag = 0;

    //解锁时钟
    clock_unlock("charge");
}

#endif

static void charge_cc_check(void *priv)
{
    if ((adc_get_voltage(AD_CH_PMU_VBAT) * 4 / 10) > CHARGE_CCVOL_V) {
        usr_timer_del(__this->cc_timer);
        __this->cc_timer = 0;
#if TCFG_CHARGE_CALIBRATION_ENABLE
        int msg[4];
        msg[0] = (int)charge_enter_calibration_mode;
        msg[1] = 1;
        msg[2] = (int)1;
        os_taskq_post_type((char *)"app_core", Q_CALLBACK, 3, msg);
#else
        set_charge_mA(__this->data->charge_mA);
        power_awakeup_gpio_enable(IO_CHGFL_DET, 1);
        charge_wakeup_isr();
#endif
    }
}

void charge_start(void)
{
    u8 check_full = 0;
    log_info("%s\n", __func__);

    if (__this->charge_timer) {
        usr_timer_del(__this->charge_timer);
        __this->charge_timer = 0;
    }

    //进入恒流充电之后,才开启充满检测
    if ((adc_get_voltage(AD_CH_PMU_VBAT) * 4 / 10) > CHARGE_CCVOL_V) {
        check_full = 1;
#if TCFG_CHARGE_CALIBRATION_ENABLE
        charge_enter_calibration_mode(NULL);
#else
        set_charge_mA(__this->data->charge_mA);
#endif
        power_awakeup_gpio_enable(IO_CHGFL_DET, 1);
    } else {
        //涓流阶段系统不进入低功耗,防止电池电量更新过慢导致涓流切恒流时间过长
        set_charge_mA(__this->data->charge_trickle_mA);
        if (!__this->cc_timer) {
            __this->cc_timer = usr_timer_add(NULL, charge_cc_check, 1000, 1);
        }
    }

    CHARGE_EN(1);
    CHGGO_EN(1);

    charge_event_to_user(CHARGE_EVENT_CHARGE_START);

    //开启充电时,充满标记为1时,主动检测一次是否充满
    if (check_full && CHARGE_FULL_FLAG_GET()) {
        charge_wakeup_isr();
    }
}

void charge_close(void)
{
    log_info("%s\n", __func__);

#if TCFG_CHARGE_CALIBRATION_ENABLE
    charge_exit_calibration_mode(NULL);
#endif

    CHARGE_EN(0);
    CHGGO_EN(0);

    power_awakeup_gpio_enable(IO_CHGFL_DET, 0);

    charge_event_to_user(CHARGE_EVENT_CHARGE_CLOSE);

    if (__this->charge_timer) {
        usr_timer_del(__this->charge_timer);
        __this->charge_timer = 0;
    }
    if (__this->cc_timer) {
        usr_timer_del(__this->cc_timer);
        __this->cc_timer = 0;
    }
}

static void charge_full_detect(void *priv)
{
    int msg[4];
    static u16 charge_full_cnt = 0;

    if (CHARGE_FULL_FILTER_GET()) {
        // putchar('F');
        if (CHARGE_FULL_FLAG_GET() && LVCMP_DET_GET()) {
            putchar('1');
            if (charge_full_cnt < 10) {
                charge_full_cnt++;
            } else {
                charge_full_cnt = 0;
#if TCFG_CHARGE_CALIBRATION_ENABLE
                if (__this->result_flag) {
                    if (__this->full_time == 0) {
                        //第一次判满退出校准模式
                        __this->full_time = 1;
                        msg[0] = (int)charge_exit_calibration_mode;
                        msg[1] = 1;
                        msg[2] = (int)NULL;
                        os_taskq_post_type((char *)"app_core", Q_CALLBACK, 3, msg);
                        return;
                    } else if (__this->enter_flag == 1) {
                        //没有退出校准模式也不能往下走
                        return;
                    }
                }
#endif
                power_awakeup_gpio_enable(IO_CHGFL_DET, 0);
                usr_timer_del(__this->charge_timer);
                __this->charge_timer = 0;
                charge_event_to_user(CHARGE_EVENT_CHARGE_FULL);
            }
        } else {
            // putchar('0');
            charge_full_cnt = 0;
        }
    } else {
        putchar('K');
        charge_full_cnt = 0;
        usr_timer_del(__this->charge_timer);
        __this->charge_timer = 0;
    }
}

static void ldo5v_detect(void *priv)
{
    /* log_info("%s\n",__func__); */

    static u16 ldo5v_on_cnt = 0;
    static u16 ldo5v_keep_cnt = 0;
    static u16 ldo5v_off_cnt = 0;

    if (__this->detect_stop) {
        return;
    }

    if (LVCMP_DET_GET()) {	//ldoin > vbat
        putchar('X');
        if (ldo5v_on_cnt < __this->data->ldo5v_on_filter) {
            ldo5v_on_cnt++;
        } else {
            // g_printf("ldo5V_IN\n");
            // if (user_var.is_ota_mode) {
            //     g_printf("ota now, no enter charge mode, return!!!\n");
            //     ldo5v_on_cnt = 0;
            //     return;
            // }
            set_charge_online_flag(1);
            ldo5v_off_cnt = 0;
            ldo5v_keep_cnt = 0;
            //消息线程未准备好接收消息,继续扫描
            // y_printf("__this->charge_event_flag = %d", __this->charge_event_flag);
            if (__this->charge_event_flag == 0) {
                return;
            }
            ldo5v_on_cnt = 0;
            usr_timer_del(__this->ldo5v_timer);
            __this->ldo5v_timer = 0;
            // y_printf("(charge_flag & BIT_LDO5V_IN) = %d", (charge_flag & BIT_LDO5V_IN));
            if ((charge_flag & BIT_LDO5V_IN) == 0) {
                charge_flag = BIT_LDO5V_IN;
                charge_event_to_user(CHARGE_EVENT_LDO5V_IN);
            }
        }
    } else if (LDO5V_DET_GET() == 0) {	//ldoin<拔出电压（0.6）
        putchar('Q');
        if (ldo5v_off_cnt < (__this->data->ldo5v_off_filter + 20)) {
            ldo5v_off_cnt++;
        } else {
            // g_printf("ldo5V_OFF\n");
            set_charge_online_flag(0);
            ldo5v_on_cnt = 0;
            ldo5v_keep_cnt = 0;
            //消息线程未准备好接收消息,继续扫描
            // y_printf("__this->charge_event_flag = %d", __this->charge_event_flag);
            if (__this->charge_event_flag == 0) {
                return;
            }
            ldo5v_off_cnt = 0;
            usr_timer_del(__this->ldo5v_timer);
            __this->ldo5v_timer = 0;
            // y_printf("(charge_flag & BIT_LDO5V_OFF) = %d", (charge_flag & BIT_LDO5V_OFF));
            if ((charge_flag & BIT_LDO5V_OFF) == 0) {
                charge_flag = BIT_LDO5V_OFF;
                charge_event_to_user(CHARGE_EVENT_LDO5V_OFF);
            }
        }
    } else {	//拔出电压（0.6左右）< ldoin < vbat
        putchar('E');
        if (ldo5v_keep_cnt < __this->data->ldo5v_keep_filter) {
            ldo5v_keep_cnt++;
        } else {
            // g_printf("ldo5V_ERR\n");
            set_charge_online_flag(1);
            ldo5v_off_cnt = 0;
            ldo5v_on_cnt = 0;
            //消息线程未准备好接收消息,继续扫描
            // y_printf("__this->charge_event_flag = %d", __this->charge_event_flag);
            if (__this->charge_event_flag == 0) {
                return;
            }
            ldo5v_keep_cnt = 0;
            usr_timer_del(__this->ldo5v_timer);
            __this->ldo5v_timer = 0;
            // y_printf("charge_flag & BIT_LDO5V_KEEP) = %d", (charge_flag & BIT_LDO5V_KEEP));
            if ((charge_flag & BIT_LDO5V_KEEP) == 0) {
                charge_flag = BIT_LDO5V_KEEP;
                if (__this->data->ldo5v_off_filter) {
                    charge_event_to_user(CHARGE_EVENT_LDO5V_KEEP);
                }
            }
        }
    }
}

void ldoin_wakeup_isr(void)
{
    if (!__this->init_ok) {
        return;
    }
    /* printf(" %s \n", __func__); */
    if (__this->ldo5v_timer == 0) {
        __this->ldo5v_timer = usr_timer_add(0, ldo5v_detect, 2, 1);
    }
}

void charge_wakeup_isr(void)
{
    if (!__this->init_ok) {
        return;
    }
    /* printf(" %s \n", __func__); */
    if (__this->charge_timer == 0) {
        __this->charge_timer = usr_timer_add(0, charge_full_detect, 2, 1);
    }
}

void charge_set_ldo5v_detect_stop(u8 stop)
{
    // y_printf("charge_set_ldo5v_detect_stop %d\n",stop);
    __this->detect_stop = stop;
}
int charge_get_ldo5v_detect_stop(void)
{
    return __this->detect_stop;
}
u8 get_charge_mA_config(void)
{
    return __this->data->charge_mA;
}

void set_charge_mA(u8 charge_mA)
{
    static u8 charge_mA_old = 0xff;
    if (charge_mA_old != charge_mA) {
        charge_mA_old = charge_mA;
        CHARGE_mA_SEL(charge_mA);
    }
}

const u16 full_table[CHARGE_FULL_V_MAX] = {
    4041, 4061, 4081, 4101, 4119, 4139, 4159, 4179,
    4199, 4219, 4238, 4258, 4278, 4298, 4318, 4338,
    4237, 4257, 4275, 4295, 4315, 4335, 4354, 4374,
    4394, 4414, 4434, 4454, 4474, 4494, 4514, 4534,
};
u16 get_charge_full_value(void)
{
    ASSERT(__this->init_ok, "charge not init ok!\n");
    ASSERT(__this->data->charge_full_V < CHARGE_FULL_V_MAX);
    return full_table[__this->data->charge_full_V];
}

const u16 current_table[CHARGE_mA_MAX] = {
    20,  30,  40,  50,  60,  70,  80,  90,
    100, 110, 120, 140, 160, 180, 200, 220,
};
u16 get_charge_current_value(u8 cur_lvl)
{
    ASSERT(__this->data->charge_mA < CHARGE_mA_MAX);
    return current_table[cur_lvl];
}

static void charge_config(void)
{
    u8 charge_trim_val;
    u8 offset = 0;
    u8 charge_full_v_val = 0;
    //判断是用高压档还是低压档
    if (__this->data->charge_full_V < CHARGE_FULL_V_4237) {
        CHG_HV_MODE(0);
        charge_trim_val = efuse_get_vbat_trim_420();//4.20V对应的trim出来的实际档位
        if (charge_trim_val == 0xf) {
            y_printf("vbat low not trim, use default config!!!!!!");
            charge_trim_val = CHARGE_FULL_V_4199;
        }
        y_printf("low charge_trim_val = %d\n", charge_trim_val);
        if (__this->data->charge_full_V >= CHARGE_FULL_V_4199) {
            offset = __this->data->charge_full_V - CHARGE_FULL_V_4199;
            charge_full_v_val = charge_trim_val + offset;
            if (charge_full_v_val > 0xf) {
                charge_full_v_val = 0xf;
            }
        } else {
            offset = CHARGE_FULL_V_4199 - __this->data->charge_full_V;
            if (charge_trim_val >= offset) {
                charge_full_v_val = charge_trim_val - offset;
            } else {
                charge_full_v_val = 0;
            }
        }
    } else {
        CHG_HV_MODE(1);
        charge_trim_val = efuse_get_vbat_trim_435();//4.35V对应的trim出来的实际档位
        if (charge_trim_val == 0xf) {
            y_printf("vbat high not trim, use default config!!!!!!");
            charge_trim_val = CHARGE_FULL_V_4354 - 16;
        }
        y_printf("high charge_trim_val = %d\n", charge_trim_val);
        if (__this->data->charge_full_V >= CHARGE_FULL_V_4354) {
            offset = __this->data->charge_full_V - CHARGE_FULL_V_4354;
            charge_full_v_val = charge_trim_val + offset;
            if (charge_full_v_val > 0xf) {
                charge_full_v_val = 0xf;
            }
        } else {
            offset = CHARGE_FULL_V_4354 - __this->data->charge_full_V;
            if (charge_trim_val >= offset) {
                charge_full_v_val = charge_trim_val - offset;
            } else {
                charge_full_v_val = 0;
            }
        }
    }

    y_printf("charge_full_v_val = %d\n", charge_full_v_val);
    // log_info("charge_full_v_val = %d\n", charge_full_v_val);

    CHARGE_FULL_V_SEL(charge_full_v_val);
    CHARGE_FULL_mA_SEL(__this->data->charge_full_mA);
    /* CHARGE_mA_SEL(__this->data->charge_mA); */
    /* CHARGE_mA_SEL(CHARGE_mA_20); */
    CHARGE_mA_SEL(__this->data->charge_trickle_mA);
}


int charge_init(struct charge_platform_data *data)
{
    log_info("%s\n", __func__);

    __this->data = data;

    ASSERT(__this->data);

    __this->init_ok = 0;
    __this->charge_online_flag = 0;

    check_log_board_status();

    /*先关闭充电使能，后面检测到充电插入再开启*/
    power_awakeup_gpio_enable(IO_CHGFL_DET, 0);
    // CHARGE_EN(0);
    if (LVCMP_DET_GET()) {
        CHARGE_EN(1);//开机有5V，不关充电，避免唤不醒锂保
    } else {
        CHARGE_EN(0);
    }

    /*LDO5V的100K下拉电阻使能*/
    L5V_RES_DET_S_SEL(__this->data->ldo5v_pulldown_lvl);
    L5V_LOAD_EN(__this->data->ldo5v_pulldown_en);

    charge_config();

    if (charge_get_log_board_flag()) {
        goto __charge_end;
    }

    if (check_charge_state()) {
        if (__this->ldo5v_timer == 0) {
            __this->ldo5v_timer = usr_timer_add(0, ldo5v_detect, 2, 1);
        }
    } else {
        charge_flag = BIT_LDO5V_OFF;
    }

#if TCFG_CHARGE_CALIBRATION_ENABLE
    int ret = syscfg_read(VM_CHARGE_CALIBRATION, (void *)&__this->result, sizeof(calibration_result));
    if (ret == sizeof(calibration_result)) {
        __this->result_flag = 1;
        log_info("calibration result:\n");
        log_info_hexdump((u8 *)&__this->result, sizeof(calibration_result));
        if (__this->result.vbg_lev == MVBG_GET()) {
            //VBG一样则在充电中不需要调各种电压
            __this->result_flag = 0;
            log_info("update charge current lev: %d, %d\n", __this->data->charge_mA, __this->result.curr_lev);
            __this->data->charge_mA = __this->result.curr_lev;
        }
    } else {
        log_info("not calibration\n");
    }
#endif

__charge_end:
    __this->init_ok = 1;
    gpio_set_mode(IO_PORT_SPILT(IO_PORTB_09), PORT_OUTPUT_LOW); 

    return 0;
}

void charge_module_stop(void)
{
    if (!__this->init_ok) {
        return;
    }
    y_printf("charge_module_stop\n");
    charge_close();
    power_awakeup_gpio_enable(IO_LDOIN_DET, 0);
    power_awakeup_gpio_enable(IO_VBTCH_DET, 0);
    if (__this->ldo5v_timer) {
        usr_timer_del(__this->ldo5v_timer);
        __this->ldo5v_timer = 0;
    }
}

void charge_module_restart(void)
{
    y_printf("charge_module_restart\n");
    if (!__this->init_ok) {
        return;
    }
    if (!__this->ldo5v_timer) {
        __this->ldo5v_timer = usr_timer_add(NULL, ldo5v_detect, 2, 1);
    }

    power_awakeup_gpio_enable(IO_LDOIN_DET, 1);
    power_awakeup_gpio_enable(IO_VBTCH_DET, 1);
}

int get_5vtimer(void)
{
    return __this->ldo5v_timer;
}