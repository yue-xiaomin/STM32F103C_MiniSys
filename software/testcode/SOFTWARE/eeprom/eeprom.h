#ifndef __EEPROM_H
#define __EEPROM_H

#include "stm32f10x.h"
#include <stdint.h>

// 函数声明

// I2C底层函数
void SoftI2C_Init(void);
void I2C_Start(void);
void I2C_Stop(void);
uint8_t I2C_Wait_Ack(void);
void I2C_Ack(void);
void I2C_NAck(void);
void I2C_SendByte(uint8_t data);
uint8_t I2C_ReadByte(uint8_t ack);


// EEPROM操作函数

uint8_t AT24C02_Write_NBytes(uint16_t addr, uint8_t *buf, uint16_t num);
uint8_t AT24C02_Read_NBytes(uint16_t addr, uint8_t *buf, uint16_t num);
uint8_t AT24C02_Test(void);



#endif /* __EEPROM_H */

