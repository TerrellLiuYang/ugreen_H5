#ifndef  __EFUSE_H__
#define  __EFUSE_H__

u32 efuse_get_chip_id();
u32 efuse_get_vbat_trim_420();
u32 efuse_get_vbat_trim_435();
u32 efuse_get_gpadc_vbg_trim();
u16 efuse_get_mvbg_trim(void);
u32 get_chip_version();


#endif  /*EFUSE_H*/
