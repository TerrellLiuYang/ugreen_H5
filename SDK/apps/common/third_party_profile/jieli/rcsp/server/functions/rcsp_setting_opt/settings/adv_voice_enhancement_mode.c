#include "adv_voice_enhancement_mode.h"
#include "app_config.h"
#include "rcsp_adv_bluetooth.h"

#if (RCSP_ADV_ANC_VOICE && RCSP_ADV_WIND_NOISE_DETECTION)

/**
 * @brief 获取人声增强模式开关
 *
 * @result bool
 */
bool get_voice_enhancement_mode_switch()
{
    printf("===%s===", __FUNCTION__);
    return true;
}

/**
 * @brief 设置人声增强模式开关
 */
void set_voice_enhancement_mode_switch(bool mode_switch)
{
    printf("%s, switch:%d\n", __FUNCTION__, mode_switch);
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_VOICE_ENHANCEMENT_MODE, NULL, 0);
}

#endif
