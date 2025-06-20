#include "generic/typedef.h"
#include "asm/pwm_led_hw.h"

void led_ui_timer_add(void *handle, u32 timeout)
{
    /* r_printf("timeout: %d ms", timeout); */
    led_ui_hw_timer_add(timeout, handle, NULL);
}

void led_ui_timer_free(void)
{
    led_ui_hw_timer_delete();
}
