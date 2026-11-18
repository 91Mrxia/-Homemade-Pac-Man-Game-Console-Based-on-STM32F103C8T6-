#include "map.h"

uint32_t g_score = 0;
uint8_t  g_power = 0;

const GZ_FLAG gz_flags[GZ_X_MAX][GZ_Y_MAX] = {
	{0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,4,4,4,4,4,4,4,4,4,4,4,0},
	{0,4,0,0,0,4,0,4,0,0,0,4,0},
	{0,5,0,0,0,4,0,4,0,0,0,5,0},
	{0,4,4,4,4,4,4,4,4,4,4,4,0},
	{0,4,0,0,0,4,0,0,0,4,0,4,0},
	{0,4,4,4,4,4,2,4,4,4,4,4,0},
	{0,4,0,0,0,4,0,0,0,4,0,4,0},
	{0,4,4,4,4,4,3,4,4,4,4,4,0},
	{0,4,0,0,0,4,0,4,0,0,0,4,0},
	{0,4,4,4,4,3,3,4,4,4,4,4,0},
	{0,0,0,4,0,4,0,4,0,4,0,0,0},
	{0,0,0,4,0,4,4,4,0,4,0,0,0},
};


GZ_t gzs[GZ_X_MAX][GZ_Y_MAX];


void GZ_Init(void)
{
	GZ_Reset();
}



void GZ_Reset(void)
{
	int x, y;
	for(x = 0; x < GZ_X_MAX; x++)
	{
		for(y = 0; y < GZ_Y_MAX; y++)
		{
			if (gz_flags[x][y] == GZ_WALL_FLAG)
			{
				gzs[x][y].flag = GZ_WALL_FLAG;
			}
			else
			{
				gzs[x][y].flag = GZ_DOU_FLAG;
			}
		}
	}
}


static void GZ_SSZ_XZ(int x, int y, int *s_x, int *s_y)
{
	*s_x = y;
	*s_y = GZ_Y_MAX - 1 - x;
}


void GZ_Point_XZ(uint8_t s_x, uint8_t s_y, uint8_t *d_x, uint8_t *d_y)
{
	if (d_x) *d_x = (uint8_t)(GZ_X_MAX - 1 - s_y);
	if (d_y) *d_y = s_x;
}


void GZ_SetFlag(int x, int y, GZ_FLAG flag)
{
	int sx, sy;
	if (x < 0 || x >= GZ_X_MAX || y < 0 || y >= GZ_Y_MAX) return;
	GZ_SSZ_XZ(x, y, &sx, &sy);
    gzs[sx][sy].flag = flag;
}


GZ_FLAG GZ_GetFlag(int x, int y)
{
	int sx, sy;
	if (x < 0 || x >= GZ_X_MAX || y < 0 || y >= GZ_Y_MAX) return GZ_WALL_FLAG;
	GZ_SSZ_XZ(x, y, &sx, &sy);
    return gzs[sx][sy].flag;
}







void GZ_ClearBeans(void)
{
	int x, y;
	for (x = 0; x < GZ_X_MAX; x++)
	{
		for (y = 0; y < GZ_Y_MAX; y++)
		{
			if (gzs[x][y].flag == GZ_DOU_FLAG)
			{
				gzs[x][y].flag = GZ_ROAD_FLAG;
			}
		}
	}
}
