#ifndef _YAOGAN_H_
#define _YAOGAN_H_

#include "stm32f10x.h"                  // Device header

void Yaogan_Init();
uint8_t Yaogan_GetNum(void);
void Yaogan_GetDebug(uint16_t *x_raw, uint16_t *y_raw);

#endif