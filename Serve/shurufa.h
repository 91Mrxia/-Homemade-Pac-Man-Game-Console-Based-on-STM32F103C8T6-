#ifndef _SHURUFA_H_
#define _SHURUFA_H_

#include "stm32f10x.h"

/* 用户名输入(竖向编辑):
   上/下 = 切换当前位的字符
   右   = 确认当前位, 光标后移(最后一位再确认 = 完成保存)
   左   = 光标前移(第 0 位再左 = 取消返回)
*/
#define SHURUFA_STAY   0
#define SHURUFA_DONE   1
#define SHURUFA_CANCEL 2

extern char g_name[9];   /* 用户名, 最多 8 字符, 以 '\0' 结尾 */

void    Shurufa_Init(void);                 /* 载入/复位用户名与光标 */
uint8_t Shurufa_Handle(uint8_t dir);        /* 处理一次按键(方向码 1~4), 返回状态 */
uint8_t Shurufa_GetCursor(void);            /* 当前光标位 0~7 */
void    Shurufa_SetName(const char *name);
const char *Shurufa_GetCharset(void);
uint8_t Shurufa_GetCharsetLen(void);  /* 直接设定用户名 */

#endif
