#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "stm32f10x.h"                  // Device header

void Motor_Init(void);
void Motor_On(void);
void Motor_Off(void);
void Motor_Vibrate(uint16_t ms);

#endif 