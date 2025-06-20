#ifndef POWER_INTERFACE_H
#define POWER_INTERFACE_H

#include "generic/typedef.h"

//p33寄存器
#include "asm/power/p33.h"

//P11寄存器
#include "asm/power/p11.h"

//电源、低功耗
#include "asm/power/power_api.h"

//唤醒
#include "asm/power/power_wakeup.h"

//复位
#include "asm/power/power_reset.h"

#include "asm/power/power_port.h"

#include "asm/power/lp_ipc.h"

#include "asm/power/power_compat.h"

#endif
