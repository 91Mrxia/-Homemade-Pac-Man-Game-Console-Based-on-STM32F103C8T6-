#ifndef _MAP_H_
#define _MAP_H_

#include "stm32f10x.h"                  // Device header

#define GZ_X_MAX     13
#define GZ_Y_MAX     13

#define POWER_TICKS   60

extern uint32_t g_score;
extern uint8_t  g_power;

typedef enum {
	GZ_WALL_FLAG = 0,
	GZ_ROAD_FLAG,
	GZ_REN_FLAG,
	GZ_YOULING_FLAG,
	GZ_DOU_FLAG,
	GZ_DAOJU_FLAG,
}GZ_FLAG;

typedef struct {
	GZ_FLAG flag;
}GZ_t;

extern const GZ_FLAG gz_flags[GZ_X_MAX][GZ_Y_MAX ];
extern GZ_t gzs[GZ_X_MAX][GZ_Y_MAX ];

void GZ_Init(void);
void GZ_Reset(void);
void GZ_ClearBeans(void);
void GZ_SetFlag(int x, int y, GZ_FLAG flag);
GZ_FLAG GZ_GetFlag(int x, int y);
void GZ_Point_XZ(uint8_t s_x, uint8_t s_y, uint8_t *d_x, uint8_t *d_y);

#endif
