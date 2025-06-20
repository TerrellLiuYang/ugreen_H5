#include "app_config.h"
#include "rcsp_config.h"
#include "rcsp_event.h"
#include "file_transfer.h"
#include "ble_rcsp_server.h"

///>>>>>>>>>>>设备接收到APP下发不需要回复数据
#if (RCSP_MODE)
/* #define RCSP_DEBUG_EN */
#ifdef RCSP_DEBUG_EN
#define rcsp_putchar(x)                	putchar(x)
#define rcsp_printf                    	printf
#define rcsp_put_buf(x,len)				put_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_put_buf(...)
#endif


void rcsp_data_recieve_no_respone(void *priv, u8 CMD_OpCode, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    rcsp_printf("data_recieve_no_respone %x\n", CMD_OpCode);
    switch (CMD_OpCode) {
#if TCFG_DEV_MANAGER_ENABLE
    case JL_OPCODE_FILE_TRANSFER:
        rcsp_file_transfer_download_doing(data, len);
        break;
#endif
    default:
        break;
    }
}
#endif//RCSP_MODE

