#include "lcd_show.h"
#include "ren.h"
#include "youling.h"
#include "dou.h"
#include "daoju.h"
#include "rank.h"
#include "shurufa.h"
#include "TIME.h"
#include "yaogan.h"
#include "music.h"
#include <string.h>

/* ---------------- 配色 ---------------- */
#define COL_BG        BLACK
#define COL_WALL      BLUE
#define COL_BEAN      YELLOW
#define COL_POWER     YELLOW
#define COL_PACMAN    YELLOW
#define COL_TEXT      WHITE
#define COL_SEL       YELLOW
#define COL_FRIGHTEN  0x041F   /* 惊吓幽灵的深蓝 */

#define SPR_R         6        /* 角色精灵半径(像素) */

static uint8_t ui_dark_mode = 0;
static uint8_t ui_title_sel = 0;
static uint8_t ui_two_player = 0;

static const uint16_t ghost_colors[YOULING_NUM] = {
	RED, MAGENTA, CYAN
};

/* ---------------- 字符串工具 ---------------- */

static void U32ToStr(uint32_t v, char *s)
{
	char tmp[11];
	int i = 0, j = 0;

	if (v == 0) { s[0] = '0'; s[1] = '\0'; return; }
	while (v) { tmp[i++] = (char)('0' + (v % 10)); v /= 10; }
	while (i > 0) s[j++] = tmp[--i];
	s[j] = '\0';
}

/* ---------------- 静态格绘制 ---------------- */

static void CellDrawStatic(uint8_t x, uint8_t y)
{
	uint16_t px = CELL_X1(x);
	uint16_t py = CELL_Y1(y);
	GZ_FLAG f = GZ_GetFlag(x, y);

	if (f == GZ_WALL_FLAG)
	{
		LCD_Fill(px, py, CELL_X3(x), CELL_Y3(y), COL_WALL);
		return;
	}

	/* 通道: 清成背景 */
	LCD_Fill(px, py, CELL_X3(x), CELL_Y3(y), COL_BG);

	if (f == GZ_DOU_FLAG)
	{
		LCD_FillCircle(CELL_CX(x), CELL_CY(y), 3, COL_BEAN);
	}
	else if (f == GZ_DAOJU_FLAG)
	{
		LCD_FillCircle(CELL_CX(x), CELL_CY(y), 5, COL_POWER);
	}
}


/* 角色绘制: 中心像素坐标 (cx,cy) */
static void PacmanDraw(uint16_t cx, uint16_t cy, uint8_t dir)
{
	int dx = 0, dy = 0;

	LCD_FillCircle(cx, cy, 6, COL_PACMAN);

	switch (dir)
	{
		case REN_DIR_UP:    dy = -1; break;
		case REN_DIR_DOWN:  dy =  1; break;
		case REN_DIR_LEFT:  dx = -1; break;
		case REN_DIR_RIGHT: dx =  1; break;
	}
	if (dx || dy)
	{
		/* 用黑色圆咬一口, 形成张嘴 */
		LCD_FillCircle((uint16_t)(cx + dx * 3), (uint16_t)(cy + dy * 3), 4, COL_BG);
	}
}


static void GhostDraw(uint16_t cx, uint16_t cy, uint8_t colorIdx, uint8_t state)
{
	uint16_t body;

	if (state == YL_ST_EATEN) return;   /* 被吃回巢时不显示身体 */

	body = (state == YL_ST_FRIGHTENED) ? COL_FRIGHTEN : ghost_colors[colorIdx];

	LCD_FillCircle(cx, cy, 6, body);
	LCD_FillCircle(cx - 2, cy - 2, 2, WHITE);
	LCD_FillCircle(cx + 2, cy - 2, 2, WHITE);
	if (state != YL_ST_FRIGHTENED)
	{
		LCD_FillCircle(cx - 2, cy - 2, 1, BLACK);
		LCD_FillCircle(cx + 2, cy - 2, 1, BLACK);
	}
}


/* ---------------- 平滑移动(像素插值) ---------------- */

typedef struct {
	uint8_t  cur_x, cur_y;   /* 当前格子 */
	uint8_t  prv_x, prv_y;   /* 上一格(插值起点) */
	uint32_t ms;             /* 本格移动开始时间 */
	uint16_t rpx, rpy;       /* 上一帧渲染的像素中心 */
} Anim_t;

static Anim_t anim[1 + YOULING_NUM];   /* 0=吃豆人, 1..3=幽灵 */
static uint8_t anim_init = 0;
static uint8_t yl_prev_state[YOULING_NUM];
static uint8_t dirty[GZ_X_MAX][GZ_Y_MAX];
static uint16_t g_ren_cx, g_ren_cy;
static uint16_t g_yl_cx[YOULING_NUM], g_yl_cy[YOULING_NUM];
static uint32_t last_render_ms = 0;


/* 在 (f,t) 两格中心之间按时间线性插值; useX=1 取 X, 否则取 Y */
static uint16_t LerpCenter(uint8_t f, uint8_t t, uint32_t ms, uint32_t now, uint32_t dur, uint8_t useX)
{
	int a, b, k;
	uint32_t el = now - ms;

	if (el >= dur) k = 255;
	else           k = (int)(el * 255 / dur);

	if (useX) { a = CELL_CX(f); b = CELL_CX(t); }
	else      { a = CELL_CY(f); b = CELL_CY(t); }

	return (uint16_t)(a + (b - a) * k / 255);
}


static uint8_t SpriteOverlap(uint16_t cx, uint16_t cy, uint16_t r, uint8_t x, uint8_t y)
{
	int sx1 = (int)cx - r, sx2 = (int)cx + r;
	int sy1 = (int)cy - r, sy2 = (int)cy + r;
	int cx1 = CELL_X1(x), cx2 = CELL_X3(x);
	int cy1 = CELL_Y1(y), cy2 = CELL_Y3(y);

	if (sx2 < cx1 || sx1 > cx2) return 0;
	if (sy2 < cy1 || sy1 > cy2) return 0;
	return 1;
}


static void MarkOverlap(uint16_t cx, uint16_t cy, uint16_t r)
{
	int left, right, top, bottom;
	int x, y;
	left = ((int)cx - (int)r - MAP_X0) / DW;
	right = ((int)cx + (int)r - MAP_X0) / DW;
	top = ((int)cy - (int)r - MAP_Y0) / DW;
	bottom = ((int)cy + (int)r - MAP_Y0) / DW;
	if (left < 0) left = 0;
	if (top < 0) top = 0;
	if (right >= GZ_X_MAX) right = GZ_X_MAX - 1;
	if (bottom >= GZ_Y_MAX) bottom = GZ_Y_MAX - 1;
	for (y = top; y <= bottom; y++)
		for (x = left; x <= right; x++)
			dirty[x][y] = 1;
}


static void CellRedraw(uint8_t x, uint8_t y)
{
	uint8_t i;

	CellDrawStatic(x, y);

	if (SpriteOverlap(g_ren_cx, g_ren_cy, SPR_R, x, y))
		PacmanDraw(g_ren_cx, g_ren_cy, ren.dir);

	for (i = 0; i < YOULING_NUM; i++)
	{
		Youling_t *g = YouLing_Get(i);
		if (ui_two_player && i > 0) continue;
		if (g->state == YL_ST_EATEN) continue;
		if (SpriteOverlap(g_yl_cx[i], g_yl_cy[i], SPR_R, x, y))
			GhostDraw(g_yl_cx[i], g_yl_cy[i], g->color, g->state);
	}
}


/* ---------------- HUD ---------------- */

static uint32_t hud_score = 0xFFFFFFFF;
static uint16_t hud_fps   = 0xFFFF;
static uint8_t  hud_lives = 0xFF;
static uint16_t fps_frames = 0;
static uint32_t fps_last_ms = 0;
static uint16_t fps_value = 0;

static void HUD_DrawScore(void)
{
	char s[16], buf[24];
	uint8_t i, n;
	U32ToStr(g_score, s);
	buf[0] = 'S'; buf[1] = 'C'; buf[2] = 'O'; buf[3] = 'R'; buf[4] = 'E'; buf[5] = ':';
	n = 6;
	for (i = 0; s[i]; i++) buf[n++] = s[i];
	buf[n] = '\0';
	LCD_ShowString(4, 4, (const u8 *)buf, COL_TEXT, COL_BG, 16);
}

static void HUD_DrawFps(void)
{
	char s[8], buf[16];
	uint8_t i, n;
	U32ToStr(fps_value, s);
	buf[0] = 'F'; buf[1] = 'P'; buf[2] = 'S'; buf[3] = ':';
	n = 4;
	for (i = 0; s[i]; i++) buf[n++] = s[i];
	buf[n] = '\0';
	LCD_ShowString(160, 4, (const u8 *)buf, COL_TEXT, COL_BG, 16);
}

static void HUD_DrawLives(void)
{
	uint8_t i;
	for (i = 0; i < REN_LIFE_MAX; i++)
	{
		uint16_t cx = 8 + i * 20;
		if (i < ren.lifes)
			LCD_FillCircle(cx, 33, 5, COL_PACMAN);
		else
			LCD_FillCircle(cx, 33, 5, COL_BG);
	}
	LCD_ShowString(76, 26, (const u8 *)"LIFE", COL_TEXT, COL_BG, 16);
}


static void HUD_Update(void)
{
	if (!ui_two_player && g_score != hud_score)
	{
		hud_score = g_score;
		HUD_DrawScore();
	}
	else if (ui_two_player && hud_score != 0xFFFFFFFE)
	{
		hud_score = 0xFFFFFFFE;
		LCD_Fill(4, 4, 110, 20, COL_BG);
		LCD_ShowString(4, 4, (const u8 *)"Gay Mode", COL_TEXT, COL_BG, 16);
	}
	if (fps_value != hud_fps)
	{
		hud_fps = fps_value;
		HUD_DrawFps();
	}
	if (ren.lifes != hud_lives)
	{
		hud_lives = ren.lifes;
		HUD_DrawLives();
	}
}

void UI_DrawJoystickDebug(void)
{
	char buf[32], num[8];
	uint16_t xr, yr;
	uint8_t d, n, i;

	Yaogan_GetDebug(&xr, &yr);
	d = Yaogan_GetNum();

	n = 0;
	buf[n++] = 'X'; buf[n++] = ':';
	U32ToStr(xr, num); for (i = 0; num[i]; i++) buf[n++] = num[i];
	buf[n++] = ' '; buf[n++] = 'Y'; buf[n++] = ':';
	U32ToStr(yr, num); for (i = 0; num[i]; i++) buf[n++] = num[i];
	buf[n++] = ' '; buf[n++] = 'D'; buf[n++] = ':';
	buf[n++] = (char)('0' + d);
	buf[n] = 0;

	LCD_Fill(4, 210, 235, 226, COL_BG);
	LCD_ShowString(4, 210, (const u8 *)buf, COL_TEXT, COL_BG, 16);
}


void UI_SetTitleSelection(uint8_t sel)
{
	ui_title_sel = (sel < 4) ? sel : 0;
}

void UI_SetTwoPlayerMode(uint8_t enabled)
{
	ui_two_player = enabled ? 1 : 0;
}

void UI_ShowTitle(void)
{
	static const char *items[4] = { "START", "Gay Mode", "RANK", "SETTING" };
	uint8_t i;
	LCD_Clear(COL_BG);
	PacmanDraw(120, 38, REN_DIR_RIGHT);
	LCD_ShowString(72, 62, (const u8 *)"PAC-MAN", COL_TEXT, COL_BG, 24);
	for (i = 0; i < 4; i++)
	{
		uint16_t y = 95 + i * 30;
		uint16_t fc = (i == ui_title_sel) ? COL_SEL : COL_TEXT;
		if (i == ui_title_sel) LCD_ShowString(42, y, (const u8 *)">", COL_SEL, COL_BG, 16);
		LCD_ShowString(62, y, (const u8 *)items[i], fc, COL_BG, 16);
	}
	LCD_ShowString(22, 225, (const u8 *)"UP/DOWN:MOVE RIGHT:OK", COL_TEXT, COL_BG, 12);
}


void UI_ShowMenu(uint8_t sel)
{
	static const char *items[4] = { "START", "Gay Mode", "RANK", "SETTING" };
	uint8_t i;

	LCD_Clear(COL_BG);
	LCD_ShowString(88, 20, (const u8 *)"MENU", COL_TEXT, COL_BG, 24);
	for (i = 0; i < 4; i++)
	{
		uint16_t y = 75 + i * 38;
		uint16_t fc = (i == sel) ? COL_SEL : COL_TEXT;
		if (i == sel) LCD_ShowString(40, y, (const u8 *)">", COL_SEL, COL_BG, 16);
		LCD_ShowString(60, y, (const u8 *)items[i], fc, COL_BG, 16);
	}
	LCD_ShowString(20, 250, (const u8 *)"UP/DOWN:MOVE", COL_TEXT, COL_BG, 12);
	LCD_ShowString(20, 264, (const u8 *)"RIGHT:OK LEFT:BACK", COL_TEXT, COL_BG, 12);
}

void UI_SetDarkMode(uint8_t dark)
{
	ui_dark_mode = dark ? 1 : 0;
}

void UI_ShowSetting(uint8_t sel)
{
	static const char *items[2] = { "COLOR MODE", "MUSIC" };
	uint8_t i;
	LCD_Clear(COL_BG);
	LCD_ShowString(72, 20, (const u8 *)"SETTING", COL_TEXT, COL_BG, 24);
	for (i = 0; i < 2; i++)
	{
		uint16_t y = 90 + i * 48;
		uint16_t fc = (i == sel) ? COL_SEL : COL_TEXT;
		if (i == sel) LCD_ShowString(28, y, (const u8 *)">", COL_SEL, COL_BG, 16);
		LCD_ShowString(48, y, (const u8 *)items[i], fc, COL_BG, 16);
	}
	LCD_ShowString(150, 90, (const u8 *)(ui_dark_mode ? "DARK" : "LIGHT"), COL_TEXT, COL_BG, 12);
	LCD_ShowString(150, 138, (const u8 *)(Music_IsEnabled() ? "ON" : "OFF"), COL_TEXT, COL_BG, 12);
	LCD_ShowString(48, 178, (const u8 *)"RIGHT:TOGGLE", COL_TEXT, COL_BG, 12);
	LCD_ShowString(20, 250, (const u8 *)"LEFT:BACK", COL_TEXT, COL_BG, 12);
}


void UI_ShowName(void)
{
	uint8_t i, cur, n, row, col;
	const char *chars = Shurufa_GetCharset();
	uint8_t chars_len = Shurufa_GetCharsetLen();
	uint16_t x0 = 24;

	LCD_Clear(COL_BG);
	LCD_ShowString(84, 8, (const u8 *)"NAME", COL_TEXT, COL_BG, 24);
	LCD_DrawLine(18, 54, 222, 54, COL_TEXT);
	for (i = 0; i < 8; i++)
	{
		char ch = g_name[i] ? g_name[i] : ' ';
		LCD_ShowChar((uint16_t)(x0 + i * 24), 28, (u8)ch, COL_TEXT, COL_BG, 16);
	}
	cur = Shurufa_GetCursor();
	LCD_DrawRectangle((uint16_t)(x0 + cur * 24 - 2), 26, (uint16_t)(x0 + cur * 24 + 14), 45, COL_SEL);

	for (n = 0; n < chars_len; n++)
	{
		row = n / 13; col = n % 13;
		LCD_ShowChar((uint16_t)(18 + col * 16), (uint16_t)(78 + row * 20), (u8)chars[n],
		             (chars[n] == g_name[cur]) ? COL_SEL : COL_TEXT, COL_BG, 16);
	}
	LCD_ShowString(18, 188, (const u8 *)"UP/DOWN:CHAR", COL_TEXT, COL_BG, 12);
	LCD_ShowString(18, 208, (const u8 *)"RIGHT:NEXT", COL_TEXT, COL_BG, 12);
	LCD_ShowString(18, 228, (const u8 *)"LONG RIGHT:START", COL_TEXT, COL_BG, 12);
	LCD_ShowString(18, 248, (const u8 *)"LEFT:BACK", COL_TEXT, COL_BG, 12);
}


void UI_ShowRank(void)
{
	uint8_t i;

	LCD_Clear(COL_BG);
	LCD_ShowString(84, 10, (const u8 *)"RANK", COL_TEXT, COL_BG, 24);

	for (i = 0; i < RANK_MAX; i++)
	{
		char line[32];
		char sc[12];
		uint8_t n = 0, j;
		uint16_t y = 40 + i * 28;

		line[n++] = (char)('1' + i);
		line[n++] = '.';
		for (j = 0; j < RANK_NAME_LEN; j++)
		{
			char ch = ranks[i].name[j];
			line[n++] = (ch >= ' ' && ch <= '~') ? ch : ' ';
		}
		line[n++] = ' ';
		U32ToStr(ranks[i].score, sc);
		for (j = 0; sc[j]; j++) line[n++] = sc[j];
		line[n] = '\0';

		LCD_ShowString(20, y, (const u8 *)line, COL_TEXT, COL_BG, 16);
	}

	LCD_ShowString(60, 250, (const u8 *)"RIGHT:BACK", COL_TEXT, COL_BG, 12);
}


void UI_ShowTwoPlayerGameOver(uint8_t pacman_won)
{
	LCD_Clear(COL_BG);
	LCD_ShowString(34, 65, (const u8 *)"Gay Mode Over", COL_TEXT, COL_BG, 24);
	LCD_ShowString(pacman_won ? 48 : 60, 112,
	               (const u8 *)(pacman_won ? "PAC-MAN WINS!" : "GHOST WINS!"),
	               COL_SEL, COL_BG, 16);
	LCD_ShowString(48, 145, (const u8 *)"RIGHT:MENU", COL_TEXT, COL_BG, 16);
}

void UI_ShowGameOver(int8_t rank)
{
	char buf[24];
	char s[12];
	uint8_t i, n;

	LCD_Clear(COL_BG);
	LCD_ShowString(36, 50, (const u8 *)"GAME OVER", COL_TEXT, COL_BG, 32);

	buf[0] = 'S'; buf[1] = 'C'; buf[2] = 'O'; buf[3] = 'R'; buf[4] = 'E'; buf[5] = ':';
	n = 6;
	U32ToStr(g_score, s);
	for (i = 0; s[i]; i++) buf[n++] = s[i];
	buf[n] = '\0';
	LCD_ShowString(72, 130, (const u8 *)buf, COL_TEXT, COL_BG, 16);

	if (rank == 0)
		LCD_ShowString(60, 170, (const u8 *)"NEW RECORD!", COL_SEL, COL_BG, 16);
	else if (rank >= 1)
		LCD_ShowString(72, 170, (const u8 *)"TOP 5!", COL_SEL, COL_BG, 16);

	LCD_ShowString(72, 230, (const u8 *)"RIGHT:MENU", COL_TEXT, COL_BG, 16);
}


/* ---------------- 游戏渲染 ---------------- */

void Game_InitScreen(void)
{
	uint8_t x, y, i;

	LCD_Clear(COL_BG);

	hud_score = 0xFFFFFFFF;
	hud_fps   = 0xFFFF;
	hud_lives = 0xFF;
	fps_frames = 0;
	fps_value = 0;
	fps_last_ms = TIM_GetTick();
	HUD_Update();

	/* 静态格: 墙 + 豆子 + 道具 */
	for (x = 0; x < GZ_X_MAX; x++)
		for (y = 0; y < GZ_Y_MAX; y++)
			CellDrawStatic(x, y);

	/* 吃豆人 */
	anim[0].cur_x = ren.ren_gz_x; anim[0].cur_y = ren.ren_gz_y;
	anim[0].prv_x = ren.ren_gz_x; anim[0].prv_y = ren.ren_gz_y;
	anim[0].ms = TIM_GetTick();
	g_ren_cx = CELL_CX(ren.ren_gz_x);
	g_ren_cy = CELL_CY(ren.ren_gz_y);
	anim[0].rpx = g_ren_cx; anim[0].rpy = g_ren_cy;
	PacmanDraw(g_ren_cx, g_ren_cy, ren.dir);

	/* 幽灵 */
	for (i = 0; i < YOULING_NUM; i++)
	{
		Youling_t *g = YouLing_Get(i);
		Anim_t *a = &anim[1 + i];
		if (ui_two_player && i > 0) continue;
		a->cur_x = g->youling_gz_x; a->cur_y = g->youling_gz_y;
		a->prv_x = g->youling_gz_x; a->prv_y = g->youling_gz_y;
		a->ms = TIM_GetTick();
		g_yl_cx[i] = CELL_CX(g->youling_gz_x);
		g_yl_cy[i] = CELL_CY(g->youling_gz_y);
		a->rpx = g_yl_cx[i]; a->rpy = g_yl_cy[i];
		yl_prev_state[i] = g->state;
		GhostDraw(g_yl_cx[i], g_yl_cy[i], g->color, g->state);
	}

	anim_init = 1;
	last_render_ms = TIM_GetTick();
}


void Game_StepScreen(void)
{
	uint32_t now = TIM_GetTick();
	uint8_t i, need = 0;
	uint8_t x, y;

	if (!anim_init) { Game_InitScreen(); return; }

	/* 帧率节流(~125fps) */
	if (now - last_render_ms < 8) return;
	last_render_ms = now;

	fps_frames++;
	if (now - fps_last_ms >= 1000u)
	{
		uint32_t elapsed = now - fps_last_ms;
		fps_value = (uint16_t)(((uint32_t)fps_frames * 1000u) / elapsed);
		if (fps_value > 999u) fps_value = 999u;
		fps_frames = 0;
		fps_last_ms = now;
	}

	/* 1. 检测格子变化 / 状态变化, 更新动画起点 */
	if (ren.ren_gz_x != anim[0].cur_x || ren.ren_gz_y != anim[0].cur_y)
	{
		anim[0].prv_x = anim[0].cur_x;
		anim[0].prv_y = anim[0].cur_y;
		anim[0].cur_x = ren.ren_gz_x;
		anim[0].cur_y = ren.ren_gz_y;
		anim[0].ms = now;
		need = 1;
	}

	for (i = 0; i < YOULING_NUM; i++)
	{
		Youling_t *g = YouLing_Get(i);
		Anim_t *a = &anim[1 + i];
		if (ui_two_player && i > 0) continue;

		if (g->youling_gz_x != a->cur_x || g->youling_gz_y != a->cur_y)
		{
			a->prv_x = a->cur_x;
			a->prv_y = a->cur_y;
			a->cur_x = g->youling_gz_x;
			a->cur_y = g->youling_gz_y;
			a->ms = now;
			need = 1;
		}
		if (g->state != yl_prev_state[i])
		{
			yl_prev_state[i] = g->state;   /* 颜色/形态变化也要重画 */
			need = 1;
		}
	}

	/* 2. 计算本帧插值后的像素中心 */
	g_ren_cx = LerpCenter(anim[0].prv_x, anim[0].cur_x, anim[0].ms, now, REN_ANIM_MS, 1);
	g_ren_cy = LerpCenter(anim[0].prv_y, anim[0].cur_y, anim[0].ms, now, REN_ANIM_MS, 0);
	if (g_ren_cx != anim[0].rpx || g_ren_cy != anim[0].rpy) need = 1;

	for (i = 0; i < YOULING_NUM; i++)
	{
		Anim_t *a = &anim[1 + i];
		if (ui_two_player && i > 0) continue;
		g_yl_cx[i] = LerpCenter(a->prv_x, a->cur_x, a->ms, now, YL_MOVE_MS, 1);
		g_yl_cy[i] = LerpCenter(a->prv_y, a->cur_y, a->ms, now, YL_MOVE_MS, 0);
		if (g_yl_cx[i] != a->rpx || g_yl_cy[i] != a->rpy) need = 1;
	}

	if (dou_respawned) need = 1;
	if (daoju_respawned) need = 1;

	/* 3. 有变化才重画(旧位置 + 新位置 + 恢复的豆子) */
	if (need)
	{
		memset(dirty, 0, sizeof(dirty));

		MarkOverlap(anim[0].rpx, anim[0].rpy, SPR_R);
		MarkOverlap(g_ren_cx, g_ren_cy, SPR_R);
		for (i = 0; i < YOULING_NUM; i++)
		{
			if (ui_two_player && i > 0) continue;
			MarkOverlap(anim[1 + i].rpx, anim[1 + i].rpy, SPR_R);
			MarkOverlap(g_yl_cx[i], g_yl_cy[i], SPR_R);
		}

		if (dou_respawned)
		{
			dou_respawned = 0;
			dirty[dou_respawn_x][dou_respawn_y] = 1;
		}
		if (daoju_respawned)
		{
			daoju_respawned = 0;
			dirty[daoju_respawn_x][daoju_respawn_y] = 1;
		}

		for (y = 0; y < GZ_Y_MAX; y++)
			for (x = 0; x < GZ_X_MAX; x++)
				if (dirty[x][y]) CellRedraw(x, y);

		anim[0].rpx = g_ren_cx; anim[0].rpy = g_ren_cy;
		for (i = 0; i < YOULING_NUM; i++)
		{
			if (ui_two_player && i > 0) continue;
			anim[1 + i].rpx = g_yl_cx[i];
			anim[1 + i].rpy = g_yl_cy[i];
		}
	}

	/* 4. HUD */
	HUD_Update();
}


