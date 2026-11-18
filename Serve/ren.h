#ifndef _REN_H_
#define _REN_H_

#include "stm32f10x.h"                  // Device header
#include "map.h"
#include "control.h"


#define REN_DIR_NONE   0
#define REN_DIR_UP     1
#define REN_DIR_DOWN   2
#define REN_DIR_LEFT   3
#define REN_DIR_RIGHT  4

#define REN_LIFE_MAX   3

#define REN_HOME_X     6
#define REN_HOME_Y     6


typedef struct {
	uint8_t ren_gz_x;
	uint8_t ren_gz_y;
	uint8_t is_run;
	uint8_t dir;
	uint8_t lifes;
}Ren_t;

extern Ren_t ren;

void Ren_Init(void);         
void Ren_ResetP(void); 
void Ren_Move(void);          
void Ren_MoveInput(uint8_t d); 
void Ren_SetScoreEnabled(uint8_t enabled);

#endif
