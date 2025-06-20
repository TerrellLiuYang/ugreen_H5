/*********************************************************************************************
    *   Filename        : lib_system_config.c

    *   Description     : Optimized Code & RAM (编译优化配置)

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-03-18 15:22

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/

#include "app_config.h"
#include "system/includes.h"


///打印是否时间打印信息
const int config_printf_time         = 1;

///异常中断，asser打印开启
#if CONFIG_DEBUG_ENABLE
const int config_asser         = TCFG_EXCEPTION_LOG_ENABLE;
const int config_exception_reset_enable = TCFG_EXCEPTION_RESET_ENABLE;
#else
const int config_asser         = 0;
const int config_exception_reset_enable = 1;
#endif

//================================================//
//                  SDFILE 精简使能               //
//================================================//
const int SDFILE_VFS_REDUCE_ENABLE = 1;

//================================================//
//                  dev使用异步读使能             //
//================================================//
#ifdef TCFG_DEVICE_BULK_READ_ASYNC_ENABLE
const int device_bulk_read_async_enable = 1;
#else
const int device_bulk_read_async_enable = 0;
#endif


#if TCFG_UPDATE_ENABLE
const int CONFIG_UPDATE_ENABLE  = 0x1;
#else
const int CONFIG_UPDATE_ENABLE  = 0x0;
#endif

//================================================//
//                  UI 							  //
//================================================//
//const int ENABLE_LUA_VIRTUAL_MACHINE = 0;

//================================================//
//          不可屏蔽中断使能配置(UNMASK_IRQ)      //
//================================================//
const int CONFIG_CPU_UNMASK_IRQ_ENABLE = 1;

//================================================//
//                  FS功能控制 					  //
//================================================//
const int FATFS_WRITE = 1; // 控制fatfs写功能开关。
const int FILT_0SIZE_ENABLE = 1; //是否过滤0大小文件
const int FATFS_LONG_NAME_ENABLE = 1; //是否支持长文件名
const int FATFS_RENAME_ENABLE = 1; //是否支持重命名
const int FATFS_FGET_PATH_ENABLE = 1; //是否支持获取路径
const int FATFS_SAVE_FAT_TABLE_ENABLE = 1; //是否支持seek加速
const int FATFS_SUPPORT_OVERSECTOR_RW = 0; //是否支持超过一个sector向设备拿数
const int FATFS_TIMESORT_TURN_ENABLE = 1; //按时排序翻转，由默认从小到大变成从大到小
const int FATFS_TIMESORT_NUM = 128; //按时间排序,记录文件数量, 每个占用14 byte

const int FILE_TIME_HIDDEN_ENABLE = 0; //创建文件是否隐藏时间
const int FILE_TIME_USER_DEFINE_ENABLE = 1;//用户自定义时间，每次创建文件前设置，如果置0 需要确定芯片是否有RTC功能。
const int FATFS_SUPPORT_WRITE_SAVE_MEANTIME = 0; //每次写同步目录项使能，会降低连续写速度。

const int VIRFAT_FLASH_ENABLE = 0; //精简jifat代码,不使用。

//================================================//
//phy_malloc碎片整理使能:            			  //
//配置为0: phy_malloc申请不到不整理碎片           //
//配置为1: phy_malloc申请不到会整理碎片           //
//配置为2: phy_malloc申请总会启动一次碎片整理     //
//================================================//
const int PHYSIC_MALLOC_DEFRAG_ENABLE = 1;

//================================================//
//低功耗流程添加内存碎片整理使能:    			  //
//配置为0: 低功耗流程不整理碎片                   //
//配置为1: 低功耗流程会整理碎片                   //
//================================================//
const int MALLOC_MEMORY_DEFRAG_ENABLE = 1;

/**
 * @brief Log (Verbose/Info/Debug/Warn/Error)
 */
/*-----------------------------------------------------------*/
const char log_tag_const_v_SYS_TMR  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_SYS_TMR  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_SYS_TMR  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_SYS_TMR  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_SYS_TMR  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_JLFS  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_JLFS  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_JLFS  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_JLFS  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_JLFS  = CONFIG_DEBUG_LIB(TRUE);

//FreeRTOS
const char log_tag_const_v_PORT  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_PORT  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_PORT  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_PORT  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_PORT  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_KTASK  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_KTASK  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_KTASK  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_KTASK  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_KTASK  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_uECC  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_uECC  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_uECC  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_uECC  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_uECC  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_HEAP_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_HEAP_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_HEAP_MEM  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_HEAP_MEM  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_HEAP_MEM  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_V_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_V_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_V_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_V_MEM  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_V_MEM  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_P_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_P_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_P_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_P_MEM  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_P_MEM  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_P_MEM_C = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_P_MEM_C = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_d_P_MEM_C = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_P_MEM_C = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_P_MEM_C = CONFIG_DEBUG_LIB(TRUE);
