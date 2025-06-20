#ifndef __PWM_LED_DEBUG_H__
#define __PWM_LED_DEBUG_H__

//发布模块注释这个宏
// #define PWM_LED_DEBUG_ENABLE
//0=不开demo

#define PWM_LED_TEST_MODE 0

// #define PWM_LED_TEST_MODE 1
// 1=单耳+powerdown测试, 连手机，不连对耳,需要连接上手机才能够sniff

// #define PWM_LED_TEST_MODE 2

// 2=双耳配对同步+powerdown测试,不连手机，连对耳,只有触发配对成功事件才会从情景模式切灯效，注意不要配置为local_action=1的动作
//tws测试，需要同时开机，30s的窗口周期，除去tws配对约剩10s的ui时间

//这个uuid需要跟进配置工具的配置填入
//如果是tws配对成功的灯效就拦截进行切换
#define PWM_PAIRED_LED_UUID 0xb37e


// #define PWM_LED_IO_TEST

#ifdef PWM_LED_DEBUG_ENABLE
//不开启局部打印
#if 1
#define PWMLED_LOG_DEBUG(fmt, ...) 	g_printf("[PWM_LED_DEBUG]\\(^o^)/" fmt "\t|\tfun:%s: line:%d", ##__VA_ARGS__, __FUNCTION__, __LINE__)

#define PWMLED_LOG_BUF put_buf

#define PWMLED_HW_LOG_INFO(fmt, ...) 	b_printf("[PWM_HW_LED_INFO](^_^)" fmt "\t|\tfun:%s: line:%d", ##__VA_ARGS__, __FUNCTION__, __LINE__)
#define PWMLED_HW_LOG_DEBUG(fmt, ...) 	g_printf("[PWM_HW_LED_DEBG](^_^)" fmt "\t|\tfun:%s: line:%d", ##__VA_ARGS__, __FUNCTION__, __LINE__)

#else

#define PWMLED_LOG_INFO printf
#define PWMLED_LOG_DEBUG(...)
#define PWMLED_LOG_SUCC(...)
#define PWMLED_LOG_ERR(...)


#define PWMLED_HW_LOG_INFO(...)
#define PWMLED_HW_LOG_DEBUG(...)
#define PWMLED_LOG_BUF(...)
#endif

//
#if 1
#define PWMLED_LOG_INFO(fmt, ...) 	y_printf("[PWM_LED_INFO] " fmt "\t|\tfun:%s: line:%d", ##__VA_ARGS__, __FUNCTION__, __LINE__)
#define PWMLED_LOG_SUCC(fmt, ...) 	b_printf("[PWM_LED_SUCC](^_^)" fmt "\t|\tfun:%s: line:%d", ##__VA_ARGS__, __FUNCTION__, __LINE__)
#define PWMLED_LOG_ERR(fmt, ...) 		r_printf("[PWM_LED_ERR](>_<)" fmt "\t|\tfun:%s: line:%d", ##__VA_ARGS__, __FUNCTION__, __LINE__)
#endif //#if 0
//

#else
#define PWMLED_LOG_INFO printf
#define PWMLED_LOG_DEBUG(...)
#define PWMLED_LOG_SUCC(...)
#define PWMLED_LOG_ERR(...)


#define PWMLED_HW_LOG_INFO(...)
#define PWMLED_HW_LOG_DEBUG(...)
#define PWMLED_LOG_BUF(...)
#endif


#ifdef PWM_LED_IO_TEST
#define LED_TIMER_TEST_PORT (JL_PORTB)
#define LED_TIMER_TEST_PIN  (5)
#endif

#endif /* #ifndef __PWM_LED_DEBUG_H__ */

