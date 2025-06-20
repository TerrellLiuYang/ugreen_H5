#ifndef APP_POWER_MANAGE_H
#define APP_POWER_MANAGE_H

#include "typedef.h"
#include "system/event.h"

#define LOW_POWER_WARN_TIME   	(20 * 60 * 1000)  //低电提醒时间

void check_power_on_voltage(void);
u16 get_vbat_value(void);
u8 get_vbat_percent(void);
void vbat_check_init(void);
void vbat_timer_update(u32 msec);
void vbat_timer_delete(void);
void tws_sync_bat_level(void);
u8 get_tws_sibling_bat_level(void);
u8 get_tws_sibling_bat_persent(void);
bool get_vbat_need_shutdown(void);
u8  get_self_battery_level(void);

void app_power_set_tws_sibling_bat_level(u8 vbat, u8 percent);

#endif

