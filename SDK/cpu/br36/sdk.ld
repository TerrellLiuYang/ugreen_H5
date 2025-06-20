







  local_irq_enable = ABSOLUTE(0x10fe00);
  local_irq_disable = ABSOLUTE(0x10fe04);
  lp_signature_set = ABSOLUTE(0x10fe08);
  memmem = ABSOLUTE(0x10fe0c);
  memcpy = ABSOLUTE(0x10fe10);
  memmove = ABSOLUTE(0x10fe14);
  memcmp = ABSOLUTE(0x10fe18);
  memset = ABSOLUTE(0x10fe1c);
  strcmp = ABSOLUTE(0x10fe20);
  strcpy = ABSOLUTE(0x10fe24);
  strlen = ABSOLUTE(0x10fe28);
  strncmp = ABSOLUTE(0x10fe2c);
  strstr = ABSOLUTE(0x10fe30);
  strchr = ABSOLUTE(0x10fe34);
  os_init = ABSOLUTE(0x10fe38);
  os_start = ABSOLUTE(0x10fe3c);
  os_get_curr_tcb_var = ABSOLUTE(0x10fe40);
  os_task_create_rom = ABSOLUTE(0x10fe44);
  os_current_task_rom = ABSOLUTE(0x10fe48);
  os_time_dly = ABSOLUTE(0x10fe4c);
  os_time_get = ABSOLUTE(0x10fe50);
  os_task_del_req = ABSOLUTE(0x10fe54);
  os_task_del_res = ABSOLUTE(0x10fe58);
  os_task_del = ABSOLUTE(0x10fe5c);
  __os_taskq_pend = ABSOLUTE(0x10fe60);
  os_taskq_pend = ABSOLUTE(0x10fe64);
  os_task_pend = ABSOLUTE(0x10fe68);
  __os_taskq_post = ABSOLUTE(0x10fe6c);
  os_taskq_post = ABSOLUTE(0x10fe70);
  __os_taskq_post_type = ABSOLUTE(0x10fe74);
  os_taskq_flush = ABSOLUTE(0x10fe78);
  task_queue_post_event = ABSOLUTE(0x10fe7c);
  os_taskq_post_msg = ABSOLUTE(0x10fe80);
  os_taskq_post_event = ABSOLUTE(0x10fe84);
  os_taskq_accept = ABSOLUTE(0x10fe88);
  os_taskq_del = ABSOLUTE(0x10fe8c);
  os_taskq_del_type = ABSOLUTE(0x10fe90);
  os_sem_create = ABSOLUTE(0x10fe94);
  os_sem_del = ABSOLUTE(0x10fe98);
  os_sem_pend = ABSOLUTE(0x10fe9c);
  os_sem_post = ABSOLUTE(0x10fea0);
  os_sem_set = ABSOLUTE(0x10fea4);
  os_sem_valid = ABSOLUTE(0x10fea8);
  os_sem_query = ABSOLUTE(0x10feac);
  os_mutex_create = ABSOLUTE(0x10feb0);
  os_mutex_del = ABSOLUTE(0x10feb4);
  os_mutex_pend = ABSOLUTE(0x10feb8);
  os_mutex_post = ABSOLUTE(0x10febc);
  os_mutex_valid = ABSOLUTE(0x10fec0);
  xTaskCreate = ABSOLUTE(0x10fec4);
  xTaskGetCurrentTaskHandle = ABSOLUTE(0x10fec8);
  xTaskGetTickCount = ABSOLUTE(0x10fecc);
  xTaskGetHandle = ABSOLUTE(0x10fed0);
  vTaskDelete = ABSOLUTE(0x10fed4);
  xTaskGetApplicationTaskTag = ABSOLUTE(0x10fed8);
  vTaskSetApplicationTaskTag = ABSOLUTE(0x10fedc);
  xQueueGenericCreateStatic = ABSOLUTE(0x10fee0);
  xQueueGenericReset = ABSOLUTE(0x10fee4);
  xQueueGenericSend = ABSOLUTE(0x10fee8);
  xQueueGenericSendFromISR = ABSOLUTE(0x10feec);
  xQueueGenericReceive = ABSOLUTE(0x10fef0);
  xQueueReceiveFromISR = ABSOLUTE(0x10fef4);
  vQueueDelete = ABSOLUTE(0x10fef8);
  uxQueueMessagesWaiting = ABSOLUTE(0x10fefc);
  uxQueueMessagesWaitingFromISR = ABSOLUTE(0x10ff00);
  uxQueueSpacesAvailable = ABSOLUTE(0x10ff04);
  pcTaskGetName = ABSOLUTE(0x10ff08);
  prvGetExpectedIdleTime = ABSOLUTE(0x10ff0c);
  eTaskConfirmSleepModeStatus = ABSOLUTE(0x10ff10);
  vTaskStepTick = ABSOLUTE(0x10ff14);
  pxPortInitialiseStack = ABSOLUTE(0x10ff18);
  xPortStartScheduler = ABSOLUTE(0x10ff1c);
  vPortEndScheduler = ABSOLUTE(0x10ff20);
  vPortYield = ABSOLUTE(0x10ff24);
  vPortExitCritical = ABSOLUTE(0x10ff28);
  OS_ClrPending = ABSOLUTE(0x10ff2c);
  vTickISR = ABSOLUTE(0x10ff30);
  pmalloc_init = ABSOLUTE(0x10ff34);
  pmalloc = ABSOLUTE(0x10ff38);
  pfree = ABSOLUTE(0x10ff3c);
  pmalloc_get_unused_pages = ABSOLUTE(0x10ff40);
  get_physic_address = ABSOLUTE(0x10ff44);
  pmm_get_var = ABSOLUTE(0x10ff48);
  vmm_init_ = ABSOLUTE(0x10ff4c);
  vmalloc_ = ABSOLUTE(0x10ff50);
  vfree_ = ABSOLUTE(0x10ff54);
  vmem_get_phy_adr = ABSOLUTE(0x10ff58);
  vmem_start = ABSOLUTE(0x10ff5c);
  vmem_end = ABSOLUTE(0x10ff60);
  vmm_get_var = ABSOLUTE(0x10ff64);
  vmalloc_v2 = ABSOLUTE(0x10ff68);
  vfree_v2 = ABSOLUTE(0x10ff6c);
  vcopy_ = ABSOLUTE(0x10ff70);
  IcuCmdKst = ABSOLUTE(0x10ff74);
  DcuCmdKst = ABSOLUTE(0x10ff78);
  IcuCmdSend = ABSOLUTE(0x10ff7c);
  DcuCmdSend = ABSOLUTE(0x10ff80);
  flush_dcache = ABSOLUTE(0x10ff84);
  flushinv_dcache = ABSOLUTE(0x10ff88);

  sfc_resume = ABSOLUTE(0x10ff90);
  sfc_drop_cache = ABSOLUTE(0x10ff94);
  chip_crc16 = ABSOLUTE(0x10ff98);
  CrcDecode = ABSOLUTE(0x10ff9c);
  mask_init = ABSOLUTE(0x10ffa0);
  wdt_clr = ABSOLUTE(0x10ffa4);
  nvram_set_boot_state = ABSOLUTE(0x10ffa8);
  chip_reset = ABSOLUTE(0x10ffac);
  the_debug_isr = ABSOLUTE(0x10ffb0);
  printf = ABSOLUTE(0x10ffb4);
  puts = ABSOLUTE(0x10ffb8);
  doe = ABSOLUTE(0x10ffbc);
  spi_get_port = ABSOLUTE(0x10ffc0);
  _pll_init = ABSOLUTE(0x10ffc4);
  flash_poweron = ABSOLUTE(0x10ffc8);
  spi_unmount = ABSOLUTE(0x10ffcc);
  spi_port_io_init = ABSOLUTE(0x10ffd0);
  LZ4_setStreamDecode = ABSOLUTE(0x10ffd4);
  LZ4_decompress_safe_continue = ABSOLUTE(0x10ffd8);
  LZ4_decompress_generic = ABSOLUTE(0x10ffdc);
  _sdk_mkey_lock = ABSOLUTE(0x10ffe0);
  get_cvsd_codec_ops = ABSOLUTE(0x10ffe4);
  B_fir_cal_s = ABSOLUTE(0x10ffe8);
  B_array_vol = ABSOLUTE(0x10ffec);
  B_Syn_filt = ABSOLUTE(0x10fff0);
  B_Residu = ABSOLUTE(0x10fff4);
  B_comput_correlataionS = ABSOLUTE(0x10fff8);
  B_iircal = ABSOLUTE(0x10fffc);
  mdct_tab_256 = ABSOLUTE(0x1085b0);
  mdct_tab_2048 = ABSOLUTE(0x1087b0);
  sine_long_1024 = ABSOLUTE(0x1097b0);
  sine_short_128 = ABSOLUTE(0x10a7b0);
  kbd_long_1024 = ABSOLUTE(0x10a9b0);
  kbd_short_128 = ABSOLUTE(0x10b9b0);
  Huffmancb_SCL = ABSOLUTE(0x10bbb0);
  num_swb_960_window = ABSOLUTE(0x10bdc0);
  num_swb_1024_window = ABSOLUTE(0x10bdcc);
  num_swb_128_window = ABSOLUTE(0x10bdd8);
  swb_offset_1024_96 = ABSOLUTE(0x10bde4);
  swb_offset_128_96 = ABSOLUTE(0x10be38);
  swb_offset_1024_64 = ABSOLUTE(0x10be52);
  swb_offset_128_64 = ABSOLUTE(0x10beb2);
  swb_offset_1024_48 = ABSOLUTE(0x10becc);
  swb_offset_128_48 = ABSOLUTE(0x10bf30);
  swb_offset_1024_32 = ABSOLUTE(0x10bf4e);
  swb_offset_1024_24 = ABSOLUTE(0x10bfb6);
  swb_offset_128_24 = ABSOLUTE(0x10c016);
  swb_offset_1024_16 = ABSOLUTE(0x10c036);
  swb_offset_128_16 = ABSOLUTE(0x10c08e);
  swb_offset_1024_8 = ABSOLUTE(0x10c0ae);
  swb_offset_128_8 = ABSOLUTE(0x10c100);
  swb_offset_1024_window = ABSOLUTE(0x10c120);
  swb_offset_128_window = ABSOLUTE(0x10c150);
  iq_table = ABSOLUTE(0x10c180);
  tns_coef_0_3 = ABSOLUTE(0x10c584);
  tns_coef_0_4 = ABSOLUTE(0x10c5c4);
  tns_coef_1_3 = ABSOLUTE(0x10c604);
  tns_coef_1_4 = ABSOLUTE(0x10c644);
  pow05_table = ABSOLUTE(0x10c684);
  sample_rates = ABSOLUTE(0x10c6a0);
  pred_sfb_max = ABSOLUTE(0x10c6d0);
  tns_sbf_max = ABSOLUTE(0x10c6dc);
  Huffmancb_1 = ABSOLUTE(0x10c71c);
  Huffmancb_2 = ABSOLUTE(0x10c8b4);
  Huffmancb_3 = ABSOLUTE(0x10c9ec);
  Huffmancb_4 = ABSOLUTE(0x10cb24);
  Huffmancb_5 = ABSOLUTE(0x10cc54);
  Huffmancb_6 = ABSOLUTE(0x10cd9c);
  Huffmancb_7 = ABSOLUTE(0x10cedc);
  Huffmancb_8 = ABSOLUTE(0x10cfd4);
  Huffmancb_9 = ABSOLUTE(0x10d0cc);
  Huffmancb_10 = ABSOLUTE(0x10d36c);
  Huffmancb_11 = ABSOLUTE(0x10d5fc);
  AACcodeBookDTable = ABSOLUTE(0x10dabc);
  AAC_SFHUFF_TAB = ABSOLUTE(0x10bdb8);
  STFT_Win_FixHalf_M512_D160 = ABSOLUTE(0x103804);
  STFT_Win_FixHalf_M256_D160 = ABSOLUTE(0x103c04);
  STFT_Win_FixHalf_M256_D80 = ABSOLUTE(0x103e04);
  STFT_Win_FixHalf_M128_D80 = ABSOLUTE(0x104004);
  WOLAAnalysisFilter_Float_M64_m4_r1 = ABSOLUTE(0x100004);
  WOLASynthesisFilter_Float_M64_m4_r1 = ABSOLUTE(0x100404);
  WOLAAnalysisFilter_Float_M128_m4_r1 = ABSOLUTE(0x100804);
  WOLASynthesisFilter_Float_M128_m4_r1 = ABSOLUTE(0x101004);
  WOLAAnalysisFilter_Float_M256_m4_r1 = ABSOLUTE(0x101804);
  WOLASynthesisFilter_Float_M256_m4_r1 = ABSOLUTE(0x102804);
  quantize_intervals_LF = ABSOLUTE(0x10db24);
  invert_quantize_dither_factors_LF = ABSOLUTE(0x10dc28);
  quantize_dither_factors_LF = ABSOLUTE(0x10dd2c);
  quantize_factor_select_offset_LF = ABSOLUTE(0x10de30);
  hd_quantize_intervals_LF = ABSOLUTE(0x10deb4);
  sf_sin_tab1 = ABSOLUTE(0x104104);
  boot_arg_list = ABSOLUTE(0x2bc04);
  _IRQ_MEM_ADDR = ABSOLUTE(0x2bf00);
  _MASK_MEM_BEGIN = ABSOLUTE(0x2bbe0);
  _MASK_MEM_SIZE = ABSOLUTE(0x30c);

EXTERN(
_start




automute_node_adapter
source_node_adapter
mixer_node_adapter
dac_node_adapter
decoder_node_adapter
resample_node_adapter
bt_audio_sync_node_adapter
adc_file_plug
tone_file_plug
ring_file_plug
key_tone_file_plug
sbc_hwaccel
sbc_decoder_plug
msbc_decoder_plug




msbc_encoder_hw_plug



aac_dec_plug


sine_dec_plug
cvsd_encoder_plug
cvsd_decoder_plug
eq_node_adapter
wav_dec_plug
wts_dec_plug
sdfile_resfile_ops
effect_dev0_node_adapter
);



UPDATA_SIZE = 0x80;
UPDATA_BEG = _MASK_MEM_BEGIN - UPDATA_SIZE;
UPDATA_BREDR_BASE_BEG = 0x2c000;

RAM_LIMIT_L = 0;
RAM_LIMIT_H = UPDATA_BEG;
PHY_RAM_SIZE = RAM_LIMIT_H - RAM_LIMIT_L;


ISR_BASE = _IRQ_MEM_ADDR;
ROM_RAM_SIZE = _MASK_MEM_SIZE;
ROM_RAM_BEG = _MASK_MEM_BEGIN;

RAM0_BEG = RAM_LIMIT_L;
RAM0_END = RAM_LIMIT_H;
RAM0_SIZE = RAM0_END - RAM0_BEG;







CODE_BEG = 0x1E00100;




MEMORY
{
 code0(rx) : ORIGIN = CODE_BEG, LENGTH = 2097152
 ram0(rwx) : ORIGIN = RAM0_BEG, LENGTH = RAM0_SIZE
}


ENTRY(_start)

SECTIONS
{

    . = ORIGIN(ram0);

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


 . = ALIGN(4);
 audio_sync_data_begin = .;
        *(.audio_sync_data)
 audio_sync_data_end = .;
 . = ALIGN(4);

 *(.mp3_dec.data)
 *(.mp3.data)
 *(.wma_dec.data)
 *(.wma.data)
    *(.wav_dec.data)
    *(.wav.data)
 *(.flac_dec.data)
 *(.flac.data)

    *(.mp3tsy_dec.data)
    *(.mty.data)
 *(.f2a_dec.data)
 *(.f2a.data)

    *(.sbc_data)
    *(.msbc_data)
    *(.cvsd_data)
    *(.bt_aac_dec.data)
    *(.aac.data)
    *(.aptx_dec.data)
    *(.ldac_dec.data)
 *(.sbc_eng_data)
 *(.bt_audioplc_data)
    *(.lc3_decoder_data)





    *(.wtg_dec.data)
    *(.g729.data)
    *(.wtgv2_dec.data)
    *(.wts.data)
    *(.dts_dec.data)
    *(.dts.data)

 *(.m4a_dec.data)
 *(.m4a.data)
 *(.alac.data)

 *(.amr_dec.data)
 *(.amr.data)

 *(.anc_data)
 *(.anc_core_data)
 *(.anc_user_data)
 *(.anc_box_data)
 *(.anc_btspp_data)
 *(.anc_debug_data)

 *(.ape_dec.data)
 *(.ape.data)
 *(.audio_track_data)

 *(.audio_eq_data)
    *(.audio_drc_data)
    *(.audio_vbass_data)
    *(.audio_surround_data)
 *(.audio_pitch_data)
 *(.audio_pitchspeed_data)
    *(.pitchshifter_data)
 *(.audio_noisegate_data)
 *(.howlings_phf_data)
    *(.audio_reverb_data)
    *(.audio_echo_data)




 audio_cvp_data_begin = .;
    *(.jlsp_aec_data)
    *(.jlsp_nlp_data)
    *(.jlsp_dns_data)
    *(.jlsp_enc_data)
    *(.jlsp_prep_data)
    *(.jlsp_wn_data)
 *(.sms_data)
 *(.dms_data)
 *(.data._*)
 *(.aec_data)
 *(.res_data)
 *(.ns_data)
 *(.jlsp_data)
 *(.nlp_data)
 *(.der_data)
 *(.qmf_data)
 *(.fft_data)
 *(.aec_mux)
 audio_cvp_data_end = .;
 audio_cvp_data_size = audio_cvp_data_end - audio_cvp_data_begin ;

  . = ALIGN(4);


  . = ALIGN(4);
  _static_hi_timer_begin = .;
  PROVIDE(static_hi_timer_begin = .);
  KEEP(*(.hi_timer))
  _static_hi_timer_end = .;
  PROVIDE(static_hi_timer_end = .);

  . = ALIGN(4);
  _sys_cpu_timer_begin = .;
  PROVIDE(sys_cpu_timer_begin = .);
  KEEP(*(.sys_cpu_timer))
  _sys_cpu_timer_end = .;
  PROVIDE(sys_cpu_timer_end = .);
  . = ALIGN(4);

     *(.crypto_ecdh_data)
     *(.crypto_data)
     *(.uECC_data)
     *(.ECDH_sample_data)

  . = ALIGN(4);

 } > ram0

 .bss ALIGN(32):SUBALIGN(4)
    {
        . = ALIGN(4);


        . = ALIGN(4);
        *(.bss)
        *(.*.data.bss)
        . = ALIGN(4);
        *(.*.data.bss.nv)

        . = ALIGN(4);


 . = ALIGN(4);
 audio_sync_bss_begin = .;
        *(.audio_sync_bss)
 audio_sync_bss_end = .;
 . = ALIGN(4);
 *(.jlsp_bss)
 *(.mp3_dec.data.bss)
 *(.mp3.data.bss)
 *(.mp3tsy.data.bss)
 *(.mty.data.bss)
 *(.flac_dec.data.bss)
 *(.flac.data.bss)
    *(.der_bss)

 *(.wma_dec.data.bss)
 *(.wma.data.bss)
 *(.wav.data.bss)
 *(.wav_dec.data.bss)
  *(.f2a_dec.data.bss)
  *(.f2a.data.bss)

    *(.cvsd_bss)
    *(.bt_aac_dec.data.bss)
    *(.btaac_ron_dec.data.bss)
    *(.aac.data.bss)
    *(.aptx_dec.data.bss)
    *(.ldac_dec.data.bss)
 *(.sbc_eng_bss)
 *(.bt_audioplc_bss)
    *(.lc3_decoder_bss)




    *(.wtg_dec.data.bss)
    *(.g729.data.bss)
    *(.wtgv2_dec.data.bss)
    *(.wts.data.bss)
    *(.dts_dec.data.bss)
    *(.dts.data.bss)

 *(.anc_bss)
 *(.anc_core_bss)
 *(.anc_user_bss)
 *(.anc_box_bss)
 *(.anc_btspp_bss)
 *(.anc_debug_bss)

 *(.ape_dec.data.bss)
 *(.ape.data.bss)

 *(.amr_dec.data.bss)
 *(.amr.data.bss)

 *(.m4a_dec.data.bss)
 *(.m4a.data.bss)
 *(.alac.data.bss)
  *(.audio_track_bss)

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

        *(.usb.data.bss.exchange)


        . = ALIGN(4);
  *(.non_volatile_ram)



        *(.uECC_bss)
        *(.ECDH_sample_bss)
 . = ALIGN(32);

    } > ram0

 .data_code ALIGN(32):SUBALIGN(4)
 {
        data_code_pc_limit_begin = .;
  *(.bank_critical_code)



        media_data_code_start = .;
        . = ALIGN(4);
        . = ALIGN(4);
        media_data_code_end = .;

        . = ALIGN(4);
    } > ram0

    .common ALIGN(32):SUBALIGN(4)
    {

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




       . = ALIGN(4);
    } > ram0






 __report_overlay_begin = .;
 overlay_code_begin = .;
 OVERLAY : AT(0x200000) SUBALIGN(4)
    {
  .overlay_aec
  {
  }

  .overlay_aac
  {
  }
    } > ram0


    .ram0_empty0 ALIGN(4) :
 {
        . = . + 4;
    } > ram0


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



  }




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

  . = ALIGN(4);
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



        . = ALIGN(4);
        stream_node_adapter_begin = .;
        KEEP(*(.stream_node_adapter))
        stream_node_adapter_end = .;

        . = ALIGN(4);
        source_node_plug_begin = .;
        KEEP(*(.source_node_plug))
        source_node_plug_end = .;

        . = ALIGN(4);
        decoder_plug_begin = .;
        KEEP(*(.decoder_plug))
        decoder_plug_end = .;

        . = ALIGN(4);
        encoder_plug_begin = .;
        KEEP(*(.encoder_plug))
        encoder_plug_end = .;

        . = ALIGN(4);
        audio_package_begin = .;
        KEEP(*(.audio_package_handler))
        audio_package_end = .;

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


  . = ALIGN(4);
        media_text_start = .;

        . = ALIGN(4);
        *(.dns_16k_data)
        *(.dns_8k_data)
  *(.jlsp_const)
  *(.jlsp_code)
        *(.jlsp_dns_code)
        *(.jlsp_dns_const)
        *(.dns_param_data_wind)
        *(.dns_common_data_wind)
  *(.der_code)
  *(.der_const)
  *(.ns_sparse_code )
  *(.aec_sparse_code)
  *(.nlp_sparse_code)
  *(.der_sparse_code)
  *(.qmf_sparse_code)
  *(.bt_audioplc_code)
  *(.bt_audioplc_sparse_code)
  *(.bt_audioplc_const)
        *(.lf_audioplc_code)
  *(.lf_audioplc_sparse_code)
  *(.lf_audioplc_const)

  *(.dms_sparse_code)
     *(.bark_const)

  . = ALIGN(4);
        *(.pcm_code)
        *(.pcm_const)

        *(.cvsd_code)
        *(.cvsd_const)

  *(.wtg_dec.text)
        *(.g729.text)
        *(.g729.text.const)
  *(.wtgv2_dec.text)
  *(.wts.text)
  *(.wts.text.const)

  *(.mp3_dec.text)
  *(.mp3.text)
  *(.mp3.text.const)
  *(.mp3tsy_dec.text)
  *(.mty.text)
  *(.mty.text.const)

     *(.f2a_dec.text)
     *(.f2a.text)
     *(.f2a.text.const)

  *(.ape_dec.text)
  *(.ape_dec.text.const)
  *(.ape.text)
  *(.ape.text.const)

        *(.wma_dec.text)
  *(.wma.text)
  *(.wma.text.const)

        *(.wav_dec.text)
  *(.wav.text)
  *(.wav.text.const)

        *( .wtg_dec_code )
        *( .wtg_dec_const)
        *( .wtg_dec_sparse_code)


  *(.msbc_code)
  *(.msbc_const)
  *(.msbc_codec_code)
  *(.msbc_codec_const)
  *(.sbc_code)
  *(.sbc_const)
  *(.sine_code)
  *(.sine_const)





        *(.bt_aac_dec.text)
     *(.bt_aac_dec.text.const)
     *(.btaac_rom_dec.text)
     *(.aac.text.const)
     *(.aac.text)
        *(.aptx_dec.text)
     *(.aptx_dec.text.const)
        *(.ldac_dec.text)
     *(.ldac_dec.text.const)
     *(.amr_dec.text)
        *(.amr_dec.text.const)
     *(.amr.text)
        *(.amr.text.const)

  *(.audio_decoder_code)
  *(.audio_decoder_const)
        *(.audio_track_const)
        *(.audio_track_code)


  *(.dts_dec.text)
  *(.dts_dec.text.const)
  *(.dts.text)
  *(.dts.text.const)

  *(.flac_dec.text)
  *(.flac_dec.text.const)
  *(.flac.text)
  *(.flac.text.const)
     *(.m4a_dec.text)
     *(.m4a_dec.text.const)
     *(.m4a_dec.bt_aac.text.const)
     *(.m4a.text)
     *(.m4a.text.const)
     *(.alac.text)
     *(.alac.text.const)


  *(.media_device_code)
  *(.media_device_const)
  *(.audio_encoder_code)
  *(.audio_encoder_const)
  *(.mixer_code)
  *(.mixer_const)
  *(.dec_server_code)
  *(.dec_server_const)
  *(.rec_server_code)
  *(.rec_server_const)
  *(.auto_mute_code)
  *(.auto_mute_const)
  *(.plc_code)
  *(.plc_const)
  *(.wireless_sync_code)
  *(.wireless_sync_const)
  *(.sbc_eng_code)
  *(.sbc_eng_const)

     anc_code_begin = .;
  *(.anc_user_const)
  *(.anc_user_code)
  *(.anc_const)
  *(.anc_code)
  *(.anc_core_const)
  *(.anc_core_code)
  *(.anc_box_const)
  *(.anc_box_code)
  *(.anc_btspp_const)
  *(.anc_btspp_code)
  *(.anc_debug_const)
  *(.anc_debug_code)
     anc_code_end = .;
  anc_code_size = anc_code_end - anc_code_begin;

  *(.audio_codec_code)

  *(.bt_compressor_sparse_const)
  *(.bt_compressor_sparse_code)
  *(.compressor_sparse_code)
  *(.compressor_sparse_const)
  *(.drc_sparse_code)
  *(.drc_sparse_const)

  *(.bt_limiter_sparse_const)
  *(.bt_limiter_sparse_code)
  *(.limiter_sparse_code)
  *(.limiter_sparse_const)

  *(.bt_crossOver_sparse_const)
  *(.bt_crossOver_sparse_code)
  *(.crossOver_sparse_code)
  *(.crossOver_sparse_const)
        *(.reverb_cal_sparse_code)


        *(.reverb_cal_code)
        *(.reverb_cal_const)
  *(.platereverb_code)
  *(.platereverb_const)
     *(.audio_reverb_code)
        *(.audio_reverb_const)
        *(.reverb0_code)
  *(.reverb0_const)



     *(.audio_echo_code)
        *(.audio_echo_const)
  *(.echo_cal_code)
  *(.echo_cal_const)





  *(.audio_eq_code)
  *(.audio_eq_const)


     *(.audio_drc_code)
  *(.audio_drc_const)
        *(.drc_const)
  *(.drc_code)
  *(.sw_drc_code)
  *(.sw_drc_const)



        *(.howlings_phf_code)
  *(.howlings_phf_const)

        *(.notchhowling_code)
  *(.notchhowling_const)
  *(.notchhowling_sparse_code)
  *(.notchhowling_sparse_const)

  *(.howlings_phf_sparse_code)


  *(.audio_noisegate_code)
  *(.audio_noisegate_const)

  *(.noisegate_code)
  *(.noisegate_const)
  *(.noisegate_sparse_code)
  *(.noisegate_sparse_const)





  *(.audio_gain_code)
  *(.audio_gain_const)



  *(.dynamic_eq_code)
  *(.dynamic_eq_const)
     *(.dynamic_eq_detect_code)
  *(.dynamic_eq_detect_const)
  *(.dynamic_eq_sparse_code)
  *(.dynamic_eq_sparse_const)
     *(.dynamic_eq_detect_sparse_code)
  *(.dynamic_eq_detect_sparse_const)




  *(.audio_mic_effect_const)
  *(.audio_mic_effect_code)


  *(.audio_mic_stream_const)
  *(.audio_mic_stream_code)


  *(.audio_effect_adj_const)
  *(.audio_effect_adj_code)



  *(.audio_vbass_code)
  *(.audio_vbass_const)
  *(.vbss_code)
  *(.vbss_const)
        *(.vbss_sparse_code)
  *(.vbss_sparse_const)


        *(.pitchshifter_code)
     *(.pitchshifter_const)
  *(.audio_pitch_code)
  *(.audio_pitch_const)
  *(.audio_pitchspeed_code)
  *(.audio_pitchspeed_const)
        *(.pitchshifter_sparse_code)




  *(.audio_surround_code)
  *(.audio_surround_const)
  *(.sur_cal_common_code)
        *(.sur_TYPE0_cal_code)
     *(.sur_TYPE1_cal_code)
  *(.sur_TYPE0_cal_const)
     *(.sur_TYPE1_cal_const)

  *(.sur_LRDRIFT2_cal_code)
  *(.sur_LRDRIFT2_cal_const)
     *(.sur_ROTATE_cal_code)
     *(.sur_ROTATE_cal_const)
        *(.sur_LRDRIFT_cal_code)
        *(.sur_LRDRIFT_cal_const)
  *(.sur_cal_sparse_code)

     *(.mp3_encode_code)
     *(.mp3_enc_code)
  *(.mp3_enc_const)
  *(.mp3_enc_sparse_code)
  *(.mp3_enc_sparse_const)
  . = ALIGN(4);
  *(.mp2_encode_code)
  *(.mp2_encode_const)
  *(.mp2_encode_sparse_code)
  *(.mp2_encode_sparse_const)
  . = ALIGN(4);
  *(.adpcm_encode_code)
  *(.adpcm_encode_const)
  *(.adpcm_encode_sparse_code)
  *(.adpcm_encode_sparse_const)
  . = ALIGN(4);


  . = ALIGN(4);
     _audio_decoder_begin = .;
     PROVIDE(audio_decoder_begin = .);
     KEEP(*(.audio_decoder))
     _audio_decoder_end = .;
     PROVIDE(audio_decoder_end = .);

     _audio_encoder_begin = .;
     PROVIDE(audio_encoder_begin = .);
     KEEP(*(.audio_encoder))
     _audio_encoder_end = .;
     PROVIDE(audio_encoder_end = .);

     _audio_package_begin = .;
     PROVIDE(audio_package_begin = .);
     KEEP(*(.audio_package))
     _audio_package_end = .;
     PROVIDE(audio_package_end = .);

     _audio_dev_begin = .;
     PROVIDE(audio_dev_begin = .);
     KEEP(*(.audio_device))
     _audio_dev_end = .;
     PROVIDE(audio_dev_end = .);

     _audio_hwaccel_begin = .;
     PROVIDE(audio_hwaccel_begin = .);
     KEEP(*(.audio_hwaccel))
     _audio_hwaccel_end = .;
     PROVIDE(audio_hwaccel_end = .);

  . = ALIGN(4);
     media_code_begin = .;
         *(.media.*.text)
  . = ALIGN(4);

  *(.app_audio_code)
  *(.app_audio_const)

  *(.compressor_code)
  *(.compressor_const)
  *(.bt_compressor_const)
  *(.bt_compressor_code)

  *(.crossOver_code)
  *(.crossOver_const)
  *(.bt_crossOver_const)
  *(.bt_crossOver_code)

  *(.limiter_code)
  *(.limiter_const)
  *(.bt_limiter_const)
  *(.bt_limiter_code)
  *(.drc_const)
  *(.drc_code)

     *(.audio_gain_code)
  *(.audio_gain_const)

     *(.vbss_code)
  *(.vbss_const)
        *(.vbss_sparse_code)
  *(.vbss_sparse_const)

        *(.specshow_code)
        *(.specshow_const)
        *(.specshow_sparse_code)
        *(.specshow_sparse_const)
  *(.spectrum_fft_code)
  *(.spectrum_fft_const)
        *(.spectrum_eq_code)
  *(.spectrum_eq_const)
        *(.audio_vocal_remove_code)


     . = ALIGN(4);
  *(.sur_cal_const)
  *(.sur_cal_code)
        . = ALIGN(4);
     *(.lib_pitchshift_code)
     *(.lib_pitchshift_const)
     . = ALIGN(4);
        *(.lc3_decoder_code)
        *(.lc3_decoder_const)
            *(.speex_code)
     *(.opus_code)
     *(.opus_encoder_code)
     *(.speex_encoder_code)
     *(.resample_fastcal_sparse_code)
            *(.resample_fastcal_code)
            *(.resample_fastcal_const)
     . = ALIGN(4);

     media_code_end = .;
  . = ALIGN(4);
  media_code_size = media_code_end - media_code_begin;

  . = ALIGN(4);
        media_text_end = .;
 audio_sync_code_begin = .;
        *(.audio_sync_code)
     audio_sync_code_end = .;
  . = ALIGN(4);
        _SPI_CODE_START = . ;
        *(.spi_code)
  . = ALIGN(4);
        _SPI_CODE_END = . ;


        *(.opcore_table_maskrom)
        *(.bfilt_table_maskroom)
        *(.bfilt_code)
        *(.bfilt_const)

  . = ALIGN(4);

            . = ALIGN(4);
   aec_code_begin = . ;
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


  . = ALIGN(4);
        system_text_start = .;

   _device_node_begin = .;
     PROVIDE(device_node_begin = .);
     KEEP(*(.device))
     _device_node_end = .;
     PROVIDE(device_node_end = .);

  config_target_begin = .;
     PROVIDE(config_target_begin = .);
     KEEP(*(.config_target))
  config_target_end = .;
     PROVIDE(config_target_end = .);

     system_code_begin = .;
     KEEP(*(.system.*.text))
     system_code_end = .;
  . = ALIGN(4);
  system_code_size = system_code_end - system_code_begin;

     _lib_version_begin = .;
     PROVIDE(lib_version_begin = .);
     KEEP(*(.lib_version))
     _lib_version_end = .;
     PROVIDE(lib_version_end = .);

  _initcall_begin = .;
  PROVIDE(initcall_begin = .);
     KEEP(*(.initcall))
  _initcall_end = .;
  PROVIDE(initcall_end = .);

  _early_initcall_begin = .;
  PROVIDE(early_initcall_begin = .);
  KEEP(*(.early.initcall))
  _early_initcall_end = .;
  PROVIDE(early_initcall_end = .);

  _late_initcall_begin = .;
  PROVIDE(late_initcall_begin = .);
  KEEP(*(.late.initcall))
  _late_initcall_end = .;
  PROVIDE(late_initcall_end = .);

  _platform_initcall_begin = .;
  PROVIDE(platform_initcall_begin = .);
  KEEP(*(.platform.initcall))
  _platform_initcall_end = .;
  PROVIDE(platform_initcall_end = .);

  _module_initcall_begin = .;
  PROVIDE(module_initcall_begin = .);
  KEEP(*(.module.initcall))
  _module_initcall_end = .;
  PROVIDE(module_initcall_end = .);

  platform_uninitcall_begin = .;
  KEEP(*(.platform.uninitcall))
  platform_uninitcall_end = .;

  _sys_event_handler_begin = .;
  PROVIDE(sys_event_handler_begin = .);
  KEEP(*(.sys_event.4.handler))
  KEEP(*(.sys_event.3.handler))
  KEEP(*(.sys_event.2.handler))
  KEEP(*(.sys_event.1.handler))
  KEEP(*(.sys_event.0.handler))
  _sys_event_handler_end = .;
  PROVIDE(sys_event_handler_end = .);

  _syscfg_ops_begin = .;
  PROVIDE(syscfg_ops_begin = .);
  KEEP(*(.syscfg.2.ops))
  KEEP(*(.syscfg.1.ops))
  KEEP(*(.syscfg.0.ops))
  _syscfg_ops_end = .;
  PROVIDE(syscfg_ops_end = .);

  . = ALIGN(4);
     lp_target_begin = .;
     PROVIDE(lp_target_begin = .);
     KEEP(*(.lp_target))
     lp_target_end = .;
     PROVIDE(lp_target_end = .);

  . = ALIGN(4);
     lp_request_begin = .;
     PROVIDE(lp_request_begin = .);
     KEEP(*(.lp_request))
     lp_request_end = .;
     PROVIDE(lp_request_end = .);

  . = ALIGN(4);
        deepsleep_target_begin = .;
        PROVIDE(deepsleep_target_begin = .);
        KEEP(*(.deepsleep_target))
        deepsleep_target_end = .;
        PROVIDE(deepsleep_target_end = .);

  . = ALIGN(4);
  p2m_msg_handler_begin = .;
  PROVIDE(p2m_msg_handler_begin = .);
  KEEP(*(.p2m_msg_handler))
  PROVIDE(p2m_msg_handler_end = .);
  p2m_msg_handler_end = .;

        *(.mem_code)
        *(.mem_const)

        *(.os_port_code)
        *(.os_port_const)

        *(.math_fast_funtion_code)
  . = ALIGN(4);

        system_text_end = .;

        system_code_total_size = system_text_end - system_text_start;

  *(.crypto_ecdh_code)
     *(.crypto_ecdh_const)

     *(.crypto_bigint_code)
     *(.crypto_bigint_const)

     *(.crypto_code)
     *(.crypto_const)

     *(.ECDH_sample_code)
     *(.ECDH_sample_const)

     *(.uECC_code)
     *(.uECC_const)

     *(.hmac_code)
     *(.hmac_const)

     *(.hash_sample_code)
     *(.hash_sample_const)

     *(.aes_cmac_sample_code)
  *(.aes_cmac_sample_const)

  . = ALIGN(4);
        tws_sync_call_begin = .;
        KEEP(*(.tws_sync_call))
        tws_sync_call_end = .;

  . = ALIGN(4);
        tws_func_stub_begin = .;
        KEEP(*(.tws_func_stub))
        tws_func_stub_end = .;

  . = ALIGN(32);
   } > code0

}


SECTIONS
{
    .data : ALIGN(4)
    {
        btstack_data_start = .;
        *(.bt_stack_data)
        *(.ble_db_data)
        *(.ble_sm_data)
        *(.ble_att_data)
        *(.ble_gatt_data)


        BTSTACK_LE_HOST_MESH_DATA_START = .;
  . = (. +3) & ~ 3;
        _net_buf_pool_list = .;
        *(._net_buf_pool.static.*)

        *(.ble_mesh_data)
        *(.ble_mesh_tinycrypt_data)
        BTSTACK_LE_HOST_MESH_DATA_SIZE = ABSOLUTE(. - BTSTACK_LE_HOST_MESH_DATA_START);

        btstack_data_end = .;

    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
        btstack_bss_start = .;
        *(.bt_stack_bss)
        *(.ble_db_bss)
        *(.ble_sm_bss)
        *(.ble_att_bss)
        *(.ble_gatt_bss)
        *(.btstack_pool)


        BTSTACK_LE_HOST_MESH_BSS_START = .;
        *(.ble_mesh_bss)
        *(.ble_mesh_tinycrypt_bss)
        BTSTACK_LE_HOST_MESH_BSS_SIZE = ABSOLUTE(. - BTSTACK_LE_HOST_MESH_BSS_START);

        btstack_bss_end = .;
    } > ram0

    .text : ALIGN(4)
    {
        btstack_code_start = .;

        . = ALIGN(4);

        a2dp_source_media_codec_begin = .;
        KEEP(*(.a2dp_source_media_codec))
            a2dp_source_media_codec_end = .;
        a2dp_sink_media_probe_begin = .;
        KEEP(*(.a2dp_sink_media_probe))
            a2dp_sink_media_probe_end = .;

        a2dp_sink_media_codec_begin = .;
        KEEP(*(.a2dp_sink_media_codec))
            a2dp_sink_media_codec_end = .;

        a2dp_event_handler_begin = .;
        KEEP(*(.a2dp_event_handler))
            a2dp_event_handler_end = .;

        sdp_record_item_begin = .;
        KEEP(*(.sdp_record_item))
            sdp_record_item_end = .;

        bt_sleep_begin = .;
        KEEP(*(.bt_sleep))
            bt_sleep_end = .;

  *(.bt_stack_const)
  *(.bt_stack_code)
        *(.ble_db_const)
        *(.ble_db_code)
        *(.ble_sm_const)
        *(.ble_sm_code)
        *(.ble_att_const)
        *(.ble_att_code)
        *(.ble_gatt_const)
        *(.ble_gatt_code)


        BTSTACK_LE_HOST_MESH_CODE_START = .;
        *(.ble_mesh_code)
        *(.ble_mesh_tinycrypt_code)

        *(.ble_mesh_const)
        *(.ble_mesh_tinycrypt_const)
        BTSTACK_LE_HOST_MESH_CODE_SIZE = ABSOLUTE(. - BTSTACK_LE_HOST_MESH_CODE_START);

        btstack_code_end = .;
  . = ALIGN(4);
    } > code0
}

BTSTACK_LE_HOST_MESH_RAM_TOTAL = BTSTACK_LE_HOST_MESH_DATA_SIZE + BTSTACK_LE_HOST_MESH_BSS_SIZE;
BTSTACK_LE_HOST_MESH_FLASH_TOTAL = BTSTACK_LE_HOST_MESH_CODE_SIZE;

BTSTACK_CODE_SIZE = (btstack_code_end - btstack_code_start) + (btstack_data_end - btstack_data_start);



SECTIONS
{
    .data : ALIGN(4)
    {
        update_data_start = .;

        update_data_end = .;
    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
        update_bss_start = .;

        *(.update_bss)
        update_bss_end = .;
    } > ram0

    .text : ALIGN(4)
    {
  update_code_start = .;

  *(.bt_updata_ram_code)
  *(.update_const)
  *(.update_code)

  update_code_end = .;

  . = ALIGN(4);
    } > code0


    UPDATE_CODE_TOTAL_SIZE = update_code_end - update_code_start;
}
SECTIONS
{
    .data ALIGN(4):SUBALIGN(4)
    {
        driver_data_start = .;


        CLOCK_DATA_START = .;
        *(.clock_data)
        CLOCK_DATA_SIZE = ABSOLUTE(. - CLOCK_DATA_START);

        *(.debug_data)
        *(.uart_data)

        driver_data_end = .;
        . = ALIGN(32);
    } > ram0

    .bss ALIGN(4):SUBALIGN(4)
    {
        driver_bss_start = .;

        CLOCK_BSS_START = .;
        *(.clock_bss)
        CLOCK_BSS_SIZE = ABSOLUTE(. - CLOCK_BSS_START);

        *(.debug_bss)

        *(.uart_bss)

        *(.sd_var)

  . = ALIGN(4);
        driver_bss_end = .;
    } > ram0

    .text ALIGN(4):SUBALIGN(4)
    {
        driver_code_start = .;

        *(.fm_code)

        CLOCK_CODE_START = .;
        *(.clock_code)
        *(.clock_const)
        CLOCK_CODE_SIZE = ABSOLUTE(. - CLOCK_CODE_START);

        *(.debug_code)
        *(.debug_const)


        *(.uart_code)
        *(.uart_const)

        driver_code_end = .;
  . = ALIGN(4);
    } > code0


    .data_code : ALIGN(4)
    {
        driver_data_code_start = .;

  driver_data_code_end = .;
  . = ALIGN(4);
    } > ram0


    DRIVER_RAM_TOTAL = (driver_data_end - driver_data_start) + (driver_bss_end - driver_bss_start);
    DRIVER_CODE_TOTAL = (driver_code_end - driver_code_start);
    DRIVER_DATA_CODE_TOTAL = (driver_data_code_end - driver_data_code_start);
}
SECTIONS
{
    .data : ALIGN(4)
    {
        utils_data_start = .;
        *(.utils_data)
        utils_data_end = .;
  . = ALIGN(4);

    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
        utils_bss_start = .;
        *(.utils_bss)
        utils_bss_end = .;
  . = ALIGN(4);

    } > ram0

    .text : ALIGN(4)
    {
        utils_text_start = .;
        *(.utils_text)

  . = ALIGN(4);
  vfs_ops_begin = .;
  KEEP(*(.vfs_operations))
  vfs_ops_end = .;

  . = ALIGN(4);
     _resfile_ops_begin = .;
  PROVIDE(resfile_ops_begin = .);
     KEEP(*(.resfile_ops))
     _resfile_ops_end = .;
     PROVIDE(resfile_ops_end = .);

        utils_text_end = .;
  . = ALIGN(4);
    } > code0

 .data_code ALIGN(4):
 {
  *(.utils_data_code)
  . = ALIGN(4);
    } > ram0
}

text_begin = ADDR(.text);
text_size = SIZEOF(.text);
text_end = text_begin + text_size;
ASSERT((text_size % 4) == 0,"!!! text_size Not Align 4 Bytes !!!");
ASSERT(2097152 > text_size,"check sdk_config.h CONFIG_FLASH_SIZE < text_size");

bss_begin = ADDR(.bss);
bss_size = SIZEOF(.bss);
bss_end = bss_begin + bss_size;
ASSERT((bss_size % 4) == 0,"!!! bss_size Not Align 4 Bytes !!!");

data_addr = ADDR(.data);
data_begin = text_begin + text_size;
data_size = SIZEOF(.data);
ASSERT((data_size % 4) == 0,"!!! data_size Not Align 4 Bytes !!!");





data_code_addr = ADDR(.data_code);
data_code_begin = data_begin + data_size;
data_code_size = SIZEOF(.data_code);
ASSERT((data_code_size % 4) == 0,"!!! data_code_size Not Align 4 Bytes !!!");


aec_addr = ADDR(.overlay_aec);
aec_begin = data_code_begin + data_code_size;
aec_size = SIZEOF(.overlay_aec);

aac_addr = ADDR(.overlay_aac);
aac_begin = aec_begin + aec_size;
aac_size = SIZEOF(.overlay_aac);

wav_addr = ADDR(.overlay_wav);
wav_begin = aec_begin + aec_size;
wav_size = SIZEOF(.overlay_wav);

ape_addr = ADDR(.overlay_ape);
ape_begin = wav_begin + wav_size;
ape_size = SIZEOF(.overlay_ape);

flac_addr = ADDR(.overlay_flac);
flac_begin = ape_begin + ape_size;
flac_size = SIZEOF(.overlay_flac);

m4a_addr = ADDR(.overlay_m4a);
m4a_begin = flac_begin + flac_size;
m4a_size = SIZEOF(.overlay_m4a);

amr_addr = ADDR(.overlay_amr);
amr_begin = m4a_begin + m4a_size;
amr_size = SIZEOF(.overlay_amr);

dts_addr = ADDR(.overlay_dts);
dts_begin = amr_begin + amr_size;
dts_size = SIZEOF(.overlay_dts);



bank_code_load_addr = data_code_begin + data_code_size;





PROVIDE(HEAP_BEGIN = _HEAP_BEGIN);
PROVIDE(HEAP_END = _HEAP_END);
_MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN;
PROVIDE(MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN);






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
