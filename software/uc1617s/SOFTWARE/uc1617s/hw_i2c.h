/**
 * @file    hw_i2c.h
 * @brief   硬件 I2C 通用驱动 (STM32F103 I2C1)
 *
 *  函数命名与 sw_i2c.h 保持一致 (HW_I2C_ 前缀),
 *  可供任何外设 (EEPROM, OLED, 传感器等) 共用.
 *
 *  ★ 已内置 STM32F103 硬件 I2C BUSY 卡死 BUG 规避:
 *    - 所有操作带超时, 绝不死等
 *    - 检测 BUSY 卡死 → 自动复位总线 → 重新初始化
 *    - SWRST + RCC 外设双重复位 (参考 ST 官方 errata)
 */

#ifndef __HW_I2C_H
#define __HW_I2C_H

#include "stm32f10x.h"
#include "stdint.h"

/* ================================================================
 *  硬件 I2C 引脚配置 (根据实际硬件修改)
 * ================================================================ */
#define HW_I2C              I2C1
#define HW_I2C_RCC_APB1     RCC_APB1Periph_I2C1
#define HW_I2C_RCC_APB2     RCC_APB2Periph_GPIOB
#define HW_I2C_GPIO         GPIOB
#define HW_I2C_SCL_PIN      GPIO_Pin_6
#define HW_I2C_SDA_PIN      GPIO_Pin_7
#define HW_I2C_SPEED        400000  /* 400kHz Fast Mode, 改 100000 为标准模式 */

/* 超时计数 (根据主频调整, 72MHz 下约 1ms) */
#define HW_I2C_TIMEOUT      10000


#define SCAN_I2C_DEVICE

/* ================================================================
 *  基础操作 (与 sw_i2c 命名对应)
 * ================================================================ */

/**
 * @brief  初始化硬件 I2C (含 GPIO 和外设配置)
 */
void HW_I2C_Init(void);

/**
 * @brief  发送 START 信号
 * @return 1=成功, 0=失败 (总线卡死已自动恢复)
 */
uint8_t HW_I2C_Start(void);

/**
 * @brief  发送 STOP 信号
 */
void HW_I2C_Stop(void);

/**
 * @brief  等待 ACK 应答
 * @return 1=收到ACK, 0=超时无ACK
 */
uint8_t HW_I2C_Wait_Ack(void);

/**
 * @brief  主机发送 ACK
 */
void HW_I2C_Ack(void);

/**
 * @brief  主机发送 NACK
 */
void HW_I2C_NAck(void);

/**
 * @brief  发送一个字节
 * @param  data  待发字节
 * @return 1=成功, 0=超时
 */
uint8_t HW_I2C_SendByte(uint8_t data);

/**
 * @brief  接收一个字节
 * @param  ack  1=回ACK继续读, 0=回NACK结束读
 * @return 接收到的字节
 */
uint8_t HW_I2C_ReadByte(uint8_t ack);


/* ================================================================
 *  高级操作 (封装常用场景)
 * ================================================================ */

/**
 * @brief  向指定设备写入数据
 * @param  dev_addr  7-bit 设备地址 (无需左移)
 * @param  data      数据指针
 * @param  len       数据长度
 * @return 1=成功, 0=失败
 */
uint8_t HW_I2C_Write(uint8_t dev_addr, const uint8_t *data, uint16_t len);

/**
 * @brief  从指定设备读取数据
 * @param  dev_addr  7-bit 设备地址 (无需左移)
 * @param  buf       接收缓冲区
 * @param  len       读取长度
 * @return 1=成功, 0=失败
 */
uint8_t HW_I2C_Read(uint8_t dev_addr, uint8_t *buf, uint16_t len);

/**
 * @brief  写寄存器 (先写寄存器地址, 再写数据)
 * @param  dev_addr   7-bit 设备地址
 * @param  reg_addr   寄存器地址
 * @param  data       数据指针
 * @param  len        数据长度
 * @return 1=成功, 0=失败
 */
uint8_t HW_I2C_WriteReg(uint8_t dev_addr, uint8_t reg_addr,
                         const uint8_t *data, uint16_t len);

/**
 * @brief  读寄存器 (先写寄存器地址, 再读数据)
 * @param  dev_addr   7-bit 设备地址
 * @param  reg_addr   寄存器地址
 * @param  buf        接收缓冲区
 * @param  len        读取长度
 * @return 1=成功, 0=失败
 */
uint8_t HW_I2C_ReadReg(uint8_t dev_addr, uint8_t reg_addr,
                        uint8_t *buf, uint16_t len);


/* ================================================================
 *  调试工具
 * ================================================================ */

/**
 * @brief  获取总线复位次数 (调试用)
 */
uint32_t HW_I2C_GetRecoveryCount(void);

#ifdef SCAN_I2C_DEVICE
#include "elog.h"   //实现log_i打印函数
#define hw_i2c_output log_i
/**
 * @brief  扫描 I2C 总线上的设备 (打印地址到串口)
 */
void HW_I2C_ScanDevices(void);

/**
 * @brief  检查指定地址是否有设备应答
 * @param  dev_addr  7-bit 设备地址
 * @return 1=有设备, 0=无应答
 */
uint8_t HW_I2C_TestDevice(uint8_t dev_addr);

#endif
#endif /* __HW_I2C_H */
