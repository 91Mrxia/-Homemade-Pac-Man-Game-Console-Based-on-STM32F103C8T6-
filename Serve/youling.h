#ifndef _YOULING_H_
#define _YOULING_H_

#include "stm32f10x.h"
#include "ren.h"

#define YOULING_NUM  3

#define YL_DIR_NONE  0
#define YL_DIR_UP    1
#define YL_DIR_DOWN  2
#define YL_DIR_LEFT  3
#define YL_DIR_RIGHT 4

/* 幽灵个性 */
#define YL_SELF_CHASE   0   /* 自动追踪吃豆人(BFS) */
#define YL_SELF_RANDOM  1   /* 随机移动 */

typedef enum {
	YL_ST_NORMAL = 0,   /* 正常追击 */
	YL_ST_FRIGHTENED,   /* 惊吓(被吃豆人吃) */
	YL_ST_EATEN,        /* 被吃, 回巢中 */
} YoulingState;

typedef struct {
	uint8_t youling_gz_x;
	uint8_t youling_gz_y;
	uint8_t is_run;       /* 是否已出发 */
	uint8_t dir;
	uint8_t wait;         /* 出生等待节拍 */
	uint8_t home_x;
	uint8_t home_y;
	uint8_t color;        /* 0/1/2 用于取色 */
	uint8_t self;         /* 个性: YL_SELF_CHASE / YL_SELF_RANDOM */
	YoulingState state;
} Youling_t;

extern Youling_t youlingA, youlingB, youlingC;

void YouLing_Init(void);
void YouLing_ResetPositions(void);
Youling_t* YouLing_Get(uint8_t i);
void Youling_All_Update(void);
void Youling_MoveInput(uint8_t index, uint8_t d);
void Youling_SetPlayerMode(uint8_t enabled);
void Youling_UpdatePlayerState(void);

#endif
