#include "asm/power_interface.h"
#include "gpadc.h"
#include "app_config.h"

extern void board_power_init();

void board_init()
{
    board_power_init();

    adc_init();

    pmu_trim(0, 0);

    pmu_trim_dump();
}
