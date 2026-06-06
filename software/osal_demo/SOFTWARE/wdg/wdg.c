#include "wdg.h"
#include "led.h"

void IWDG_Init(u8 prer,u16 rlr)
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable); //取消寄存器写保护
	
	IWDG_SetPrescaler(prer);  //确定分频系数
	
	IWDG_SetReload(rlr); //设置重装值
	
	IWDG_ReloadCounter(); //按照 IWDG 重装载寄存器的值重装载 IWDG 计数器
	
	IWDG_Enable(); //独立看门狗使能
}

u8 CNT;//保存 WWDG 计数器的设置值,默认为最大.

void WWDG_Init(u8 wv,u8 cnt,u32 prer)
{
	NVIC_InitTypeDef NVIC_InitStruct;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG,ENABLE);//窗口看门狗使能
	
	WWDG_SetPrescaler(prer); //设置窗口看门狗预分频值
	
	WWDG_SetWindowValue(wv); //设置窗口值
	
	WWDG_EnableIT(); //使能看门狗中断
	
	WWDG_ClearFlag(); 
	
	NVIC_InitStruct.NVIC_IRQChannel = WWDG_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStruct);//NVIC初始化
	
	CNT =cnt&0x7F;
	WWDG_Enable(cnt);//使能窗口看门狗
}
//窗口看门狗中断服务函数
void WWDG_IRQHandler()
{
	WWDG_SetCounter(0x7F);//当禁掉此句后,窗口看门狗将产生复位
	WWDG_ClearFlag();// Clear EWI flag
	LEDG =~LEDG;

}








