#include "cfg_tool.h"
#include "boot.h"
#include "ioctl_cmds.h"
#include "app_online_cfg.h"
#include "asm/crc16.h"
#include "asm/cpu.h"
#include "app_config.h"
#include "online_db_deal.h"
#include "init.h"
#include "usb/device/cdc.h"
#include "app_msg.h"
#include "classic/tws_api.h"
#include "bt_tws.h"
#include "timer.h"
#include "sniff.h"

#if CFG_TOOL_VER == CFG_TOOL_VER_VISUAL
#include "system/malloc.h"
#include "system/task.h"
#include "cfg_tool_statistics.h"
#endif

#define LOG_TAG_CONST       APP_CFG_TOOL
#define LOG_TAG             "[APP_CFG_TOOL]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_CFG_TOOL_ENABLE

#if (TCFG_COMM_TYPE == TCFG_UART_COMM) && (TCFG_ONLINE_TX_PORT == IO_PORT_DP || TCFG_ONLINE_RX_PORT == IO_PORT_DM) && \
						  (TCFG_USB_HOST_ENABLE || TCFG_PC_ENABLE)
#error "online config tool select DP/DM but usb module is enable"
#endif

struct cfg_tool_info {
    R_QUERY_BASIC_INFO 		r_basic_info;
    R_QUERY_FILE_SIZE		r_file_size;
    R_QUERY_FILE_CONTENT	r_file_content;
    R_PREPARE_WRITE_FILE	r_prepare_write_file;
    R_READ_ADDR_RANGE		r_read_addr_range;
    R_ERASE_ADDR_RANGE      r_erase_addr_range;
    R_WRITE_ADDR_RANGE      r_write_addr_range;
    R_ENTER_UPGRADE_MODE    r_enter_upgrade_mode;

    S_QUERY_BASIC_INFO 		s_basic_info;
    S_QUERY_FILE_SIZE		s_file_size;
    S_PREPARE_WRITE_FILE    s_prepare_write_file;
};

static struct cfg_tool_info info = {
    .s_basic_info.protocolVer = PROTOCOL_VER_AT_OLD,
};

#define __this  (&info)
#define TEMP_BUF_SIZE	1024
#if (TCFG_COMM_TYPE == TCFG_USB_COMM || TCFG_USER_TWS_ENABLE)
static u8 local_packet[TEMP_BUF_SIZE];
#endif

extern const char *sdk_version_info_get(void);
extern u8 *sdfile_get_burn_code(u8 *len);
extern int norflash_erase(u32 cmd, u32 addr);
extern void doe(u16 k, void *pBuf, u32 lenIn, u32 addr);
extern void go_mask_usb_updata();
static void tws_sync_cfg_tool_data(struct cfg_tool_event *cfg_tool_dev);

#if (CFG_TOOL_VER == CFG_TOOL_VER_VISUAL)
const char fa_return[] = "ER";	//失败
#else
const char fa_return[] = "FA";	//失败
#endif
const char ok_return[] = "OK";	//成功
const char er_return[] = "ER";	//不能识别的命令
static u32 size_total_write = 0;
static u8 parse_seq = 0;
struct cfg_tool_event cfg_packet;
#define TWS_FUNC_ID_CFGTOOL_SYNC    TWS_FUNC_ID('C', 'F', 'G', 'S')

#ifdef ALIGN
#undef ALIGN
#endif
#define ALIGN(a, b) \
	({ \
	 	int m = (u32)(a) & ((b)-1); \
		int ret = (u32)(a) + (m?((b)-m):0);	 \
		ret;\
	})

static u32 encode_data_by_user_key(u16 key, u8 *buff, u16 size, u32 dec_addr, u8 dec_len)
{
    u16 key_addr;
    u16 r_len;

    while (size) {
        r_len = (size > dec_len) ? dec_len : size;
        key_addr = (dec_addr >> 2)^key;
        doe(key_addr, buff, r_len, 0);
        buff += r_len;
        dec_addr += r_len;
        size -= r_len;
    }
    return dec_addr;
}

void hex2text(u8 *buf, u8 *out)
{
    //sprintf(out, "%02x%02x-%02x%02x%02x%02x", buf[5], buf[4], buf[3], buf[2], buf[1], buf[0]);
}

u32 packet_combined(u8 *packet, u8 num)
{
    u32 _packet = 0;
    _packet = (packet[num] | (packet[num + 1] << 8) | (packet[num + 2] << 16) | (packet[num + 3] << 24));
    return _packet;
}

/*延时复位防止工具升级进度条显示错误*/
static void delay_cpu_reset(void *priv)
{
    cpu_reset();
}

RESFILE *cfg_open_file(u32 file_id)
{
    RESFILE *cfg_fp = NULL;
    if (file_id == CFG_TOOL_FILEID) {
        cfg_fp = resfile_open(CFG_TOOL_FILE);
#if (CFG_TOOL_VER != CFG_TOOL_VER_VISUAL)
    } else if (file_id == CFG_OLD_EQ_FILEID) {
        cfg_fp = resfile_open(CFG_OLD_EQ_FILE);
    } else if (file_id == CFG_OLD_EFFECT_FILEID) {
        cfg_fp = resfile_open(CFG_OLD_EFFECT_FILE);
    } else if (file_id == CFG_EQ_FILEID) {
        cfg_fp = resfile_open(CFG_EQ_FILE);
#else
    } else if (file_id == CFG_STREAM_FILEID) {
        cfg_fp = resfile_open(CFG_STREAM_FILE);
    } else if (file_id == CFG_EFFECT_CFG_FILEID) {
        cfg_fp = resfile_open(CFG_EFFECT_CFG_FILE);
#endif
    }
    return cfg_fp;
}

void all_assemble_package_send_to_pc(u8 id, u8 sq, u8 *buf, u32 len)
{
    u8 *send_buf = NULL;
    u16 crc16_data;
    send_buf = (u8 *)dma_malloc(TEMP_BUF_SIZE);
    if (send_buf == NULL) {
        log_error("send_buf malloc err!");
        return;
    }

    send_buf[0] = 0x5A;
    send_buf[1] = 0xAA;
    send_buf[2] = 0xA5;
    CFG_TOOL_WRITE_LIT_U16(send_buf + 5, (2 + len)); /*L*/
    send_buf[7] = id;/*T*/
    send_buf[8] = sq;/*SQ*/
    memcpy(send_buf + 9, buf, len);
    crc16_data = CRC16(&send_buf[5], len + 4);
    send_buf[3] = crc16_data & 0xff;
    send_buf[4] = (crc16_data >> 8) & 0xff;

#if (TCFG_COMM_TYPE == TCFG_UART_COMM)
    ci_uart_write(send_buf, len + 9);
#elif (TCFG_COMM_TYPE == TCFG_USB_COMM)
    cdc_write_data(0, send_buf, len + 9);
#elif (TCFG_COMM_TYPE == TCFG_SPP_COMM)
#if SPP_DATA_USED_LVT
    app_online_db_ack(parse_seq, buf, len);
#else
    app_online_db_ack(0xff, send_buf, len + 9);
#endif
#endif
    dma_free(send_buf);
}

void app_cfg_tool_event_handler(struct cfg_tool_event *cfg_tool_dev)
{
    u8 *buf = NULL;
    buf = (u8 *)malloc(TEMP_BUF_SIZE);
    if (buf == NULL) {
        log_error("buf malloc err!");
        return;
    }

    buf = (u8 *)ALIGN(buf, 4);
    memset(buf, 0, TEMP_BUF_SIZE);
    /* printf_buf(cfg_tool_dev->packet, cfg_tool_dev->size); */
    memcpy(buf, cfg_tool_dev->packet, cfg_tool_dev->size);

    const struct tool_interface *p;
    list_for_each_tool_interface(p) {
        if (p->id == cfg_tool_dev->packet[2]) {
            p->tool_message_deal(buf + 2, cfg_tool_dev->size - 2);
        }
    }
    free(buf);
}

u8 online_cfg_tool_data_deal(void *buf, u32 len)
{
    u8 *data_buf = (u8 *)buf;
    u16 crc16_data;

    /* printf_buf(buf, len); */
    if ((data_buf[0] != 0x5a) || (data_buf[1] != 0xaa) || (data_buf[2] != 0xa5)) {
        /* log_error("Header check error, receive an invalid message!\n"); */
        return 1;
    }
    crc16_data = (data_buf[4] << 8) | data_buf[3];
    if (crc16_data != CRC16(data_buf + 5, len - 5)) {
        log_error("CRC16 check error, receive an invalid message!\n");
        return 1;
    }
    device_online_timeout_check();

    u32 cmd = packet_combined(data_buf, 9);
    /* switch (cmd) { */
    /* case ONLINE_SUB_OP_QUERY_BASIC_INFO: */
    /* case ONLINE_SUB_OP_ONLINE_INSPECTION: */
    /* case ONLINE_SUB_OP_CPU_RESET: */
    /* case ONLINE_SUB_OP_QUERY_FILE_SIZE: */
    /* case ONLINE_SUB_OP_QUERY_FILE_CONTENT: */
    /* case ONLINE_SUB_OP_PREPARE_WRITE_FILE: */
    /* case ONLINE_SUB_OP_READ_ADDR_RANGE: */
    /* case ONLINE_SUB_OP_ERASE_ADDR_RANGE: */
    /* case ONLINE_SUB_OP_WRITE_ADDR_RANGE: */
    /* case ONLINE_SUB_OP_ENTER_UPGRADE_MODE: */
    /* case ONLINE_SUB_OP_REFRESH_TASK_INFO_LIST: */
    /* case ONLINE_SUB_OP_GET_TASK_INFO_LIST: */
    /* case ONLINE_SUB_OP_GET_CACHE_INFO_LIST: */
    /* 	break; */
    /* default: */
    /*     cmd = DEFAULT_ACTION; */
    /*     break; */
    /* } */

    cfg_packet.event = cmd;
    cfg_packet.size = CFG_TOOL_READ_LIT_U16(data_buf + 5) + 2;
#if ((TCFG_COMM_TYPE == TCFG_UART_COMM) || (TCFG_COMM_TYPE == TCFG_SPP_COMM))
    cfg_packet.packet = &data_buf[5];
#if (TCFG_USER_TWS_ENABLE && (TCFG_COMM_TYPE == TCFG_UART_COMM))
    tws_sync_cfg_tool_data(&cfg_packet);
#endif
    app_cfg_tool_event_handler(&cfg_packet);
#elif (TCFG_COMM_TYPE == TCFG_USB_COMM)//in irq
    if (cfg_packet.size > sizeof(local_packet)) {
        return 0;
    }
    cfg_packet.packet = local_packet;
    memcpy(cfg_packet.packet, &data_buf[5], cfg_packet.size);
    os_taskq_post_type("app_core", MSG_FROM_CDC, 0, NULL);
#endif
    return 0;
}

#if (TCFG_COMM_TYPE == TCFG_SPP_COMM && SPP_DATA_USED_LVT)
int cfg_tool_spp_rx_data(u8 *packet, u8 size, u8 *ext_data, u16 ext_size)
{
    u8 *rx_buf = NULL;
    rx_buf = (u8 *)malloc(size + ext_size + 1);
    if (rx_buf == NULL) {
        log_error("rx_buf malloc err!");
        return -1;
    }

    rx_buf[0] = size + 1;
    memcpy(rx_buf + 1, ext_data, ext_size);
    memcpy(rx_buf + 1 + ext_size, packet, size);

    parse_seq = rx_buf[2];
    u32 event = (rx_buf[3] | (rx_buf[4] << 8) | (rx_buf[5] << 16) | (rx_buf[6] << 24));
    cfg_packet.event = event;
    cfg_packet.packet = rx_buf;
    cfg_packet.size = size + ext_size + 1;
    app_cfg_tool_event_handler(&cfg_packet);
    free(rx_buf);
    return 0;
}

static int cfg_tool_spp_init(void)
{
    app_online_db_register_handle(SPP_NEW_EQ_CH, cfg_tool_spp_rx_data);
    app_online_db_register_handle(SPP_OLD_EQ_CH, cfg_tool_spp_rx_data);
    app_online_db_register_handle(SPP_CFG_CH, cfg_tool_spp_rx_data);
    return 0;
}
early_initcall(cfg_tool_spp_init);
#endif //#if (TCFG_COMM_TYPE == TCFG_SPP_COMM && SPP_DATA_USED_LVT)

#if (TCFG_COMM_TYPE == TCFG_USB_COMM)
static int cfg_tool_cdc_rx_data(int *msg)
{
#if TCFG_USER_TWS_ENABLE
    tws_sync_cfg_tool_data(&cfg_packet);
#endif
    app_cfg_tool_event_handler(&cfg_packet);
    return 0;
}

APP_MSG_HANDLER(cdc_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_CDC,
    .handler    = cfg_tool_cdc_rx_data,
};
#endif//#if (TCFG_COMM_TYPE == TCFG_USB_COMM)

#if TCFG_USER_TWS_ENABLE
static void tws_sync_cfg_tool_data(struct cfg_tool_event *cfg_tool_dev)
{
    tws_api_send_data_to_sibling(cfg_tool_dev->packet, cfg_tool_dev->size, TWS_FUNC_ID_CFGTOOL_SYNC);
}

static void online_cfg_tool_tws_sibling_data_deal(void *_data, u16 len, bool rx)
{
    if (rx) {
        cfg_packet.event = DEFAULT_ACTION;
        cfg_packet.size = len;
        if (cfg_packet.size > sizeof(local_packet)) {
            return;
        }
        cfg_packet.packet = local_packet;
        memcpy(cfg_packet.packet, (u8 *)_data, cfg_packet.size);
        os_taskq_post_type("app_core", MSG_FROM_CFGTOOL_TWS_SYNC, 0, NULL);
    }
}

static int cfg_tool_tws_sync_rx_data(int *msg)
{
    app_cfg_tool_event_handler(&cfg_packet);
    return 0;
}

REGISTER_TWS_FUNC_STUB(cfg_tool_stub) = {
    .func_id = TWS_FUNC_ID_CFGTOOL_SYNC,
    .func    = online_cfg_tool_tws_sibling_data_deal,
};

APP_MSG_HANDLER(tws_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_CFGTOOL_TWS_SYNC,
    .handler    = cfg_tool_tws_sync_rx_data,
};
#endif//#if TCFG_USER_TWS_ENABLE

#if CFG_TOOL_VER == CFG_TOOL_VER_VISUAL
// 在线检测设备
static u32 _device_online_timeout = 0;
static u16 _timer_id = 0;
static CFG_TOOL_ONLINE_STATUS _cfg_tool_online_status = CFG_TOOL_ONLINE_STATUS_OFFLINE;
static void device_online_timeout_task(void *priv)
{
    _device_online_timeout++;
    if (_device_online_timeout >= 3) {
        if (_timer_id) {
            sys_timer_del(_timer_id);
            _timer_id = 0;
#if (TCFG_COMM_TYPE == TCFG_SPP_COMM)
            bt_sniff_enable();
#endif
        }
        _cfg_tool_online_status = CFG_TOOL_ONLINE_STATUS_OFFLINE;
        // 清理任务统计的资源
        cfg_tool_statistics_resource_release();
        /* log_info("cfg tool is offline!\n"); */
    }
}

void device_online_timeout_check()
{
    _device_online_timeout = 0;
    _cfg_tool_online_status = CFG_TOOL_ONLINE_STATUS_ONLINE;
#if (TCFG_COMM_TYPE == TCFG_SPP_COMM)
    bt_sniff_disable();
#endif
    if (!_timer_id) {
        /* log_info("cfg tool online sys timer add!\n"); */
        _timer_id = sys_timer_add(NULL, device_online_timeout_task, 1000);
    }
}
CFG_TOOL_ONLINE_STATUS get_cfg_tool_online_status()
{
    return _cfg_tool_online_status;
}
#else
CFG_TOOL_ONLINE_STATUS get_cfg_tool_online_status()
{
    return CFG_TOOL_ONLINE_STATUS_ONLINE;
}
#endif

static void cfg_tool_callback(u8 *packet, u32 size)
{
    u32 erase_cmd = 0;
    u32 write_len = 0;
    u32 send_len = 0;
    u8 crc_temp_len, sdkname_temp_len;
    u8 *buf = NULL;
    u8 *buf_temp = NULL;
    struct resfile_attrs attr;
    RESFILE *cfg_fp = NULL;

    buf = (u8 *)malloc(TEMP_BUF_SIZE);
    if (buf == NULL) {
        log_error("buf malloc err!");
        return;
    }
    memset(buf, 0, TEMP_BUF_SIZE);

    switch (cfg_packet.event) {
    case ONLINE_SUB_OP_QUERY_BASIC_INFO:
        u8 *p = sdfile_get_burn_code(&crc_temp_len);
        memcpy(__this->s_basic_info.progCrc, p + 8, 6);
        hex2text(__this->s_basic_info.progCrc, __this->s_basic_info.progCrc);
        sdkname_temp_len = strlen(sdk_version_info_get());
        memcpy(__this->s_basic_info.sdkName, sdk_version_info_get(), sdkname_temp_len);

        struct flash_head flash_head_for_pid_vid;
        for (u8 i = 0; i < 5; i++) {
            norflash_read(NULL, (u8 *)&flash_head_for_pid_vid, 32, 0x1000 * i);
            doe(0xffff, (u8 *)&flash_head_for_pid_vid, 32, 0);
            if (flash_head_for_pid_vid.crc == 0xffff) {
                continue;
            } else {
                log_info("flash head addr = 0x%x\n", 0x1000 * i);
                break;
            }
        }
        if (flash_head_for_pid_vid.crc == 0xffff) {
            log_error("Can't find flash head addr");
            break;
        }

        memcpy(__this->s_basic_info.pid, &(flash_head_for_pid_vid.pid), sizeof(flash_head_for_pid_vid.pid));
        memcpy(__this->s_basic_info.vid, &(flash_head_for_pid_vid.vid), sizeof(flash_head_for_pid_vid.vid));
        for (u8 i = 0; i < sizeof(__this->s_basic_info.pid); i++) {
            if (__this->s_basic_info.pid[i] == 0xff) {
                __this->s_basic_info.pid[i] = 0x00;
            }
        }
        send_len = sizeof(__this->s_basic_info);
        memcpy(buf, &(__this->s_basic_info), send_len);
        break;

    case ONLINE_SUB_OP_CPU_RESET:
        send_len = 2;
        u8 cpu_reset_ret[] = "OK";
        memcpy(buf, cpu_reset_ret, send_len);
        sys_timeout_add(NULL, delay_cpu_reset, 500);
        break;

    case ONLINE_SUB_OP_QUERY_FILE_SIZE:
        __this->r_file_size.file_id = packet_combined(cfg_packet.packet, 8);
        cfg_fp = cfg_open_file(__this->r_file_size.file_id);
        if (cfg_fp == NULL) {
            log_error("file open error!\n");
            goto _exit_;
        }

        resfile_get_attrs(cfg_fp, &attr);
        __this->s_file_size.file_size = attr.fsize;
        send_len = sizeof(__this->s_file_size.file_size);//长度
        memcpy(buf, &(__this->s_file_size.file_size), send_len);
        resfile_close(cfg_fp);
        break;

    case ONLINE_SUB_OP_QUERY_FILE_CONTENT:
        __this->r_file_content.file_id = packet_combined(cfg_packet.packet, 8);
        __this->r_file_content.offset = packet_combined(cfg_packet.packet, 12);
        __this->r_file_content.size = packet_combined(cfg_packet.packet, 16);

        cfg_fp = cfg_open_file(__this->r_file_content.file_id);
        if (cfg_fp == NULL) {
            log_error("file open error!\n");
            goto _exit_;
        }
        resfile_get_attrs(cfg_fp, &attr);
        if (__this->r_file_content.size > attr.fsize) {
            resfile_close(cfg_fp);
            log_error("reading size more than actual size!\n");
            break;
        }

        u32 flash_addr = sdfile_cpu_addr2flash_addr(attr.sclust);
        buf_temp = (u8 *)malloc(__this->r_file_content.size);
        if (buf_temp == NULL) {
            log_error("buf_temp malloc err!");
            goto _exit_;
        }
        norflash_read(NULL, (void *)buf_temp, __this->r_file_content.size, flash_addr + __this->r_file_content.offset);
        send_len = __this->r_file_content.size;
        memcpy(buf, buf_temp, __this->r_file_content.size);
        if (buf_temp) {
            free(buf_temp);
        }
        resfile_close(cfg_fp);
        break;

    case ONLINE_SUB_OP_PREPARE_WRITE_FILE:
        __this->r_prepare_write_file.file_id = packet_combined(cfg_packet.packet, 8);
        __this->r_prepare_write_file.size = packet_combined(cfg_packet.packet, 12);

        cfg_fp = cfg_open_file(__this->r_prepare_write_file.file_id);
        if (cfg_fp == NULL) {
            log_error("file open error!\n");
            break;
        }
        resfile_get_attrs(cfg_fp, &attr);

        __this->s_prepare_write_file.file_size = attr.fsize;
        __this->s_prepare_write_file.file_addr = sdfile_cpu_addr2flash_addr(attr.sclust);
        __this->s_prepare_write_file.earse_unit = boot_info.vm.align * 256;
        send_len = sizeof(__this->s_prepare_write_file);
        memcpy(buf, &(__this->s_prepare_write_file), send_len);
        resfile_close(cfg_fp);

        if (__this->r_prepare_write_file.file_id == CFG_STREAM_FILEID) {
            app_send_message(APP_MSG_WRITE_RESFILE_START, (int)"stream.bin");
        }
        break;

    case ONLINE_SUB_OP_READ_ADDR_RANGE:
        __this->r_read_addr_range.addr = packet_combined(cfg_packet.packet, 8);
        __this->r_read_addr_range.size = packet_combined(cfg_packet.packet, 12);
        buf_temp = (u8 *)malloc(__this->r_read_addr_range.size);
        if (buf_temp == NULL) {
            log_error("buf_temp malloc err!");
            goto _exit_;
        }
        norflash_read(NULL, (void *)buf_temp, __this->r_read_addr_range.size, __this->r_read_addr_range.addr);
        send_len = __this->r_read_addr_range.size;
        memcpy(buf, buf_temp, __this->r_read_addr_range.size);
        if (buf_temp) {
            free(buf_temp);
        }
        break;

    case ONLINE_SUB_OP_ERASE_ADDR_RANGE:
        __this->r_erase_addr_range.addr = packet_combined(cfg_packet.packet, 8);
        __this->r_erase_addr_range.size = packet_combined(cfg_packet.packet, 12);

        switch (__this->s_prepare_write_file.earse_unit) {
        case 256:
            erase_cmd = IOCTL_ERASE_PAGE;
            break;
        case (4*1024):
            erase_cmd = IOCTL_ERASE_SECTOR;
            break;
        case (64*1024):
            erase_cmd = IOCTL_ERASE_BLOCK;
            break;
        default:
            memcpy(buf, fa_return, sizeof(fa_return));
            log_error("erase error!");
            break;
        }

        for (u8 i = 0; i < (__this->r_erase_addr_range.size / __this->s_prepare_write_file.earse_unit); i ++) {
            u8 ret = norflash_erase(erase_cmd, __this->r_erase_addr_range.addr + (i * __this->s_prepare_write_file.earse_unit));
            if (ret) {
                memcpy(buf, fa_return, sizeof(fa_return));
                log_error("erase error!");
                break;
            } else {
                memcpy(buf, ok_return, sizeof(ok_return));
            }
        }
        send_len = sizeof(fa_return);
        break;

    case ONLINE_SUB_OP_WRITE_ADDR_RANGE:
        __this->r_write_addr_range.addr = packet_combined(cfg_packet.packet, 8);
        __this->r_write_addr_range.size = packet_combined(cfg_packet.packet, 12);
        buf_temp = (u8 *)malloc(__this->r_write_addr_range.size);
        if (buf_temp == NULL) {
            log_error("buf_temp malloc err!");
            goto _exit_;
        }
        memcpy(buf_temp, cfg_packet.packet + 16, __this->r_write_addr_range.size);
        encode_data_by_user_key(boot_info.chip_id, buf_temp, __this->r_write_addr_range.size, __this->r_write_addr_range.addr - boot_info.sfc.sfc_base_addr, 0x20);
        write_len = norflash_write(NULL, buf_temp, __this->r_write_addr_range.size, __this->r_write_addr_range.addr);

        if (write_len != __this->r_write_addr_range.size) {
            memcpy(buf, fa_return, sizeof(fa_return));
            log_error("write error!");
        } else {
            memcpy(buf, ok_return, sizeof(ok_return));
        }
        send_len = sizeof(fa_return);
        if (buf_temp) {
            free(buf_temp);
        }

#if (CFG_TOOL_VER != CFG_TOOL_VER_VISUAL)
        if (__this->r_prepare_write_file.file_id == CFG_TOOL_FILEID) {
            size_total_write += __this->r_write_addr_range.size;
            if (size_total_write >= __this->r_erase_addr_range.size) {
                size_total_write = 0;
                log_info("cpu_reset\n");
                sys_timeout_add(NULL, delay_cpu_reset, 500);
            }
        }
#endif
        int a = __this->r_write_addr_range.addr + __this->r_write_addr_range.size;
        int b = __this->s_prepare_write_file.file_addr + __this->s_prepare_write_file.file_size;
        if (__this->r_prepare_write_file.file_id == CFG_STREAM_FILEID) {
            if (a >= b) {
                app_send_message(APP_MSG_WRITE_RESFILE_STOP, (int)"stream.bin");
            }
        }
        break;

#if TCFG_COMM_TYPE == TCFG_USB_COMM
    case ONLINE_SUB_OP_ENTER_UPGRADE_MODE:
        log_info("event_ONLINE_SUB_OP_ENTER_UPGRADE_MODE\n");
        go_mask_usb_updata();
        break;
#endif

    case ONLINE_SUB_OP_ONLINE_INSPECTION:
        R_QUERY_SYS_INFO upload_param = {0};
        upload_param.use_mem = memory_get_size(P_MEMORY_USED);//使用的mem;
        upload_param.total_mem = memory_get_size(P_MEMORY_TOTAL);//总的mem
        int cpu_usage = os_cpu_usage(NULL, NULL) / CPU_CORE_NUM;
        cpu_usage = (cpu_usage == 0) ? 1 : cpu_usage;
        upload_param.cpu_use_ratio = cpu_usage;
        upload_param.task_num = get_task_info_list_num();
        send_len = sizeof(upload_param);
        memcpy(buf, &upload_param, send_len);
        break;

    default: // DEFAULT_ACTION
#if (CFG_TOOL_VER == CFG_TOOL_VER_VISUAL)
        free(buf);
        return;
#else
        log_error("invalid data\n");
        memcpy(buf, er_return, sizeof(er_return));//不认识的命令返回ER
        send_len = sizeof(er_return);
#endif
        break;
_exit_:
        memcpy(buf, fa_return, sizeof(fa_return));//文件打开失败返回FA
        send_len = sizeof(fa_return);
        break;
    }

    all_assemble_package_send_to_pc(REPLY_STYLE, cfg_packet.packet[3], buf, send_len);
    free(buf);
}


#if (CFG_TOOL_VER == CFG_TOOL_VER_VISUAL)
REGISTER_DETECT_TARGET(cfg_tool_target) = {
    .id         		= VISUAL_CFG_TOOL_CHANNEL_STYLE,
    .tool_message_deal  = cfg_tool_callback,
};
#else
REGISTER_DETECT_TARGET(cfg_tool_target) = {
    .id         		= INITIATIVE_STYLE,
    .tool_message_deal  = cfg_tool_callback,
};
#endif

//在线调试不进power down
static u8 cfg_tool_idle_query(void)
{
    return !get_cfg_tool_online_status();
}

REGISTER_LP_TARGET(cfg_tool_lp_target) = {
    .name = "new_cfg_tool",
    .is_idle = cfg_tool_idle_query,
};

#endif


