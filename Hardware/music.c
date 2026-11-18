#include "stm32f10x.h"                  // Device header
#include "misc.h"
#include "music.h"
#include "buzzer.h"

#define NOTE_GAP_MS     12u      
#define ARRAY_LEN(a)    (sizeof(a) / sizeof((a)[0]))


//游戏开场音乐
const MusicNote Music_Opening[] = {
	/* ===== 引子(2拍): 上行琶音 do-mi-sol-高音do ===== */
	{ M1, 240 }, /* do */
	{ M3, 240 }, /* mi */
	{ M5, 240 }, /* sol */
	{ H1, 240 }, /* 高音do */
	/* ===== 主题乐句A(8拍): 八分音符为主, 活泼跳跃 ===== */
	{ H1, 240 }, /* 高音do(延续) */
	{ H1, 240 }, /* 高音do(延续) */
	{ M3, 240 }, /* mi */
	{ M5, 240 }, /* sol */
	{ M3, 240 }, /* mi */
	{ M3, 240 }, /* mi */
	{ M1, 240 }, /* do */
	{ H1, 240 }, /* 高音do 八度大跳 */
	{ M2, 240 }, /* re */
	{ M3, 240 }, /* mi */
	{ M2, 240 }, /* re */
	{ M1, 240 }, /* do */
	{ M6, 240 }, /* la */
	{ M7, 240 }, /* si */
	{ H1, 240 }, /* 高音do */
	{ M2, 240 }, /* re */
	/* ===== 对比乐句B(8拍): 附点节奏 + 下行级进 + 属和弦琶音 ===== */
	{ M6, 720 }, /* la(附点) */
	{ M5, 240 }, /* sol */
	{ M4, 720 }, /* fa(附点) */
	{ M3, 240 }, /* mi */
	{ M3, 720 }, /* mi(附点) */
	{ M2, 240 }, /* re */
	{ M5, 240 }, /* sol */
	{ M7, 240 }, /* si */
	{ H2, 240 }, /* 高音re */
	{ M7, 240 }, /* si */
	/* ===== 收束(2拍): 主音二分长音, 干净结束 ===== */
	{ H1, 960 }, /* 高音do 二分长音 */
};

//游戏进行中音乐
const MusicNote Music_Game[] = {
	/* 第1小节 Am: la-la mi-mi 弹跳固定音型, fa# 多利亚大六度 */
	{ M6, 200 },  /* la */
	{ M6, 200 },  /* la */
	{ M3, 200 },  /* mi */
	{ M3, 200 },  /* mi */
	{ M4s, 200 }, /* fa# 多利亚大六度 */
	{ M5, 200 },  /* sol */
	{ H1, 400 },  /* 高音do 小节强调点缀 */
	/* 第2小节 D(多利亚四级): re-re la-la 弹跳, 低音la八度下落点缀 */
	{ M2, 200 },  /* re */
	{ M2, 200 },  /* re */
	{ M6, 200 },  /* la */
	{ L6, 200 },  /* 低音la 点缀 */
	{ M4s, 200 }, /* fa# */
	{ M6, 200 },  /* la */
	{ H2, 400 },  /* 高音re 小节强调点缀 */
	/* 第3小节 Am: mi-mi la-la 反向弹跳, 推向高音mi */
	{ M3, 200 },  /* mi */
	{ M3, 200 },  /* mi */
	{ M6, 200 },  /* la */
	{ M6, 200 },  /* la */
	{ M4s, 200 }, /* fa# */
	{ M5, 200 },  /* sol */
	{ H3, 400 },  /* 高音mi 小节强调点缀 */
	/* 第4小节 v级和弦(自然小调): sol-sol mi-mi si-si 推向属音 */
	{ M5, 200 },  /* sol */
	{ M5, 200 },  /* sol */
	{ M3, 200 },  /* mi */
	{ M3, 200 },  /* mi */
	{ M7, 200 },  /* si */
	{ M7, 200 },  /* si */
	{ H3, 400 },  /* 高音mi 属音收尾, 回主音la无缝循环 */
};

// 待机音乐
const MusicNote Music_Idle[] = {
	{ M1, 750 },   /* do 主音开始 */
	{ M2, 750 },   /* re */
	{ M3, 750 },   /* mi */
	{ M5, 750 },   /* sol */
	{ M6, 750 },   /* la */
	{ M5, 750 },   /* sol */
	{ M3, 750 },   /* mi */
	{ M2, 750 },   /* re */
	{ M1, 750 },   /* do */
	{ H1, 750 },   /* 高音do 八度上跳亮点 */
	{ M6, 750 },   /* la */
	{ M5, 750 },   /* sol */
	{ M3, 375 },   /* mi 八分 */
	{ M5, 375 },   /* sol 八分 */
	{ M6, 750 },   /* la */
	{ M1, 750 },   /* do 回到主音 */
	{ M5, 750 },   /* sol 属音收尾, 循环回主音do */
};

//按键音效
const MusicNote Music_Key[] = {
	{ M5, 75 },   /* sol */
	{ H1, 75 },   /* 高音do */
	{ H3, 75 },   /* 高音mi */
	{ H5, 75 },   /* 高音sol */
};

//播放状态
volatile uint32_t g_tick_ms = 0;   
static volatile const MusicNote *pMelody = 0;	
static volatile uint16_t melodyLen = 0;			
static volatile uint16_t noteIdx   = 0;			
static volatile uint8_t  loopFlag  = 0;			
static volatile uint8_t  playing   = 0;
static volatile uint8_t  soundOn   = 0;			
static volatile uint8_t  musicEnabled = 1;
static volatile uint16_t tickLeft  = 0;			


static void StartMelody(const MusicNote *melody, uint16_t len, uint8_t loop)
{
	if (!musicEnabled) { playing = 0; Buzzer_SetHZ(30); return; }
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
	pMelody   = melody;
	melodyLen = len;
	noteIdx   = 0;
	loopFlag  = loop;
	soundOn   = 0;
	tickLeft  = 1;				//下一个 tick 立即装载第一个音符
	playing   = 1;
	Buzzer_SetHZ(30);			//先静音
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}

/* 装载下一个音符 (在 TIM2 中断里调用) */
static void LoadNextNote(void)
{
	uint16_t n = pMelody[noteIdx].note;
	uint16_t d = pMelody[noteIdx].durMs;
	noteIdx++;
	
	if (n != REST)
	{
		Buzzer_SetHZ(n);		//发声
		soundOn = 1;
		//发声时长 = 时值 - 尾间隙(12ms)
		tickLeft = (d > NOTE_GAP_MS) ? (uint16_t)(d - NOTE_GAP_MS) : d;
	}
	else
	{
		soundOn = 0;
		tickLeft = d;			//休止符不响
	}
}

/* 播放状态机, 由 TIM2 中断每 1ms 调用一次 */
static void Music_Tick(void)
{
	if (!playing) return;
	if (--tickLeft > 0u) return;

	if (soundOn)				//发声结束 -> 静音间隙
	{
		Buzzer_SetHZ(30);
		soundOn  = 0;
		tickLeft = NOTE_GAP_MS;
	}
	else						//间隙结束 -> 下一个音符
	{
		if (noteIdx >= melodyLen)
		{
			if (loopFlag)
			{
				noteIdx = 0;
			}
			else
			{
				playing = 0;
				Buzzer_SetHZ(30);
				return;
			}
		}
		LoadNextNote();
	}
}

/* ==================== 对外接口 ==================== */

void Music_Init(void)
{
    Buzzer_Init();
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_InternalClockConfig(TIM2);
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 999;
    TIM_TimeBaseInitStructure.TIM_Prescaler = SystemCoreClock / 1000000 - 1;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    TIM_Cmd(TIM2, ENABLE);
}

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        g_tick_ms++;
        Music_Tick();
    }
}

void Music_PlayOpening(void)
{
    StartMelody(Music_Opening, ARRAY_LEN(Music_Opening), 0);
}

void Music_PlayGame(void)
{
    StartMelody(Music_Game, ARRAY_LEN(Music_Game), 1);
}

void Music_PlayIdle(void)
{
    StartMelody(Music_Idle, ARRAY_LEN(Music_Idle), 1);
}

void Music_PlayKeySound(void)
{
    StartMelody(Music_Key, ARRAY_LEN(Music_Key), 0);
}

void Music_Stop(void)
{
    playing = 0;
    soundOn = 0;
    tickLeft = 0;
    pMelody = 0;
    Buzzer_SetHZ(30);
}

uint8_t Music_IsPlaying(void)
{
    return playing;
}

void Music_SetEnabled(uint8_t enabled)
{
    musicEnabled = enabled ? 1 : 0;
    if (!musicEnabled) Music_Stop();
}

uint8_t Music_IsEnabled(void)
{
    return musicEnabled;
}