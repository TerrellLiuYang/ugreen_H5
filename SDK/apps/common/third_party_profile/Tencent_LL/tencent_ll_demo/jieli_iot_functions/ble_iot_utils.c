#include "ble_iot_utils.h"
#include "syscfg_id.h"
#include "app_config.h"

#if (BT_AI_SEL_PROTOCOL == LL_SYNC_EN)

/**
 * @brief 获取vm数据
 *
 * @param buf
 * @param buf_len
 * @result 0
 */
u8 iot_read_data_from_vm(u8 syscfg_id, u8 *buf, u8 buf_len)
{
    int len = 0;
    u8 i = 0;

    len = syscfg_read(syscfg_id, buf, buf_len);
    if (buf) {
        put_buf(buf, buf_len);
    }

    if (len > 0) {
        for (i = 0; i < buf_len; i++) {
            if (buf[i] != 0xff) {
                return (buf_len == len);
            }
        }
    }

    return 0;
}

#endif // (BT_AI_SEL_PROTOCOL == LL_SYNC_EN)

