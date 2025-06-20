#ifndef __POWER_COMPAT__
#define __POWER_COMPAT__

void power_reset_close();

int cpu_reset_by_soft();

u8 is_ldo5v_wakeup(void);

void port_edge_wkup_set_callback(void (*wakeup_callback)(u8 index, u8 gpio));

#endif
