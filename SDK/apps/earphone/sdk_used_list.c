#include "app_config.h"


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

#if TCFG_BT_DONGLE_ENABLE
msbc_encoder_soft_plug
#else
msbc_encoder_hw_plug
#endif

#if TCFG_BT_SUPPORT_AAC
aac_dec_plug
#endif

sine_dec_plug
cvsd_encoder_plug
cvsd_decoder_plug

#ifdef TCFG_3BAND_MERGE_ENABLE
three_band_merge_node_adapter
#endif

#ifdef TCFG_2BAND_MERGE_ENABLE
two_band_merge_node_adapter
#endif


#ifdef TCFG_CROSSOVER_NODE_ENABLE
crossover_node_adapter
crossover_2band_node_adapter
#endif


#ifdef TCFG_WDRC_NODE_ENABLE
wdrc_node_adapter
#endif



#ifdef TCFG_ENERGY_DETECT_NODE_ENABLE
energy_detect_node_adapter
#endif


#ifdef TCFG_CONVERT_NODE_ENABLE
convert_node_adapter
convert_data_round_node_adapter
#endif

#ifdef TCFG_BASS_TREBLE_NODE_ENABLE
bass_treble_eq_node_adapter
#endif

#ifdef TCFG_VOICE_CHANGER_NODE_ENABLE
voice_changer_node_adapter
#endif


#ifdef TCFG_SPEAKER_EQ_NODE_ENABLE
spk_eq_node_adapter
#endif

#ifdef TCFG_EQ_ENABLE
eq_node_adapter
#endif

#ifdef TCFG_GAIN_NODE_ENABLE
gain_node_adapter
#endif

#ifdef TCFG_STEROMIX_NODE_ENABLE
steromix_node_adapter
#endif


#ifdef TCFG_VBASS_NODE_ENABLE
vbass_node_adapter
#endif

#ifdef  TCFG_NOISEGATE_NODE_ENABLE
noisegate_node_adapter
#endif

#ifdef TCFG_NS_NODE_ENABLE
ns_node_adapter
#endif

#if TCFG_DEC_WTG_ENABLE
g729_dec_plug
#endif

#if TCFG_DEC_F2A_ENABLE
f2a_dec_plug
#endif

#if TCFG_DEC_MTY_ENABLE
mty_dec_plug
#endif

#if TCFG_DEC_WAV_ENABLE
wav_dec_plug
#endif

#if TCFG_DEC_OPUS_ENABLE
opus_dec_plug
#endif
#if TCFG_ENC_OPUS_ENABLE
opus_encoder_plug
#endif

#if TCFG_DEC_WTS_ENABLE
wts_dec_plug
#endif

#if TCFG_DEC_MP3_ENABLE
mp3_dec_plug
#endif

#if CONFIG_FATFS_ENABLE
fat_vfs_ops
#endif


sdfile_resfile_ops

#if TCFG_NORFLASH_DEV_ENABLE && VFS_ENABLE
nor_fs_vfs_ops
nor_sdfile_vfs_ops
#endif

#if FLASH_INSIDE_REC_ENABLE
inside_nor_fs_vfs_ops
#endif


#ifdef TCFG_EFFECT_DEV0_NODE_ENABLE
effect_dev0_node_adapter
#endif

#ifdef TCFG_EFFECT_DEV1_NODE_ENABLE
effect_dev1_node_adapter
#endif

#ifdef TCFG_EFFECT_DEV2_NODE_ENABLE
effect_dev2_node_adapter
#endif
#ifdef TCFG_EFFECT_DEV3_NODE_ENABLE
effect_dev3_node_adapter
#endif
#ifdef TCFG_EFFECT_DEV4_NODE_ENABLE
effect_dev4_node_adapter
#endif

#ifdef TCFG_DYNAMIC_EQ_NODE_ENABLE
dynamic_eq_node_adapter
#endif

#ifdef TCFG_SOUND_SPLITTER_NODE_ENABLE
sound_splitter_node_adapter
#endif

#ifdef TCFG_SOURCE_DEV0_NODE_ENABLE
source_dev0_file_plug
#endif

#ifdef TCFG_SOURCE_DEV1_NODE_ENABLE
source_dev1_file_plug
#endif

#ifdef TCFG_SINK_DEV0_NODE_ENABLE
sink_dev0_adapter
#endif

#ifdef TCFG_SINK_DEV1_NODE_ENABLE
sink_dev1_adapter
#endif

#ifdef TCFG_AGC_NODE_ENABLE
agc_node_adapter
#endif
