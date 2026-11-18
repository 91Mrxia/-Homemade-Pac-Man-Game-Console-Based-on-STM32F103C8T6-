#include "control.h"
#include "Time.h"

#define CONTROL_LONG_MS 700u

void Control_Init(void)
{
	Key_Init();
#if CONTROL_USE_JOYSTICK
	Yaogan_Init();
#endif
#if CONTROL_USE_TOUCH
	Touch_Init();
#endif
}

uint8_t Control_GetLongPress(void)
{
	static uint8_t held = 0, fired = 0;
	static uint32_t start_ms = 0;
	uint8_t now = Control_Scan();
	if (now == 0) { held = 0; fired = 0; return 0; }
	if (now != held) { held = now; fired = 0; start_ms = TIM_GetTick(); return 0; }
	if (!fired && (TIM_GetTick() - start_ms) >= CONTROL_LONG_MS) { fired = 1; return now; }
	return 0;
}


uint8_t Control_Scan(void)
{
	uint8_t keyNum = Key_GetNum();
	uint8_t touchNum = 0;
	uint8_t ygNum = 0;

#if CONTROL_USE_TOUCH
	touchNum = Touch_GetNum();
#endif
#if CONTROL_USE_JOYSTICK
	ygNum = Yaogan_GetNum();
#endif
	
	if (keyNum != 0) return keyNum;
	if (touchNum != 0) return touchNum;
	return ygNum;
}


uint8_t Control_GetKey(void)
{
	return Key_GetNum();
}

uint8_t Control_GetJoystick(void)
{
#if CONTROL_USE_JOYSTICK
	return Yaogan_GetNum();
#else
	return 0;
#endif
}


uint8_t Control_GetPress(void)
{
	static uint8_t last = 0;
	uint8_t now = Control_Scan();
	uint8_t ret = 0;

	if (now != 0 && last == 0) ret = now;
	last = now;
	return ret;
}
