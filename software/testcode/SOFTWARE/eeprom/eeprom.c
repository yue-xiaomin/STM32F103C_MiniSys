#include "eeprom.h"
#include "soft_i2c.h"
#include "delay.h"



// 等待EEPROM写入完成
void EEPROM_WaitWriteComplete(void) {
    uint16_t timeout = 1000; // 超时计数器
    
    do {
        I2C_Start();
        I2C_SendByte(0xA0); // 发送写地址
        if(I2C_Wait_Ack()) {
            I2C_Stop();
            break; // 收到ACK，写入完成
        }
        I2C_Stop();
        
        timeout--;
        if(timeout == 0) break;
        delay_us(100);
    } while(1);
}

// 写入多个字节（简化版）
uint8_t AT24C02_Write_NBytes(uint16_t addr, uint8_t *buf, uint16_t num) {
    uint16_t i;
    
    if(addr + num > 256) {
        return 1; // 地址超出范围
    }
    
    for(i = 0; i < num; i++) {
        I2C_Start();
        I2C_SendByte(0xA0); // 器件地址 + 写
        if(!I2C_Wait_Ack()) {
            I2C_Stop();
            return 2; // 器件无应答
        }
        
        I2C_SendByte(addr + i); // 内存地址
        if(!I2C_Wait_Ack()) {
            I2C_Stop();
            return 3; // 地址无应答
        }
        
        I2C_SendByte(buf[i]); // 数据
        if(!I2C_Wait_Ack()) {
            I2C_Stop();
            return 4; // 数据无应答
        }
        
        I2C_Stop();
        
        // 等待写入完成
        EEPROM_WaitWriteComplete();
    }
    
    return 0; // 成功
}

// 读取多个字节（简化版）
uint8_t AT24C02_Read_NBytes(uint16_t addr, uint8_t *buf, uint16_t num) {
    uint16_t i;
    
    if(addr + num > 256) {
        return 1; // 地址超出范围
    }
    
    // 发送起始地址
    I2C_Start();
    I2C_SendByte(0xA0); // 器件地址 + 写
    if(!I2C_Wait_Ack()) {
        I2C_Stop();
        return 2; // 器件无应答
    }
    
    I2C_SendByte(addr); // 内存地址
    if(!I2C_Wait_Ack()) {
        I2C_Stop();
        return 3; // 地址无应答
    }
    
    // 重新启动为读模式
    I2C_Start();
    I2C_SendByte(0xA1); // 器件地址 + 读
    if(!I2C_Wait_Ack()) {
        I2C_Stop();
        return 4; // 切换读模式失败
    }
    
    // 连续读取数据
    for(i = 0; i < num; i++) {
        if(i == num - 1) {
            buf[i] = I2C_ReadByte(0); // 最后一个字节，发送NACK
        } else {
            buf[i] = I2C_ReadByte(1); // 发送ACK继续读取
        }
    }
    
    I2C_Stop();
    
    return 0; // 成功
}

// 测试函数
uint8_t AT24C02_Test(void) {
    uint8_t write_data[4] = {0x55, 0xAA, 0x12, 0x34};
    uint8_t read_data[4] = {0};
    uint8_t result;
    
    // 写入测试数据
    result = AT24C02_Write_NBytes(0x00, write_data, 4);
    if(result != 0) return result;
    
    delay_ms(10);
    
    // 读取数据
    result = AT24C02_Read_NBytes(0x00, read_data, 4);
    if(result != 0) return result + 10;
    
    // 验证数据
    for(uint8_t i = 0; i < 4; i++) {
        if(read_data[i] != write_data[i]) {
            return 20 + i; // 数据不匹配
        }
    }
    
    return 0; // 测试成功
}

