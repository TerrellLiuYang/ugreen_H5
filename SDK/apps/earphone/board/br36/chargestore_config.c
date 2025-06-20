#include "app_config.h"
#include "asm/chargestore.h"
#include "asm/charge.h"
#include "gpio_config.h"
#include "system/init.h"

#if TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE || TCFG_ANC_BOX_ENABLE

static const struct chargestore_platform_data chargestore_data = {
    .io_port                = TCFG_CHARGESTORE_PORT,
    .baudrate               = 9600,
    .init                   = chargestore_init,
    .open                   = chargestore_open,
    .close                  = chargestore_close,
    .write                  = chargestore_write,
};

int board_chargestore_config()
{
    if (charge_get_log_board_flag() == 0) {
        chargestore_api_init(&chargestore_data);
    }
    return 0;
}
__initcall(board_chargestore_config);

#endif

