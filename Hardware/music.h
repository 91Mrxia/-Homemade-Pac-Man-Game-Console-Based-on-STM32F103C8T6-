#ifndef __MUSIC_H__
#define __MUSIC_H__

#include "stm32f10x.h"


#define REST    30u     /* 休止符(乐谱里用, 引擎自动静音) */

/* 低音区 (C4 八度, 简谱低音) */
#define L1   2747u      /* do,  约262Hz */
#define L1s  2596u      /* do#, 约277Hz */
#define L2   2448u      /* re,  约294Hz */
#define L2s  2313u      /* re#, 约311Hz */
#define L3   2181u      /* mi,  约330Hz */
#define L4   2062u      /* fa,  约349Hz */
#define L4s  1945u      /* fa#, 约370Hz */
#define L5   1836u      /* sol, 约392Hz */
#define L5s  1732u      /* sol#,约415Hz */
#define L6   1635u      /* la,  约440Hz */
#define L6s  1543u      /* la#, 约466Hz */
#define L7   1456u      /* si,  约494Hz */

/* 中音区 (C5 八度, 简谱中音) */
#define M1   1376u      /* do,  约523Hz */
#define M1s  1297u      /* do#, 约554Hz */
#define M2   1226u      /* re,  约587Hz */
#define M2s  1156u      /* re#, 约622Hz */
#define M3   1092u      /* mi,  约659Hz */
#define M4   1031u      /* fa,  约698Hz */
#define M4s   971u      /* fa#, 约740Hz */
#define M5    917u      /* sol, 约784Hz */
#define M5s   865u      /* sol#,约831Hz */
#define M6    817u      /* la,  约880Hz */
#define M6s   771u      /* la#, 约932Hz */
#define M7    728u      /* si,  约988Hz */

/* 高音区 (C6 八度, 简谱高音) */
#define H1    687u      /* do,  约1047Hz */
#define H1s   648u      /* do#, 约1109Hz */
#define H2    612u      /* re,  约1175Hz */
#define H2s   577u      /* re#, 约1245Hz */
#define H3    545u      /* mi,  约1319Hz */
#define H4    514u      /* fa,  约1397Hz */
#define H4s   485u      /* fa#, 约1480Hz */
#define H5    458u      /* sol, 约1568Hz */
#define H5s   432u      /* sol#,约1661Hz */
#define H6    408u      /* la,  约1760Hz */
#define H6s   385u      /* la#, 约1865Hz */
#define H7    363u      /* si,  约1976Hz */

typedef struct
{
    uint16_t note;      
    uint16_t durMs;     
} MusicNote;

extern volatile uint32_t g_tick_ms;
void    Music_Init(void);        
void    Music_PlayOpening(void);  
void    Music_PlayGame(void);     
void    Music_PlayIdle(void);     
void    Music_PlayKeySound(void); 
void    Music_Stop(void);         
uint8_t Music_IsPlaying(void);
void    Music_SetEnabled(uint8_t enabled);
uint8_t Music_IsEnabled(void);   

#endif