#ifndef _EEPROM_H_
#define _EEPROM_H_

#include "stm32f10x.h"                  // Device header

#define AT24C02_ADDR_WRITE  0xA0
#define AT24C02_ADDR_READ   0xA1
#define AT24C02_PAGE_SIZE   8       
#define AT24C02_WRITE_DELAY 5       

void AT24C02_Init(void);
uint8_t AT24C02_WriteByte(uint8_t WordAddress, uint8_t Data);
uint8_t AT24C02_ReadByte(uint8_t WordAddress);
uint8_t AT24C02_WriteBuffer(uint8_t WordAddress, uint8_t *pBuffer, uint16_t NumToWrite);
uint8_t AT24C02_ReadBuffer(uint8_t WordAddress, uint8_t *pBuffer, uint16_t NumToRead);

#endif