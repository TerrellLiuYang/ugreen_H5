
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
#if (TCFG_ENC_LC3_ENABLE || TCFG_DEC_LC3_ENABLE)
		*(.lc3_codec_ari_c_const)
		*(.lc3_codec_ari_c_data)
		*(.lc3_codec_c_data)
#endif
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
