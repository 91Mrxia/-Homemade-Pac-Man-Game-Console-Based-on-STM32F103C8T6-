#include "daoju.h"
#include "map.h"
#include "ren.h"
#include "youling.h"
#include "TIME.h"
#include <stdlib.h>


Daoju_t daojus[DAOJU_NUM];

static uint8_t  dao_respawn_enabled = 0;
static uint32_t dao_respawn_at[DAOJU_NUM];

static const uint8_t dao_home_x[DAOJU_NUM] = { 3, 3 };
static const uint8_t dao_home_y[DAOJU_NUM] = { 1, 11 };
static uint8_t       dao_active_count = DAOJU_NUM;

volatile uint8_t daoju_respawned = 0;
uint8_t          daoju_respawn_x  = 0;
uint8_t          daoju_respawn_y  = 0;


void Daoju_Reset(void)
{
	uint8_t i;

	for (i = 0; i < DAOJU_NUM; i++)
	{
		if (i < dao_active_count)
		{
			GZ_Point_XZ(dao_home_x[i], dao_home_y[i], &daojus[i].daoju_gz_x, &daojus[i].daoju_gz_y);
			GZ_SetFlag(daojus[i].daoju_gz_x, daojus[i].daoju_gz_y, GZ_DAOJU_FLAG);
		}
		else
		{
			daojus[i].daoju_gz_x = 0xFF;
			daojus[i].daoju_gz_y = 0xFF;
		}
	}

	for (i = 0; i < DAOJU_NUM; i++) dao_respawn_at[i] = 0;
	daoju_respawned = 0;
}

void Daoju_Init(void)
{
	Daoju_Reset();
}


/* 吃到道具: 进入能量模式 */
void Daoju_MarkEaten(uint8_t x, uint8_t y)
{
	uint8_t i;
	for (i = 0; i < DAOJU_NUM; i++)
	{
		if (daojus[i].daoju_gz_x == x && daojus[i].daoju_gz_y == y)
		{
			g_power = POWER_TICKS;
			if (dao_respawn_enabled)
				dao_respawn_at[i] = TIM_GetTick() + DAOJU_RESPAWN_MS;
			return;
		}
	}
}


/* skip cells currently occupied by pac-man or a ghost */
static uint8_t Daoju_IsBlocked(uint8_t x, uint8_t y)
{
	uint8_t i;

	if (ren.ren_gz_x == x && ren.ren_gz_y == y) return 1;
	for (i = 0; i < YOULING_NUM; i++)
	{
		Youling_t *g = YouLing_Get(i);
		if (g && g->youling_gz_x == x && g->youling_gz_y == y) return 1;
	}
	return 0;
}


/* 15s respawn: place one item at a random road cell */
static uint8_t Daoju_SpawnRandom(uint8_t i)
{
	uint8_t n = 0, x, y, pick, k;
	uint8_t found_x = 0xFF, found_y = 0xFF;

	for (y = 0; y < GZ_Y_MAX; y++)
		for (x = 0; x < GZ_X_MAX; x++)
			if (GZ_GetFlag(x, y) == GZ_ROAD_FLAG && !Daoju_IsBlocked(x, y)) n++;

	if (n == 0) return 0;

	pick = (uint8_t)(rand() % n);
	k = 0;
	for (y = 0; y < GZ_Y_MAX; y++)
	{
		for (x = 0; x < GZ_X_MAX; x++)
		{
			if (GZ_GetFlag(x, y) != GZ_ROAD_FLAG || Daoju_IsBlocked(x, y)) continue;
			if (k == pick) { found_x = x; found_y = y; break; }
			k++;
		}
		if (found_x != 0xFF) break;
	}

	daojus[i].daoju_gz_x = found_x;
	daojus[i].daoju_gz_y = found_y;
	GZ_SetFlag(found_x, found_y, GZ_DAOJU_FLAG);

	daoju_respawn_x = found_x;
	daoju_respawn_y = found_y;
	daoju_respawned = 1;
	return 1;
}


void Daoju_SetRespawnEnabled(uint8_t enabled)
{
	dao_respawn_enabled = enabled ? 1 : 0;
}


void Daoju_SetCount(uint8_t count)
{
	dao_active_count = (count > DAOJU_NUM) ? DAOJU_NUM : count;
}


void Daoju_Update(void)
{
	uint8_t i;

	if (!dao_respawn_enabled) return;

	for (i = 0; i < DAOJU_NUM; i++)
	{
		if (dao_respawn_at[i] == 0) continue;
		if (TIM_GetTick() < dao_respawn_at[i]) continue;

		if (Daoju_SpawnRandom(i)) dao_respawn_at[i] = 0;
	}
}
