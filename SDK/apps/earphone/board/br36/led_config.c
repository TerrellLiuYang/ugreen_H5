#include "app_config.h"
#include "system/init.h"
#include "led/led_ui_manager.h"

#if TCFG_PWMLED_ENABLE

int board_led_config()
{
    led_ui_manager_init();

    return 0;
}
platform_initcall(board_led_config);

#endif /* #if TCFG_PWMLED_ENABLE */
