#include "touch.h"
#include "delay.h"

void Touch_Init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
}


uint8_t Touch_GetNum()
{
	uint8_t KeyNum = 0;
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12)==1)
	{
		Delay_ms(10);
		KeyNum = 4;
	}	
	
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13)==1)
	{
		Delay_ms(10);
		KeyNum = 3;
	}
	
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_14)==1)
	{
		Delay_ms(10);
		KeyNum = 2;
	}

	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_15)==1)
	{
		Delay_ms(10);
		KeyNum = 1;
	}
	
	return KeyNum;
}
















