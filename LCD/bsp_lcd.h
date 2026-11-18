#ifndef _BSP_LCD_H_
#define _BSP_LCD_H_

#include "stm32f10x.h"                  // Device header

/* ==================== 颜色定义 ==================== */
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000
#define BLUE           	 0x001F
#define BRED             0XF81F
#define GRED 			       0XFFE0
#define GBLUE			       0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			     0XBC40  //棕色
#define BRRED 			     0XFC07  //棕红色
#define GRAY  			     0X8430  //灰色
#define DARKBLUE      	 0X01CF	//深蓝色
#define LIGHTBLUE      	 0X7D7C	//浅蓝色
#define GRAYBLUE       	 0X5458  //灰蓝色
#define LIGHTGREEN     	 0X841F  //浅绿色
#define LGRAY 			     0XC618  //浅灰色(PANNEL),窗体背景色
#define LGRAYBLUE        0XA651  //浅灰蓝色(中间层颜色)
#define LBBLUE           0X2B12  //浅棕蓝色(选中条目的反色)

/* ==================== 引脚定义 ==================== */
#define SPI_GPIO_RCC   	 RCC_APB2Periph_GPIOA|RCC_APB2Periph_SPI1
#define SPI_GPIO    	   GPIOA
#define SPI_CLK_Port     GPIO_Pin_5  //SCK
#define SPI_MOSI_Port    GPIO_Pin_7  //SDA

#define DC_RCC           RCC_APB2Periph_GPIOA
#define DC_GPIO          GPIOA
#define DC_Pin           GPIO_Pin_3  //DC 数据/命令选择

#define CS_RCC           RCC_APB2Periph_GPIOA
#define CS_GPIO          GPIOA
#define CS_Pin           GPIO_Pin_4  //CS 片选

#define RST_RCC          RCC_APB2Periph_GPIOA
#define RST_GPIO         GPIOA
#define RST_Pin          GPIO_Pin_2  //RST 复位

/* ==================== 屏幕尺寸 ==================== */
#define LCD_X 240
#define LCD_Y 280

/* ST7789V 显存为 240x320,本屏为 240x280,竖向偏移 20 行(可见区居中) */
#define LCD_OFFSET_Y 20

/* ==================== 对外接口函数 ==================== */
void ST7789_Init(void);                                    //初始化液晶
void LCD_Clear(u16 color);                                 //清屏(整屏填充一种颜色)
void LCD_DrawRectangle(u16 x1,u16 y1,u16 x2,u16 y2,u16 color);   //画矩形(空心边框)
void LCD_DrawLine(u16 x1,u16 y1,u16 x2,u16 y2,u16 color);         //画线
/* 深浅色模式 */
#define LCD_LIGHT_MODE 0   //浅色模式(正常)
#define LCD_DARK_MODE  1   //深色模式(反色,白变黑)
void LCD_ColorMode(u8 mode);                                 //切换深浅色模式(整屏反色)
void LCD_ShowChar(u16 x,u16 y,u8 num,u16 fc,u16 bc,u8 sizey);    //显示一个字符  sizey:12/16/24/32
void LCD_ShowString(u16 x,u16 y,const u8 *p,u16 fc,u16 bc,u8 sizey); //显示字符串
void LCD_ShowChinese(u16 x,u16 y,const u8 *s,u16 fc,u16 bc,u8 sizey); //显示汉字  sizey:12/16/24/32


/* fill helpers for game drawing */
void LCD_Fill(u16 x1,u16 y1,u16 x2,u16 y2,u16 color);
void LCD_FillCircle(u16 x,u16 y,u16 r,u16 color);

#endif
