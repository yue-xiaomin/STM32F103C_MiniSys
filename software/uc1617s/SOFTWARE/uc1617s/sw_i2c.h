#ifndef __SW_I2C_H
#define __SW_I2C_H

#include "delay.h"  //实现delay_ms和delay_us函数
#define i2c_delay_ms    delay_ms
#define i2c_delay_us    delay_us


// 定义I2C引脚参数
#define SW_I2C_PORT        GPIOB
#define SW_I2C_SCL_PIN         GPIO_Pin_6
#define SW_I2C_SDA_PIN         GPIO_Pin_7
#define SW_I2C_RCC_APB2Periph  RCC_APB2Periph_GPIOB

// 引脚操作宏
#define SCL_HIGH()  GPIO_SetBits(SW_I2C_PORT, SW_I2C_SCL_PIN)
#define SCL_LOW()   GPIO_ResetBits(SW_I2C_PORT, SW_I2C_SCL_PIN)
#define SDA_HIGH()  GPIO_SetBits(SW_I2C_PORT, SW_I2C_SDA_PIN)
#define SDA_LOW()   GPIO_ResetBits(SW_I2C_PORT, SW_I2C_SDA_PIN)
#define SDA_READ()  GPIO_ReadInputDataBit(SW_I2C_PORT, SW_I2C_SDA_PIN)
#define SCL_READ()  GPIO_ReadInputDataBit(SW_I2C_PORT, SW_I2C_SCL_PIN)


#define SCAN_I2C_DEVICE


// 初始化软件I2C
void SW_I2C_Init(void);
// I2C起始信号
void SW_I2C_Start(void);
// I2C停止信号
void SW_I2C_Stop(void);
// 等待ACK应答
uint8_t SW_I2C_Wait_Ack(void);
// 发送ACK
void SW_I2C_Ack(void);
// 发送NACK
void SW_I2C_NAck(void);
// 发送一个字节
void SW_I2C_SendByte(uint8_t data);
// 接收一个字节
uint8_t SW_I2C_ReadByte(uint8_t ack);

#ifdef SCAN_I2C_DEVICE
#include "elog.h"   //实现log_i打印函数
#define i2c_output  log_i

// 扫描I2C设备
void SW_I2C_ScanDevices(void);
// 快速扫描（只扫描常见地址）
void SW_I2C_QuickScan(void);
// 高级扫描（测试设备是否应答读写）
void SW_I2C_AdvancedScan(void);
// 测试特定设备地址
uint8_t SW_I2C_TestDevice(uint8_t addr);
// 检查I2C总线状态
void SW_I2C_Bus_Check(void);
// 显示找到的I2C设备信息
void SW_I2C_Scan_ShowDeviceInfo(uint8_t addr);
#endif

#endif /* __SW_I2C_H */
