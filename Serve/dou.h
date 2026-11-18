#ifndef _DOU_H_
#define _DOU_H_

#include "stm32f10x.h"                  // Device header

extern volatile uint8_t dou_respawned;
extern uint8_t          dou_respawn_x;
extern uint8_t          dou_respawn_y;

void Dou_Reset(void);
void Dou_Eaten(uint8_t x, uint8_t y);

#endif
