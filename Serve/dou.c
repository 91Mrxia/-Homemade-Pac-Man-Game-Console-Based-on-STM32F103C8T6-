#include "dou.h"
#include "map.h"


#define TRAIL_MAX   5

static uint8_t t_x[TRAIL_MAX];
static uint8_t t_y[TRAIL_MAX];
static uint8_t t_count = 0;   
static uint8_t t_head  = 0;   

volatile uint8_t dou_respawned  = 0;
uint8_t          dou_respawn_x  = 0;
uint8_t          dou_respawn_y  = 0;


void Dou_Reset(void)
{
	t_count = 0;
	t_head  = 0;
	dou_respawned = 0;
}



void Dou_Eaten(uint8_t x, uint8_t y)
{
	uint8_t tail;

	if (t_count == TRAIL_MAX)
	{
		GZ_SetFlag(t_x[t_head], t_y[t_head], GZ_DOU_FLAG);

		/* 通知显示层: 这一格重新长出豆子 */
		dou_respawn_x  = t_x[t_head];
		dou_respawn_y  = t_y[t_head];
		dou_respawned  = 1;

		t_head = (uint8_t)((t_head + 1) % TRAIL_MAX);
		t_count--;
	}

	tail = (uint8_t)((t_head + t_count) % TRAIL_MAX);
	t_x[tail] = x;
	t_y[tail] = y;
	t_count++;
}
