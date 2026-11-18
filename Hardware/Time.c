#include "TIME.h"
#include "music.h"

uint32_t TIM_GetTick(void)
{
	return g_tick_ms;
}
