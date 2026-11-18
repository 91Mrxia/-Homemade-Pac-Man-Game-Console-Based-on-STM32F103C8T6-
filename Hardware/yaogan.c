#include "yaogan.h"
#define YAOGAN_CENTER   2048
#define YAOGAN_SIQU     120
#define YAOGAN_CENTER_MIN  1024
#define YAOGAN_CENTER_MAX  3072
#define YAOGAN_BOOT_SPAN   120

static uint16_t x_center = YAOGAN_CENTER;
static uint16_t y_center = YAOGAN_CENTER;
static uint8_t joystick_ready = 0;

uint16_t Get_ADC_Value(uint8_t channel);

void Yaogan_Init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	RCC_ADCCLKConfig(RCC_PCLK2_Div6);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	ADC_InitTypeDef ADC_InitStructure;
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_Cmd(ADC1, ENABLE);

	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1) == SET);
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1) == SET);
	{
		uint32_t sx = 0, sy = 0;
		uint16_t x_min = 4095, x_max = 0;
		uint16_t y_min = 4095, y_max = 0;
		uint16_t x, y;
		uint8_t i;
		for (i = 0; i < 16; i++)
		{
			x = Get_ADC_Value(ADC_Channel_0);
			y = Get_ADC_Value(ADC_Channel_1);
			sx += x;
			sy += y;
			if (x < x_min) x_min = x;
			if (x > x_max) x_max = x;
			if (y < y_min) y_min = y;
			if (y > y_max) y_max = y;
		}
		x_center = (uint16_t)(sx / 16);
		y_center = (uint16_t)(sy / 16);
		joystick_ready = (x_center >= YAOGAN_CENTER_MIN && x_center <= YAOGAN_CENTER_MAX
		               && y_center >= YAOGAN_CENTER_MIN && y_center <= YAOGAN_CENTER_MAX
		               && (x_max - x_min) <= YAOGAN_BOOT_SPAN
		               && (y_max - y_min) <= YAOGAN_BOOT_SPAN);
	}
}

uint16_t Get_ADC_Value(uint8_t channel)
{
    ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_55Cycles5);

    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    {
        uint16_t _t = 0;
        while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC))
            if (++_t > 0xFFFF) break;   /* timeout guard */
    }

    return ADC_GetConversionValue(ADC1);
}


void Yaogan_GetDebug(uint16_t *x_raw, uint16_t *y_raw)
{
	if (x_raw) *x_raw = Get_ADC_Value(ADC_Channel_0);
	if (y_raw) *y_raw = Get_ADC_Value(ADC_Channel_1);
}



uint8_t Yaogan_GetNum(void)
{
    int16_t x_offset, y_offset;
    uint16_t ax, ay;
    uint16_t x, y;

    if (!joystick_ready) return 0;
    x = (Get_ADC_Value(ADC_Channel_0) + Get_ADC_Value(ADC_Channel_0)) / 2;
    y = (Get_ADC_Value(ADC_Channel_1) + Get_ADC_Value(ADC_Channel_1)) / 2;
    x_offset = (int16_t)x - (int16_t)x_center;
    y_offset = (int16_t)y - (int16_t)y_center;
    ax = (x_offset >= 0) ? x_offset : -x_offset;
    ay = (y_offset >= 0) ? y_offset : -y_offset;

    if (ax <= YAOGAN_SIQU && ay <= YAOGAN_SIQU) return 0;

    if (ax > ay) return (x_offset > 0) ? 2 : 1;
    return (y_offset > 0) ? 3 : 4;
}
