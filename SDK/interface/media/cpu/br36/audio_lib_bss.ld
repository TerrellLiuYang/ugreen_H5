
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
#if (TCFG_ENC_LC3_ENABLE || TCFG_DEC_LC3_ENABLE)
	*(.lc3_codec_ari_c_bss)
	*(.lc3_codec_c_bss)
#endif
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

