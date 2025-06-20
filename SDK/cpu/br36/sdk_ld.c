// *INDENT-OFF*
#include "app_config.h"
#include "audio_config_def.h"

/* =================  BR34 SDK memory ========================
 _______________ ___ 0x2C000(176K)
|   isr base    |
|_______________|___ _IRQ_MEM_ADDR(size = 0x100)
|rom export ram |
|_______________|
|    update     |
|_______________|___ RAM_LIMIT_H
|     HEAP      |
|_______________|___ data_code_pc_limit_H
| audio overlay |
|_______________|
|   data_code   |
|_______________|___ data_code_pc_limit_L
|     bss       |
|_______________|
|     data      |
|_______________|
|   irq_stack   |
|_______________|
|   boot info   |
|_______________|
|     TLB       |
|_______________|0 RAM_LIMIT_L

 =========================================================== */

#include "maskrom_stubs.ld"

EXTERN(
_start
#include "sdk_used_list.c"
);

//============ About RAM ================

UPDATA_SIZE     = 0x80;
UPDATA_BEG      = _MASK_MEM_BEGIN - UPDATA_SIZE;
UPDATA_BREDR_BASE_BEG = 0x2c000;

RAM_LIMIT_L     = 0;
RAM_LIMIT_H     = UPDATA_BEG;
PHY_RAM_SIZE    = RAM_LIMIT_H - RAM_LIMIT_L;

//from mask export
ISR_BASE       = _IRQ_MEM_ADDR;
ROM_RAM_SIZE   = _MASK_MEM_SIZE;
ROM_RAM_BEG    = _MASK_MEM_BEGIN;

RAM0_BEG 		= RAM_LIMIT_L;
RAM0_END 		= RAM_LIMIT_H;
RAM0_SIZE 		= RAM0_END - RAM0_BEG;

//=============== About BT RAM ===================
//CONFIG_BT_RX_BUFF_SIZE = (1024 * 18);


//============ About CODE ================
#ifdef CONFIG_NEW_CFG_TOOL_ENABLE
CODE_BEG        = 0x1E00100;
#else
CODE_BEG        = 0x1E00120;
#endif

MEMORY
{
	code0(rx)       : ORIGIN = CODE_BEG,        LENGTH = CONFIG_FLASH_SIZE
	ram0(rwx)       : ORIGIN = RAM0_BEG,        LENGTH = RAM0_SIZE
}


ENTRY(_start)

SECTIONS
{

    . = ORIGIN(ram0);
	//TLB 起始需要16K 对齐；
    .mmu_tlb ALIGN(0x4000):
    {
        *(.mmu_tlb_segment);
    } > ram0

	.boot_info ALIGN(32):
	{
		*(.boot_info)
        . = ALIGN(32);
	} > ram0

	.irq_stack ALIGN(32):
    {
		*(.stack_magic)
        _cpu0_sstack_begin = .;
        PROVIDE(cpu0_sstack_begin = .);
        *(.stack)
        _cpu0_sstack_end = .;
        PROVIDE(cpu0_sstack_end = .);
    	_stack_end = . ;
		*(.stack_magic0)
		. = ALIGN(4);

    } > ram0

	.data ALIGN(32):SUBALIGN(4)
	{
		//cpu start
        . = ALIGN(4);
        *(.data_magic)

        *(.data*)
        *(.*.data)

         . = ALIGN(4);
         __a2dp_movable_slot_start = .;
         *(.movable.slot.1);
         __a2dp_movable_slot_end = .;


		. = ALIGN(4);
		app_mode_begin = .;
		KEEP(*(.app_mode))
        app_mode_end = .;
		#include "media/cpu/br36/audio_lib_data.ld"

		. = ALIGN(4);
		#include "system/system_lib_data.ld"
        #include "btctrler/crypto/data.ld"

		. = ALIGN(4);

	} > ram0

	.bss ALIGN(32):SUBALIGN(4)
    {
        . = ALIGN(4);
		#include "system/system_lib_bss.ld"

        . = ALIGN(4);
        *(.bss)
        *(.*.data.bss)
        . = ALIGN(4);
        *(.*.data.bss.nv)

        . = ALIGN(4);
		#include "media/cpu/br36/audio_lib_bss.ld"

        . = ALIGN(4);

        *(.volatile_ram)
		*(.btstack_pool)

        . = ALIGN(8);

		*(.audio_buf)
        *(.src_filt)
        *(.src_dma)
        *(.mem_heap)
		*(.memp_memory_x)

        . = ALIGN(4);
#if USB_MEM_USE_OVERLAY == 0
        *(.usb.data.bss.exchange)
#endif

        . = ALIGN(4);
		*(.non_volatile_ram)

        #include "btctrler/crypto/bss.ld"
        . = ALIGN(32);

    } > ram0

	.data_code ALIGN(32):SUBALIGN(4)
	{
        data_code_pc_limit_begin = .;
		*(.bank_critical_code)
		#include "media/cpu/br36/audio_lib_data_text.ld"

        . = ALIGN(4);
    } > ram0

    .common ALIGN(32):SUBALIGN(4)
    {
        //common bank code addr
        common_code_run_addr = .;
        *(.common*)

        *(.math_fast_funtion_code)
		*(.flushinv_icache)
        *(.cache)
        *(.volatile_ram_code)
        *(.chargebox_code)

        *(.os_critical_code)
        *(.os.text*)
        *(.movable.stub.1)
        *(.*.text.cache.L1)
        *(.*.text.const.cache.L2)


        /* *(.rodata*) */

       . = ALIGN(4);
    } > ram0

	/* .moveable_slot ALIGN(4): */
	/* { */
		/* *(movable.slot.*) */
	/* } >ram0 */

	__report_overlay_begin = .;
	overlay_code_begin = .;
	OVERLAY : AT(0x200000) SUBALIGN(4)
    {
		.overlay_aec
		{
#if (TCFG_AUDIO_CVP_CODE_AT_RAM == 2)
            . = ALIGN(4);
			aec_code_begin  = . ;
			*(.text._*)
			*(.aec_code)
			*(.aec_const)
			*(.res_code)
			*(.res_const)
			. = ALIGN(4);
            *(.dns_common_data)
            *(.dns_param_data_single)
            *(.dns_param_data_dual)
            *(.jlsp_nlp_code)
            *(.jlsp_nlp_const)
            *(.jlsp_aec_code)
            *(.jlsp_aec_const)
            *(.jlsp_prep_code)
            *(.jlsp_prep_const)
            *(.jlsp_enc_code)
            *(.jlsp_enc_const)
            *(.jlsp_wn_code)
            *(.jlsp_wn_const)
			*(.ns_code)
			*(.ns_const)
			*(.fft_code)
			*(.fft_const)
			*(.nlp_code)
			*(.nlp_const)
			*(.der_code)
			*(.der_const)
			*(.qmf_code)
			*(.qmf_const)
			*(.dms_code)
			*(.dms_const)
			*(.agc_code)
            *(.opcore_maskrom)
			aec_code_end = . ;
			aec_code_size = aec_code_end - aec_code_begin ;
#elif (TCFG_AUDIO_CVP_CODE_AT_RAM == 1)	//仅DNS部分放在overlay
            . = ALIGN(4);
			aec_code_begin  = . ;
            *(.dns_common_data)
            *(.dns_param_data_single)
            *(.dns_param_data_dual)
            *(.jlsp_nlp_code)
            *(.jlsp_nlp_const)
            *(.jlsp_aec_code)
            *(.jlsp_aec_const)
            *(.jlsp_prep_code)
            *(.jlsp_prep_const)
            *(.jlsp_enc_code)
            *(.jlsp_enc_const)
            *(.jlsp_wn_code)
            *(.jlsp_wn_const)
			aec_code_end = . ;
			aec_code_size = aec_code_end - aec_code_begin ;

#endif/*TCFG_AUDIO_CVP_CODE_AT_RAM*/
		}

		.overlay_aac
		{
#if TCFG_AUDIO_AAC_CODE_AT_RAM
			. = ALIGN(4);
			aac_dec_code_begin = .;
			*(.bt_aac_dec.text)
			*(.bt_aac_dec.text.cache.*)
			aac_dec_code_end = .;
			aac_dec_code_size  = aac_dec_code_end - aac_dec_code_begin ;

			. = ALIGN(4);
			bt_aac_dec_const_begin = .;
			*(.bt_aac_dec.text.const)
			. = ALIGN(4);
			bt_aac_dec_const_end = .;
			bt_aac_dec_const_size = bt_aac_dec_const_end -  bt_aac_dec_const_begin ;
			*(.bt_aac_dec_data)
			. = ALIGN(4);
#endif/*TCFG_AUDIO_AAC_CODE_AT_RAM*/
		}
    } > ram0

   //加个空段防止下面的OVERLAY地址不对
    .ram0_empty0 ALIGN(4) :
	{
        . = . + 4;
    } > ram0

	//__report_overlay_begin = .;
	OVERLAY : AT(0x210000) SUBALIGN(4)
    {
		.overlay_aec_ram
		{
            . = ALIGN(4);
			*(.msbc_enc)
			*(.cvsd_codec)
			*(.aec_bss)
			*(.res_bss)
			*(.ns_bss)
			*(.nlp_bss)
        	*(.der_bss)
        	*(.qmf_bss)
        	*(.fft_bss)
			*(.aec_mem)
			*(.dms_bss)
            *(.jlsp_aec_bss)
            *(.jlsp_nlp_bss)
            *(.jlsp_dns_bss)
            *(.jlsp_enc_bss)
            *(.jlsp_prep_bss)
            *(.jlsp_wn_bss)
		}

		.overlay_aac_ram
		{
            . = ALIGN(4);
			*(.bt_aac_dec_bss)

			. = ALIGN(4);
			*(.aac_mem)
			*(.aac_ctrl_mem)
			/* 		. += 0x5fe8 ; */
			/*		. += 0xef88 ; */
		}

		.overlay_mp3
		{
			*(.mp3_mem)
			*(.mp3_ctrl_mem)
			*(.mp3pick_mem)
			*(.mp3pick_ctrl_mem)
			*(.dec2tws_mem)
		}

    	.overlay_wav
		{
			/* *(.wav_bss_id) */
			*(.wav_mem)
			*(.wav_ctrl_mem)
		}


		.overlay_wma
		{
			*(.wma_mem)
			*(.wma_ctrl_mem)
			*(.wmapick_mem)
			*(.wmapick_ctrl_mem)
		}
		.overlay_ape
        {
            *(.ape_mem)
            *(.ape_ctrl_mem)
		}
		.overlay_flac
        {
            *(.flac_mem)
            *(.flac_ctrl_mem)
		}
		.overlay_m4a
        {
            *(.m4a_mem)
            *(.m4a_ctrl_mem)
	        *(.m4apick_mem)
			*(.m4apick_ctrl_mem)

		}
		.overlay_amr
        {
            *(.amr_mem)
            *(.amr_ctrl_mem)
		}
		.overlay_alac
		{
			*(.alac_ctrl_mem)
		}
		.overlay_dts
        {
            *(.dts_mem)
            *(.dts_ctrl_mem)
		}
		.overlay_fm
		{
			*(.fm_mem)
		}
        .overlay_pc
		{
#if USB_MEM_USE_OVERLAY
            *(.usb.data.bss.exchange)
#endif
		}
		/* .overlay_moveable */
		/* { */
			/* . += 0xa000 ; */
		/* } */
    } > ram0


	data_code_pc_limit_end = .;
	__report_overlay_end = .;

	_HEAP_BEGIN = . ;
	_HEAP_END = RAM0_END;

    . = ORIGIN(code0);
    .text ALIGN(4):SUBALIGN(4)
    {
        PROVIDE(text_rodata_begin = .);

        *(.entry_text)
        *(.startup.text)

        bank_stub_start = .;
        *(.bank.stub.*)
        bank_stub_size = . - bank_stub_start;

        __media_start = .;
        *(.media.*.text)
        *(.wtg_dec_code)
        *(.wtg_dec_sparse_code)
        *(.msbc_code)
        *(.msbc_codec_code)
        *(.dac.text*)
        *(.mixer.text*)
        *(.wtg_dec.text*)
        *(.audio_energy_detect.text)
        *(.audio_dac.text.cache.L2.dac)
        *(.stream.text.cache.L2)

        . = ALIGN(4);
        *(.media.*.text.const)
        *(.wtg_dec_const)
        *(.msbc_const)
        *(.msbc_codec_const)

        __media_size = . - __media_start;

        __system_start = .;
        *(.system.*)
        . = ALIGN(4);
        __system_size = . - __system_start;

        __btstack_start = .;
        *(.bt_stack_code)
        . = ALIGN(4);
        *(.bt_stack_const)
        __btstack_size = . - __btstack_start;

        __btctrler_start = .;
        *(.classic_tws_code)
        *(.classic_tws_code.esco)
        *(.link_bulk.text.cache.L1)
        *(.link_bulk_code)
        *(.link_bulk_code)
        *(.lmp.text*)
        *(.lmp_auth.text*)
        *(.tws_data_forward_code)
        *(.tws_irq_code)
        *(.tws_media_sync_code)
        *(.ble_hci.text)
        *(.ble_ll.text)
        *(.ble_rf.text)
        *(.bredr_irq_code)
        *(.bredr_irq_code)
        *(.bt_rf.text*)
        *(.classic.text*)
        *(.cte_algorithm_code)
        *(.hci_controller.text*)
        *(.hci_interface.text)
        *(.hci_interface_code)
        *(.inquriy_scan.text)
        *(.vendor_manager.text)
        *(.baseband.text.cache.L1)
        __btctrler_size = . - __btctrler_start;

        __crypto_start = .;
         *(.crypto_bigint_code)
         *(.crypto_code)
         *(.crypto_const)
         *(.uECC_code)
         *(.uECC_const)
         *(.hmac_code)
        __crypto_size = . - __crypto_start;

		*(.text)
		*(.*.text)
		*(.*.text.const)
        *(.*.text.const.cache.L2)
        *(.*.text.cache.L2*)

		*(.LOG_TAG_CONST*)
        *(.rodata*)

		. = ALIGN(4); // must at tail, make rom_code size align 4
        PROVIDE(text_rodata_end = .);

        clock_critical_handler_begin = .;
        KEEP(*(.clock_critical_txt))
        clock_critical_handler_end = .;

        . = ALIGN(4);
        chargestore_handler_begin = .;
        KEEP(*(.chargestore_callback_txt))
        chargestore_handler_end = .;

        . = ALIGN(4);
        app_msg_handler_begin = .;
		KEEP(*(.app_msg_handler))
        app_msg_handler_end = .;

        . = ALIGN(4);
        app_msg_prob_handler_begin = .;
		KEEP(*(.app_msg_prob_handler))
        app_msg_prob_handler_end = .;

        . = ALIGN(4);
        app_charge_handler_begin = .;
		KEEP(*(.app_charge_handler.0))
		KEEP(*(.app_charge_handler.1))
        app_charge_handler_end = .;

        scene_ability_begin = .;
		KEEP(*(.scene_ability))
        scene_ability_end = .;

        #include "media/framework/section_text.ld"

        gsensor_dev_begin = .;
        KEEP(*(.gsensor_dev))
        gsensor_dev_end = .;

        fm_dev_begin = .;
        KEEP(*(.fm_dev))
        fm_dev_end = .;

        fm_emitter_dev_begin = .;
        KEEP(*(.fm_emitter_dev))
        fm_emitter_dev_end = .;

        storage_device_begin = .;
        KEEP(*(.storage_device))
        storage_device_end = .;

		. = ALIGN(4);
    	key_ops_begin = .;
        KEEP(*(.key_operation))
    	key_ops_end = .;

        . = ALIGN(4);
        key_callback_begin = .;
		KEEP(*(.key_cb))
        key_callback_end = .;

		. = ALIGN(4);
		tool_interface_begin = .;
		KEEP(*(.tool_interface))
		tool_interface_end = .;

        . = ALIGN(4);
        effects_online_adjust_begin = .;
        KEEP(*(.effects_online_adjust))
        effects_online_adjust_end = .;


        . = ALIGN(4);
        __VERSION_BEGIN = .;
        KEEP(*(.sys.version))
        __VERSION_END = .;

        . = ALIGN(4);
        tws_tone_cb_begin = .;
        KEEP(*(.tws_tone_callback))
        tws_tone_cb_end = .;



        *(.noop_version)

		. = ALIGN(4);
        MEDIA_CODE_BEGIN = .;
		#include "media/cpu/br36/audio_lib_text.ld"
    	audio_sync_code_begin = .;
        *(.audio_sync_code)
    	audio_sync_code_end = .;
		. = ALIGN(4);
        _SPI_CODE_START = . ;
        *(.spi_code)
		. = ALIGN(4);
        _SPI_CODE_END = . ;

		/********maskrom arithmetic ****/
        *(.opcore_table_maskrom)
        *(.bfilt_table_maskroom)
        *(.bfilt_code)
        *(.bfilt_const)
		/********maskrom arithmetic end****/
		. = ALIGN(4);
#if (TCFG_AUDIO_CVP_CODE_AT_RAM == 0)
            . = ALIGN(4);
			aec_code_begin  = . ;
			*(.text._*)
			*(.aec_code)
			*(.aec_const)
			*(.res_code)
			*(.res_const)
            . = ALIGN(4);
            *(.dns_common_data)
            *(.dns_param_data_single)
            *(.dns_param_data_dual)
            *(.jlsp_nlp_code)
            *(.jlsp_nlp_const)
            *(.jlsp_aec_code)
            *(.jlsp_aec_const)
            *(.jlsp_prep_code)
            *(.jlsp_prep_const)
            *(.jlsp_enc_code)
            *(.jlsp_enc_const)
            *(.jlsp_wn_code)
            *(.jlsp_wn_const)
			*(.ns_code)
			*(.ns_const)
			*(.fft_code)
			*(.fft_const)
			*(.nlp_code)
			*(.nlp_const)
			*(.der_code)
			*(.der_const)
			*(.qmf_code)
			*(.qmf_const)
			*(.dms_code)
			*(.dms_const)
			*(.agc_code)
            *(.opcore_maskrom)
			aec_code_end = . ;
			aec_code_size = aec_code_end - aec_code_begin ;

#elif (TCFG_AUDIO_CVP_CODE_AT_RAM == 1)	//仅DNS部分放在overlay
            . = ALIGN(4);
			aec_code_begin  = . ;
			*(.text._*)
			. = ALIGN(4);
			*(.aec_code)
			*(.aec_const)
			*(.res_code)
			*(.res_const)
			*(.ns_code)
			*(.ns_const)
			*(.fft_code)
			*(.fft_const)
			*(.nlp_code)
			*(.nlp_const)
			*(.der_code)
			*(.der_const)
			*(.qmf_code)
			*(.qmf_const)
			*(.dms_code)
			*(.dms_const)
			*(.agc_code)
            *(.opcore_maskrom)
			aec_code_end = . ;
			aec_code_size = aec_code_end - aec_code_begin ;

#endif/*TCFG_AUDIO_CVP_CODE_AT_RAM*/
        MEDIA_CODE_SIZE = . - MEDIA_CODE_BEGIN;


		. = ALIGN(4);
	    update_target_begin = .;
	    PROVIDE(update_target_begin = .);
	    KEEP(*(.update_target))
	    update_target_end = .;
	    PROVIDE(update_target_end = .);
		. = ALIGN(4);

         . = ALIGN(4);
         __a2dp_text_cache_L2_start = .;
         *(.movable.region.1);
         . = ALIGN(4);
         __a2dp_text_cache_L2_end = .;
         . = ALIGN(4);
		#include "system/system_lib_text.ld"
        #include "btctrler/crypto/text.ld"
        #include "btctrler/btctler_lib_text.ld"

		. = ALIGN(32);
	  } > code0

}

#include "btstack/btstack_lib.ld"
//#include "btctrler/port/br36/btctler_lib.ld"
#include "update/update.ld"
#include "driver/cpu/br36/driver_lib.ld"
#include "utils/utils_lib.ld"
//================== Section Info Export ====================//
text_begin  = ADDR(.text);
text_size   = SIZEOF(.text);
text_end    = text_begin + text_size;
ASSERT((text_size % 4) == 0,"!!! text_size Not Align 4 Bytes !!!");
ASSERT(CONFIG_FLASH_SIZE > text_size,"check sdk_config.h CONFIG_FLASH_SIZE < text_size");

bss_begin = ADDR(.bss);
bss_size  = SIZEOF(.bss);
bss_end    = bss_begin + bss_size;
ASSERT((bss_size % 4) == 0,"!!! bss_size Not Align 4 Bytes !!!");

data_addr = ADDR(.data);
data_begin = text_begin + text_size;
data_size =  SIZEOF(.data);
ASSERT((data_size % 4) == 0,"!!! data_size Not Align 4 Bytes !!!");

/* moveable_slot_addr = ADDR(.moveable_slot); */
/* moveable_slot_begin = data_begin + data_size; */
/* moveable_slot_size =  SIZEOF(.moveable_slot); */

data_code_addr = ADDR(.data_code);
data_code_begin = data_begin + data_size;
data_code_size =  SIZEOF(.data_code);
ASSERT((data_code_size % 4) == 0,"!!! data_code_size Not Align 4 Bytes !!!");

//================ OVERLAY Code Info Export ==================//
aec_addr = ADDR(.overlay_aec);
aec_begin = data_code_begin + data_code_size;
aec_size =  SIZEOF(.overlay_aec);

aac_addr = ADDR(.overlay_aac);
aac_begin = aec_begin + aec_size;
aac_size =  SIZEOF(.overlay_aac);

wav_addr = ADDR(.overlay_wav);
wav_begin = aec_begin + aec_size;
wav_size =  SIZEOF(.overlay_wav);

ape_addr = ADDR(.overlay_ape);
ape_begin = wav_begin + wav_size;
ape_size =  SIZEOF(.overlay_ape);

flac_addr = ADDR(.overlay_flac);
flac_begin = ape_begin + ape_size;
flac_size =  SIZEOF(.overlay_flac);

m4a_addr = ADDR(.overlay_m4a);
m4a_begin = flac_begin + flac_size;
m4a_size =  SIZEOF(.overlay_m4a);

amr_addr = ADDR(.overlay_amr);
amr_begin = m4a_begin + m4a_size;
amr_size =  SIZEOF(.overlay_amr);

dts_addr = ADDR(.overlay_dts);
dts_begin = amr_begin + amr_size;
dts_size =  SIZEOF(.overlay_dts);


//================ BANK ==================//
bank_code_load_addr = data_code_begin + data_code_size;
/* bank_code_load_addr = aac_begin + aac_size; */

/* moveable_addr = ADDR(.overlay_moveable) ; */
/* moveable_size = SIZEOF(.overlay_moveable) ; */
//===================== HEAP Info Export =====================//
PROVIDE(HEAP_BEGIN = _HEAP_BEGIN);
PROVIDE(HEAP_END = _HEAP_END);
_MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN;
PROVIDE(MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN);

/* ASSERT(MALLOC_SIZE  >= 0x8000, "heap space too small !") */

//============================================================//
//=== report section info begin:
//============================================================//
report_text_beign = ADDR(.text);
report_text_size = SIZEOF(.text);
report_text_end = report_text_beign + report_text_size;

report_mmu_tlb_begin = ADDR(.mmu_tlb);
report_mmu_tlb_size = SIZEOF(.mmu_tlb);
report_mmu_tlb_end = report_mmu_tlb_begin + report_mmu_tlb_size;

report_boot_info_begin = ADDR(.boot_info);
report_boot_info_size = SIZEOF(.boot_info);
report_boot_info_end = report_boot_info_begin + report_boot_info_size;

report_irq_stack_begin = ADDR(.irq_stack);
report_irq_stack_size = SIZEOF(.irq_stack);
report_irq_stack_end = report_irq_stack_begin + report_irq_stack_size;

report_data_begin = ADDR(.data);
report_data_size = SIZEOF(.data);
report_data_end = report_data_begin + report_data_size;

report_bss_begin = ADDR(.bss);
report_bss_size = SIZEOF(.bss);
report_bss_end = report_bss_begin + report_bss_size;

report_data_code_begin = ADDR(.data_code);
report_data_code_size = SIZEOF(.data_code);
report_data_code_end = report_data_code_begin + report_data_code_size;

report_overlay_begin = __report_overlay_begin;
report_overlay_size = __report_overlay_end - __report_overlay_begin;
report_overlay_end = __report_overlay_end;

report_heap_beign = _HEAP_BEGIN;
report_heap_size = _HEAP_END - _HEAP_BEGIN;
report_heap_end = _HEAP_END;

//============================================================//
//=== report section info end
//============================================================//

