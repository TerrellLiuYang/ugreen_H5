#include "driver/clock.h"
#include "timer.h"
#include "asm/power/p33.h"
#include "asm/charge.h"
#include "driver/gpadc.h"
#include "uart.h"
#include "device/device.h"
#include "asm/power_interface.h"
#include "system/event.h"
#include "asm/efuse.h"
#include "gpio.h"
#include "app_config.h"
#include "asm/charge_calibration.h"
#include "asm/voltage.h"
#include "clock_manager/clock_manager.h"

#if TCFG_CHARGE_CALIBRATION_ENABLE

#define LOG_TAG_CONST   CHARGE
#define LOG_TAG         "[CHARGE]"
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"

//配置默认的校准区间范围在 (TCFG_CHARGE_MA + MAX_CUR_OFFSET) ~ (TCFG_CHARGE_MA - MIN_CUR_OFFSET)
#define MAX_CUR_OFFSET      5
#define MIN_CUR_OFFSET      5

static u32 calc_max_cur, calc_min_cur, now_current;
struct calibration_data {
    u8 cur_lev;//用于记录当前电流档位
    u8 cur_vbg;//用于记录当前VBG档位
    u8 cur_vbat;//用于记录当前VBAT档位
    u8 div_vbg;//用于计算vbg变化的档位
    u8 div_vbat;//用于计算vbat变化的档位
    u8 vbg_set;//用于保存vbg需要设置的档位
    u8 vbat_set;//用于保存VBT需要设置的档位
    u8 hv_mode;//用于保存HV需要设置的模式
    u32 div_curr;//用于计算两个档位之间的电流差值
    u32 cali_cur;//调整后预想的电流值
};

static void charge_fix_vbg_deg(u8 vbg_aim, u8 vbat_aim, u8 hv_mode)
{
    CHARGE_EN(0);
    CHGGO_EN(0);

    adc_suspend();
    voltage_api_set_mvbg(vbg_aim);
    adc_resuem();

    CHG_HV_MODE(hv_mode);
    CHARGE_FULL_V_SEL(vbat_aim);
    CHARGE_EN(1);
    CHGGO_EN(1);
}

static void charge_fix_vbg_inc(u8 vbg_aim, u8 vbat_aim, u8 hv_mode)
{
    CHARGE_EN(0);
    CHGGO_EN(0);

    adc_suspend();
    voltage_api_set_mvbg(vbg_aim);
    adc_resuem();

    CHG_HV_MODE(hv_mode);
    CHARGE_FULL_V_SEL(vbat_aim);
    CHARGE_EN(1);
    CHGGO_EN(1);
}

static u8 charge_calibration_vbat_inc(struct calibration_data *p)
{
    //增大VBAT档位时,舍弃小数位,防止超过4.2V
    p->div_vbat = (((u32)p->div_vbg * 15 * 4200) / 800) / 20;//20mV一个档位
    log_info("cur_vbat: %d, div_vbat: %d, hv_mode: %d\n", p->cur_vbat, p->div_vbat, IS_CHG_HV_MODE());
    if (IS_CHG_HV_MODE()) {
        p->hv_mode = 1;
        p->vbat_set = p->cur_vbat + p->div_vbat;
        if (p->vbat_set > 25) {
            log_info("111 too bigger: %d\n", p->vbat_set);
            return CALIBRATION_VBAT_ERR;
        }
        if (p->vbat_set >= 15) {
            p->vbat_set = 15;
        }
    } else {
        if (p->cur_vbat + p->div_vbat <= 15) {
            p->hv_mode = 0;
            p->vbat_set = p->cur_vbat + p->div_vbat;
        } else {
            p->hv_mode = 1;
            p->vbat_set = p->cur_vbat + p->div_vbat - 10;//hv模式相当于10档
            if (p->vbat_set > 25) {
                log_info("222 too bigger: %d\n", p->vbat_set);
                return CALIBRATION_VBAT_ERR;
            }
            if (p->vbat_set >= 15) {
                p->vbat_set = 15;
            }
        }
    }
    p->vbg_set = p->cur_vbg - p->div_vbg;
    log_info("vbg_set: %d, vbat_set: %d, hv_mode: %d\n", p->vbg_set, p->vbat_set, p->hv_mode);
    return CALIBRATION_CONTINUE;
}

static u8 charge_calibration_vbat_dec(struct calibration_data *p)
{
    //减小VBAT档位时,应该进位,防止超过4.2V
    p->div_vbat = (((u32)p->div_vbg * 15 * 4200) / 800 + 19) / 20;//20mV一个档位
    log_info("cur_vbat: %d, div_vbat: %d, hv_mode: %d\n", p->cur_vbat, p->div_vbat, IS_CHG_HV_MODE());
    if (p->div_vbat > p->cur_vbat) {
        if (!IS_CHG_HV_MODE()) {
            return CALIBRATION_VBAT_ERR;
        } else if ((p->cur_vbat + 10) < p->div_vbat) {
            return CALIBRATION_VBAT_ERR;
        }
        p->vbat_set = p->cur_vbat + 10 - p->div_vbat;
        p->hv_mode = 0;
    } else {
        p->vbat_set = p->cur_vbat - p->div_vbat;
        p->hv_mode = IS_CHG_HV_MODE();
    }
    p->vbg_set = p->cur_vbg + p->div_vbg;
    log_info("vbg_set: %d, vbat_set: %d, hv_mode: %d\n", p->vbg_set, p->vbat_set, p->hv_mode);
    return CALIBRATION_CONTINUE;
}

static u8 charge_calibration_bigger(u32 curr_err)
{
    u8 ret;
    struct calibration_data p;
    p.cur_lev = GET_CHARGE_mA_SEL();//获取当前的实时充电电流档位
    p.cur_vbg = MVBG_GET();//获取实时的VBG档位
    p.cur_vbat = GET_CHARGE_FULL_SET();//获取实时的VBAT档位

    log_info("[bigger] cur_lev: %d, cur_vbg: %d, curr_err: %d\n", p.cur_lev, p.cur_vbg, curr_err);

    //第一步判断是否减小档位
    if (p.cur_lev > CHARGE_mA_20) {
        p.div_curr = (get_charge_current_value(p.cur_lev) - get_charge_current_value(p.cur_lev - 1)) * 100;//电流减小一个档位的差值
        p.cali_cur = now_current - p.div_curr;
        log_info("cali_cur: %d\n", p.cali_cur);
        if ((p.cali_cur > calc_min_cur) && (p.cali_cur < calc_max_cur)) {
            //调整充电电流档位可能满足条件
            p.cur_lev = p.cur_lev - 1;;
            set_charge_mA(p.cur_lev);
            log_info("charge current deg 1\n");
            return CALIBRATION_CONTINUE;
        }
    }

    //第二步判断VBG是否够档位减小
    p.div_vbg = ((curr_err * MVBG_DEF_VOLT / MVBG_LEV_DIFF / get_charge_current_value(p.cur_lev)) + 99) / 100; //vbg一个档位对应15mV,调整的电流值为15mV/800mV*Icharge
    log_info("cur_vbg: %d, div_vbg: %d\n", p.cur_vbg, p.div_vbg);
    if (p.cur_vbg >= p.div_vbg) {
        //调整VBG可能可以满足条件,需要调整一堆电源及VBAT充满电压
        ret = charge_calibration_vbat_inc(&p);
        if (ret == CALIBRATION_CONTINUE) {
            charge_fix_vbg_deg(p.vbg_set, p.vbat_set, p.hv_mode);
            return CALIBRATION_CONTINUE;
        }
    }

    //第三步判断能够先减小一个档位电流再提高VBG档位
    if (p.cur_lev == CHARGE_mA_20) {
        return CALIBRATION_MA_ERR;
    } else {
        curr_err = calc_min_cur - (now_current - p.div_curr);
        p.cur_lev = p.cur_lev - 1;
        p.div_vbg = ((curr_err * MVBG_DEF_VOLT / MVBG_LEV_DIFF / get_charge_current_value(p.cur_lev)) + 99) / 100;
        log_info("curr_err: %d, div_vbg: %d\n", curr_err, p.div_vbg);
        if ((p.cur_vbg + p.div_vbg) <= 15) {
            ret = charge_calibration_vbat_dec(&p);
            if (ret == CALIBRATION_CONTINUE) {
                set_charge_mA(p.cur_lev);
                charge_fix_vbg_inc(p.vbg_set, p.vbat_set, p.hv_mode);
            }
        } else {
            return CALIBRATION_VBG_ERR;
        }
        return ret;
    }
}

static u8 charge_calibration_smaller(u32 curr_err)
{
    u8 ret;
    struct calibration_data p;
    p.cur_lev = GET_CHARGE_mA_SEL();//获取当前的实时充电电流档位
    p.cur_vbg = MVBG_GET();//获取实时的VBG档位
    p.cur_vbat = GET_CHARGE_FULL_SET();//获取实时的VBAT档位
    p.div_curr = (get_charge_current_value(p.cur_lev + 1) - get_charge_current_value(p.cur_lev)) * 100;//电流增加一个档位的差值
    p.hv_mode = IS_CHG_HV_MODE();

    log_info("[smaller] cur_lev: %d, cur_vbg: %d, div_curr: %d, curr_err: %d\n", p.cur_lev, p.cur_vbg, p.div_curr, curr_err);

    //第一步判断是否增加档位
    if (p.cur_lev < CHARGE_mA_220) {
        p.cali_cur = now_current + p.div_curr;
        log_info("cali_cur: %d\n", p.cali_cur);
        if ((p.cali_cur > calc_min_cur) && (p.cali_cur < calc_max_cur)) {
            //调整充电电流档位可能满足条件
            p.cur_lev = p.cur_lev + 1;
            set_charge_mA(p.cur_lev);
            log_info("charge current inc 1\n");
            return CALIBRATION_CONTINUE;
        }
    }

    //第二步判断VBG是否够档位增大
    p.div_vbg = ((curr_err * 800 / 15 / get_charge_current_value(p.cur_lev)) + 99) / 100; //vbg一个档位对应15mV,调整的电流值为15mV/800mV*Icharge
    log_info("curr_err: %d, div_vbg: %d\n", curr_err, p.div_vbg);
    if ((p.cur_vbg + p.div_vbg) <= 15) {
        //调整VBG可能可以满足条件,需要调整一堆电源及VBAT充满电压
        ret = charge_calibration_vbat_dec(&p);
        if (ret == CALIBRATION_CONTINUE) {
            charge_fix_vbg_inc(p.vbg_set, p.vbat_set, p.hv_mode);
            return ret;
        }
    }

    //第三步判断能够先增大一个档位电流再减小VBG档位
    if (p.cur_lev >= CHARGE_mA_220) {
        return CALIBRATION_MA_ERR;
    } else {
        curr_err = (now_current + p.div_curr) - calc_max_cur;
        p.cur_lev = p.cur_lev + 1;
        p.div_vbg = ((curr_err * 800 / 15 / get_charge_current_value(p.cur_lev)) + 99) / 100;
        log_info("curr_err: %d, div_vbg: %d\n", curr_err, p.div_vbg);
        if (p.cur_vbg >= p.div_vbg) {
            //调整VBG可能可以满足条件,需要调整一堆电源及VBAT充满电压
            charge_calibration_vbat_inc(&p);
            set_charge_mA(p.cur_lev);
            charge_fix_vbg_deg(p.vbg_set, p.vbat_set, p.hv_mode);
        } else {
            return CALIBRATION_VBG_ERR;
        }
    }
    return CALIBRATION_CONTINUE;
}

void charge_calibration_set_current_limit(u32 max_current, u32 min_current)
{
    ASSERT(max_current > min_current, "current limit set err, max: %d, min: %d\n", max_current, min_current);
    calc_max_cur = max_current;
    calc_min_cur = min_current;
}

u8 charge_calibration_report_current(u32 current)
{
    u8 ret = CALIBRATION_SUCC;
    now_current = current;

    //进入校准流程,时钟锁在24M -- 改流程不需要clock_unlock
    clock_lock("charge", 24000000L);

    if (calc_max_cur == 0) {
        calc_max_cur = (get_charge_current_value(get_charge_mA_config()) + MAX_CUR_OFFSET) * 100;
    }
    if (calc_min_cur == 0) {
        calc_max_cur = (get_charge_current_value(get_charge_mA_config()) - MIN_CUR_OFFSET) * 100;
    }
    log_info("max: %d, min: %d, now: %d\n", calc_max_cur, calc_min_cur, now_current);
    if (now_current > calc_max_cur) {
        ret = charge_calibration_bigger(now_current - calc_max_cur);
    } else if (now_current < calc_min_cur) {
        ret = charge_calibration_smaller(calc_min_cur - now_current);
    }
    return ret;
}

calibration_result charge_calibration_get_result(void)
{
    calibration_result res;
    res.vbg_lev = MVBG_GET();
    res.curr_lev = GET_CHARGE_mA_SEL();
    res.vbat_lev = GET_CHARGE_FULL_SET();
    res.hv_mode = IS_CHG_HV_MODE();
    return res;
}

#endif

