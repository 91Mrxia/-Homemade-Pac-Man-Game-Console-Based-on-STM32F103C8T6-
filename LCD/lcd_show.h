#ifndef _LCD_SHOW_H_
#define _LCD_SHOW_H_

#include "stm32f10x.h"                  // Device header
#include "bsp_lcd.h"
#include "map.h"

/* ==================== ��ͼ����Ļ�ϵ�λ�� / ���ӳߴ� ====================
   ��������ͳһ������ĺ껻��? ���� map ģ�����㡣 */
#define MAP_X0   15
#define MAP_Y0   59
#define DW       16

/* ��Ԫ��(c,r) ���Ͻ�����(�� 1px �߷�) */
#define CELL_X1(c)  (MAP_X0 + (c) * DW + 1)
#define CELL_Y1(r)  (MAP_Y0 + (r) * DW + 1)

/* ��Ԫ��(c,r) ���½����� */
#define CELL_X3(c)  (MAP_X0 + (c + 1) * DW)
#define CELL_Y3(r)  (MAP_Y0 + (r + 1) * DW)

/* ��Ԫ���������� */
#define CELL_CX(c)  ((CELL_X1(c) + CELL_X3(c)) / 2)
#define CELL_CY(r)  ((CELL_Y1(r) + CELL_Y3(r)) / 2)

/* Movement intervals and Pac-Man's shorter visual transition. */
#define REN_MOVE_MS  140
#define REN_ANIM_MS  100
#define YL_MOVE_MS   270

/* ==================== ������Ļ ==================== */
void UI_ShowTitle(void);
void UI_SetTitleSelection(uint8_t sel);
void UI_SetTwoPlayerMode(uint8_t enabled);
void UI_ShowTwoPlayerGameOver(uint8_t pacman_won);
void UI_DrawJoystickDebug(void);
void UI_ShowMenu(uint8_t sel);
void UI_ShowSetting(uint8_t sel);
void UI_SetDarkMode(uint8_t dark);
void UI_ShowName(void);
void UI_ShowRank(void);
void UI_ShowGameOver(int8_t rank);   /* rank: -1 δ����, 0 �ھ�, 1~4 ��ǰ�� */

/* ==================== ��Ϸ����Ⱦ ==================== */
void Game_InitScreen(void);   /* ������Ϸ: �� HUD + ǽ�� + ȫ������ + ��ɫ */
void Game_StepScreen(void);   /* ÿ tick: ����ˢ�½�ɫ / ����β�� / HUD */

#endif
