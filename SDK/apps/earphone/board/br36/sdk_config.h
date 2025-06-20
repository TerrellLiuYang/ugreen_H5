#ifndef SDK_CONFIG_H
#define SDK_CONFIG_H


// 表单配置宏定义
// group enable 
#define TCFG_CHARGESTORE_ENABLE 1 // 智能仓
#define TCFG_CHARGE_ENABLE 1 // 充电配置
#define TCFG_SYS_LVD_EN 1 // 电池电量检测
#define TCFG_BATTERY_CURVE_ENABLE 0 // 电池曲线配置
#define TCFG_DEBUG_UART_ENABLE 1 // 调试串口
#define TCFG_CFG_TOOL_ENABLE 0 // FW编辑、在线调音
#define TCFG_IO_CFG_AT_POWER_ON 1 // 开机时IO配置
#define TCFG_IO_CFG_AT_POWER_OFF 1 // 关机时IO配置
#define TCFG_IOKEY_ENABLE 0 // IO按键配置
#define TCFG_ADKEY_ENABLE 0 // AD按键配置
#define TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE 0 // 内置触摸在线调试
#define TCFG_LP_TOUCH_KEY_ENABLE 1 // 内置触摸按键配置
#define TCFG_LP_EARTCH_KEY_ENABLE 0 // 内置触摸入耳检测配置
#define TCFG_PWMLED_ENABLE 0 // LED配置
#define TCFG_PWMLED_STA_ENABLE 0 // LED状态配置
#define TCFG_BT_NAME_SEL_BY_AD_ENABLE 0 // 蓝牙名(AD采样)
#define TCFG_USER_TWS_ENABLE 1 // TWS
#define TCFG_BT_SNIFF_ENABLE 1 // sniff
#define TCFG_USER_BLE_ENABLE 1 // BLE
#define TCFG_BT_AI_ENABLE 1 // AI配置
#define TCFG_UPDATE_ENABLE 1 // 升级选择
#define TCFG_AUDIO_DAC_ENABLE 1 // DAC配置
#define TCFG_AUDIO_ADC_ENABLE 1 // ADC配置
#define CONFIG_ANC_ENABLE 0 // ANC 配置

// ------------流程图宏定义------------
#define TCFG_AUDIO_BIT_WIDTH 0 // 位宽
#define TCFG_EFFECT_DEV0_NODE_ENABLE 
#define TCFG_EQ_ENABLE 
#define TCFG_PLC_NODE_ENABLE 
#define TCFG_AUDIO_CVP_DMS_ANS_MODE 
#define TCFG_ESCO_RX_NODE_ENABLE 
#define TCFG_ESCO_TX_NODE_ENABLE 
#define EQ_SECTION_MAX 0x14 // EQ_SECTION_MAX
#define TCFG_STREAM_BIN_ENC_ENABLE 0 // stream.bin加密使能
// ------------流程图宏定义------------

// ------------流程图EQ节点类型宏定义------------
#define EQ_CFG_TYPE_HIGH_PASS 1 // High Pass
#define EQ_CFG_TYPE_LOW_PASS 0 // Low Pass
#define EQ_CFG_TYPE_PEAKING 1 // Peaking
#define EQ_CFG_TYPE_HIGH_SHELF 0 // High Shelf
#define EQ_CFG_TYPE_LOW_SHELF 0 // Low Shelf
#define EQ_CFG_TYPE_HIGH_SHELF_Q 0 // High Shelf Q
#define EQ_CFG_TYPE_LOW_SHELF_Q 0 // Low Shelf Q
#define EQ_CFG_TYPE_HP 0 // Hp
#define EQ_CFG_TYPE_LP 0 // Lp
// ------------流程图EQ节点类型宏定义------------
// 电源配置.json
#define TCFG_CLOCK_OSC_HZ 24000000 // 晶振频率
#define TCFG_LOWPOWER_POWER_SEL PWR_DCDC15 // 电源模式
#define TCFG_LOWPOWER_OSC_TYPE OSC_TYPE_LRC // 低功耗时钟源
#define TCFG_LOWPOWER_VDDIOM_LEVEL VDDIOM_VOL_28V // 强VDDIO
#define TCFG_LOWPOWER_VDDIOW_LEVEL VDDIOW_VOL_26V // 弱VDDIO
#define TCFG_LOWPOWER_VDDIO_KEEP 1 // 关机保持VDDIO
#define TCFG_MAX_LIMIT_SYS_CLOCK 128000000 // 上限时钟设置
#define TCFG_LOWPOWER_LOWPOWER_SEL 1 // 低功耗模式
#define TCFG_AUTO_POWERON_ENABLE 0 // 上电自动开机
#define TCFG_CHARGE_TRICKLE_MA 0 // 涓流电流
#define TCFG_CHARGE_MA 5 // 恒流电流
#define TCFG_CHARGE_FULL_MA 2 // 截止电流
#define TCFG_CHARGE_FULL_V 25 // 截止电压
#define TCFG_CHARGE_POWERON_ENABLE 0 // 开机充电
#define TCFG_CHARGE_OFF_POWERON_EN 1 // 拔出开机
#define TCFG_LDOIN_PULLDOWN_EN 1 // 下拉电阻开关
#define TCFG_LDOIN_PULLDOWN_LEV 1 // 下拉电阻档位
#define TCFG_LDOIN_PULLDOWN_KEEP 0 // 下拉电阻保持开关
#define TCFG_LDOIN_ON_FILTER_TIME 50 // 入舱滤波时间(ms)
#define TCFG_LDOIN_OFF_FILTER_TIME 300 // 出舱滤波时间(ms)
#define TCFG_LDOIN_KEEP_FILTER_TIME 440 // 维持电压滤波时间(ms)
#define TCFG_POWER_OFF_VOLTAGE 3200 // 关机电压(mV)
#define TCFG_POWER_WARN_VOLTAGE 3670 // 低电电压(mV)
// 板级配置.json
#define TCFG_DEBUG_UART_TX_PIN IO_PORT_DP // 输出IO
#define TCFG_DEBUG_UART_BAUDRATE 2000000 // 波特率
#define TCFG_EXCEPTION_LOG_ENABLE 1 // 打印异常信息
#define TCFG_EXCEPTION_RESET_ENABLE 1 // 异常自动复位
#define CONFIG_SPI_DATA_WIDTH 2 // flash通信
#define CONFIG_FLASH_SIZE 2097152 // flash容量
#define TCFG_VM_SIZE 40 // VM大小(K)
#define TCFG_TWS_COMBINATIION_KEY_ENABLE 0 // TWS两边同时按消息使能
#define TCFG_SEND_HOLD_SEC_MSG_DURING_HOLD 1 // 按住过程中发送按住几秒消息
#define TCFG_MAX_HOLD_SEC ((KEY_ACTION_HOLD_5SEC << 8) | 5) // 最长按住消息
#define TCFG_CHARGESTORE_PORT IO_PORT_LDOIN // 通信IO
// LED配置.json
// 蓝牙配置.json
#define TCFG_BT_PAGE_TIMEOUT 12 // 单次回连时间(s)
#define TCFG_BT_POWERON_PAGE_TIME 12 // 开机回连超时(s)
#define TCFG_BT_TIMEOUT_PAGE_TIME 120 // 超距断开回连超时(s)
#define TCFG_DUAL_CONN_INQUIRY_SCAN_TIME 0 // 开机可被发现时间 (s)
#define TCFG_DUAL_CONN_PAGE_SCAN_TIME 0 // 等待第二台连接时间(s)
#define TCFG_AUTO_SHUT_DOWN_TIME 300 // 无连接关机时间(s)
#define TCFG_BT_DUAL_CONN_ENABLE 1 // 一拖二
#define TCFG_A2DP_PREEMPTED_ENABLE 0 // A2DP抢播
#define TCFG_BT_VOL_SYNC_ENABLE 1 // 音量同步
#define TCFG_BT_DISPLAY_BAT_ENABLE 1 // 电量显示
#define TCFG_BT_INBAND_RING 1 // 手机铃声
#define TCFG_BT_PHONE_NUMBER_ENABLE 0 // 来电报号
#define TCFG_BT_SUPPORT_AAC 1 // AAC
#define TCFG_BT_MSBC_EN 1 // MSBC
#define TCFG_BT_SBC_BITPOOL 38 // sbcBitPool
#define TCFG_BT_SUPPORT_HFP 1 // HFP
#define TCFG_BT_SUPPORT_AVCTP 1 // AVRCP
#define TCFG_BT_SUPPORT_A2DP 1 // A2DP
#define TCFG_BT_SUPPORT_HID 1 // HID
#define TCFG_BT_SUPPORT_SPP 1 // SPP
#define CONFIG_BT_MODE 1 // 模式选择
#define TCFG_NORMAL_SET_DUT_MODE 0 // NORMAL模式下使能DUT测试
#define CONFIG_COMMON_ADDR_MODE 2 // MAC地址
#define TCFG_BT_TWS_PAIR_MODE CONFIG_TWS_PAIR_BY_CLICK // 配对方式
#define TCFG_BT_TWS_CHANNEL_SELECT CONFIG_TWS_EXTERN_DOWN_AS_LEFT // 声道选择
#define TCFG_TWS_PAIR_TIMEOUT 10 // 开机配对超时(s)
#define TCFG_TWS_CONN_TIMEOUT 5 // 单次连接超时(s)
#define TCFG_TWS_POWERON_AUTO_PAIR_ENABLE 0 // 开机自动配对/连接
#define TCFG_TWS_AUTO_ROLE_SWITCH_ENABLE 1 // 自动主从切换
#define TCFG_TWS_POWER_BALANCE_ENABLE 1 // 主从电量平衡
#define CONFIG_TWS_AUTO_PAIR_WITHOUT_UNPAIR 0 // TWS连接超时自动配对新耳机
#define TCFG_SNIFF_CHECK_TIME 5 // 检测时间
#define CONFIG_LRC_WIN_SIZE 400 // LRC窗口初值
#define CONFIG_LRC_WIN_STEP 400 // LRC窗口步进
#define CONFIG_OSC_WIN_SIZE 400 // OSC窗口初值
#define CONFIG_OSC_WIN_STEP 400 // OSC窗口步进
#define TCFG_BT_BLE_TX_POWER 9 // 最大发射功率
#define TCFG_BT_BLE_BREDR_SAME_ADDR 1 // 和2.1同地址
#define TCFG_BT_BLE_ADV_ENABLE 0 // 广播
#define TCFG_BT_AI_SEL_PROTOCOL 1 // AI协议选择
#define TCFG_BT_RCSP_DUAL_CONN_ENABLE 1 // 支持连接两路rcsp
#define TCFG_BT_DONGLE_ENABLE 0 // 支持dongle连接
// 升级配置.json
#define TCFG_TEST_BOX_ENABLE 1 // 测试盒串口升级
#define TCFG_UPDATE_UART_IO_EN 0 // 普通io串口升级
#define TCFG_UPDATE_UART_ROLE 0 // 串口升级主从机选择
// 音频配置.json
#define TCFG_AUDIO_DAC_CONNECT_MODE DAC_OUTPUT_MONO_L // 输出方式
#define TCFG_AUDIO_DAC_LDO_VOLT 18 // DACVDD挡位
#define TCFG_AUDIO_DAC_LIGHT_CLOSE_ENABLE 0x0 // 轻量关闭
#define TCFG_AUDIO_VCM_CAP_EN 0x1 // VCM电容
#define TCFG_AUDIO_DAC_BUFFER_TIME_MS 50 // 最大缓存时间(msec)
#define TCFG_AUDIO_MIC_LDO_VSEL 5 // MIC LDO 电压
#define TCFG_AUDIO_MIC_LDO_ISEL 2 // MIC LDO 电流
#define TCFG_AEC_TOOL_ONLINE_ENABLE 0 // 手机APP在线调试
#define TCFG_AUDIO_CVP_SYNC 1 // 通话上行同步
#define TCFG_ESCO_DL_CVSD_SR_USE_16K 0 // 通话下行固定16k
#define TCFG_AUDIO_DUT_ENABLE 1 // 音频/通话产测
#define TCFG_AUDIO_MIC_ARRAY_TRIM_ENABLE 0 // 麦克风阵列校准
#define AUDIO_ENC_MPT_SELF_ENABLE 0 // JL自研ENC产测
#define TCFG_KWS_VOICE_RECOGNITION_ENABLE 0 // 关键词检测KWS
#define TCFG_KWS_MIC_CHA_SELECT AUDIO_ADC_MIC_0 // 麦克风选择
#define TCFG_AUDIO_DAC_DEFAULT_VOL_MODE  0 // 音量增强模式
// string2uuid的宏
// 提示音宏定义
#define TCFG_TONE_EN_ENABLE 1
#define TCFG_TONE_ZH_ENABLE 0
#define TCFG_DEC_WTS_ENABLE 1
#define TCFG_DEC_SIN_ENABLE 1
#define TCFG_DEC_WAV_ENABLE 1
#endif