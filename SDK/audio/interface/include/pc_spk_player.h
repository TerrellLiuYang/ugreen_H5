#ifndef __PC_RX_PLAYER_H
#define __PC_RX_PLAYER_H

#include "cpu.h"


int pc_spk_player_open(void);
void pc_spk_player_close(void);
int pcspk_close_player_by_taskq(void);
int pcspk_open_player_by_taskq(void);
int pcspk_restart_player_by_taskq(void);
bool pc_spk_player_runing();


#endif


