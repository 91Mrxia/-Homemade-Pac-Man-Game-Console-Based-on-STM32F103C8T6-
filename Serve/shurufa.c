#include "shurufa.h"
#include <string.h>

#define NAME_LEN   8

static const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";
#define CHARSET_N  (sizeof(charset) - 1)

char g_name[9] = "PLAYER1";


static uint8_t s_cursor = 0;

//作用：查找字符在字符集中的索引
static uint8_t FindIndex(char c)
{
	uint8_t i;
	for (i = 0; i < CHARSET_N; i++)
	{
		if (charset[i] == c) return i;
	}
	return 0;
}

//作用：初始化输入法
void Shurufa_Init(void)
{
	s_cursor = 0;
	/* 若名字为空则给个默认值 */
	if (g_name[0] == '\0') strcpy(g_name, "PLAYER1");
}

//作用：设置名字
void Shurufa_SetName(const char *name)
{
	uint8_t i;
	for (i = 0; i < NAME_LEN; i++)
	{
		g_name[i] = (name && name[i]) ? name[i] : ' ';
	}
	g_name[NAME_LEN] = '\0';
}

//作用：获取当前光标位置
uint8_t Shurufa_GetCursor(void)
{
	return s_cursor;
}

const char *Shurufa_GetCharset(void)
{
	return charset;
}

uint8_t Shurufa_GetCharsetLen(void)
{
	return CHARSET_N;
}

//作用：获取当前光标位置的字符
/* dir: 1上 2下 3左 4右(与方向码一致) */
uint8_t Shurufa_Handle(uint8_t dir)
{
	uint8_t idx;

	/* 确保名字长度足够(补齐空格) */
	{
		uint8_t i;
		for (i = 0; i < NAME_LEN; i++)
			if (g_name[i] == '\0') g_name[i] = ' ';
		g_name[NAME_LEN] = '\0';
	}

	if (dir == 1)   /* 上: 上一个字符 */
	{
		idx = FindIndex(g_name[s_cursor]);
		idx = (uint8_t)((idx + CHARSET_N - 1) % CHARSET_N);
		g_name[s_cursor] = charset[idx];
	}
	else if (dir == 2)   /* 下: 下一个字符 */
	{
		idx = FindIndex(g_name[s_cursor]);
		idx = (uint8_t)((idx + 1) % CHARSET_N);
		g_name[s_cursor] = charset[idx];
	}
	else if (dir == 4)   /* 右: 确认当前位 */
	{
		if (s_cursor >= NAME_LEN - 1)
		{
			/* 最后一位确认 = 完成 */
			return SHURUFA_DONE;
		}
		s_cursor++;
	}
	else if (dir == 3)   /* 左: 返回/前移 */
	{
		if (s_cursor == 0)
		{
			return SHURUFA_CANCEL;
		}
		s_cursor--;
	}

	return SHURUFA_STAY;
}
