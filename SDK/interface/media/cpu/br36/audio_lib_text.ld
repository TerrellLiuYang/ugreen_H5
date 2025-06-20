
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
        /*
		*(.aec_code)
		*(.aec_const)

		. = ALIGN(4);
		*(.res_code)
		*(.res_const)

		. = ALIGN(4);
		*(.ns_code)
		*(.ns_const)

		*(.nlp_code)
		*(.nlp_const)
		*(.der_code)
		*(.der_const)
		*(.qmf_code)
		*(.qmf_const)
		*(.fft_code)
		*(.fft_const)
        */

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

        /**(.bfilt_code)*/
		*(.msbc_code)
		*(.msbc_const)
		*(.msbc_codec_code)
		*(.msbc_codec_const)
		*(.sbc_code)
		*(.sbc_const)
		*(.sine_code)
		*(.sine_const)
#ifdef CONFIG_MOVABLE_ENABLE
    	audio_sync_code_begin = .;
        *(.audio_sync_code)
    	audio_sync_code_end = .;
#endif
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
		/* *(.cvsd_code) */
		/* *(.cvsd_const) */
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

#if (AUDIO_EFFECTS_REVERB_AT_RAM == 0)
        *(.reverb_cal_code)
        *(.reverb_cal_const)
		*(.platereverb_code)
		*(.platereverb_const)
	    *(.audio_reverb_code)
        *(.audio_reverb_const)
        *(.reverb0_code)
		*(.reverb0_const)
#endif/*AUDIO_EFFECTS_REVERB_AT_RAM*/

#if (AUDIO_EFFECTS_ECHO_AT_RAM == 0)
	    *(.audio_echo_code)
        *(.audio_echo_const)
		*(.echo_cal_code)
		*(.echo_cal_const)
#endif



#if (AUDIO_EFFECTS_EQ_AT_RAM == 0)
		*(.audio_eq_code)
		*(.audio_eq_const)
#endif/*AUDIO_EFFECTS_EQ_AT_RAM*/
#if (AUDIO_EFFECTS_DRC_AT_RAM == 0)
	    *(.audio_drc_code)
		*(.audio_drc_const)
        *(.drc_const)
		*(.drc_code)
		*(.sw_drc_code)
		*(.sw_drc_const)
#endif/*AUDIO_EFFECTS_DRC_AT_RAM*/

#if (AUDIO_EFFECTS_AFC_AT_RAM == 0)
        *(.howlings_phf_code)
		*(.howlings_phf_const)
        /*AFC_Lib_陷波*/
        *(.notchhowling_code)
		*(.notchhowling_const)
		*(.notchhowling_sparse_code)
		*(.notchhowling_sparse_const)
#endif/*AUDIO_EFFECTS_AFC_AT_RAM*/
		*(.howlings_phf_sparse_code)

#if (AUDIO_EFFECTS_NOISEGATE_AT_RAM == 0)
		*(.audio_noisegate_code)
		*(.audio_noisegate_const)
		/*Lib_NoiseGate*/
		*(.noisegate_code)
		*(.noisegate_const)
		*(.noisegate_sparse_code)
		*(.noisegate_sparse_const)
#endif/*AUDIO_EFFECTS_NOISEGATE_AT_RAM*/



#if (AUDIO_EFFECTS_GAIN_AT_RAM == 0)
		*(.audio_gain_code)
		*(.audio_gain_const)
#endif

#if (AUDIO_EFFECTS_DYNAMIC_EQ_AT_RAM == 0)
		*(.dynamic_eq_code)
		*(.dynamic_eq_const)
	    *(.dynamic_eq_detect_code)
		*(.dynamic_eq_detect_const)
		*(.dynamic_eq_sparse_code)
		*(.dynamic_eq_sparse_const)
	    *(.dynamic_eq_detect_sparse_code)
		*(.dynamic_eq_detect_sparse_const)

#endif/*AUDIO_EFFECTS_DYNAMIC_EQ_AT_RAM*/

#if (AUDIO_EFFECTS_MIC_EFFECT_AT_RAM == 0)
		*(.audio_mic_effect_const)
		*(.audio_mic_effect_code)
#endif/*AUDIO_EFFECTS_MIC_EFFECT_AT_RAM*/
#if (AUDIO_EFFECTS_MIC_STREAM_AT_RAM == 0)
		*(.audio_mic_stream_const)
		*(.audio_mic_stream_code)
#endif/*AUDIO_EFFECTS_MIC_STREAM_AT_RAM*/
#if (AUDIO_EFFECTS_DEBUG_AT_RAM == 0)
		*(.audio_effect_adj_const)
		*(.audio_effect_adj_code)
#endif/*AUDIO_EFFECTS_DEBUG_AT_RAM*/

#if (AUDIO_EFFECTS_VBASS_AT_RAM == 0)
		*(.audio_vbass_code)
		*(.audio_vbass_const)
		*(.vbss_code)
		*(.vbss_const)
        *(.vbss_sparse_code)
		*(.vbss_sparse_const)
#endif/*AUDIO_EFFECTS_VBASS_AT_RAM*/

        *(.pitchshifter_code)
	    *(.pitchshifter_const)
		*(.audio_pitch_code)
		*(.audio_pitch_const)
		*(.audio_pitchspeed_code)
		*(.audio_pitchspeed_const)
        *(.pitchshifter_sparse_code)



#if (AUDIO_EFFECTS_SURROUND_AT_RAM == 0)
		*(.audio_surround_code)
		*(.audio_surround_const)
		*(.sur_cal_common_code)
        *(.sur_TYPE0_cal_code)
	    *(.sur_TYPE1_cal_code)
		*(.sur_TYPE0_cal_const)
	    *(.sur_TYPE1_cal_const)
#endif
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
#if (TCFG_ENC_LC3_ENABLE || TCFG_DEC_LC3_ENABLE)
		*( .lc3_codec_c_sparse_code)
		*(.lc3_codec_d_code)
		/* *(.lc3_codec_c_const) */
		/* *(.lc3_codec_c_code) */
		*(.lc3_codec_ari_c_code)
		*(.lc3_codec_e_code)
		*(.lc3_codec_c_const)
		*(.lc3_codec_c_code)
#endif
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
