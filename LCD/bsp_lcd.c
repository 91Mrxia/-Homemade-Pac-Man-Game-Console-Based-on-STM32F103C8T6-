#include "bsp_lcd.h"
#include "lcdfont.h"
#include "delay.h"
#include "stm32f10x.h"                  // Device header


/* 发送缓冲：一行像素，每像素占 2 字节 */
static u8 SendBuff[2 * LCD_X];
static u32 DMA1_MEM_LEN;

static DMA_InitTypeDef DMA_InitStructure;
static SPI_InitTypeDef  SPI_InitStructure;

/* ---------------- 底层：DMA / SPI ---------------- */

//配置 DMA 通道
static void MYDMA_Config(DMA_Channel_TypeDef *DMA_CHx, u32 cpar, u32 cmar, u16 cndtr)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	DMA_DeInit(DMA_CHx);
	DMA1_MEM_LEN = cndtr;
	DMA_InitStructure.DMA_PeripheralBaseAddr = cpar;            //外设地址
	DMA_InitStructure.DMA_MemoryBaseAddr     = cmar;            //存储器地址
	DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralDST;  //从存储器到外设
	DMA_InitStructure.DMA_BufferSize         = cndtr;           //传输数量
	DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable; //外设地址不增
	DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;      //存储器地址自增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority           = DMA_Priority_Medium;
	DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
	DMA_Init(DMA_CHx, &DMA_InitStructure);
}

//使能 DMA 并重新装载计数
static void MYDMA_Enable(DMA_Channel_TypeDef *DMA_CHx)
{
	DMA_Cmd(DMA_CHx, DISABLE);
	DMA_SetCurrDataCounter(DMA_CHx, DMA1_MEM_LEN);
	DMA_Cmd(DMA_CHx, ENABLE);
}

//SPI 收发一个字节
static u8 SPI1_ReadWriteByte(u8 TxData)
{
	u8 retry = 0;
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)  //等待发送缓冲空
	{
		retry++;
		if (retry > 200) return 0;
	}
	SPI_I2S_SendData(SPI1, TxData);

	retry = 0;
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) //等待接收非空
	{
		retry++;
		if (retry > 200) return 0;
	}
	return SPI_I2S_ReceiveData(SPI1);
}

//设置 SPI 速度
static void SPI1_SetSpeed(u8 SpeedSet)
{
	SPI_InitStructure.SPI_BaudRatePrescaler = SpeedSet;
	SPI_Init(SPI1, &SPI_InitStructure);
	SPI_Cmd(SPI1, ENABLE);
}

//初始化 SPI1
static void SPI1_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(SPI_GPIO_RCC, ENABLE);

	GPIO_InitStructure.GPIO_Pin   = SPI_CLK_Port | SPI_MOSI_Port;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SPI_GPIO, &GPIO_InitStructure);
	GPIO_SetBits(SPI_GPIO, SPI_CLK_Port | SPI_MOSI_Port);

	SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL              = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA              = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial     = 7;
	SPI_Init(SPI1, &SPI_InitStructure);
	SPI_Cmd(SPI1, ENABLE);
	SPI1_ReadWriteByte(0xff);
}

//初始化 DC / CS / RST 控制引脚
static void SPI1other_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(DC_RCC, ENABLE);

	//DC 引脚 推挽输出
	GPIO_InitStructure.GPIO_Pin   = DC_Pin;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(DC_GPIO, &GPIO_InitStructure);
	GPIO_SetBits(DC_GPIO, DC_Pin);

	//CS 引脚 推挽输出
	GPIO_InitStructure.GPIO_Pin   = CS_Pin;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(CS_GPIO, &GPIO_InitStructure);

	//RST 引脚 推挽输出
	GPIO_InitStructure.GPIO_Pin   = RST_Pin;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(RST_GPIO, &GPIO_InitStructure);
}

/* ---------------- 底层：写命令 / 写数据 ---------------- */

static void writeData(u8 data)
{
	GPIO_ResetBits(CS_GPIO, CS_Pin);
	GPIO_SetBits(DC_GPIO, DC_Pin);
	SPI1_ReadWriteByte(data);
	GPIO_SetBits(CS_GPIO, CS_Pin);
}

static void writeCommand(u8 cmd)
{
	GPIO_ResetBits(CS_GPIO, CS_Pin);
	GPIO_ResetBits(DC_GPIO, DC_Pin);
	SPI1_ReadWriteByte(cmd);
	GPIO_SetBits(CS_GPIO, CS_Pin);
}

//设置显示窗口范围
static void LCD_Address_Set(u16 x1, u16 y1, u16 x2, u16 y2)
{
	y1 += LCD_OFFSET_Y;
	y2 += LCD_OFFSET_Y;
	writeCommand(0x2A);
	writeData(x1 >> 8);
	writeData(x1);
	writeData(x2 >> 8);
	writeData(x2);
	writeCommand(0x2B);
	writeData(y1 >> 8);
	writeData(y1);
	writeData(y2 >> 8);
	writeData(y2);
	writeCommand(0x2C);
}

//启动一次 DMA 传输(发送一行像素数据)
static void DMA_GO(void)
{
	GPIO_ResetBits(CS_GPIO, CS_Pin);
	GPIO_SetBits(DC_GPIO, DC_Pin);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
	MYDMA_Enable(DMA1_Channel3);
	while (1)
	{
		if (DMA_GetFlagStatus(DMA1_FLAG_TC3) != RESET)
		{
			DMA_ClearFlag(DMA1_FLAG_TC3);
			break;
		}
	}
}

/* ---------------- 初始化 ---------------- */

void ST7789_Init(void)
{
	SPI1other_Init();
	SPI1_Init();
	SPI1_SetSpeed(SPI_BaudRatePrescaler_2);

	//配置 DMA
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	MYDMA_Config(DMA1_Channel3, (u32)&SPI1->DR, (u32)SendBuff, 2 * LCD_X);

	//复位 ST7789
	GPIO_SetBits(RST_GPIO, RST_Pin);
	delay_ms(200);
	GPIO_ResetBits(RST_GPIO, RST_Pin);
	delay_ms(100);
	GPIO_SetBits(RST_GPIO, RST_Pin);
	delay_ms(200);

	//初始化显示方向并配置 ST7789
	writeCommand(0x11);    //退出睡眠模式
	delay_ms(200);
	writeCommand(0x36);    //内存数据访问控制
	writeData(0x00);
	writeCommand(0xB6);    //显示功能控制
	writeData(0x0A);
	writeData(0x82);       //垂直扫描方向
	writeCommand(0x3A);    //256K 色 16bit/像素
	writeData(0x55);
	writeCommand(0xB2);
	writeData(0x0C);
	writeData(0x0C);
	writeData(0x00);
	writeData(0x33);
	writeData(0x33);
	writeCommand(0xB7);
	writeData(0x35);
	writeCommand(0xBB);
	writeData(0x28);
	writeCommand(0xC0);
	writeData(0x2C);
	writeCommand(0xC2);
	writeData(0x01);
	writeCommand(0xC3);
	writeData(0x10);
	writeCommand(0xC4);
	writeData(0x20);
	writeCommand(0xC6);
	writeData(0x0F);
	writeCommand(0xD0);
	writeData(0xA4);
	writeData(0xA1);
	writeCommand(0xE0);
	writeData(0xD0);
	writeData(0x00);
	writeData(0x02);
	writeData(0x07);
	writeData(0x0A);
	writeData(0x28);
	writeData(0x32);
	writeData(0x44);
	writeData(0x42);
	writeData(0x06);
	writeData(0x0E);
	writeData(0x12);
	writeData(0x14);
	writeData(0x17);
	writeCommand(0xE1);
	writeData(0xD0);
	writeData(0x00);
	writeData(0x02);
	writeData(0x07);
	writeData(0x0A);
	writeData(0x28);
	writeData(0x31);
	writeData(0x54);
	writeData(0x47);
	writeData(0x0E);
	writeData(0x1C);
	writeData(0x17);
	writeData(0x1B);
	writeData(0x1E);
	writeCommand(0x21);    // Display Inversion On
	writeCommand(0x29);    //开显示
}

/* ---------------- 清屏 ---------------- */

void LCD_Clear(u16 color)
{
	u16 i, j;
	DMA1_MEM_LEN = 2 * LCD_X;
	LCD_Address_Set(0, 0, LCD_X - 1, LCD_Y - 1);

	for (j = 0; j < 2 * LCD_X; )
	{
		SendBuff[j]     = color >> 8;
		SendBuff[j + 1] = color;
		j += 2;
	}
	for (i = 0; i < LCD_Y; i++)
	{
		DMA_GO();
	}
	GPIO_SetBits(CS_GPIO, CS_Pin);
}

/* ---------------- 深浅色模式切换 ---------------- */

//切换深浅色模式  mode: LCD_LIGHT_MODE=浅色(正常), LCD_DARK_MODE=深色(反色,白变黑)
void LCD_ColorMode(u8 mode)
{
	if (mode == LCD_DARK_MODE)
		writeCommand(0x21);   //显示反转开: 白变黑(深色)
	else
		writeCommand(0x20);   //显示反转关: 黑变白(浅色)
}

/* ---------------- 画点 / 画线 / 画矩形 ---------------- */

//画一个点
static void LCD_DrawPoint(u16 x, u16 y, u16 color)
{
	DMA1_MEM_LEN = 2;
	LCD_Address_Set(x, y, x, y);
	SendBuff[0] = color >> 8;
	SendBuff[1] = color;
	DMA_GO();
	GPIO_SetBits(CS_GPIO, CS_Pin);
}

//画一条水平线
static void LCD_DrawHLine(u16 x1, u16 x2, u16 y, u16 color)
{
	u16 i, w, tmp;
	if (x1 > x2) { tmp = x1; x1 = x2; x2 = tmp; }
	w = x2 - x1 + 1;

	DMA1_MEM_LEN = 2 * w;
	LCD_Address_Set(x1, y, x2, y);
	for (i = 0; i < w; i++)
	{
		SendBuff[2 * i]     = color >> 8;
		SendBuff[2 * i + 1] = color;
	}
	DMA_GO();
	GPIO_SetBits(CS_GPIO, CS_Pin);
}

//画一条垂直线
static void LCD_DrawVLine(u16 x, u16 y1, u16 y2, u16 color)
{
	u16 i, h, tmp;
	if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; }
	h = y2 - y1 + 1;

	DMA1_MEM_LEN = 2;
	LCD_Address_Set(x, y1, x, y2);
	SendBuff[0] = color >> 8;
	SendBuff[1] = color;
	for (i = 0; i < h; i++)
	{
		DMA_GO();
	}
	GPIO_SetBits(CS_GPIO, CS_Pin);
}

//画一条任意直线(Bresenham 算法)
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	int dx, dy, sx, sy, err, e2;
	int x = x1, y = y1;

	if (x1 == x2) { LCD_DrawVLine(x1, y1, y2, color); return; }
	if (y1 == y2) { LCD_DrawHLine(x1, x2, y1, color); return; }

	dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
	dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
	sx = (x1 < x2) ? 1 : -1;
	sy = (y1 < y2) ? 1 : -1;
	err = dx - dy;

	while (1)
	{
		LCD_DrawPoint((u16)x, (u16)y, color);
		if (x == x2 && y == y2) break;
		e2 = 2 * err;
		if (e2 > -dy) { err -= dy; x += sx; }
		if (e2 <  dx) { err += dx; y += sy; }
	}
}

//画空心矩形(四条边,线宽 1 像素)
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	u16 tmp;
	if (x1 > x2) { tmp = x1; x1 = x2; x2 = tmp; }
	if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; }

	LCD_DrawHLine(x1, x2, y1, color);  //上边
	LCD_DrawHLine(x1, x2, y2, color);  //下边
	LCD_DrawVLine(x1, y1, y2, color);  //左边
	LCD_DrawVLine(x2, y1, y2, color);  //右边
}

/* ---------------- 显示字符 ---------------- */

//显示一个 ASCII 字符  sizey:12/16/24/32
void LCD_ShowChar(u16 x, u16 y, u8 num, u16 fc, u16 bc, u8 sizey)
{
	u8 temp, sizex, t, m = 0;
	u16 i, TypefaceNum;   //一个字符点阵数据所占的字节数
	sizex = sizey / 2;
	TypefaceNum = (sizex / 8 + ((sizex % 8) ? 1 : 0)) * sizey;
	DMA1_MEM_LEN = 2 * sizex;

	num = num - ' ';      //得到偏移后的位置
	LCD_Address_Set(x, y, x + sizex - 1, y + sizey - 1);

	for (i = 0; i < TypefaceNum; i++)
	{
		if (sizey == 12)      temp = ascii_1206[num][i];   //6x12
		else if (sizey == 16) temp = ascii_1608[num][i];   //8x16
		else if (sizey == 24) temp = ascii_2412[num][i];   //12x24
		else if (sizey == 32) temp = ascii_3216[num][i];   //16x32
		else return;

		for (t = 0; t < 8; t++)
		{
			if (temp & (0x01 << t))
			{
				SendBuff[2 * m]     = fc >> 8;
				SendBuff[2 * m + 1] = fc;
			}
			else
			{
				SendBuff[2 * m]     = bc >> 8;
				SendBuff[2 * m + 1] = bc;
			}
			m++;
			if (m % sizex == 0)
			{
				DMA_GO();
				m = 0;
				break;
			}
		}
	}
	GPIO_SetBits(CS_GPIO, CS_Pin);
}

/* ---------------- 显示字符串 ---------------- */

void LCD_ShowString(u16 x, u16 y, const u8 *p, u16 fc, u16 bc, u8 sizey)
{
	while (*p != '\0')
	{
		LCD_ShowChar(x, y, *p, fc, bc, sizey);
		x += sizey / 2;
		p++;
	}
}

/* ---------------- 显示汉字 ---------------- */

//按指定字库查找汉字并显示一个汉字(通用)
static void LCD_ShowChineseFont(u16 x, u16 y, const u8 *s, u16 fc, u16 bc, u8 sizey,
                                const u8 *font, u16 count, u16 elemSize)
{
	u8 i, j, m = 0;
	u16 k;
	u16 TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;

	DMA1_MEM_LEN = 2 * sizey;
	LCD_Address_Set(x, y, x + sizey - 1, y + sizey - 1);

	for (k = 0; k < count; k++)
	{
		const u8 *p = font + k * elemSize;
		if (p[0] == s[0] && p[1] == s[1])   //匹配汉字内码
		{
			for (i = 0; i < TypefaceNum; i++)
			{
				u8 byte = p[2 + i];
				for (j = 0; j < 8; j++)
				{
					if (byte & (0x01 << j))
					{
						SendBuff[2 * m]     = fc >> 8;
						SendBuff[2 * m + 1] = fc;
					}
					else
					{
						SendBuff[2 * m]     = bc >> 8;
						SendBuff[2 * m + 1] = bc;
					}
					m++;
					if (m % sizey == 0)
					{
						DMA_GO();
						m = 0;
						break;
					}
				}
			}
			break;
		}
	}
	GPIO_SetBits(CS_GPIO, CS_Pin);
}

//显示一串汉字  sizey:12/16/24/32
void LCD_ShowChinese(u16 x, u16 y, const u8 *s, u16 fc, u16 bc, u8 sizey)
{
	while (*s != 0)
	{
		if (sizey == 12)
			LCD_ShowChineseFont(x, y, s, fc, bc, sizey,
			                    (const u8 *)tfont12, sizeof(tfont12) / sizeof(typFNT_GB12), sizeof(typFNT_GB12));
		else if (sizey == 16)
			LCD_ShowChineseFont(x, y, s, fc, bc, sizey,
			                    (const u8 *)tfont16, sizeof(tfont16) / sizeof(typFNT_GB16), sizeof(typFNT_GB16));
		else if (sizey == 24)
			LCD_ShowChineseFont(x, y, s, fc, bc, sizey,
			                    (const u8 *)tfont24, sizeof(tfont24) / sizeof(typFNT_GB24), sizeof(typFNT_GB24));
		else if (sizey == 32)
			LCD_ShowChineseFont(x, y, s, fc, bc, sizey,
			                    (const u8 *)tfont32, sizeof(tfont32) / sizeof(typFNT_GB32), sizeof(typFNT_GB32));
		else
			return;

		s += 2;
		x += sizey;
	}
}

/* ==================== fill shapes (for game) ==================== */

void LCD_Fill(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	u16 tmp, y;
	if (x1 > x2) { tmp = x1; x1 = x2; x2 = tmp; }
	if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; }
	for (y = y1; y <= y2; y++)
		LCD_DrawHLine(x1, x2, y, color);
}

void LCD_FillCircle(u16 x, u16 y, u16 r, u16 color)
{
	int f = 1 - (int)r;
	int ddF_x = 1;
	int ddF_y = -2 * (int)r;
	int px = 0;
	int py = (int)r;

	LCD_DrawHLine(x - r, x + r, y, color);
	while (px < py)
	{
		if (f >= 0) { py--; ddF_y += 2; f += ddF_y; }
		px++; ddF_x += 2; f += ddF_x;
		LCD_DrawHLine(x - px, x + px, y + py, color);
		LCD_DrawHLine(x - px, x + px, y - py, color);
		LCD_DrawHLine(x - py, x + py, y + px, color);
		LCD_DrawHLine(x - py, x + py, y - px, color);
	}
}
