#ifndef _CONTROL_H
#define _CONTROL_H

#include "stm32f10x.h"                  // Device header
#include "key.h"
#include "yaogan.h"
#include "touch.h"


#define CONTROL_USE_TOUCH     1
#define CONTROL_USE_JOYSTICK  1


void Control_Init(void);
uint8_t Control_Scan(void);
uint8_t Control_GetKey(void);
uint8_t Control_GetJoystick(void);     
uint8_t Control_GetPress(void);
uint8_t Control_GetLongPress(void);


#endif
