#ifndef _DWT_TIMER_H_
#define _DWT_TIMER_H_

#include <stdint.h>

// 系统主频定义（默认为72MHz）
#ifndef CPU_FREQ_HZ
#define CPU_FREQ_HZ 72000000UL
#endif

// DWT测量宏定义 - 用于测量代码段执行时间
#define DWT_MEASURE(code, result) \
    do{ \
        dwt_start_timer();  \
        code;   \
        dwt_stop_timer();   \
        result = dwt_get_elapsed_ticks();   \
    }while(0)

    
    
    

// 初始化DWT单元
void dwt_init(void);

// 启动DWT定时器（清零并开始计数）
void dwt_start_timer(void);

// 停止DWT定时器并保存计数值
void dwt_stop_timer(void);

// 获取经过的周期数
uint32_t dwt_get_elapsed_ticks(void);

// 获取经过的时间（微秒）
uint32_t dwt_get_elapsed_time_us(void);
    
// 获取经过的时间（毫秒）
uint32_t dwt_get_elapsed_time_ms(void);

// 直接测量函数执行时间（周期数）- 可选择是否禁用中断
uint32_t dwt_measure_function_ticks(void (*func)(void), uint8_t disable_irq);

// 直接测量函数执行时间（微秒）- 可选择是否禁用中断
uint32_t dwt_measure_function_time_us(void (*func)(void), uint8_t disable_irq);

// 直接测量函数执行时间（毫秒）- 可选择是否禁用中断
uint32_t dwt_measure_function_time_ms(void (*func)(void), uint8_t disable_irq);

#endif

