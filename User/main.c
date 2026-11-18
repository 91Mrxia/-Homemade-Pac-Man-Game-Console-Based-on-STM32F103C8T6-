#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "bsp_lcd.h"
#include "lcd_show.h"
#include "control.h"
#include "map.h"
#include "ren.h"
#include "youling.h"
#include "dou.h"
#include "daoju.h"
#include "rank.h"
#include "shurufa.h"
#include "music.h"
#include "motor.h"
#include "eeprom.h"
#include <stdlib.h>

typedef enum {
	ST_TITLE = 0,
	ST_MENU,
	ST_NAME,
	ST_RANK,
	ST_SETTING,
	ST_PLAY,
	ST_OVER,
} GameState;



#define POWER_DEC_MS  100
#define GAME_MODE_NORMAL 0
#define GAME_MODE_2P 1

static GameState g_state  = ST_TITLE;
static uint8_t    g_menu  = 0;
static uint8_t    g_setting = 0;
static uint8_t    g_dark_mode = 1;
static uint8_t    g_mode = 0;   /* 菜单光标 */

static uint32_t last_ren_ms, last_yl_ms, last_power_ms;


static void Game_StartNew(uint8_t two_player)
{
	srand((unsigned)g_tick_ms);
	GZ_Reset();
	Ren_Init();
	YouLing_Init();
	Daoju_SetCount(two_player ? 1 : DAOJU_NUM);
	Daoju_Init();
	Dou_Reset();

	if (two_player) GZ_ClearBeans();

	g_score = 0;
	g_power = 0;
	g_mode = two_player ? GAME_MODE_2P : GAME_MODE_NORMAL;
	Ren_SetScoreEnabled(two_player ? 0 : 1);
	Daoju_SetRespawnEnabled(two_player ? 1 : 0);
	if (two_player) ren.lifes = 1;
	Youling_SetPlayerMode(two_player ? 1 : 0);
	UI_SetTwoPlayerMode(two_player ? 1 : 0);

	last_ren_ms = last_yl_ms = last_power_ms = g_tick_ms;

	Music_Stop();
	Music_PlayGame();

	g_state = ST_PLAY;
	Game_InitScreen();
}


static void LoseLife(void)
{
	Motor_Vibrate(150);
	ren.lifes--;

	if (ren.lifes == 0)
	{
		int8_t r;
		Music_Stop();
		r = (g_mode == GAME_MODE_2P) ? -1 : Rank_Insert(g_score, g_name);
		g_state = ST_OVER;
		if (g_mode == GAME_MODE_2P) UI_ShowTwoPlayerGameOver(0);
		else UI_ShowGameOver(r);
	}
	else
	{
		/* 重开本回合: 豆子铺满, 角色回出生点, 分数/生命保留 */
		GZ_Reset();
		Dou_Reset();
		Daoju_Reset();
		Ren_ResetP();
		YouLing_ResetPositions();
		g_power = 0;
		Game_InitScreen();

		delay_ms(1200);   /* 死亡停顿 */

		last_ren_ms = last_yl_ms = last_power_ms = g_tick_ms;
		}
}


static void CheckCollision(void)
{
	uint8_t i;
	if (g_mode == GAME_MODE_2P)
	{
		Youling_t *g = YouLing_Get(0);
		if (g && g->youling_gz_x == ren.ren_gz_x && g->youling_gz_y == ren.ren_gz_y)
		{
			uint8_t pacman_won = (g->state == YL_ST_FRIGHTENED);
			if (pacman_won) g->state = YL_ST_EATEN;
			Motor_Vibrate(150);
			Music_Stop();
			g_state = ST_OVER;
			UI_ShowTwoPlayerGameOver(pacman_won);
		}
		return;
	}
	for (i = 0; i < YOULING_NUM; i++)
	{
		Youling_t *g = YouLing_Get(i);
		if (!g) continue;
		if (g->youling_gz_x != ren.ren_gz_x || g->youling_gz_y != ren.ren_gz_y) continue;
		if (g->state == YL_ST_EATEN) continue;   /* 回巢中的幽灵不碰 */

		if (g_power > 0)
		{
			g->state = YL_ST_EATEN;   /* 吃掉幽灵 */
			g_score += 100;
			Music_PlayKeySound();
		}
		else
		{
			LoseLife();
			return;
		}
	}
}


static void Menu_Activate(void)
{
	if (g_menu == 0)
	{
		Shurufa_SetName("        ");
		Shurufa_Init();
		g_state = ST_NAME;
		UI_ShowName();
	}
	else if (g_menu == 1)
	{
		Music_PlayKeySound();
		Game_StartNew(1);
	}
	else if (g_menu == 2)
	{
		g_state = ST_RANK;
		UI_ShowRank();
	}
	else
	{
		g_setting = 0;
		g_state = ST_SETTING;
		UI_ShowSetting(g_setting);
	}
}

static void Title_Handle(void)
{
	uint8_t k = Control_GetPress();
	if (k == 1)
	{
		g_menu = (g_menu == 0) ? 3 : g_menu - 1;
		Music_PlayKeySound();
		UI_SetTitleSelection(g_menu);
		UI_ShowTitle();
	}
	else if (k == 2)
	{
		g_menu = (g_menu == 3) ? 0 : g_menu + 1;
		Music_PlayKeySound();
		UI_SetTitleSelection(g_menu);
		UI_ShowTitle();
	}
	else if (k == 4)
	{
		Music_PlayKeySound();
		Music_Stop();
		Menu_Activate();
	}
}


static void Setting_Handle(void)
{
	uint8_t k = Control_GetPress();
	if (k == 1 || k == 2)
	{
		g_setting = (g_setting == 0) ? 1 : 0;
		Music_PlayKeySound();
		UI_ShowSetting(g_setting);
	}
	else if (k == 4)
	{
		Music_PlayKeySound();
		if (g_setting == 0)
		{
			g_dark_mode = (g_dark_mode == 0) ? 1 : 0;
			LCD_ColorMode(g_dark_mode ? LCD_DARK_MODE : LCD_LIGHT_MODE);
			UI_SetDarkMode(g_dark_mode);
		}
		else
		{
			Music_SetEnabled(!Music_IsEnabled());
		}
		UI_ShowSetting(g_setting);
	}
	else if (k == 3)
	{
		Music_PlayKeySound();
		g_state = ST_MENU;
		UI_ShowMenu(g_menu);
	}
}

static void Menu_Handle(void)
{
	uint8_t k = Control_GetPress();

	if (k == 1)
	{
		g_menu = (g_menu == 0) ? 3 : g_menu - 1;
		Music_PlayKeySound();
		UI_ShowMenu(g_menu);
	}
	else if (k == 2)
	{
		g_menu = (g_menu == 3) ? 0 : g_menu + 1;
		Music_PlayKeySound();
		UI_ShowMenu(g_menu);
	}
	else if (k == 4)
	{
		Music_PlayKeySound();
		Menu_Activate();
	}
	else if (k == 3)
	{
		g_state = ST_TITLE;
		Music_Stop();
		Music_PlayOpening();
		UI_ShowTitle();
	}
}


static void Name_Handle(void)
{
	uint8_t k = Control_GetPress();
	uint8_t long_k = Control_GetLongPress();
	uint8_t r;

	if (long_k == 4)
	{
		Game_StartNew(0);
		return;
	}
	if (k == 0) return;
	if (k == 3)
	{
		Music_PlayKeySound();
		g_state = ST_MENU;
		UI_ShowMenu(g_menu);
		return;
	}
	r = Shurufa_Handle(k);
	Music_PlayKeySound();
	if (r == SHURUFA_DONE)
	{
		/* ?????????????????????????????????????????????????????????????? */
		UI_ShowName();
	}
	else
	{
		UI_ShowName();
	}
}


static void Rank_Handle(void)
{
	uint8_t k = Control_GetPress();
	if (k == 4 || k == 3)
	{
		Music_PlayKeySound();
		g_state = ST_MENU;
		UI_ShowMenu(g_menu);
	}
}


static void Over_Handle(void)
{
	uint8_t k = Control_GetPress();
	if (k == 4)
	{
		g_state = ST_MENU;
		Music_PlayIdle();
		UI_ShowMenu(g_menu);
	}
}


static void Play_Handle(void)
{
	uint32_t now = g_tick_ms;

	Daoju_Update();

	/* 能量倒计时 */
	if (g_power > 0 && (now - last_power_ms) >= POWER_DEC_MS)
	{
		g_power--;
		last_power_ms = now;
	}

	/* 吃豆人移动 */
	if ((now - last_ren_ms) >= REN_MOVE_MS)
	{
		if (g_mode == GAME_MODE_2P) Ren_MoveInput(Control_GetJoystick());
		else Ren_Move();
		last_ren_ms = now;
	}

	/* 幽灵移动 */
	if ((now - last_yl_ms) >= YL_MOVE_MS)
	{
		if (g_mode == GAME_MODE_2P) Youling_MoveInput(0, Control_GetKey());
		else Youling_All_Update();
		last_yl_ms = now;
	}
	if (g_mode == GAME_MODE_2P) Youling_UpdatePlayerState();

	/* 碰撞检测(每轮都查, 与移动节拍无关) */
	CheckCollision();

	/* 增量刷新画面 */
	Game_StepScreen();
}


int main(void)
{
	delay_init();          /* SysTick 延时 */
	delay_ms(200);         /* 上电稳定 */

	AT24C02_Init();        /* I2C + EEPROM */
	Control_Init();        /* 按键/摇杆/触摸 */
	Motor_Init();
	ST7789_Init();         /* LCD */
	Music_Init();          /* 蜂鸣器 + TIM2 1ms 时钟/乐曲 */
	Rank_Init();           /* 排行榜 */
	Shurufa_Init();        /* 用户名 */

	GZ_Init();
	Ren_Init();
	YouLing_Init();
	Daoju_Init();
	Dou_Reset();

	g_state = ST_TITLE;
	LCD_ColorMode(LCD_DARK_MODE);
	UI_SetDarkMode(g_dark_mode);
	UI_SetTitleSelection(g_menu);
	UI_ShowTitle();
	Music_PlayOpening();

	while (1)
	{
		switch (g_state)
		{
			case ST_TITLE: Title_Handle(); break;
			case ST_MENU:  Menu_Handle();  break;
			case ST_NAME:  Name_Handle();  break;
			case ST_RANK:  Rank_Handle();  break;
			case ST_SETTING: Setting_Handle(); break;
			case ST_PLAY:  Play_Handle();  break;
			case ST_OVER:  Over_Handle();  break;
		}
	}
}
