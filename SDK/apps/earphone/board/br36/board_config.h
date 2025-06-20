#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*
 *  板级配置选择
 */

#define CONFIG_BOARD_AC700N_DEMO
// #define CONFIG_BOARD_AC7006F_EARPHONE
// #define CONFIG_BOARD_AC700N_SD_PC_DEMO
//#define CONFIG_BOARD_AC7003F_DEMO
// #define CONFIG_BOARD_AC700N_ANC
//#define CONFIG_BOARD_AC700N_DMS

#include "media/audio_def.h"
#include "board_ac700n_demo_cfg.h"

#define  DUT_AUDIO_DAC_LDO_VOLT                 DACVDD_LDO_1_35V

#endif
