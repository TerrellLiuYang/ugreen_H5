
# make 编译并下载
# make VERBOSE=1 显示编译详细过程
# make clean 清除编译临时文件
#
# 注意： Linux 下编译方式：
#     1. 从 http://pkgman.jieliapp.com/doc/all 处找到下载链接
#     2. 下载后，解压到 /opt/jieli 目录下，保证
#       /opt/jieli/common/bin/clang 存在（注意目录层次）
#     3. 确认 ulimit -n 的结果足够大（建议大于8096），否则链接可能会因为打开文件太多而失败
#       可以通过 ulimit -n 8096 来设置一个较大的值
#

# 工具路径设置
ifeq ($(OS), Windows_NT)
# Windows 下工具链位置
TOOL_DIR := C:/JL/pi32/bin
CC    := clang.exe
CXX   := clang.exe
LD    := pi32v2-lto-wrapper.exe
AR    := llvm-ar.exe
MKDIR := mkdir_win -p
RM    := rm -rf

SYS_LIB_DIR := C:/JL/pi32/pi32v2-lib/r3
SYS_INC_DIR := C:/JL/pi32/pi32v2-include
EXT_CFLAGS  := # Windows 下不需要 -D__SHELL__
export PATH:=$(TOOL_DIR);$(PATH)

## 后处理脚本
FIXBAT          := tools\utils\fixbat.exe # 用于处理 utf8->gbk 编码问题
POST_SCRIPT     := cpu/br36/tools/download.bat
RUN_POST_SCRIPT := cpu\br36\tools\download.bat
else
# Linux 下工具链位置
TOOL_DIR := /opt/jieli/pi32v2/bin
CC    := clang
CXX   := clang
LD    := lto-wrapper
AR    := lto-ar
MKDIR := mkdir -p
RM    := rm -rf
export OBJDUMP := $(TOOL_DIR)/objdump
export OBJCOPY := $(TOOL_DIR)/objcopy
export OBJSIZEDUMP := $(TOOL_DIR)/objsizedump

SYS_LIB_DIR := $(TOOL_DIR)/../lib/r3
SYS_INC_DIR := $(TOOL_DIR)/../include
EXT_CFLAGS  := -D__SHELL__ # Linux 下需要这个保证正确处理 download.c
export PATH:=$(TOOL_DIR):$(PATH)

## 后处理脚本
FIXBAT          := touch # Linux下不需要处理 bat 编码问题
POST_SCRIPT     := cpu/br36/tools/download.sh
RUN_POST_SCRIPT := bash $(POST_SCRIPT)
endif

CC  := $(TOOL_DIR)/$(CC)
CXX := $(TOOL_DIR)/$(CXX)
LD  := $(TOOL_DIR)/$(LD)
AR  := $(TOOL_DIR)/$(AR)
# 输出文件设置
OUT_ELF   := cpu/br36/tools/sdk.elf
OBJ_FILE  := $(OUT_ELF).objs.txt
# 编译路径设置
BUILD_DIR := objs

# 编译参数设置
CFLAGS := \
	-target pi32v2 \
	-mcpu=r3 \
	-integrated-as \
	-flto \
	-Wuninitialized \
	-Wno-invalid-noreturn \
	-fno-common \
	-integrated-as \
	-Oz \
	-g \
	-flto \
	-fallow-pointer-null \
	-fprefer-gnu-section \
	-Wno-shift-negative-value \
	-Werror \
	-fms-extensions \
	-w \


# C++额外的编译参数
CXXFLAGS :=


# 宏定义
DEFINES := \
	-DSUPPORT_MS_EXTENSIONS \
	-DCONFIG_USB_SUPPORT_MRX_TX \
	-DCONFIG_RELEASE_ENABLE \
	-DCONFIG_CPU_BR36 \
	-DCONFIG_NEW_BREDR_ENABLE \
	-DCONFIG_NEW_MODEM_ENABLE \
	-DCONFIG_NEW_TWS_FORWARD_ENABLE \
	-DCONFIG_FREE_RTOS_ENABLE \
	-DCONFIG_EQ_SUPPORT_ASYNC \
	-DCONFIG_SIN_TONE_V2 \
	-DCONFIG_DNS_ENABLE \
	-DCONFIG_DMS_MALLOC \
	-DCONFIG_MMU_ENABLE \
	-DCONFIG_SBC_CODEC_HW \
	-DCONFIG_MSBC_CODEC_HW \
	-DCONFIG_AEC_M=128 \
	-DCONFIG_AUDIO_ONCHIP \
	-DCONFIG_MEDIA_NEW_ENABLE \
	-DEQ_CORE_V1 \
	-DCONFIG_SUPPORT_WIFI_DETECT \
	-DCONFIG_SUPPORT_EX_TWS_ADJUST \
	-D__GCC_PI32V2__ \
	-DCONFIG_NEW_ECC_ENABLE \
	-DCONFIG_EFFECT_CORE_V2_ENABLE \
	-DCONFIG_OS_AFFINITY_ENABLE=1 \
	-DTCFG_APP_BT_EN=1 \
	-DEVENT_HANDLER_NUM_CONFIG=2 \
	-DEVENT_TOUCH_ENABLE_CONFIG=0 \
	-DEVENT_POOL_SIZE_CONFIG=256 \
	-DCONFIG_EVENT_KEY_MAP_ENABLE=0 \
	-DTIMER_POOL_NUM_CONFIG=15 \
	-DAPP_ASYNC_POOL_NUM_CONFIG=0 \
	-DSDFILE_STORAGE=0 \
	-DVFS_FILE_POOL_NUM_CONFIG=1 \
	-DFS_VERSION=0x020001 \
	-DFATFS_VERSION=0x020101 \
	-DSDFILE_VERSION=0x020000 \
	-DVFS_ENABLE=1 \
	-DVM_MAX_SIZE_CONFIG=16*1024 \
	-DVM_ITEM_MAX_NUM=256 \
	-DCONFIG_EARPHONE_CASE_ENABLE \
	-DCONFIG_NEW_CFG_TOOL_ENABLE \
	-DCONFIG_LITE_AEC_ENABLE=0 \
	-DAUDIO_REC_LITE \
	-DAUDIO_DEC_LITE \
	-DAUDIO_REC_POOL_NUM=1 \
	-DAUDIO_DEC_POOL_NUM=3 \
	-DCONFIG_BTCTRLER_TASK_DEL_ENABLE \
	-DCONFIG_LINK_DISTURB_SCAN_ENABLE=0 \
	-DCONFIG_UPDATA_ENABLE \
	-DCONFIG_OTA_UPDATA_ENABLE \


DEFINES += $(EXT_CFLAGS) # 额外的一些定义

# 头文件搜索路径
INCLUDES := \
	-Iinterface \
	-Iinterface/driver \
	-Iinterface/driver/device \
	-Iinterface/driver/cpu/br36 \
	-Iinterface/media \
	-Iinterface/system \
	-Iinterface/system/generic \
	-Iinterface/system/device \
	-Iinterface/utils/ui \
	-Iinterface/btctrler \
	-Iinterface/btctrler/port/br36 \
	-Iinterface/update \
	-Iinterface/agreement \
	-Iinterface/btstack/third_party/common \
	-Iinterface/utils \
	-Iinterface/utils/device \
	-Iinterface/utils/fs \
	-Iinterface/utils/generic \
	-Iinterface/utils/math/cpu/br36 \
	-Iinterface/btstack/third_party/rcsp \
	-Iinterface/media/media_new \
	-Iinterface/media/media_new/media \
	-Iinterface/media/media_new/media/cpu/br36 \
	-Iinterface/media/media_new/media/cpu/br36/asm \
	-Iapps/earphone \
	-Iapps/earphone/include \
	-Iapps/earphone/include/wireless_mic \
	-Iapps/earphone/board/br36 \
	-Iapps/earphone/ui \
	-Iapps/common \
	-Iapps/common/power_manage \
	-Iapps/common/device \
	-Iapps/common/device/sensor \
	-Iapps/common/device/sensor/ntc \
	-Iapps/common/device/key \
	-Iapps/common/include \
	-Iapps/common/config/include \
	-Iapps/common/dev_manager \
	-Iapps/common/third_party_profile \
	-Iapps/common/third_party_profile/common \
	-Iapps/common/third_party_profile/interface \
	-Iapps/common/third_party_profile/jieli \
	-Iapps/common/third_party_profile/jieli/trans_data_demo \
	-Iapps/common/third_party_profile/jieli/online_db \
	-Icpu \
	-Icpu/br36 \
	-Iaudio \
	-Iaudio/cpu/br36 \
	-Iaudio/cpu/br36/audio_dec \
	-Iaudio/common \
	-Iaudio/effect \
	-Iapps/common/device/sensor/imu_sensor \
	-Iapps/common/third_party_profile/jieli/rcsp \
	-Iapps/common/third_party_profile/jieli/rcsp/adv \
	-Iapps/common/third_party_profile/jieli/rcsp/rcsp_functions \
	-Iapps/common/third_party_profile/jieli/rcsp/server/include \
	-Iapps/common/third_party_profile/jieli/rcsp/server/functions \
	-Iapps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings \
	-Iapps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt \
	-Iapps/common/third_party_profile/jieli/rcsp/server/functions/device_info \
	-Iapps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd \
	-Iapps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_update \
	-Iapps/common/third_party_profile/jieli/rcsp/server/functions/external_flash \
	-Iapps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer \
	-Iapps/common/third_party_profile/jieli/rcsp/server/functions/sensors \
	-Iapps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_opt \
	-Iapps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_info_opt \
	-Iapps/common/third_party_profile/jieli/rcsp/server/functions/browser \
	-Iapps/common/third_party_profile/jieli/rcsp/client/rcsp_m_update \
	-Iapps/common/cJSON \
	-Iinterface/media/cpu/br36 \
	-Iapps/common/third_party_profile/tuya_protocol \
	-Iapps/common/third_party_profile/tuya_protocol/app/demo \
	-Iapps/common/third_party_profile/tuya_protocol/app/product_test \
	-Iapps/common/third_party_profile/tuya_protocol/app/uart_common \
	-Iapps/common/third_party_profile/tuya_protocol/extern_components/mbedtls \
	-Iapps/common/third_party_profile/tuya_protocol/port \
	-Iapps/common/third_party_profile/tuya_protocol/sdk/include \
	-Iapps/common/third_party_profile/tuya_protocol/sdk/lib \
	-Iapps/earphone/third_part/tuya \
	-Iapps/common/device/usb \
	-Iapps/common/device/usb/device \
	-Iapps/common/device/usb/host \
	-Iaudio/interface/include \
	-Iaudio/framework/include \
	-Iaudio/framework/plugs/source \
	-Iaudio/CVP \
	-Iaudio/test_tools \
	-Iaudio/common/icsd/adt \
	-Iaudio/common/icsd/anc \
	-Iaudio/anc \
	-Iaudio/smart_voice \
	-Iinterface/media/aispeech/enc \
	-Iinterface/media/framework/include \
	-Iinterface/media/cvp \
	-Iapps/common/third_party_profile/Tencent_LL/include \
	-Iapps/common/third_party_profile/Tencent_LL/Tencent_LL_demo \
	-Iapps/common/third_party_profile/Tencent_LL/tencent_ll_demo/jieli_iot_functions \
	-I$(SYS_INC_DIR) \


# 需要编译的 .c 文件
c_SRC_FILES := \
	apps/common/app_mode_manager/app_mode_manager.c \
	apps/common/cJSON/cJSON.c \
	apps/common/clock_manager/clock_manager.c \
	apps/common/config/app_config.c \
	apps/common/config/bt_name_parse.c \
	apps/common/config/bt_profile_config.c \
	apps/common/config/cfg_tool_statistics_functions/cfg_tool_statistics.c \
	apps/common/config/ci_transport_uart.c \
	apps/common/config/new_cfg_tool.c \
	apps/common/debug/debug.c \
	apps/common/debug/debug_lite.c \
	apps/common/debug/debug_uart_config.c \
	apps/common/dev_manager/dev_manager.c \
	apps/common/device/in_ear_detect/in_ear_detect.c \
	apps/common/device/in_ear_detect/in_ear_manage.c \
	apps/common/device/key/adkey.c \
	apps/common/device/key/adkey_rtcvdd.c \
	apps/common/device/key/ctmu_touch_key.c \
	apps/common/device/key/iokey.c \
	apps/common/device/key/irkey.c \
	apps/common/device/key/key_driver.c \
	apps/common/device/key/touch_key.c \
	apps/common/device/key/uart_key.c \
	apps/common/device/sensor/imu_sensor/icm_42670p/icm_42670p.c \
	apps/common/device/sensor/imu_sensor/icm_42670p/inv_imu_apex.c \
	apps/common/device/sensor/imu_sensor/icm_42670p/inv_imu_driver.c \
	apps/common/device/sensor/imu_sensor/icm_42670p/inv_imu_transport.c \
	apps/common/device/sensor/imu_sensor/imuSensor_manage.c \
	apps/common/device/sensor/imu_sensor/lsm6dsl/lsm6dsl.c \
	apps/common/device/sensor/imu_sensor/mpu6887/mpu6887p.c \
	apps/common/device/sensor/imu_sensor/mpu9250/mpu9250.c \
	apps/common/device/sensor/imu_sensor/qmi8658/qmi8658c.c \
	apps/common/device/sensor/imu_sensor/sh3001/sh3001.c \
	apps/common/device/sensor/ir_sensor/ir_manage.c \
	apps/common/device/sensor/ir_sensor/jsa1221.c \
	apps/common/device/sensor/ntc/ntc_det.c \
	apps/common/device/usb/demo/host/audio_demo.c \
	apps/common/device/usb/demo/host/udisk_block_test.c \
	apps/common/device/usb/device/cdc.c \
	apps/common/device/usb/device/custom_hid.c \
	apps/common/device/usb/device/descriptor.c \
	apps/common/device/usb/device/hid.c \
	apps/common/device/usb/device/msd.c \
	apps/common/device/usb/device/msd_upgrade.c \
	apps/common/device/usb/device/rcsp_hid_inter.c \
	apps/common/device/usb/device/task_pc.c \
	apps/common/device/usb/device/uac1.c \
	apps/common/device/usb/device/uac2.c \
	apps/common/device/usb/device/uac_stream.c \
	apps/common/device/usb/device/usb_device.c \
	apps/common/device/usb/device/user_setup.c \
	apps/common/device/usb/host/adb.c \
	apps/common/device/usb/host/aoa.c \
	apps/common/device/usb/host/audio.c \
	apps/common/device/usb/host/hid.c \
	apps/common/device/usb/host/usb_bulk_transfer.c \
	apps/common/device/usb/host/usb_ctrl_transfer.c \
	apps/common/device/usb/host/usb_host.c \
	apps/common/device/usb/host/usb_storage.c \
	apps/common/device/usb/usb_config.c \
	apps/common/device/usb/usb_epbuf_manager.c \
	apps/common/device/usb/usb_host_config.c \
	apps/common/device/usb/usb_task.c \
	apps/common/fat_nor/cfg_private.c \
	apps/common/scene_manager/scene_manager.c \
	apps/common/temp_trim/dtemp_pll_trim.c \
	apps/common/third_party_profile/Tencent_LL/tencent_ll_demo/jieli_iot_functions/ble_iot_anc_manager.c \
	apps/common/third_party_profile/Tencent_LL/tencent_ll_demo/jieli_iot_functions/ble_iot_msg_manager.c \
	apps/common/third_party_profile/Tencent_LL/tencent_ll_demo/jieli_iot_functions/ble_iot_power_manager.c \
	apps/common/third_party_profile/Tencent_LL/tencent_ll_demo/jieli_iot_functions/ble_iot_tws_manager.c \
	apps/common/third_party_profile/Tencent_LL/tencent_ll_demo/jieli_iot_functions/ble_iot_utils.c \
	apps/common/third_party_profile/Tencent_LL/tencent_ll_demo/ll_demo.c \
	apps/common/third_party_profile/Tencent_LL/tencent_ll_demo/ll_sync_event_handler.c \
	apps/common/third_party_profile/Tencent_LL/tencent_ll_demo/ll_task.c \
	apps/common/third_party_profile/Tencent_LL/tencent_protocol/ble_qiot_import.c \
	apps/common/third_party_profile/Tencent_LL/tencent_protocol/ble_qiot_llsync_data.c \
	apps/common/third_party_profile/Tencent_LL/tencent_protocol/ble_qiot_llsync_device.c \
	apps/common/third_party_profile/Tencent_LL/tencent_protocol/ble_qiot_llsync_event.c \
	apps/common/third_party_profile/Tencent_LL/tencent_protocol/ble_qiot_llsync_ota.c \
	apps/common/third_party_profile/Tencent_LL/tencent_protocol/ble_qiot_service.c \
	apps/common/third_party_profile/Tencent_LL/tencent_protocol/ble_qiot_template.c \
	apps/common/third_party_profile/Tencent_LL/tencent_protocol/ble_qiot_utils_base64.c \
	apps/common/third_party_profile/Tencent_LL/tencent_protocol/ble_qiot_utils_crc.c \
	apps/common/third_party_profile/Tencent_LL/tencent_protocol/ble_qiot_utils_hmac.c \
	apps/common/third_party_profile/Tencent_LL/tencent_protocol/ble_qiot_utils_log.c \
	apps/common/third_party_profile/Tencent_LL/tencent_protocol/ble_qiot_utils_md5.c \
	apps/common/third_party_profile/Tencent_LL/tencent_protocol/ble_qiot_utils_sha1.c \
	apps/common/third_party_profile/ancs_client_demo/le_ancs_client.c \
	apps/common/third_party_profile/common/3th_profile_api.c \
	apps/common/third_party_profile/common/custom_cfg.c \
	apps/common/third_party_profile/common/mic_rec.c \
	apps/common/third_party_profile/gfps_protocol.c \
	apps/common/third_party_profile/interface/app_protocol_api.c \
	apps/common/third_party_profile/interface/app_protocol_common.c \
	apps/common/third_party_profile/interface/app_protocol_dma.c \
	apps/common/third_party_profile/interface/app_protocol_gma.c \
	apps/common/third_party_profile/interface/app_protocol_mma.c \
	apps/common/third_party_profile/interface/app_protocol_ota.c \
	apps/common/third_party_profile/interface/app_protocol_tme.c \
	apps/common/third_party_profile/jieli/online_db/online_db_deal.c \
	apps/common/third_party_profile/jieli/online_db/spp_online_db.c \
	apps/common/third_party_profile/jieli/rcsp/adv/ble_rcsp_adv.c \
	apps/common/third_party_profile/jieli/rcsp/ble_rcsp_client.c \
	apps/common/third_party_profile/jieli/rcsp/ble_rcsp_multi_common.c \
	apps/common/third_party_profile/jieli/rcsp/ble_rcsp_server.c \
	apps/common/third_party_profile/jieli/rcsp/client/rcsp_c_cmd_recieve.c \
	apps/common/third_party_profile/jieli/rcsp/client/rcsp_c_cmd_recieve_no_response.c \
	apps/common/third_party_profile/jieli/rcsp/client/rcsp_c_cmd_response.c \
	apps/common/third_party_profile/jieli/rcsp/client/rcsp_m_update/rcsp_update_master.c \
	apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp.c \
	apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp_bt_manage.c \
	apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp_config.c \
	apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp_event.c \
	apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp_manage.c \
	apps/common/third_party_profile/jieli/rcsp/rcsp_functions/rcsp_task.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/browser/rcsp_browser.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_bt_func.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_fm_func.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_linein_func.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_music_func.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/device_status_cmd/rcsp_rtc_func.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/rcsp_device_feature.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/device_info/rcsp_device_status.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/external_flash/rcsp_extra_flash_cmd.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/external_flash/rcsp_extra_flash_opt.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/dev_format.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/file_bluk_trans_prepare.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/file_delete.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/file_simple_transfer.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/file_trans_back.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/file_transfer/file_transfer.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/rcsp_adv_bluetooth.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/rcsp_setting_opt.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/rcsp_setting_sync.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_1t2_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_adaptive_noise_reduction.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_ai_no_pick.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_anc_voice.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_anc_voice_key.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_bt_name_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_hearing_aid_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_key_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_led_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_mic_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_scene_noise_reduction.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_time_stamp_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_voice_enhancement_mode.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_wind_noise_detection.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/adv_work_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_color_led_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_eq_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_high_low_vol_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_karaoke_eq_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_karaoke_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_misc_drc_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_misc_reverbration_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_misc_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_music_info_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_setting_opt/settings/rcsp_vol_setting.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_switch_device.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_update/rcsp_ch_loader_download.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_update/rcsp_update.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/rcsp_update/rcsp_update_tws.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/nfc_data_opt.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sensor_log_notify.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sensors_data_opt.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_func.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_info_opt/sport_info_bt_disconn.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_info_opt/sport_info_continuous_heart_rate.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_info_opt/sport_info_exercise_heart_rate.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_info_opt/sport_info_fall_detection.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_info_opt/sport_info_personal_info.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_info_opt/sport_info_pressure_detection.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_info_opt/sport_info_raise_wrist.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_info_opt/sport_info_sedentary.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_info_opt/sport_info_sensor_opt.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_info_opt/sport_info_sleep_detection.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_opt/sport_data_air_pressure.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_opt/sport_data_altitude.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_opt/sport_data_blood_oxygen.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_opt/sport_data_exercise_recovery_time.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_opt/sport_data_exercise_steps.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_opt/sport_data_heart_rate.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_opt/sport_data_max_oxygen_uptake.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_opt/sport_data_pressure_detection.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_opt/sport_data_sports_information.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_data_opt/sport_data_training_load.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_info_opt.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_info_sync.c \
	apps/common/third_party_profile/jieli/rcsp/server/functions/sensors/sport_info_vm.c \
	apps/common/third_party_profile/jieli/rcsp/server/rcsp_cmd_recieve.c \
	apps/common/third_party_profile/jieli/rcsp/server/rcsp_cmd_recieve_no_respone.c \
	apps/common/third_party_profile/jieli/rcsp/server/rcsp_cmd_respone.c \
	apps/common/third_party_profile/jieli/rcsp/server/rcsp_cmd_user.c \
	apps/common/third_party_profile/jieli/rcsp/server/rcsp_command.c \
	apps/common/third_party_profile/jieli/rcsp/server/rcsp_data_recieve.c \
	apps/common/third_party_profile/jieli/rcsp/server/rcsp_data_recieve_no_respone.c \
	apps/common/third_party_profile/jieli/rcsp/server/rcsp_data_respone.c \
	apps/common/third_party_profile/jieli/trans_data_demo/le_trans_data.c \
	apps/common/third_party_profile/jieli/trans_data_demo/spp_trans_data.c \
	apps/common/third_party_profile/multi_protocol_common.c \
	apps/common/third_party_profile/multi_protocol_event.c \
	apps/common/third_party_profile/multi_protocol_main.c \
	apps/common/third_party_profile/tuya_protocol/app/demo/tuya_app_func.c \
	apps/common/third_party_profile/tuya_protocol/app/demo/tuya_ble_app_demo.c \
	apps/common/third_party_profile/tuya_protocol/app/demo/tuya_ota.c \
	apps/common/third_party_profile/tuya_protocol/app/product_test/tuya_ble_app_production_test.c \
	apps/common/third_party_profile/tuya_protocol/app/uart_common/tuya_ble_app_uart_common_handler.c \
	apps/common/third_party_profile/tuya_protocol/extern_components/mbedtls/aes.c \
	apps/common/third_party_profile/tuya_protocol/extern_components/mbedtls/ccm.c \
	apps/common/third_party_profile/tuya_protocol/extern_components/mbedtls/hmac.c \
	apps/common/third_party_profile/tuya_protocol/extern_components/mbedtls/md5.c \
	apps/common/third_party_profile/tuya_protocol/extern_components/mbedtls/sha1.c \
	apps/common/third_party_profile/tuya_protocol/extern_components/mbedtls/sha256.c \
	apps/common/third_party_profile/tuya_protocol/port/tuya_ble_port.c \
	apps/common/third_party_profile/tuya_protocol/port/tuya_ble_port_JL.c \
	apps/common/third_party_profile/tuya_protocol/port/tuya_ble_port_peripheral.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_api.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_bulk_data.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_data_handler.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_event.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_event_handler.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_event_handler_weak.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_feature_weather.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_gatt_send_queue.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_heap.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_main.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_mem.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_mutli_tsf_protocol.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_queue.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_storage.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_unix_time.c \
	apps/common/third_party_profile/tuya_protocol/sdk/src/tuya_ble_utils.c \
	apps/common/update/testbox_update.c \
	apps/common/update/update.c \
	apps/common/update/update_tws.c \
	apps/common/update/update_tws_new.c \
	apps/earphone/app_main.c \
	apps/earphone/audio/jlstream_event_handler.c \
	apps/earphone/audio/tone_table.c \
	apps/earphone/audio/vol_sync.c \
	apps/earphone/battery/battery_level.c \
	apps/earphone/battery/charge.c \
	apps/earphone/battery/charge_clibration.c \
	apps/earphone/battery/charge_device_handle.c \
	apps/earphone/battery/charge_store.c \
	apps/earphone/ble/ble_adv.c \
	apps/earphone/ble/bt_ble.c \
	apps/earphone/board/adkey_config.c \
	apps/earphone/board/br36/board_config.c \
	apps/earphone/board/br36/charge_config.c \
	apps/earphone/board/br36/chargestore_config.c \
	apps/earphone/board/br36/gpio_config.c \
	apps/earphone/board/br36/led_config.c \
	apps/earphone/board/br36/lp_touch_key_config.c \
	apps/earphone/board/br36/power_config.c \
	apps/earphone/board/br36/sdk_config.c \
	apps/earphone/board/iokey_config.c \
	apps/earphone/demo/pbg_demo.c \
	apps/earphone/demo/trans_data_demo.c \
	apps/earphone/device_config.c \
	apps/earphone/log_config/app_config.c \
	apps/earphone/log_config/lib_btctrler_config.c \
	apps/earphone/log_config/lib_btstack_config.c \
	apps/earphone/log_config/lib_driver_config.c \
	apps/earphone/log_config/lib_media_config.c \
	apps/earphone/log_config/lib_system_config.c \
	apps/earphone/log_config/lib_update_config.c \
	apps/earphone/message/adapter/app_msg.c \
	apps/earphone/message/adapter/audio_player.c \
	apps/earphone/message/adapter/battery.c \
	apps/earphone/message/adapter/btstack.c \
	apps/earphone/message/adapter/driver.c \
	apps/earphone/message/adapter/key.c \
	apps/earphone/message/adapter/tws.c \
	apps/earphone/mode/bt/bt_call_kws_handler.c \
	apps/earphone/mode/bt/bt_slience_detect.c \
	apps/earphone/mode/bt/bt_tws.c \
	apps/earphone/mode/bt/earphone.c \
	apps/earphone/mode/bt/key_msg_handler.c \
	apps/earphone/mode/bt/kws_voice_event_deal.c \
	apps/earphone/mode/bt/sniff.c \
	apps/earphone/mode/bt/tone.c \
	apps/earphone/mode/bt/tws_a2dp_play.c \
	apps/earphone/mode/bt/tws_dual_conn.c \
	apps/earphone/mode/bt/tws_phone_call.c \
	apps/earphone/mode/bt/tws_poweroff.c \
	apps/earphone/mode/default_msg_handler.c \
	apps/earphone/mode/idle/idle.c \
	apps/earphone/mode/key_tone.c \
	apps/earphone/scene/adio_ability.c \
	apps/earphone/scene/app_ability.c \
	apps/earphone/scene/audio_ability.c \
	apps/earphone/scene/battery_ability.c \
	apps/earphone/scene/bt_ability.c \
	apps/earphone/scene/counter_ability.c \
	apps/earphone/scene/gpio_ability.c \
	apps/earphone/scene/key_ability.c \
	apps/earphone/scene/led_ability.c \
	apps/earphone/scene/sys_timer_ability.c \
	apps/earphone/scene/tone_ability.c \
	apps/earphone/scene/tws_ability.c \
	apps/earphone/test/rssi.c \
	apps/earphone/third_part/app_protocol_deal.c \
	apps/earphone/third_part/tuya/tuya_app.c \
	apps/earphone/third_part/tuya/tuya_demo.c \
	apps/earphone/tools/app_ancbox.c \
	apps/earphone/tools/app_anctool.c \
	apps/earphone/tools/app_key_dut.c \
	apps/earphone/tools/app_testbox.c \
	apps/earphone/ui/led/led_ui_manager.c \
	apps/earphone/ui/led/led_ui_timer.c \
	apps/earphone/ui/led/led_ui_tws_sync.c \
	apps/earphone/ui/led/pwm_led_ui.c \
	apps/earphone/user_cfg.c \
	audio/CVP/audio_cvp.c \
	audio/CVP/audio_cvp_3mic.c \
	audio/CVP/audio_cvp_ais_3mic.c \
	audio/CVP/audio_cvp_demo.c \
	audio/CVP/audio_cvp_develop.c \
	audio/CVP/audio_cvp_dms.c \
	audio/CVP/audio_cvp_online.c \
	audio/CVP/audio_cvp_ref_task.c \
	audio/CVP/audio_cvp_sync.c \
	audio/anc/audio_anc_fade_ctr.c \
	audio/common/amplitude_statistic.c \
	audio/common/audio_dvol.c \
	audio/common/audio_export_demo.c \
	audio/common/audio_general.c \
	audio/common/audio_node_config.c \
	audio/common/audio_noise_gate.c \
	audio/common/audio_ns.c \
	audio/common/audio_plc.c \
	audio/common/audio_utils.c \
	audio/common/bt_audio_energy_detection.c \
	audio/common/decode/audio_key_tone.c \
	audio/common/decode/decode.c \
	audio/common/demo/audio_codec_demo.c \
	audio/common/demo/audio_demo.c \
	audio/common/frame_length_adaptive.c \
	audio/common/online_debug/aud_data_export.c \
	audio/common/online_debug/aud_mic_dut.c \
	audio/common/online_debug/audio_capture.c \
	audio/common/online_debug/audio_online_debug.c \
	audio/common/uartPcmSender.c \
	audio/common/wm8978/iic.c \
	audio/common/wm8978/wm8978.c \
	audio/cpu/br36/audio_accelerator/hw_fft.c \
	audio/cpu/br36/audio_anc.c \
	audio/cpu/br36/audio_config.c \
	audio/cpu/br36/audio_dac_digital_volume.c \
	audio/cpu/br36/audio_dai/audio_link.c \
	audio/cpu/br36/audio_dai/audio_pdm.c \
	audio/cpu/br36/audio_demo/audio_adc_demo.c \
	audio/cpu/br36/audio_demo/audio_dac_demo.c \
	audio/cpu/br36/audio_mic_capless.c \
	audio/cpu/br36/audio_setup.c \
	audio/cpu/br36/audio_volume_mixer.c \
	audio/effect/audio_dc_offset_remove.c \
	audio/effect/audio_voice_changer_api.c \
	audio/effect/bass_treble.c \
	audio/effect/demo/bass_treble_demo.c \
	audio/effect/demo/drc_demo.c \
	audio/effect/demo/dynamic_eq_demo.c \
	audio/effect/demo/eq_demo.c \
	audio/effect/demo/noisegate_demo.c \
	audio/effect/demo/surround_demo.c \
	audio/effect/demo/virtual_bass_demo.c \
	audio/effect/demo/voice_changer_demo.c \
	audio/effect/effects_adj.c \
	audio/effect/effects_default_param.c \
	audio/effect/eq_config.c \
	audio/effect/esco_ul_voice_changer.c \
	audio/effect/node_param_update.c \
	audio/effect/spk_eq.c \
	audio/framework/nodes/agc_node.c \
	audio/framework/nodes/ai_tx_node.c \
	audio/framework/nodes/channle_swap_node.c \
	audio/framework/nodes/cvp_3mic_node.c \
	audio/framework/nodes/cvp_develop_node.c \
	audio/framework/nodes/cvp_dms_node.c \
	audio/framework/nodes/cvp_sms_node.c \
	audio/framework/nodes/effect_dev0_node.c \
	audio/framework/nodes/effect_dev1_node.c \
	audio/framework/nodes/effect_dev2_node.c \
	audio/framework/nodes/effect_dev3_node.c \
	audio/framework/nodes/effect_dev4_node.c \
	audio/framework/nodes/esco_tx_node.c \
	audio/framework/nodes/ns_node.c \
	audio/framework/nodes/plc_node.c \
	audio/framework/nodes/sink_dev0_node.c \
	audio/framework/nodes/sink_dev1_node.c \
	audio/framework/nodes/surround_demo_node.c \
	audio/framework/nodes/uart_node.c \
	audio/framework/nodes/volume_node.c \
	audio/framework/plugs/source/a2dp_file.c \
	audio/framework/plugs/source/a2dp_streamctrl.c \
	audio/framework/plugs/source/adc_file.c \
	audio/framework/plugs/source/esco_file.c \
	audio/framework/plugs/source/pdm_mic_file.c \
	audio/framework/plugs/source/source_dev0_file.c \
	audio/framework/plugs/source/source_dev1_file.c \
	audio/interface/player/a2dp_player.c \
	audio/interface/player/dev_flow_player.c \
	audio/interface/player/esco_player.c \
	audio/interface/player/key_tone_player.c \
	audio/interface/player/reference_time.c \
	audio/interface/player/ring_player.c \
	audio/interface/player/tone_player.c \
	audio/interface/player/tws_tone_player.c \
	audio/interface/recoder/ai_voice_recoder.c \
	audio/interface/recoder/dev_flow_recoder.c \
	audio/interface/recoder/esco_recoder.c \
	audio/jl_kws/jl_kws_algo.c \
	audio/jl_kws/jl_kws_audio.c \
	audio/jl_kws/jl_kws_event.c \
	audio/jl_kws/jl_kws_main.c \
	audio/smart_voice/jl_kws_platform.c \
	audio/smart_voice/kws_event.c \
	audio/smart_voice/nn_vad.c \
	audio/smart_voice/smart_voice_config.c \
	audio/smart_voice/smart_voice_core.c \
	audio/smart_voice/user_asr.c \
	audio/smart_voice/voice_mic_data.c \
	audio/test_tools/audio_dut_control.c \
	audio/test_tools/audio_dut_control_old.c \
	audio/test_tools/audio_enc_mpt_cvp_ctr.c \
	audio/test_tools/audio_enc_mpt_self.c \
	audio/test_tools/cvp_tool.c \
	audio/test_tools/mic_dut_process.c \
	cpu/br36/charge/charge.c \
	cpu/br36/charge/charge_calibration.c \
	cpu/br36/charge/chargestore.c \
	cpu/br36/overlay_code.c \
	cpu/br36/periph/gpadc.c \
	cpu/br36/periph/irflt.c \
	cpu/br36/periph/ledc.c \
	cpu/br36/periph/pwm_led_hw.c \
	cpu/br36/periph/rdec.c \
	cpu/br36/periph/spi.c \
	cpu/br36/periph/touch/lp_touch_key.c \
	cpu/br36/periph/touch/lp_touch_key_alog.c \
	cpu/br36/periph/touch/lp_touch_key_tool.c \
	cpu/br36/periph/touch/plcnt.c \
	cpu/br36/setup.c \
	cpu/components/gpadc_utils.c \
	cpu/components/iic_api.c \
	cpu/components/iic_soft.c \
	cpu/config/gpio_file_parse.c \
	cpu/periph_demo/iic_master_demo.c \
	cpu/periph_demo/iic_slave_demo.c \
	cpu/periph_demo/ledc_test.c \
	cpu/periph_demo/sd_test.c \
	cpu/periph_demo/spi_test.c \
	cpu/periph_demo/timer_demo.c \
	cpu/periph_demo/uart_test.c \


# 需要编译的 .S 文件
S_SRC_FILES := \
	apps/earphone/sdk_version.z.S \


# 需要编译的 .s 文件
s_SRC_FILES :=


# 需要编译的 .cpp 文件
cpp_SRC_FILES :=


# 需要编译的 .cc 文件
cc_SRC_FILES :=


# 需要编译的 .cxx 文件
cxx_SRC_FILES :=


# 链接参数
LFLAGS := \
	--plugin-opt=-pi32v2-always-use-itblock=false \
	--plugin-opt=-enable-ipra=true \
	--plugin-opt=-pi32v2-merge-max-offset=4096 \
	--plugin-opt=-pi32v2-enable-simd=true \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-global-merge-on-const \
	--plugin-opt=-inline-threshold=5 \
	--plugin-opt=-inline-max-allocated-size=32 \
	--plugin-opt=-inline-normal-into-special-section=true \
	--plugin-opt=-dont-used-symbol-list=malloc,free,sprintf,printf,puts,putchar \
	--plugin-opt=save-temps \
	--plugin-opt=-pi32v2-enable-rep-memop \
	--plugin-opt=-warn-stack-size=1024 \
	--sort-common \
	--dont-complain-call-overflow \
	--plugin-opt=-used-symbol-file=apps/earphone/sdk_used_list.used \
	--gc-sections \
	--plugin-opt=-pi32v2-csync-before-rti=true \
	--start-group \
	cpu/br36/liba/cpu.a \
	cpu/br36/liba/system.a \
	cpu/br36/liba/btstack.a \
	cpu/br36/liba/btctrler.a \
	cpu/br36/liba/aec.a \
	cpu/br36/liba/media.a \
	cpu/br36/liba/libAptFilt_pi32v2_OnChip.a \
	cpu/br36/liba/libEchoSuppress_pi32v2_OnChip.a \
	cpu/br36/liba/libNoiseSuppress_pi32v2_OnChip.a \
	cpu/br36/liba/libSplittingFilter_pi32v2_OnChip.a \
	cpu/br36/liba/libDelayEstimate_pi32v2_OnChip.a \
	cpu/br36/liba/libDualMicSystem_pi32v2_OnChip.a \
	cpu/br36/liba/libDualMicSystem_flexible_pi32v2_OnChip.a \
	cpu/br36/liba/libOpcore_maskrom_pi32v2_OnChip.a \
	cpu/br36/liba/lib_resample_cal.a \
	cpu/br36/liba/lib_resample_fast_cal.a \
	cpu/br36/liba/sbc_eng_lib.a \
	cpu/br36/liba/lib_esco_repair.a \
	cpu/br36/liba/limiter_noiseGate.a \
	cpu/br36/liba/noisegate.a \
	cpu/br36/liba/agreement.a \
	cpu/br36/liba/bt_hash_enc.a \
	cpu/br36/liba/rcsp_stack.a \
	cpu/br36/liba/lib_wma_dec.a \
	cpu/br36/liba/lib_mp3_dec.a \
	cpu/br36/liba/lib_flac_dec.a \
	cpu/br36/liba/lib_wav_dec.a \
	cpu/br36/liba/lib_f2a_dec.a \
	cpu/br36/liba/lib_wtgv2_dec.a \
	cpu/br36/liba/lib_mty_dec.a \
	cpu/br36/liba/lib_ape_dec.a \
	cpu/br36/liba/lib_m4a_dec.a \
	cpu/br36/liba/lib_amr_dec.a \
	cpu/br36/liba/lib_dts_dec.a \
	cpu/br36/liba/dueros_stack.a \
	cpu/br36/liba/opus_enc_lib.a \
	cpu/br36/liba/speex_enc_lib.a \
	cpu/br36/liba/lc3_codec_lib.a \
	cpu/br36/liba/msbc_codec_lib.a \
	cpu/br36/liba/lfaudio_plc_lib.a \
	cpu/br36/liba/crossover_coff.a \
	cpu/br36/liba/drc.a \
	cpu/br36/liba/lib_voiceChanger.a \
	cpu/br36/liba/VirtualBass.a \
	cpu/br36/liba/tme_stack.a \
	cpu/br36/liba/gma_stack.a \
	cpu/br36/liba/google_fps.a \
	cpu/br36/liba/realme_stack.a \
	cpu/br36/liba/JL_Phone_Call.a \
	cpu/br36/liba/lib_dvad.a \
	cpu/br36/liba/libkwscommon.a \
	cpu/br36/liba/libjlsp_kws.a \
	cpu/br36/liba/libjlsp_kws_india_english.a \
	cpu/br36/liba/libjlsp.a \
	cpu/br36/liba/DynamicEQ.a \
	cpu/br36/liba/libFFT_pi32v2_OnChip.a \
	cpu/br36/liba/lib_wtg_dec.a \
	cpu/br36/liba/lib_bt_aac_dec.a \
	cpu/br36/liba/crypto_toolbox_Osize.a \
	apps/common/third_party_profile/tuya_protocol/sdk/lib/libtuya_lib.a \
	cpu/br36/liba/lib_dns.a \
	cpu/br36/liba/update.a \
	cpu/br36/liba/cbuf.a \
	cpu/br36/liba/lbuf.a \
	cpu/br36/liba/printf.a \
	cpu/br36/liba/device.a \
	cpu/br36/liba/fs.a \
	cpu/br36/liba/ascii.a \
	cpu/br36/liba/cfg_tool.a \
	cpu/br36/liba/vm.a \
	--end-group \
	-Tcpu/br36/sdk.ld \
	-M=cpu/br36/tools/sdk.map \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-mattr=+fprev1 \


LIBPATHS := \
	-L$(SYS_LIB_DIR) \


LIBS := \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libc.a \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libcompiler-rt.a \



c_OBJS    := $(c_SRC_FILES:%.c=%.c.o)
S_OBJS    := $(S_SRC_FILES:%.S=%.S.o)
s_OBJS    := $(s_SRC_FILES:%.s=%.s.o)
cpp_OBJS  := $(cpp_SRC_FILES:%.cpp=%.cpp.o)
cxx_OBJS  := $(cxx_SRC_FILES:%.cxx=%.cxx.o)
cc_OBJS   := $(cc_SRC_FILES:%.cc=%.cc.o)

OBJS      := $(c_OBJS) $(S_OBJS) $(s_OBJS) $(cpp_OBJS) $(cxx_OBJS) $(cc_OBJS)
DEP_FILES := $(OBJS:%.o=%.d)


OBJS      := $(addprefix $(BUILD_DIR)/, $(OBJS))
DEP_FILES := $(addprefix $(BUILD_DIR)/, $(DEP_FILES))


VERBOSE ?= 0
ifeq ($(VERBOSE), 1)
QUITE :=
else
QUITE := @
endif

# 一些旧的 make 不支持 file 函数，需要 make 的时候指定 LINK_AT=0 make
LINK_AT ?= 1

# 表示下面的不是一个文件的名字，无论是否存在 all, clean, pre_build 这样的文件
# 还是要执行命令
# see: https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean pre_build

# 不要使用 make 预设置的规则
# see: https://www.gnu.org/software/make/manual/html_node/Suffix-Rules.html
.SUFFIXES:

all: pre_build $(OUT_ELF)
	$(info +POST-BUILD)
	$(QUITE) $(RUN_POST_SCRIPT) sdk

pre_build:
	$(info +PRE-BUILD)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P apps/earphone/sdk_used_list.c -o apps/earphone/sdk_used_list.used
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P cpu/br36/sdk_ld.c -o cpu/br36/sdk.ld
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P cpu/br36/tools/download.c -o $(POST_SCRIPT)
	$(QUITE) $(FIXBAT) $(POST_SCRIPT)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P cpu/br36/tools/isd_config_rule.c -o cpu/br36/tools/isd_config.ini

clean:
	$(QUITE) $(RM) $(OUT_ELF)
	$(QUITE) $(RM) $(BUILD_DIR)



ifeq ($(LINK_AT), 1)
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(file >$(OBJ_FILE), $(OBJS))
	$(QUITE) $(LD) -o $(OUT_ELF) @$(OBJ_FILE) $(LFLAGS) $(LIBPATHS) $(LIBS)
else
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(QUITE) $(LD) -o $(OUT_ELF) $(OBJS) $(LFLAGS) $(LIBPATHS) $(LIBS)
endif


$(BUILD_DIR)/%.c.o : %.c
	$(info +CC $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.S.o : %.S
	$(info +AS $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.s.o : %.s
	$(info +AS $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.cpp.o : %.cpp
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.cxx.o : %.cxx
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.cc.o : %.cc
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

-include $(DEP_FILES)
