#include "buzzer.h"
#include "delay.h"

void Buzzer_Init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	TIM_InternalClockConfig(TIM1);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 99;		
	TIM_TimeBaseInitStructure.TIM_Prescaler = 1439;	
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 50;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);

	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;				
	TIM_OC2Init(TIM1, &TIM_OCInitStructure);		

	TIM_CtrlPWMOutputs(TIM1, ENABLE);				
	TIM_Cmd(TIM1, ENABLE);
}



/*
 * 设置音高: a = 预分频值(音高码), 音高码见 music.h 宏表
 * a <= 30 时占空比给 0, 静音; 否则 50% 占空比并更新预分频
 */
void Buzzer_SetHZ(uint16_t a)
{
	if (a <= 30)
	{
		TIM_SetCompare2(TIM1, 0);					//占空比 0 = 静音
	}
	else
	{
		TIM_SetCompare2(TIM1, 50);					//50/100 = 50% 占空比
		TIM_PrescalerConfig(TIM1, a, TIM_PSCReloadMode_Immediate);
	}
}









































