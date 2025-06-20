#ifndef _LINEIN_FILE_H_
#define _LINEIN_FILE_H_

#include "generic/typedef.h"
#include "media/includes.h"
#include "app_config.h"

struct linein_file_param {
    u8 len;
    u8 linein_mode;
    u8 linein_gain;
    u8 linein_pre_gain;   // 0:0dB   1:6dB
    u8 linein_ain_sel;    // 0/1/2
    u8 linein_dcc;        // DCC level
} __attribute__((packed));

//stream.bin ADC参数文件解析
struct linein_file_cfg {
    u8 len;
    u8  linein_det_en;
    u16 linein_det_port;
    u8  linein_det_port_up;
    u8  linein_det_port_down;
    u8  linein_det_ad_en;
    u16 linein_det_ad_value;
    u8 len1;
    u32 linein_en_map;  // BIT(ch) 1 为 ch 使能，0 为不使能
    struct linein_file_param param[AUDIO_ADC_LINEIN_MAX_NUM];
} __attribute__((packed));

extern void audio_linein_file_init(void);
extern int adc_file_linein_open(struct adc_linein_ch *linein, int ch);
extern struct linein_file_cfg *audio_linein_file_get_cfg(void);


#endif // #ifndef _ADC_FILE_H_
