#include "ren.h"
#include "daoju.h"
#include "dou.h"


static const int8_t r_dir_x[5] = { 0,  0, 0, -1, 1 };
static const int8_t r_dir_y[5] = { 0, -1, 1,  0, 0 };


Ren_t ren;
static uint8_t ren_score_enabled = 1;


void Ren_Init(void)
{
	GZ_Point_XZ(REN_HOME_X, REN_HOME_Y, &ren.ren_gz_x, &ren.ren_gz_y);
	ren.dir      = REN_DIR_NONE;
	ren.is_run   = 0;
	ren.lifes    = REN_LIFE_MAX;
	ren_score_enabled = 1;
}


void Ren_ResetP(void)
{
	GZ_Point_XZ(REN_HOME_X, REN_HOME_Y, &ren.ren_gz_x, &ren.ren_gz_y);
	ren.dir      = REN_DIR_NONE;
	ren.is_run   = 0;
}


static uint8_t Ren_IsAbleGo(int x, int y)
{
	if (x >= 0 && x < GZ_X_MAX && y >= 0 && y < GZ_Y_MAX
	    && GZ_GetFlag(x, y) != GZ_WALL_FLAG)
	{
		return 1;
	}
	return 0;
}


static uint8_t Ren_TryMove(uint8_t d)
{
	int x, y;

	if (d == REN_DIR_NONE) return 0;

	x = ren.ren_gz_x + r_dir_x[d];
	y = ren.ren_gz_y + r_dir_y[d];
	if (!Ren_IsAbleGo(x, y)) return 0;

	ren.ren_gz_x = (uint8_t)x;
	ren.ren_gz_y = (uint8_t)y;
	ren.dir      = d;
	ren.is_run   = 1;
	return 1;
}


static void Ren_Eat(void)
{
	GZ_FLAG f = GZ_GetFlag(ren.ren_gz_x, ren.ren_gz_y);

	if (f == GZ_DOU_FLAG)
	{
		if (ren_score_enabled) g_score += 10;
		GZ_SetFlag(ren.ren_gz_x, ren.ren_gz_y, GZ_ROAD_FLAG);
		Dou_Eaten(ren.ren_gz_x, ren.ren_gz_y);
	}
	else if (f == GZ_DAOJU_FLAG)
	{
		if (ren_score_enabled) g_score += 50;
		GZ_SetFlag(ren.ren_gz_x, ren.ren_gz_y, GZ_ROAD_FLAG);
		Daoju_MarkEaten(ren.ren_gz_x, ren.ren_gz_y);
	}
}


void Ren_SetScoreEnabled(uint8_t enabled)
{
	ren_score_enabled = enabled ? 1 : 0;
}


void Ren_MoveInput(uint8_t d)
{
	if (d == 0) return;
	if (Ren_TryMove(d)) Ren_Eat();
}


void Ren_Move(void)
{
	Ren_MoveInput(Control_Scan());
}


