#include "eeprom.h"
#include "i2c.h"
#include "delay.h"

void AT24C02_Init(void)
{
    MyI2C_Init();
}


uint8_t AT24C02_WriteByte(uint8_t WordAddress, uint8_t Data)
{
    MyI2C_Start();
    
    MyI2C_SendByte(AT24C02_ADDR_WRITE);
    if (MyI2C_ReceiveAck() != 0) { MyI2C_Stop(); return 1; }
    
    MyI2C_SendByte(WordAddress);
    if (MyI2C_ReceiveAck() != 0) { MyI2C_Stop(); return 1; }
    
    MyI2C_SendByte(Data);
    if (MyI2C_ReceiveAck() != 0) { MyI2C_Stop(); return 1; }
    
    MyI2C_Stop();
    
    Delay_ms(AT24C02_WRITE_DELAY); 
    
    return 0;
}

uint8_t AT24C02_ReadByte(uint8_t WordAddress)
{
    uint8_t Data;
    
    MyI2C_Start();
    MyI2C_SendByte(AT24C02_ADDR_WRITE);
    MyI2C_ReceiveAck();
    MyI2C_SendByte(WordAddress);
    MyI2C_ReceiveAck();
    
    MyI2C_Start();
    
    MyI2C_SendByte(AT24C02_ADDR_READ);
    MyI2C_ReceiveAck();
    
    Data = MyI2C_ReceiveByte();
    
    MyI2C_SendAck(1); 
    
    MyI2C_Stop();
    
    return Data;
}


uint8_t AT24C02_WriteBuffer(uint8_t WordAddress, uint8_t *pbuf, uint16_t NumToWrite)
{
    uint16_t i = 0;
    
    while (i < NumToWrite)
    {
        uint8_t page_remain = AT24C02_PAGE_SIZE - (WordAddress % AT24C02_PAGE_SIZE);

        uint8_t write_len = (NumToWrite - i < page_remain) ? (NumToWrite - i) : page_remain;
        
        MyI2C_Start();
        MyI2C_SendByte(AT24C02_ADDR_WRITE);
        if (MyI2C_ReceiveAck() != 0) { MyI2C_Stop(); return 1; }
        
        MyI2C_SendByte(WordAddress);
        if (MyI2C_ReceiveAck() != 0) { MyI2C_Stop(); return 1; }
        
        for (uint8_t j = 0; j < write_len; j++)
        {
            MyI2C_SendByte(pbuf[i + j]);
            if (MyI2C_ReceiveAck() != 0) { MyI2C_Stop(); return 1; }
        }
        
        MyI2C_Stop();
        
        Delay_ms(AT24C02_WRITE_DELAY);
        
        i += write_len;
        WordAddress += write_len;
    }
    
    return 0;
}

uint8_t AT24C02_ReadBuffer(uint8_t WordAddress, uint8_t *pbuf, uint16_t NumToRead)
{
    uint16_t i;
    
    MyI2C_Start();
    MyI2C_SendByte(AT24C02_ADDR_WRITE);
    if (MyI2C_ReceiveAck() != 0) { MyI2C_Stop(); return 1; }
    
    MyI2C_SendByte(WordAddress);
    if (MyI2C_ReceiveAck() != 0) { MyI2C_Stop(); return 1; }
    
    MyI2C_Start();
    MyI2C_SendByte(AT24C02_ADDR_READ);
    if (MyI2C_ReceiveAck() != 0) { MyI2C_Stop(); return 1; }
    
    for (i = 0; i < NumToRead; i++)
    {
        pbuf[i] = MyI2C_ReceiveByte();
        if (i == NumToRead - 1)
            MyI2C_SendAck(1); 
        else
            MyI2C_SendAck(0);
    }
    
    MyI2C_Stop();
    return 0;
}














