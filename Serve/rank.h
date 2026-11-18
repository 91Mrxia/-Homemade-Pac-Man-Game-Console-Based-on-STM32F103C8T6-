#ifndef _RANK_H_
#define _RANK_H_

#include "stm32f10x.h"

#define RANK_MAX       5
#define RANK_NAME_LEN  8

typedef struct {
    uint32_t score;
    char     name[RANK_NAME_LEN];
} Rank_t;

extern Rank_t ranks[RANK_MAX];

void    Rank_Init(void);                               
void    Rank_Save(void);                               
int8_t  Rank_Insert(uint32_t score, const char *name); 
void    Rank_Clear(void);                              

#endif
