#include "rank.h"
#include "eeprom.h"
#include <string.h>


#define RANK_MAGIC      0xA5
#define RANK_MAGIC_ADDR 0
#define RANK_DATA_ADDR  1
#define RANK_REC_SIZE   (RANK_NAME_LEN + 4)


Rank_t ranks[RANK_MAX];


static void Rank_Load(void)
{
	uint8_t magic;
	uint8_t buf[RANK_MAX * RANK_REC_SIZE];
	uint8_t i;

	magic = AT24C02_ReadByte(RANK_MAGIC_ADDR);
	if (magic != RANK_MAGIC)
	{
		memset(ranks, 0, sizeof(ranks));
		Rank_Save();
		return;
	}

	AT24C02_ReadBuffer(RANK_DATA_ADDR, buf, sizeof(buf));
	for (i = 0; i < RANK_MAX; i++)
	{
		memcpy(ranks[i].name, &buf[i * RANK_REC_SIZE], RANK_NAME_LEN);
		ranks[i].score = (uint32_t)buf[i * RANK_REC_SIZE + 8]
		               | ((uint32_t)buf[i * RANK_REC_SIZE + 9]  << 8)
		               | ((uint32_t)buf[i * RANK_REC_SIZE + 10] << 16)
		               | ((uint32_t)buf[i * RANK_REC_SIZE + 11] << 24);
	}
}


void Rank_Save(void)
{
	uint8_t buf[RANK_MAX * RANK_REC_SIZE];
	uint8_t i;

	AT24C02_WriteByte(RANK_MAGIC_ADDR, RANK_MAGIC);
	for (i = 0; i < RANK_MAX; i++)
	{
		memcpy(&buf[i * RANK_REC_SIZE], ranks[i].name, RANK_NAME_LEN);
		buf[i * RANK_REC_SIZE + 8]  = (uint8_t)(ranks[i].score & 0xFF);
		buf[i * RANK_REC_SIZE + 9]  = (uint8_t)((ranks[i].score >> 8)  & 0xFF);
		buf[i * RANK_REC_SIZE + 10] = (uint8_t)((ranks[i].score >> 16) & 0xFF);
		buf[i * RANK_REC_SIZE + 11] = (uint8_t)((ranks[i].score >> 24) & 0xFF);
	}
	AT24C02_WriteBuffer(RANK_DATA_ADDR, buf, sizeof(buf));
}


void Rank_Init(void)
{
	Rank_Load();
}


int8_t Rank_Insert(uint32_t score, const char *name)
{
	int8_t pos = -1, i;

	if (score == 0) return -1;

	for (i = RANK_MAX - 1; i >= 0; i--)
	{
		if (score > ranks[i].score)
		{
			pos = i;
		}
	}
	if (pos < 0) return -1;

	for (i = RANK_MAX - 1; i > pos; i--)
	{
		ranks[i] = ranks[i - 1];
	}
	ranks[pos].score = score;
	memset(ranks[pos].name, 0, RANK_NAME_LEN);
	if (name)
	{
		for (i = 0; i < RANK_NAME_LEN && name[i]; i++)
			ranks[pos].name[i] = name[i];
	}
	Rank_Save();
	return pos;
}


void Rank_Clear(void)
{
	memset(ranks, 0, sizeof(ranks));
	Rank_Save();
}
