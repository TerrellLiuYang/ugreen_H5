/******************************************
	audio_node_common.c
	用于管理情景配置对多个节点的统一操作

******************************************/
#include "audio_config.h"

struct audio_node_common_t {
    u8 mic_mute_en;		//MIC静音标志
};

static struct audio_node_common_t node_p;


u8 audio_common_mic_mute_en_get(void)
{
    return node_p.mic_mute_en;
}

void audio_common_mic_mute_en_set(u8 mute_en)
{
    node_p.mic_mute_en = mute_en;
}



