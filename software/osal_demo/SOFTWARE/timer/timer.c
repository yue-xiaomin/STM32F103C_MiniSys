#include "timer.h"
#include "led.h"
void TIME2_Interuput_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitS;
	NVIC_InitTypeDef NVIC_InitStruct;
	//初始化定时器2时钟使能
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	//初始化定时器，配置重载值，分频系数
	TIM_TimeBaseInitS.TIM_Period = arr;  //定时器重载值
	TIM_TimeBaseInitS.TIM_Prescaler = psc;  //定时器分频系数
	TIM_TimeBaseInitS.TIM_CounterMode =TIM_CounterMode_Up;  //向上计时
	TIM_TimeBaseInitS.TIM_ClockDivision = TIM_CKD_DIV1;  
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitS);
	//开启定时器更新中断
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);  //更新中断使能
	//设置中断优先级，并初始化
	NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(&NVIC_InitStruct);
	//使能定时器2
	TIM_Cmd(TIM2,ENABLE);
		
}

void TIME1_PWM_Init(u16 arr,u16 psc)
{
	GPIO_InitTypeDef GPIO_InitStruct; //GPIO配置结构体
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct; //定时器配置结构体
	TIM_OCInitTypeDef TIM_OCInitStruct; //定时器输出配置结构体
	//使能定时器1和GPIO时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1|RCC_APB2Periph_GPIOA,ENABLE);
	//GPIOA引脚输出配置，设置该引脚为复用输出功能,输出TIM1 CH1的PWM脉冲波形
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP; //复用推挽输出
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8; //GPIO->A.8
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruct);
	//定时器输出配置
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up; //TIM向上计数模式
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseInitStruct.TIM_Prescaler = psc; //设置用来作为TIMx时钟频率除数的预分频值  不分频
	TIM_TimeBaseInitStruct.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	 80K
	TIM_TimeBaseInit(TIM1,&TIM_TimeBaseInitStruct);
	//PWM输出配置，PWM2输出模式，高电平有效
	TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM2;  //选择定时器模式:TIM脉冲宽度调制模式2
	TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;  //输出极性:TIM输出比较极性高
	TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;  //比较输出使能
	TIM_OCInitStruct.TIM_Pulse = 0;  //设置待装入捕获比较寄存器的脉冲值
	TIM_OC1Init(TIM1,&TIM_OCInitStruct);  //根据TIM_OCInitStruct中指定的参数初始化外设TIMx
	//使能TIM1
	TIM_Cmd(TIM1,ENABLE);
	//MOE 主输出使能
	TIM_CtrlPWMOutputs(TIM1,ENABLE);
	
}


void TIME2_CAP_Init(u16 arr,u16 psc)
{
	GPIO_InitTypeDef GPIO_InitStruct; //GPIO配置结构体
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct; //定时器配置结构体
	TIM_ICInitTypeDef TIM_ICInitStruct; //输出输入配置结构体
	NVIC_InitTypeDef NVIC_InitStruct; //中断分组配置结构体
	//使能GPIO和定时器外设时钟使能
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE); //GPIOA使能
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);  //定时器2使能
	//GPIO输入配置
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD;  //输入下拉
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0; //PA0 清除之前设置 
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruct);
	GPIO_ResetBits(GPIOA,GPIO_Pin_0); //PA0 下拉
	//初始化定时器2 TIM2
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;  //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;   //TIM向上计数模式
	TIM_TimeBaseInitStruct.TIM_Period = arr;  //设定计数器自动重装值
	TIM_TimeBaseInitStruct.TIM_Prescaler = psc;  //预分频器 
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStruct);//根据TIM_TimeBaseInitStruct中指定的参数初始化
	//初始化TIM2输入捕获参数
	TIM_ICInitStruct.TIM_Channel = TIM_Channel_1;  //CC1S=01 	选择输入端 IC1映射到TI1上
	TIM_ICInitStruct.TIM_ICPolarity = TIM_ICPolarity_Rising;   //上升沿捕获
	TIM_ICInitStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;   //配置输入分频,不分频 
	TIM_ICInitStruct.TIM_ICSelection = TIM_ICSelection_DirectTI;   //映射到TI1上
	TIM_ICInitStruct.TIM_ICFilter = 0x00;  //IC1F=0000 配置输入滤波器 不滤波
	TIM_ICInit(TIM2,&TIM_ICInitStruct);
	//中断分组初始化
	NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;   //TIM2中断
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;  //IRQ通道被使能
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;  //抢占优先级2级
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;//子优先级0级
	NVIC_Init(&NVIC_InitStruct);
	//允许更新中断 ,允许CC1IE捕获中断
	TIM_ITConfig(TIM2,TIM_IT_Update|TIM_IT_CC1,ENABLE);
	//使能定时器2
	TIM_Cmd(TIM2,ENABLE);
	
}
u8 TIM2_CAP_STA = 0; //输入捕获状态
u16 TIM2_CAP_VAL = 0; //输入捕获值
//定时器5中断服务程序
void TIM2_IRQHandler(void)
{
	/*
	基础定时中断刷新程序
	if(TIM_GetFlagStatus(TIM2,TIM_FLAG_Update )!=RESET)  //如果定时器更新中断，翻转绿灯
	{
		LEDG =~LEDG;
		TIM_ClearFlag(TIM2,TIM_FLAG_Update); //清除更新中断标记
	}*/
	if((TIM2_CAP_STA&0X80) == 0)//还未成功捕获
	{
		if(TIM_GetITStatus(TIM2,TIM_IT_Update) != RESET)
		{
			if(TIM2_CAP_STA&0x40) //已经捕获到高电平了
			{
				if((TIM2_CAP_STA&0x3F) == 0x3F) //高电平太长了
				{
					TIM2_CAP_STA |= 0x80; //标记成功捕获了一次
					TIM2_CAP_VAL = 0xFFFF;		//标记为最大值
				}
				else
				{
					TIM2_CAP_STA++;  //溢出计数++
				}
			}
		}
		
		if(TIM_GetITStatus(TIM2,TIM_IT_CC1) != RESET) //捕获1发生捕获事件
		{
			if(TIM2_CAP_STA&0x40)   //捕获到一个下降沿 	
			{
				TIM2_CAP_STA |=0x80;  //标记成功捕获到一次上升沿
				TIM2_CAP_VAL=TIM_GetCapture1(TIM2);
				TIM_OC1PolarityConfig(TIM2,TIM_ICPolarity_Rising);   //CC1P=0 设置为上升沿捕获
			}
			else     //还未开始,第一次捕获上升沿
			{
				TIM2_CAP_STA = 0;  //清空
				TIM2_CAP_VAL =0;
				TIM_SetCounter(TIM2,0);
				TIM2_CAP_STA |= 0x40;  //标记捕获到了上升沿
				TIM_OC1PolarityConfig(TIM2,TIM_ICPolarity_Falling); //CC1P=1 设置为下降沿捕获
			}
		}
			
	}
	TIM_ClearITPendingBit(TIM2,TIM_IT_Update|TIM_IT_CC1); //清除中断标志位
}


