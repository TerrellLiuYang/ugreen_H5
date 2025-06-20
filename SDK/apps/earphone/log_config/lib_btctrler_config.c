/*********************************************************************************************
    *   Filename        : btctrler_config.c

    *   Description     : Optimized Code & RAM (编译优化配置)

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-03-16 11:49

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#include "app_config.h"
#include "system/includes.h"
#include "btcontroller_config.h"
#include "bt_common.h"

// *INDENT-OFF*
/**
 * @brief Bluetooth Module
 */
#if TCFG_BT_DONGLE_ENABLE
	const int CONFIG_DONGLE_SPEAK_ENABLE  = 1;
#else
	const int CONFIG_DONGLE_SPEAK_ENABLE  = 0;
#endif

//固定使用正常发射功率的等级:0-使用不同模式的各自等级;1~10-固定发射功率等级
const int config_force_bt_pwr_tab_using_normal_level  = 0;
//配置BLE广播发射功率的等级:0-最大功率等级;1~10-固定发射功率等级
const int config_ble_adv_tx_pwr_level  = 5;

const int CONFIG_BLE_SYNC_WORD_BIT = 30;
const int CONFIG_LNA_CHECK_VAL = -80;

#if TCFG_USER_TWS_ENABLE
	#if (BT_FOR_APP_EN || TCFG_USER_BLE_ENABLE)
		const int config_btctler_modules        = BT_MODULE_CLASSIC | BT_MODULE_LE;
	#else
 		#ifdef CONFIG_NEW_BREDR_ENABLE
 	    	#if (BT_FOR_APP_EN)
 		    	const int config_btctler_modules        = (BT_MODULE_CLASSIC | BT_MODULE_LE);
 	        #else
 		        const int config_btctler_modules        = (BT_MODULE_CLASSIC);
 	        #endif
        #else
 	        const int config_btctler_modules        = (BT_MODULE_CLASSIC | BT_MODULE_LE);
        #endif
    #endif


	#ifdef CONFIG_NEW_BREDR_ENABLE
		const int CONFIG_BTCTLER_FAST_CONNECT_ENABLE     = 0;

	     #if TCFG_DEC2TWS_ENABLE
	     	const int CONFIG_TWS_WORK_MODE                  = 1;
	     #else
	     	const int CONFIG_TWS_WORK_MODE                  = 2;
	     #endif

		#ifdef CONFIG_SUPPORT_EX_TWS_ADJUST
			const int CONFIG_EX_TWS_ADJUST                  = 1;
		#else
			const int CONFIG_EX_TWS_ADJUST                  = 0;
		#endif

		const int CONFIG_EXTWS_NACK_LIMIT_INT_CNT       = 4;

	#else//CONFIG_NEW_BREDR_ENABLE
		const int CONFIG_BTCTLER_FAST_CONNECT_ENABLE     = 0;
	#endif//end CONFIG_NEW_BREDR_ENABLE

	const int CONFIG_BTCTLER_TWS_ENABLE     = 1;

#if TCFG_TWS_AUTO_ROLE_SWITCH_ENABLE
		const int CONFIG_TWS_AUTO_ROLE_SWITCH_ENABLE = 1;
	#else
		const int CONFIG_TWS_AUTO_ROLE_SWITCH_ENABLE = 0;
	#endif

    const int CONFIG_TWS_POWER_BALANCE_ENABLE   = TCFG_TWS_POWER_BALANCE_ENABLE;
    const int CONFIG_LOW_LATENCY_ENABLE         = 1;
#else //TCFG_USER_TWS_ENABLE
	#if (TCFG_USER_BLE_ENABLE)
		const int config_btctler_modules        = BT_MODULE_CLASSIC | BT_MODULE_LE;
	#else
		const int config_btctler_modules        = BT_MODULE_CLASSIC;
	#endif

	const int config_btctler_le_tws         = 0;
	const int CONFIG_BTCTLER_TWS_ENABLE     = 0;
	const int CONFIG_LOW_LATENCY_ENABLE     = 0;
	const int CONFIG_TWS_POWER_BALANCE_ENABLE   = 0;
	const int CONFIG_BTCTLER_FAST_CONNECT_ENABLE     = 0;
#endif//end TCFG_USER_TWS_ENABLE


const int CONFIG_A2DP_MAX_BUF_SIZE          = 20 * 1024;
const int CONFIG_TWS_SUPER_TIMEOUT          = 2000;
const int CONFIG_BTCTLER_QOS_ENABLE         = 1;
const int CONFIG_A2DP_DATA_CACHE_LOW_AAC    = 100;
const int CONFIG_A2DP_DATA_CACHE_HI_AAC     = 150;//250;
const int CONFIG_A2DP_DATA_CACHE_LOW_SBC    = 120;//150;
const int CONFIG_A2DP_DATA_CACHE_HI_SBC     = 150;//260;
// const int CONFIG_A2DP_DATA_CACHE_HI_AAC     = 250;
// const int CONFIG_A2DP_DATA_CACHE_LOW_SBC    = 150;
// const int CONFIG_A2DP_DATA_CACHE_HI_SBC     = 260;

const int CONFIG_JL_DONGLE_PLAYBACK_DYNAMIC_LATENCY_ENABLE  = 1;    //jl_dongle 动态延时

const int CONFIG_PAGE_POWER                 = 9;
const int CONFIG_PAGE_SCAN_POWER            = 9;
const int CONFIG_INQUIRY_POWER              = 7;
const int CONFIG_INQUIRY_SCAN_POWER         = 7;
const int CONFIG_DUT_POWER                  = 10;

#if (CONFIG_BT_MODE != BT_NORMAL)
	const int config_btctler_hci_standard   = 1;
#else
	const int config_btctler_hci_standard   = 0;
#endif

const int config_btctler_mode        = CONFIG_BT_MODE;
const int CONFIG_BTCTLER_TWS_FUN     = TWS_ESCO_FORWARD ; // TWS_ESCO_FORWARD

/*-----------------------------------------------------------*/

/**
 * @brief Bluetooth Classic setting
 */
const u8 rx_fre_offset_adjust_enable = 1;

const int config_bredr_fcc_fix_fre = 0;
const int ble_disable_wait_enable = 1;

const int config_btctler_eir_version_info_len = 21;

#ifdef CONFIG_256K_FLASH
	const int CONFIG_TEST_DUT_CODE            = 1;
	const int CONFIG_TEST_FCC_CODE            = 0;
	const int CONFIG_TEST_DUT_ONLY_BOX_CODE   = 1;
#else
	const int CONFIG_TEST_DUT_CODE            = 1;
	const int CONFIG_TEST_FCC_CODE            = 1;
	const int CONFIG_TEST_DUT_ONLY_BOX_CODE   = 0;
#endif//end CONFIG_256K_FLASH

const int CONFIG_ESCO_MUX_RX_BULK_ENABLE  =  0;

const int CONFIG_BREDR_INQUIRY   =  0;
const int CONFIG_INQUIRY_PAGE_OFFSET_ADJUST =  0;

#if TCFG_RCSP_DUAL_CONN_ENABLE
	const int CONFIG_LMP_NAME_REQ_ENABLE  =  1;
#else
	const int CONFIG_LMP_NAME_REQ_ENABLE  =  0;
#endif
const int CONFIG_LMP_PASSKEY_ENABLE  =  0;
const int CONFIG_LMP_OOB_ENABLE  =  0;
const int CONFIG_LMP_MASTER_ESCO_ENABLE  =  0;

#ifdef CONFIG_SUPPORT_WIFI_DETECT
	#if TCFG_USER_TWS_ENABLE
		const int CONFIG_WIFI_DETECT_ENABLE = 1;
		const int CONFIG_TWS_AFH_ENABLE     = 1;
	#else
		const int CONFIG_WIFI_DETECT_ENABLE = 0;
		const int CONFIG_TWS_AFH_ENABLE     = 0;
	#endif

#else
	const int CONFIG_WIFI_DETECT_ENABLE = 0;
    const int CONFIG_TWS_AFH_ENABLE     = 0;
#endif//end CONFIG_SUPPORT_WIFI_DETECT

const int ESCO_FORWARD_ENABLE = 0;


const int config_bt_function  = 0;

///bredr 强制 做 maseter
const int config_btctler_bredr_master = 0;
const int config_btctler_dual_a2dp  = 0;

///afh maseter 使用app设置的map 通过USER_CTRL_AFH_CHANNEL 设置
const int config_bredr_afh_user = 0;
//bt PLL 温度跟随trim
const int config_bt_temperature_pll_trim = 0;
/*security check*/
const int config_bt_security_vulnerability = 0;

const int config_delete_link_key          = 1;           //配置是否连接失败返回PIN or Link Key Missing时删除linkKey

/*-----------------------------------------------------------*/

/**
 * @brief Bluetooth LE setting
 */

#if (TCFG_USER_BLE_ENABLE)

	#if TCFG_BLE_AUDIO_TEST_EN
		const int config_btctler_le_roles    = (LE_ADV | LE_SLAVE | LE_SCAN | LE_INIT | LE_MASTER);
		const uint64_t config_btctler_le_features = LE_ENCRYPTION | LE_CORE_V50_FEATURES | LE_FEATURES_ISO ;//| LE_FEATURES_CONST_TONE;

	#else /* TCFG_BLE_AUDIO_TEST_EN */

		#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
			const int config_btctler_le_roles    = (LE_ADV);
			const uint64_t config_btctler_le_features = 0;
		#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_WIRELESS_MIC_CLIENT)
			const uint64_t config_btctler_le_features = LE_ENCRYPTION | LE_CODED_PHY | LE_DATA_PACKET_LENGTH_EXTENSION | LE_2M_PHY;
			const int config_btctler_le_roles    = (LE_SCAN | LE_INIT | LE_MASTER);

		#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_WIRELESS_MIC_SERVER)
			const int config_btctler_le_roles    = (LE_ADV | LE_SLAVE);
			const uint64_t config_btctler_le_features = LE_ENCRYPTION | LE_CODED_PHY | LE_DATA_PACKET_LENGTH_EXTENSION | LE_2M_PHY;

		#elif (TCFG_BLE_DEMO_SELECT == DEF_LE_AUDIO_CENTRAL)
			const int config_btctler_le_roles    = (LE_SCAN | LE_INIT | LE_MASTER);
			const uint64_t config_btctler_le_features = LE_ENCRYPTION | LE_CODED_PHY | LE_FEATURES_ISO | LE_2M_PHY;

		#elif (TCFG_BLE_DEMO_SELECT == DEF_LE_AUDIO_PERIPHERAL)
			const int config_btctler_le_roles    = (LE_ADV | LE_SLAVE);
			const uint64_t config_btctler_le_features = LE_ENCRYPTION | LE_CODED_PHY | LE_FEATURES_ISO | LE_2M_PHY;

		#else
			const int config_btctler_le_roles    = (LE_ADV | LE_SLAVE);
			// const uint64_t config_btctler_le_features = LE_ENCRYPTION;
			const uint64_t config_btctler_le_features = LE_ENCRYPTION | LE_DATA_PACKET_LENGTH_EXTENSION | LE_2M_PHY;
		#endif /* (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV) */

	#endif /* TCFG_BLE_AUDIO_TEST_EN */

#else /* TCFG_USER_BLE_ENABLE */
	const int config_btctler_le_roles    = 0;
	const uint64_t config_btctler_le_features = 0;
#endif//end TCFG_USER_BLE_ENABLE

// Slave multi-link
const int config_btctler_le_slave_multilink = 1;
// Master multi-link
const int config_btctler_le_master_multilink = 0;
// LE RAM Control

#if TCFG_BLE_AUDIO_TEST_EN
	const int config_btctler_le_hw_nums = 5;
	#else

	#ifdef CONFIG_NEW_BREDR_ENABLE
		const int config_btctler_le_hw_nums = 2;
	#else
		const int config_btctler_le_hw_nums = 2;
	#endif

#endif /* TCFG_BLE_AUDIO_TEST_EN */


const int config_btctler_le_slave_conn_update_winden = 2500;//range:100 to 2500

#if TCFG_BLE_AUDIO_TEST_EN
	const int config_btctler_le_afh_en = 0;
	const u32 config_vendor_le_bb = 0;
	const bool config_le_high_priority = 0;

	const int config_btctler_le_rx_nums = 20;
	const int config_btctler_le_acl_packet_length = 255;
	const int config_btctler_le_acl_total_nums = 10;

#else /* TCFG_BLE_AUDIO_TEST_EN */

	// LE vendor baseband
	#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_WIRELESS_MIC_CLIENT)
	//const u32 config_vendor_le_bb = VENDOR_BB_MD_CLOSE |
	//VENDOR_BB_CONNECT_SLOT |
	//VENDOR_BB_EVT_HOLD |
	//VENDOR_BB_EVT_HOLD_TRIGG(25) |
	//VENDOR_BB_EVT_HOLD_TICK(0); //LE vendor baseband
		const u32 config_vendor_le_bb = VENDOR_BB_MD_CLOSE | VENDOR_BB_CONNECT_SLOT | VENDOR_BB_ADV_PDU_INT(3);//LE vendor baseband
		// Master AFH
		const int config_btctler_le_afh_en = 1;
		const int config_btctler_le_rx_nums = 8;
		const int config_btctler_le_acl_packet_length = 251;
		const int config_btctler_le_acl_total_nums = 1;

	#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_WIRELESS_MIC_SERVER)
		const u32 config_vendor_le_bb = VENDOR_BB_MD_CLOSE | VENDOR_BB_CONNECT_SLOT | VENDOR_BB_ADV_PDU_INT(3);//LE vendor baseband
		// Master AFH
		const int config_btctler_le_afh_en = 1;
		const int config_btctler_le_rx_nums = 8;
		const int config_btctler_le_acl_packet_length = 251;
		const int config_btctler_le_acl_total_nums = 1;

// 	#else
// 		// Master AFH
// 		const int config_btctler_le_afh_en = 0;
// 		const u32 config_vendor_le_bb = 0;
// 		const bool config_le_high_priority = 0;
// #if RCSP_MODE
// 		const bool config_tws_le_role_sw = 0;
// #else
// 		const bool config_tws_le_role_sw = 1;
// #endif

// 		const int config_btctler_le_rx_nums = 5;
// 		const int config_btctler_le_acl_packet_length = 27;
// 		const int config_btctler_le_acl_total_nums = 5;
// 	#endif
#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_RCSP_DEMO)
	// Master AFH
	const int config_btctler_le_afh_en = 0;
	const u32 config_vendor_le_bb = 0;
	const bool config_le_high_priority = 0;
	const bool config_tws_le_role_sw = 0;
	const int config_btctler_le_rx_nums = 15;
	const int config_btctler_le_acl_packet_length = 255;
	const int config_btctler_le_acl_total_nums = 15;
#else

	// Master AFH

	const int config_btctler_le_afh_en = 0;

	const u32 config_vendor_le_bb = 0;

	const bool config_le_high_priority = 0;

	const bool config_tws_le_role_sw = 1;



	const int config_btctler_le_rx_nums = 10;

	const int config_btctler_le_acl_packet_length = 27;

	const int config_btctler_le_acl_total_nums = 5;

#endif

#endif /* TCFG_BLE_AUDIO_TEST_EN */

/*-----------------------------------------------------------*/
/**
 * @brief Bluetooth Analog setting
 */
/*-----------------------------------------------------------*/
#if ((!TCFG_USER_BT_CLASSIC_ENABLE) && TCFG_USER_BLE_ENABLE)
	const int config_btctler_single_carrier_en = 1;   ////单模ble才设置
#else
	const int config_btctler_single_carrier_en = 0;
#endif

const int sniff_support_reset_anchor_point = 0;   //sniff状态下是否支持reset到最近一次通信点，用于HID
const int sniff_long_interval = (500 / 0.625);    //sniff状态下进入long interval的通信间隔(ms)
const int config_rf_oob = 0;

// *INDENT-ON*
/*-----------------------------------------------------------*/

/**
 * @brief Log (Verbose/Info/Debug/Warn/Error)
 */
/*-----------------------------------------------------------*/
//RF part
const char log_tag_const_v_Analog  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_Analog  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_Analog  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_Analog  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_Analog  = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_RF  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_RF  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_RF  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_RF  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_RF  = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_BDMGR   = CONFIG_DEBUG_ENABLE;
const char log_tag_const_i_BDMGR   = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_BDMGR   = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_BDMGR   = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_BDMGR   = CONFIG_DEBUG_LIB(1);

//Classic part
const char log_tag_const_v_HCI_LMP   = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_HCI_LMP   = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_HCI_LMP   = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_HCI_LMP   = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_HCI_LMP   = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LMP  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LMP  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_LMP  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LMP  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LMP  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LINK   = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LINK   = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_LINK   = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LINK   = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LINK   = CONFIG_DEBUG_LIB(0);

//LE part
const char log_tag_const_v_LE_BB  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LE_BB  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LE_BB  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LE_BB  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LE_BB  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LE5_BB  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LE5_BB  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LE5_BB  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LE5_BB  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LE5_BB  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_HCI_LL  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_HCI_LL  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_HCI_LL  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_HCI_LL  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_HCI_LL  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LL  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LL  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_E  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_E  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_E  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_E  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_E  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_M  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_M  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_M  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_M  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_M  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_ADV  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_ADV  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_ADV  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_ADV  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_ADV  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_SCAN  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_SCAN  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_SCAN  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_SCAN  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_SCAN  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_INIT  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_INIT  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_INIT  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_INIT  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_INIT  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_EXT_ADV  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_EXT_ADV  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_EXT_ADV  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_EXT_ADV  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_EXT_ADV  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_EXT_SCAN  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_EXT_SCAN  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_EXT_SCAN  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_EXT_SCAN  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_EXT_SCAN  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_EXT_INIT  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_EXT_INIT  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_EXT_INIT  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_EXT_INIT  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_EXT_INIT  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_TWS_ADV  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_TWS_ADV  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_TWS_ADV  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_TWS_ADV  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_TWS_ADV  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_TWS_SCAN  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_TWS_SCAN  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_TWS_SCAN  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_TWS_SCAN  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_TWS_SCAN  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_S  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_S  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_LL_S  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_S  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_S  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_RL  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_RL  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_LL_RL  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_RL  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_RL  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_WL  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_WL  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_LL_WL  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_WL  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_WL  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_AES  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AES  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_AES  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_AES  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_AES  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_PADV  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_PADV  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_PADV  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_PADV  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_PADV  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_DX  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_DX  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_DX  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_DX  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_DX  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_PHY  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_PHY  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_PHY  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_PHY  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_PHY  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_AFH  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_AFH  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_LL_AFH  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LL_AFH  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_AFH  = CONFIG_DEBUG_LIB(1);

//HCI part
const char log_tag_const_v_Thread  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_Thread  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_Thread  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_Thread  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_Thread  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_HCI_STD  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_HCI_STD  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_HCI_STD  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_HCI_STD  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_HCI_STD  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_HCI_LL5  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_HCI_LL5  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_HCI_LL5  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_HCI_LL5  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_HCI_LL5  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_ISO  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_ISO  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_ISO  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LL_ISO  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_ISO  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_BIS  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_BIS  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_BIS  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_BIS  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_BIS  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_CIS  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_CIS  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_CIS  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_CIS  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_CIS  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_BL  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_BL  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_BL  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_BL  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_BL  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_c_BL  = CONFIG_DEBUG_LIB(1);


const char log_tag_const_v_TWS_LE  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_i_TWS_LE  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_TWS_LE  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_TWS_LE  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_TWS_LE  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_c_TWS_LE  = CONFIG_DEBUG_LIB(1);


const char log_tag_const_v_TWS_LMP  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_i_TWS_LMP  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_TWS_LMP  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_TWS_LMP  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_TWS_LMP  = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_TWS  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_TWS  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_TWS  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_TWS  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_TWS  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_TWS_ESCO  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_i_TWS_ESCO  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_TWS_ESCO  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_TWS_ESCO  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_TWS_ESCO  = CONFIG_DEBUG_LIB(1);
