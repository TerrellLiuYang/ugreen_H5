#ifndef _CFG_PRIVATE_H
#define _CFG_PRIVATE_H

#include "fs/resfile.h"

int cfg_private_delete_file(RESFILE *file);
int cfg_private_write(RESFILE *file, void *buf, u32 len);
RESFILE *cfg_private_open(const char *path, const char *mode);
int cfg_private_read(RESFILE *file, void *buf, u32 len);
int cfg_private_close(RESFILE *file);
int cfg_private_seek(RESFILE *file, int offset, int fromwhere);
void cfg_private_erase(u32 addr, u32 len);


#endif
