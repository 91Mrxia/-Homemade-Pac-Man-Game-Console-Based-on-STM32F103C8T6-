#include "youling.h"
#include "map.h"
#include "ren.h"
#include <string.h>
#include <stdlib.h>

Youling_t youlingA, youlingB, youlingC;

static Youling_t* youlings[YOULING_NUM];
static uint8_t youling_player_mode = 0;

static const uint8_t yl_home_x[YOULING_NUM] = { 8, 10, 10 };
static const uint8_t yl_home_y[YOULING_NUM] = { 6,  6,  5 };
static const uint8_t yl_wait[YOULING_NUM]   = { 0,  6, 12 };

static const uint8_t yl_self[YOULING_NUM]   = { YL_SELF_CHASE, YL_SELF_CHASE, YL_SELF_RANDOM };

static const int8_t  yl_dir_dx[5] = { 0,  0, 0, -1, 1 };
static const int8_t  yl_dir_dy[5] = { 0, -1, 1,  0, 0 };
static const uint8_t fan_dir[5]   = { 0, 2, 1, 4, 3 };


static uint8_t Youling_IsAbleGo(int x, int y)
{
	if (x >= 0 && x < GZ_X_MAX && y >= 0 && y < GZ_Y_MAX
	    && GZ_GetFlag(x, y) != GZ_WALL_FLAG)
	{
		return 1;
	}
	return 0;
}


static uint8_t Youling_AutoTrackGetDir(int yx, int yy, int rx, int ry)
{
	static uint8_t q[GZ_X_MAX * GZ_Y_MAX];
	static uint8_t dir[GZ_X_MAX * GZ_Y_MAX];//被访问的标记+第一步方向

	int head = 0, tail = 0, i;
	int start, target;

	if (yx == rx && yy == ry) return YL_DIR_NONE;

	start  = yy * GZ_X_MAX + yx;
	target = ry * GZ_X_MAX + rx;

	memset(dir, 0xFF, sizeof(dir));
	dir[start] = 0;
	q[tail++] = (uint8_t)start;

	while (head < tail)
	{
		int c = q[head++];
		int cx = c % GZ_X_MAX;
		int cy = c / GZ_X_MAX;

		for (i = YL_DIR_UP; i <= YL_DIR_RIGHT; i++)
		{
			int nx = cx + yl_dir_dx[i];
			int ny = cy + yl_dir_dy[i];
			int n;
			if (!Youling_IsAbleGo(nx, ny)) continue;
			n = ny * GZ_X_MAX + nx;
			if (dir[n] != 0xFF) continue;
			dir[n] = (c == start) ? (uint8_t)i : dir[c];
			if (n == target) return dir[n];
			q[tail++] = (uint8_t)n;
		}
	}
	return YL_DIR_NONE;
}

static uint8_t Youling_RandomDir(Youling_t *yl)
{
	uint8_t dirs[4];
	uint8_t n = 0, d;

	for (d = YL_DIR_UP; d <= YL_DIR_RIGHT; d++)
	{
		int nx = yl->youling_gz_x + yl_dir_dx[d];
		int ny = yl->youling_gz_y + yl_dir_dy[d];
		if (!Youling_IsAbleGo(nx, ny)) continue;
		if (d == fan_dir[yl->dir]) continue;   
		dirs[n++] = d;
	}

	if (n == 0)   
	{
		for (d = YL_DIR_UP; d <= YL_DIR_RIGHT; d++)
		{
			int nx = yl->youling_gz_x + yl_dir_dx[d];
			int ny = yl->youling_gz_y + yl_dir_dy[d];
			if (Youling_IsAbleGo(nx, ny)) dirs[n++] = d;
		}
	}

	if (n == 0) return YL_DIR_NONE;
	return dirs[rand() % n];
}



static uint8_t Youling_FairDir(Youling_t *yl, Ren_t *r)
{
	int bx = yl->youling_gz_x, by = yl->youling_gz_y;
	int best = YL_DIR_NONE, bestdist = -1;
	uint8_t d;

	for (d = YL_DIR_UP; d <= YL_DIR_RIGHT; d++)
	{
		int nx = bx + yl_dir_dx[d], ny = by + yl_dir_dy[d];
		int dist;
		if (!Youling_IsAbleGo(nx, ny)) continue;
		if (d == fan_dir[yl->dir]) continue;   /* 尽量不掉头 */
		dist = (nx > r->ren_gz_x ? nx - r->ren_gz_x : r->ren_gz_x - nx)
		     + (ny > r->ren_gz_y ? ny - r->ren_gz_y : r->ren_gz_y - ny);
		if (dist > bestdist) { bestdist = dist; best = d; }
	}

	if (best == YL_DIR_NONE)   /* 都堵死/都需掉头: 随便走一个可走方向 */
	{
		for (d = YL_DIR_UP; d <= YL_DIR_RIGHT; d++)
		{
			if (Youling_IsAbleGo(bx + yl_dir_dx[d], by + yl_dir_dy[d])) { best = d; break; }
		}
	}
	return (uint8_t)best;
}


static void Youling_MoveOneTap(Youling_t *yl, uint8_t d)
{
	if (d == YL_DIR_NONE) { yl->dir = YL_DIR_NONE; return; }
	yl->dir = d;
	yl->youling_gz_x = (uint8_t)(yl->youling_gz_x + yl_dir_dx[d]);
	yl->youling_gz_y = (uint8_t)(yl->youling_gz_y + yl_dir_dy[d]);
}


static void Youling_Update(Youling_t *yl, Ren_t *r)
{
	uint8_t d;

	if (!yl->is_run)
	{
		if (yl->wait > 0) yl->wait--;
		else yl->is_run = 1;
		return;
	}

	/* 被吃: 回巢 */
	if (yl->state == YL_ST_EATEN)
	{
		d = Youling_AutoTrackGetDir(yl->youling_gz_x, yl->youling_gz_y,
		                            yl->home_x, yl->home_y);
		Youling_MoveOneTap(yl, d);
		if (yl->youling_gz_x == yl->home_x && yl->youling_gz_y == yl->home_y)
			yl->state = YL_ST_NORMAL;
		return;
	}

	/* 能量模式 -> 惊吓逃跑(无论个性) */
	if (g_power > 0)
	{
		yl->state = YL_ST_FRIGHTENED;
		d = Youling_FairDir(yl, r);
		Youling_MoveOneTap(yl, d);
		return;
	}

	/* 正常: 按个性选择 追踪 / 随机 */
	yl->state = YL_ST_NORMAL;
	if (yl->self == YL_SELF_RANDOM)
	{
		d = Youling_RandomDir(yl);
	}
	else
	{
		d = Youling_AutoTrackGetDir(yl->youling_gz_x, yl->youling_gz_y,
		                            r->ren_gz_x, r->ren_gz_y);
	}
	Youling_MoveOneTap(yl, d);
}


void Youling_SetPlayerMode(uint8_t enabled)
{
	youling_player_mode = enabled ? 1 : 0;
}

void Youling_UpdatePlayerState(void)
{
	Youling_t *g;
	if (!youling_player_mode) return;
	g = youlings[0];
	if (g->state != YL_ST_EATEN)
		g->state = (g_power > 0) ? YL_ST_FRIGHTENED : YL_ST_NORMAL;
}

void Youling_MoveInput(uint8_t index, uint8_t d)
{

	Youling_t *g;
	int nx, ny;
	if (!youling_player_mode || index >= YOULING_NUM || d == YL_DIR_NONE) return;
	g = youlings[index];
	nx = g->youling_gz_x + yl_dir_dx[d];
	ny = g->youling_gz_y + yl_dir_dy[d];
	if (!Youling_IsAbleGo(nx, ny)) return;
	g->youling_gz_x = (uint8_t)nx;
	g->youling_gz_y = (uint8_t)ny;
	g->dir = d;
	g->is_run = 1;
}

void Youling_All_Update(void)
{
	uint8_t i;
	for (i = 0; i < YOULING_NUM; i++)
	{
		Youling_Update(youlings[i], &ren);
	}
}


Youling_t* YouLing_Get(uint8_t i)
{
	if (i >= YOULING_NUM) return 0;
	return youlings[i];
}


void YouLing_Init(void)
{
	uint8_t i;
	youlings[0] = &youlingA;
	youlings[1] = &youlingB;
	youlings[2] = &youlingC;

	for (i = 0; i < YOULING_NUM; i++)
	{
		Youling_t* g = youlings[i];
		GZ_Point_XZ(yl_home_x[i], yl_home_y[i], &g->youling_gz_x, &g->youling_gz_y);
		GZ_Point_XZ(yl_home_x[i], yl_home_y[i], &g->home_x, &g->home_y);
		g->is_run = 0;
		g->wait = yl_wait[i];
		g->dir = YL_DIR_NONE;
		g->color = i;
		g->self  = yl_self[i];
		g->state = YL_ST_NORMAL;
	}
}


void YouLing_ResetPositions(void)
{
	uint8_t i;
	for (i = 0; i < YOULING_NUM; i++)
	{
		Youling_t* g = youlings[i];
		GZ_Point_XZ(yl_home_x[i], yl_home_y[i], &g->youling_gz_x, &g->youling_gz_y);
		GZ_Point_XZ(yl_home_x[i], yl_home_y[i], &g->home_x, &g->home_y);
		g->is_run = 0;
		g->wait = yl_wait[i];
		g->dir = YL_DIR_NONE;
		g->state = YL_ST_NORMAL;
	}
}
