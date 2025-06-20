#include "app_config.h"
#include "app_tone.h"
#include "fs/resfile.h"
#include "app_msg.h"
#include "init.h"

#include "classic/tws_api.h"
extern APP_INFO user_app_info;


static const struct tone_files english_tone_files = {
    .num = {
        "tone_en/0.*",
        "tone_en/1.*",
        "tone_en/2.*",
        "tone_en/3.*",
        "tone_en/4.*",
        "tone_en/5.*",
        "tone_en/6.*",
        "tone_en/7.*",
        "tone_en/8.*",
        "tone_en/9.*",
    },
    .power_on           = "tone_en/power_on_e.*",
    .power_off          = "tone_en/power_off_e.*",
    .bt_mode            = "tone_en/bt.*",
    .bt_connect         = "tone_en/bt_conn_e.*",
    .bt_disconnect      = "tone_en/bt_dconn_e.*",
    .phone_in           = "tone_en/ring.*",
    .phone_out          = "tone_en/ring.*",
    .low_power          = "tone_en/low_power_e.*",
    .max_vol            = "tone_en/vol_max.*",
    .tws_connect        = "tone_en/tws_conn.*",
    .tws_disconnect     = "tone_en/tws_dconn.*",
    .normal             = "tone_en/normal.*",
    .low_latency_in     = "tone_en/game_in_e.*",
    .low_latency_out    = "tone_en/game_out_e.*",
    .anc_on    			= "tone_en/anc_on.*",
    .anc_trans    		= "tone_en/anc_trans.*",
    .anc_off    		= "tone_en/anc_off.*",
    .key_tone  		    = "tone_en/key_tone.*",
    .pairing  		    = "tone_en/pairing_e.*",
    .tone_sw  		    = "tone_en/tone_sw.*",
    .buzzer  		    = "tone_en/buzzer.*",
    .eq_sw  		    = "tone_en/eq_sw.*",
    .anc_adaptive       = "tone_en/adaptive.*",
    .anc_adaptive_coeff = "tone_en/anc_on.*",
    .anc_normal_coeff   = "tone_en/anc_on.*",
    .spkchat_on         = "tone_en/spkchat_on.*",
    .spkchat_off        = "tone_en/spkchat_off.*",
    .winddet_on         = "tone_en/winddet_on.*",
    .winddet_off        = "tone_en/winddet_off.*",
    .wclick_on          = "tone_en/wclick_on.*",
    .wclick_off         = "tone_en/wclick_off.*",
};

static const struct tone_files chinese_tone_files = {
    .num = {
        "tone_en/0.*",
        "tone_en/1.*",
        "tone_en/2.*",
        "tone_en/3.*",
        "tone_en/4.*",
        "tone_en/5.*",
        "tone_en/6.*",
        "tone_en/7.*",
        "tone_en/8.*",
        "tone_en/9.*",
    },
    .power_on           = "tone_en/power_on_z.*",
    .power_off          = "tone_en/power_off_z.*",
    .bt_mode            = "tone_en/bt.*",
    .bt_connect         = "tone_en/bt_conn_z.*",
    .bt_disconnect      = "tone_en/bt_dconn_z.*",
    .phone_in           = "tone_en/ring.*",
    .phone_out          = "tone_en/ring.*",
    .low_power          = "tone_en/low_power_z.*",
    .max_vol            = "tone_en/vol_max.*",
    .tws_connect        = "tone_en/tws_conn.*",
    .tws_disconnect     = "tone_en/tws_dconn.*",
    .normal             = "tone_en/normal.*",
    .low_latency_in     = "tone_en/game_in_z.*",
    .low_latency_out    = "tone_en/game_out_z.*",
    .anc_on    			= "tone_en/anc_on.*",
    .anc_trans    		= "tone_en/anc_trans.*",
    .anc_off    		= "tone_en/anc_off.*",
    .key_tone  		    = "tone_en/key_tone.*",
    .pairing  		    = "tone_en/pairing_z.*",
    .tone_sw 		    = "tone_en/tone_sw.*",
    .buzzer  		    = "tone_en/buzzer.*",
    .eq_sw  		    = "tone_en/eq_sw.*",
    .anc_adaptive       = "tone_en/adaptive.*",
    .anc_adaptive_coeff = "tone_en/anc_on.*",
    .anc_normal_coeff   = "tone_en/anc_on.*",
    .spkchat_on         = "tone_en/spkchat_on.*",
    .spkchat_off        = "tone_en/spkchat_off.*",
    .winddet_on         = "tone_en/winddet_on.*",
    .winddet_off        = "tone_en/winddet_off.*",
    .wclick_on          = "tone_en/wclick_on.*",
    .wclick_off         = "tone_en/wclick_off.*",
};

static const struct tone_files ring_tone_files = {
    .num = {
        "tone_en/0.*",
        "tone_en/1.*",
        "tone_en/2.*",
        "tone_en/3.*",
        "tone_en/4.*",
        "tone_en/5.*",
        "tone_en/6.*",
        "tone_en/7.*",
        "tone_en/8.*",
        "tone_en/9.*",
    },
    .power_on           = "tone_en/power_on_r.*",
    .power_off          = "tone_en/power_off_r.*",
    .bt_mode            = "tone_en/bt.*",
    .bt_connect         = "tone_en/bt_conn_r.*",
    .bt_disconnect      = "tone_en/bt_dconn_r.*",
    .phone_in           = "tone_en/ring.*",
    .phone_out          = "tone_en/ring.*",
    .low_power          = "tone_en/low_power_r.*",
    .max_vol            = "tone_en/vol_max.*",
    .tws_connect        = "tone_en/tws_conn.*",
    .tws_disconnect     = "tone_en/tws_dconn.*",
    .normal             = "tone_en/normal.*",
    .low_latency_in     = "tone_en/game_in_r.*",
    .low_latency_out    = "tone_en/game_out_r.*",
    .anc_on    			= "tone_en/anc_on.*",
    .anc_trans    		= "tone_en/anc_trans.*",
    .anc_off    		= "tone_en/anc_off.*",
    .key_tone  		    = "tone_en/key_tone.*",
    .pairing  		    = "tone_en/pairing_r.*",
    .tone_sw 		    = "tone_en/tone_sw.*",
    .buzzer  		    = "tone_en/buzzer.*",
    .eq_sw  		    = "tone_en/eq_sw.*",
    .anc_adaptive       = "tone_en/adaptive.*",
    .anc_adaptive_coeff = "tone_en/anc_on.*",
    .anc_normal_coeff   = "tone_en/anc_on.*",
    .spkchat_on         = "tone_en/spkchat_on.*",
    .spkchat_off        = "tone_en/spkchat_off.*",
    .winddet_on         = "tone_en/winddet_on.*",
    .winddet_off        = "tone_en/winddet_off.*",
    .wclick_on          = "tone_en/wclick_on.*",
    .wclick_off         = "tone_en/wclick_off.*",
};

#if TCFG_TONE_EN_ENABLE
// static enum tone_language g_lang_used = TONE_ENGLISH;
static enum tone_language g_lang_used = TONE_CHINESE;
#else
static enum tone_language g_lang_used = TONE_CHINESE;
#endif

enum tone_language tone_language_get()
{
    return g_lang_used;
}

void tone_language_set(enum tone_language lang)
{
    y_printf("tone_language_set");
    // if (get_bt_tws_connect_status()) {
    //     if (tws_api_get_role() == TWS_ROLE_MASTER) {
    //         tws_play_tone_file_alone(get_tone_files()->tone_sw, 400);
    //     }
    // } else {
    //     play_tone_file_alone(get_tone_files()->tone_sw);
    // }
    // tws_play_tone_file_alone(get_tone_files()->tone_sw, 400);
    g_lang_used = lang;
}

const struct tone_files *get_tone_files()
{
    g_lang_used = user_app_info.tone_language;//user_app_info.tone_language;
    printf("get_tone_files---g_lang_used=%d\n", g_lang_used);
    if (g_lang_used == TONE_RING) {
        return &ring_tone_files;
    } else if (g_lang_used == TONE_CHINESE) {
        return &chinese_tone_files;
    }
    return &english_tone_files;
}


#if USER_PRODUCT_SET_TONE_EN
// u8 user_default_tone_language = 0;
u8 user_default_tone_language = 1;
void user_set_default_tone_language(u8 default_tone)
{
    y_printf("set_default_tone:%d", default_tone);
    syscfg_write(CFG_USER_DEFAULT_TONE, &default_tone, sizeof(default_tone));
    user_default_tone_language = default_tone;
    user_app_info.tone_language = default_tone;
    g_lang_used = (enum tone_language)user_default_tone_language;
}

void user_default_tone_language_init()
{
    u8 default_tone = 0;
    int ret = syscfg_read(CFG_USER_DEFAULT_TONE, &default_tone, 1);
    if (ret != 1 || default_tone < 0 || default_tone > 2) {
        y_printf("default_tone_language_init err:%d, %d", ret, default_tone);
        //默认英文/中文
        // user_default_tone_language = 0;
        user_default_tone_language = 1;
        // g_lang_used = TONE_ENGLISH;
        g_lang_used = TONE_CHINESE;
        return;
    }
    y_printf("default_tone_init:%d", default_tone);
    user_default_tone_language = default_tone;
    g_lang_used = user_default_tone_language;
    y_printf("g_lang_used:%d", g_lang_used);
}
#endif