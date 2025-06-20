/*
 *******************************************************************
 *						Audio Common Definitions
 *
 *Note(s):
 *		 (1)Only macro definitions can be defined here.
 *		 (2)Use (1UL << (n)) instead of BIT(n)
 *******************************************************************
 */
#ifndef _AUDIO_DEF_H_
#define _AUDIO_DEF_H_



/*
 *******************************************************************
 *						DAC Definitions
 *******************************************************************
 */
#define DAC_DSM_6MHz                       (0)
#define DAC_DSM_12MHz                      (1)

#define DAC_OUTPUT_MONO_L                  (0)   //左声道
#define DAC_OUTPUT_MONO_R                  (1)   //右声道
#define DAC_OUTPUT_LR                      (2)   //立体声
#define DAC_OUTPUT_MONO_LR_DIFF            (3)   //左右差分输出

#define DAC_OUTPUT_DUAL_LR_DIFF            (6)   //双声道差分
#define DAC_OUTPUT_FRONT_LR_REAR_L         (7)   //三声道单端输出 前L+前R+后L (不可设置vcmo公共端)
#define DAC_OUTPUT_FRONT_LR_REAR_R         (8)   //三声道单端输出 前L+前R+后R (可设置vcmo公共端)
#define DAC_OUTPUT_FRONT_LR_REAR_LR        (9)   //四声道单端输出

#define DAC_BIT_WIDTH_16                   (0)   //16bit 位宽
#define DAC_BIT_WIDTH_24                   (1)   //24bit 位宽

#define DAC_CH_FL                          BIT(0)
#define DAC_CH_FR                          BIT(1)
#define DAC_CH_RL                          BIT(2)
#define DAC_CH_RR                          BIT(3)

/*
 *******************************************************************
 *						ADC Definitions
 *******************************************************************
 */
#define ADC_BIT_WIDTH_16                   (0)   //16bit 位宽
#define ADC_BIT_WIDTH_24                   (1)   //24bit 位宽

#define ADC_AIN_PORT0                      BIT(0)
#define ADC_AIN_PORT1                      BIT(1)
#define ADC_AIN_PORT2                      BIT(2)
/*
 *******************************************************************
 *						Codec Definitions
 *******************************************************************
 */
//MSBC Dec Debug
#define CONFIG_MSBC_INPUT_FRAME_REPLACE_DISABLE		0
#define CONFIG_MSBC_INPUT_FRAME_REPLACE_SILENCE		1
#define CONFIG_MSBC_INPUT_FRAME_REPLACE_SINE		2

/*
 *******************************************************************
 *						Linein(Aux) Definitions
 *******************************************************************
 */


/*
 *******************************************************************
 *						ANC Definitions
 *******************************************************************
 */
//ANC Mode Enable
#define ANC_FF_EN 		 			(1UL << (0))
#define ANC_FB_EN  					(1UL << (1))
#define ANC_HYBRID_EN  			 	(1UL << (2))

//ANC芯片版本定义
#define ANC_VERSION_BR30 			0x01	//AD697N/AC897N
#define ANC_VERSION_BR30C			0x02	//AC699N/AD698N
#define ANC_VERSION_BR36			0x03	//AC700N
#define ANC_VERSION_BR28			0x04	//JL701N/BR40
#define ANC_VERSION_BR28_MULT		0xA4	//JL701N 多滤波器
#define ANC_VERSION_BR50			0x05	//JL708N

/*
 *******************************************************************
 *						Smart Voice Definitions
 *******************************************************************
 */
/*离线语音识别语言选择*/
#define KWS_CH          1   /*中文*/
#define KWS_INDIA_EN    2   /*印度英语*/
#define KWS_FAR_CH      3   /*音箱中文*/

/*
 *******************************************************************
 *						Common Definitions
 *******************************************************************
 */
#define  VOL_TYPE_DIGITAL			0	//软件数字音量
#define  VOL_TYPE_ANALOG			1	//硬件模拟音量
#define  VOL_TYPE_AD				2	//联合音量(模拟数字混合调节)
#define  VOL_TYPE_DIGITAL_HW		3  	//硬件数字音量

//数据饱和位宽定义
#define  BIT_WIDE_16BIT  			0
#define  BIT_WIDE_24BIT  			1


#endif/*_AUDIO_DEF_H_*/
