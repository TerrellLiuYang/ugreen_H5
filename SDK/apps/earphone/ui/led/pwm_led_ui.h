#ifndef __PWM_LED_UI_H__
#define __PWM_LED_UI_H__

int pwm_led_ui_period_action_display_try(u16 offset);

void pwm_led_ui_action_display(struct led_action *action, struct led_hw_config *hw_config);

#endif /* #ifndef __PWM_LED_UI_H__ */
