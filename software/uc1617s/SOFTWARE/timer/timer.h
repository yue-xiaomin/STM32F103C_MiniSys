#ifndef __TIMER_H
#define __TIMER_H

#include "sys.h"

extern u8 TIM2_CAP_STA; //输入捕获状态
extern u16 TIM2_CAP_VAL; //输入捕获值

//  Tout = (arr+1)(psc+1)/72000000 ms
//	第一个参数重载值，第二个预分频系数
void TIME2_Interuput_Init(u16 arr,u16 psc); //定时器2计时初始化

//  Tout = (arr+1)(psc+1)/72000000 ms
//	第一个参数重载值，第二个预分频系数
void TIME1_PWM_Init(u16 arr,u16 psc); //定时器1CH1输出可调PWM

//  Tout = (arr+1)(psc+1)/72000000 ms
//	第一个参数重载值，第二个预分频系数
void TIME2_CAP_Init(u16 arr,u16 psc) ;//捕获定时器2通道1初始化

#endif



