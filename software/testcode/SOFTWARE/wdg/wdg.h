#ifndef __IWDG_H
#define __IWDG_H

#include "sys.h"

void IWDG_Init(u8,u16); //初始化独立看门狗，第一个参数分屏系数，第二个参数重载值
void WWDG_Init(u8,u8,u32);//初始化窗口看门狗，第一个参数窗口值，第二个计算值，第三个分频系数

#endif


