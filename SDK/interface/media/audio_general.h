#ifndef _AUDIO_GENERAL_H
#define _AUDIO_GENERAL_H

#include "generic/typedef.h"
struct audio_general_params {
    int sample_rate;
};

struct audio_general_params *audio_general_get_param(void);

#endif
