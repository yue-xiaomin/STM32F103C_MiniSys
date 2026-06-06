/**
 * @file timer.c
 * @brief 硬件定时器实现，为osal操作系统提供系统滴答心跳时钟，移植时需要修改该文件
 * @version 0.1
 * @date 
 * @author 
 */

//#include <stdint.h>
//#include <string.h>
#include <stdio.h>
//#include <stdlib.h>
#include "stm32f10x.h"

#include "osal_timer.h"
#include "timer.h"





//此处添加硬件定时器中断溢出函数，中断周期1～10ms，并在中断函数中调用系统时钟更新函数osal_update_timers()
//即每次系统滴答心跳时调用一次osal_update_timers()

// SysTick 中断周期计数寄存器值（72MHz / 1000 = 72000 个时钟周期）
#define SYSTICK_LOAD_VALUE   (SystemCoreClock / 1000UL - 1UL)

void SysTick_Init(void)
{
    // 设置重装载值
    SysTick->LOAD = SYSTICK_LOAD_VALUE;
    // 清除当前计数值
    SysTick->VAL = 0;
    // 选择处理器时钟（AHB），使能中断，使能计数器
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    
    // 设置 SysTick 中断优先级（可选，默认优先级为最低）
    // 使用 CMSIS 函数设置优先级（数值越小优先级越高）
    NVIC_SetPriority(SysTick_IRQn, 2);  
}


/**
 * @brief 硬件定时器初始化，设定系统时钟
 */
void OSAL_TIMER_TICKINIT(void)
{
    SysTick_Init();

    printf("Init hal timer ok !\n");
}

/**
 * @brief 开启硬件定时器，OSAL会根据程序中软件定时器的实际使用动态开启和关闭，为空则一直开启
 */
void OSAL_TIMER_TICKSTART(void)
{

}

/**
 * @brief 关闭硬件定时器，为空则一直不关闭
 */
void OSAL_TIMER_TICKSTOP(void)
{

}
