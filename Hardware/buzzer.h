#ifndef __BUZZER_H__
#define __BUZZER_H__

#include "stm32f10x.h"


void Buzzer_Init(void);
void Buzzer_SetHZ(uint16_t a);	/* a = Ô¤·ÖÆµÖµ(Òô¸ßÂë), a<=30 = ¾²Òô */

#endif