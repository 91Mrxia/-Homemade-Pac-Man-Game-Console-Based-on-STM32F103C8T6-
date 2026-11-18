#ifndef _DAOJU_H_
#define _DAOJU_H_

#include "stm32f10x.h"                  // Device header

#define DAOJU_NUM   2
#define DAOJU_RESPAWN_MS   10000  

typedef struct {
    uint8_t daoju_gz_x;
    uint8_t daoju_gz_y;
} Daoju_t;

extern Daoju_t daojus[DAOJU_NUM];

extern volatile uint8_t daoju_respawned;
extern uint8_t          daoju_respawn_x;
extern uint8_t          daoju_respawn_y;

void Daoju_Reset(void);
void Daoju_Init(void);
void Daoju_SetCount(uint8_t count);
void Daoju_MarkEaten(uint8_t x, uint8_t y);
void Daoju_SetRespawnEnabled(uint8_t enabled);
void Daoju_Update(void);

#endif
