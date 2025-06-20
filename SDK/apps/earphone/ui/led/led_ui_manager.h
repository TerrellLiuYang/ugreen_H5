#ifndef __LED_UI_MANAGER_H__
#define __LED_UI_MANAGER_H__

#include "led_ui_cfg.h"
#include "asm/pwm_led_hw.h"

int led_ui_manager_init(void);

void led_ui_manager_display(u8 local_action, u16 uuid, u8 mode);

void led_ui_manager_display_by_name(u8 local_action, char *name, u8 mode);

void led_ui_do_display(u16 uuid, u8 sync);

u16 led_ui_state_config_get(struct led_state_config *config, u16 offset);

u16 led_ui_action_config_get(struct led_action *config, u16 offset);

u8 led_ui_hw_info_get_by_uuid(u16 index, struct led_hw_config *hw_config);

u8 led_ui_manager_status_busy_query(void);

u32 led_ui_JBHash(u8 *data, int len);


/* --------------------------------------------------------------------------*/
/**
 * @brief  按照index获取对应可视化配置的led_io，需要在led_config_file_init之后执行
 *
 * @param  [in] index 想要获取的第几个led_io，从0开始,传参0代表获取第一个io的值
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
u8 led_ui_hw_info_get_led_io(u8 index);

void led_ui_manager_enable(u8 en);

void led_ui_manager_set_wait(u8 wait);

u16 led_ui_get_current_uuid();

#define LED_TWS_SYNC    0       //sync
#define LED_LOCAL       1       //local

#define LED_MODE_C      0       //state change
#define LED_MODE_I      1       //insert
#define LED_MODE_R      2       //recover

#define LED_UUID_WHITE_ON                   0x5ab7    //常亮
#define LED_UUID_WHITE_OFF                  0x6020    //常灭
#define LED_UUID_WHITE_ON_1S                0x0876    //亮1秒
#define LED_UUID_WHITE_FAST_FLASH           0x9dd2    //快闪
#define LED_UUID_WHITE_SLOW_FLASH           0x49d0    //慢闪
#define LED_UUID_WHITE_FLASH_ONE_OF_3S      0x0f27    //3秒闪1下
#define LED_UUID_WHITE_1S_FLASH_THREE       0xc10e    //1秒闪3下


#endif /* #ifndef __LED_UI_MANAGER_H__ */
