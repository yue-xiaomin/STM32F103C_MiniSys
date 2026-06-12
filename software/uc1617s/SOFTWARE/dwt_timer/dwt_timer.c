#include "dwt_timer.h"

//#include <stm32f10x_conf.h>


// DWT寄存器定义
#define DWT_BASE           (0xE0001000UL)
#define DWT_CTRL          (*(volatile uint32_t*)(DWT_BASE + 0x0))
#define DWT_CYCCNT        (*(volatile uint32_t*)(DWT_BASE + 0x4))

// DEMCR寄存器定义
#define DEMCR             (*(volatile uint32_t*)(0xE000EDFCUL))
#define TRCENA_BIT        (1 << 24)

// DWT控制寄存器位定义
#define CYCCNTENA_BIT     (1 << 0)


// 全局变量保存测量值
static uint32_t dwt_start_ticks = 0;
static uint32_t dwt_end_ticks = 0;
static uint32_t dwt_elapsed_ticks = 0;

// 初始化DWT单元
void dwt_init(void) {
    // 启用跟踪单元
    DEMCR |= TRCENA_BIT;
    
    // 清零周期计数器
    DWT_CYCCNT = 0;
    
    // 启用周期计数器
    DWT_CTRL |= CYCCNTENA_BIT;
}

// 启动DWT定时器（清零并开始计数）
void dwt_start_timer(void) {
    // 确保DWT已初始化
    if ((DEMCR & TRCENA_BIT) == 0) {
        dwt_init();
    }
    
    // 清零计数器
    DWT_CYCCNT = 0;
    
    // 记录开始时间
    dwt_start_ticks = 0;
    
    // 确保计数器已启用
    DWT_CTRL |= CYCCNTENA_BIT;
}

// 停止DWT定时器并保存计数值
void dwt_stop_timer(void) {
    // 保存结束时间
    dwt_end_ticks = DWT_CYCCNT;
    
    // 计算经过的周期数
    if (dwt_end_ticks >= dwt_start_ticks) {
        dwt_elapsed_ticks = dwt_end_ticks - dwt_start_ticks;
    } else {
        // 处理计数器溢出
        dwt_elapsed_ticks = (0xFFFFFFFF - dwt_start_ticks) + dwt_end_ticks + 1;
    }
}

// 获取经过的周期数
uint32_t dwt_get_elapsed_ticks(void) {
    return dwt_elapsed_ticks;
}

// 获取经过的时间（微秒）
uint32_t dwt_get_elapsed_time_us(void) {
    // 将周期数转换为微秒
    return (uint32_t)((dwt_elapsed_ticks * 1000000ULL) / CPU_FREQ_HZ);
}

// 获取经过的时间（毫秒）
uint32_t dwt_get_elapsed_time_ms(void) {
    // 将周期数转换为毫秒
    return (uint32_t)((dwt_elapsed_ticks * 1000ULL) / CPU_FREQ_HZ);
}

// 直接测量函数执行时间（周期数）- 可选择是否禁用中断
uint32_t dwt_measure_function_ticks(void (*func)(void), uint8_t disable_irq) {
    uint32_t start, end;

    
    // 确保DWT已初始化
    if ((DEMCR & TRCENA_BIT) == 0) {
        dwt_init();
    }
    
    // 如果需要禁用中断，保存当前中断状态并禁用中断
    if (disable_irq) {

        __disable_irq();
    }
    
    // 清零计数器
    DWT_CYCCNT = 0;
    
    // 记录开始时间
    start = DWT_CYCCNT;
    
    // 调用要测量的函数
    func();
    
    // 记录结束时间
    end = DWT_CYCCNT;
    
    // 如果需要禁用中断，恢复之前的中断状态
    if (disable_irq) {
        __enable_irq();
    }
    
    // 计算并返回经过的周期数
    if (end >= start) {
        return end - start;
    } else {
        // 处理计数器溢出
        return (0xFFFFFFFF - start) + end + 1;
    }
}

// 直接测量函数执行时间（微秒）- 可选择是否禁用中断
uint32_t dwt_measure_function_time_us(void (*func)(void), uint8_t disable_irq) {
    uint32_t ticks = dwt_measure_function_ticks(func, disable_irq);
    return (uint32_t)((ticks * 1000000ULL) / CPU_FREQ_HZ);
}

// 直接测量函数执行时间（毫秒）- 可选择是否禁用中断
uint32_t dwt_measure_function_time_ms(void (*func)(void), uint8_t disable_irq) {
    uint32_t ticks = dwt_measure_function_ticks(func, disable_irq);
    return (uint32_t)((ticks * 1000ULL) / CPU_FREQ_HZ);
}

