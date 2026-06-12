/**
 * @file    hw_i2c.c
 * @brief   硬件 I2C 通用驱动 (STM32F103 I2C1)
 *
 *  函数命名与 sw_i2c.c 保持一致 (HW_I2C_ 前缀),
 *  可供任何外设 (EEPROM, OLED, 传感器等) 共用.
 *
 *  ★ 已内置 STM32F103 硬件 I2C BUG 规避:
 *    - 所有操作带超时, 绝不死等
 *    - 检测 BUSY 卡死 → 自动复位总线 → 重新初始化
 *    - SWRST + RCC 外设双重复位 (参考 ST 官方 errata)
 *
 *  BUG 根因:
 *    STM32F103 的硬件 I2C 在总线受干扰 (短路/毛刺/从设备异常) 后,
 *    SR2.BUSY 标志会卡死为 1, 无法发送 START 信号.
 *    单纯设 SWRST 无效 (读寄存器后值会恢复), 必须同时做:
 *      1. I2C->CR1 |= SWRST    (进入复位)
 *      2. I2C->CR1 &= ~SWRST   (退出复位)
 *      3. RCC APB1 外设强制复位 (通过 RCC->APB1RSTR)
 *      4. 重新配置 GPIO
 *      5. 重新初始化 I2C
 */

#include "hw_i2c.h"

/* ================================================================
 *  内部变量
 * ================================================================ */
static volatile uint32_t i2c_bus_recovery_count = 0;


/* ================================================================
 *  超时等待辅助 (内部)
 * ================================================================ */

/**
 * @brief  等待 I2C 标志位, 带超时
 * @param  flag  标志位 (如 I2C_FLAG_BUSY)
 * @param  wait  1=等待置位, 0=等待清除
 * @return 1=成功, 0=超时
 */
static uint8_t HW_I2C_WaitFlag(uint32_t flag, uint8_t wait)
{
    uint32_t timeout = HW_I2C_TIMEOUT;
    while (timeout--) {
        uint8_t set = (I2C_GetFlagStatus(HW_I2C, flag) == SET);
        if (wait ? set : !set) return 1;
    }
    return 0; /* 超时 */
}


/* ================================================================
 *  ★ I2C 总线复位 (BUG 规避核心, 内部)
 * ================================================================ */

/**
 * @brief  复位卡死的 I2C 总线并重新初始化
 *
 *  执行步骤:
 *    1. I2C SWRST (CR1 bit 15) — 让 I2C 外设进入复位态
 *    2. RCC APB1 外设强制复位 — 彻底重置所有寄存器
 *    3. 重新配置 GPIO — PB6/PB7 复用开漏
 *    4. 重新初始化 I2C — 时钟/地址/模式
 */
static void HW_I2C_BusRecovery(void)
{
    GPIO_InitTypeDef gpio;
    I2C_InitTypeDef i2c;

    i2c_bus_recovery_count++;

    /* ---- Step 1: I2C SWRST ---- */
    HW_I2C->CR1 |= I2C_CR1_SWRST;     /* 进入复位 */
    HW_I2C->CR1 &= ~I2C_CR1_SWRST;    /* 退出复位 */

    /* ---- Step 2: RCC APB1 外设强制复位 ---- */
    RCC->APB1RSTR |= HW_I2C_RCC_APB1;   /* 强制复位 */
    RCC->APB1RSTR &= ~HW_I2C_RCC_APB1;  /* 释放复位 */

    /* ---- Step 3: 重新配置 GPIO ---- */
    RCC_APB2PeriphClockCmd(HW_I2C_RCC_APB2, ENABLE);

    gpio.GPIO_Pin   = HW_I2C_SCL_PIN | HW_I2C_SDA_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_AF_OD;      /* 复用开漏 */
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(HW_I2C_GPIO, &gpio);

    /* ---- Step 4: 重新初始化 I2C ---- */
    RCC_APB1PeriphClockCmd(HW_I2C_RCC_APB1, ENABLE);

    I2C_DeInit(HW_I2C);

    I2C_StructInit(&i2c);
    i2c.I2C_Mode                = I2C_Mode_I2C;
    i2c.I2C_DutyCycle           = I2C_DutyCycle_2;
    i2c.I2C_OwnAddress1         = 0x00;
    i2c.I2C_Ack                 = I2C_Ack_Enable;
    i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i2c.I2C_ClockSpeed          = HW_I2C_SPEED;
    I2C_Init(HW_I2C, &i2c);
    I2C_Cmd(HW_I2C, ENABLE);
}


/* ================================================================
 *  基础操作 (与 sw_i2c 命名对应)
 * ================================================================ */

void HW_I2C_Init(void)
{
    GPIO_InitTypeDef gpio;
    I2C_InitTypeDef i2c;

    /* 时钟使能 */
    RCC_APB2PeriphClockCmd(HW_I2C_RCC_APB2 | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(HW_I2C_RCC_APB1, ENABLE);

    /* GPIO: PB6=SCL, PB7=SDA, 复用开漏 */
    gpio.GPIO_Pin   = HW_I2C_SCL_PIN | HW_I2C_SDA_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_AF_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(HW_I2C_GPIO, &gpio);

    /* I2C 外设配置 */
    I2C_DeInit(HW_I2C);

    I2C_StructInit(&i2c);
    i2c.I2C_Mode                = I2C_Mode_I2C;
    i2c.I2C_DutyCycle           = I2C_DutyCycle_2;
    i2c.I2C_OwnAddress1         = 0x00;
    i2c.I2C_Ack                 = I2C_Ack_Enable;
    i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i2c.I2C_ClockSpeed          = HW_I2C_SPEED;
    I2C_Init(HW_I2C, &i2c);

    I2C_Cmd(HW_I2C, ENABLE);
}

uint8_t HW_I2C_Start(void)
{
    /* 先检查 BUSY, 如果卡死直接复位 */
    if (!HW_I2C_WaitFlag(I2C_FLAG_BUSY, 0)) {
        HW_I2C_BusRecovery();
    }

    I2C_GenerateSTART(HW_I2C, ENABLE);

    /* 等 SB (Start Bit) */
    if (!HW_I2C_WaitFlag(I2C_FLAG_SB, 1)) {
        HW_I2C_Stop();
        return 0;
    }
    return 1;
}

void HW_I2C_Stop(void)
{
    I2C_GenerateSTOP(HW_I2C, ENABLE);
    /* 等 BUSY 清除 */
    if (!HW_I2C_WaitFlag(I2C_FLAG_BUSY, 0)) {
        /* BUSY 清不掉 → 总线卡死 → 复位 */
        HW_I2C_BusRecovery();
    }
}

uint8_t HW_I2C_Wait_Ack(void)
{
    /* 等 ADDR 标志 (从设备应答后硬件自动置位) */
    if (!HW_I2C_WaitFlag(I2C_FLAG_ADDR, 1)) {
        /* 超时: 发 STOP 清理 */
        I2C_GenerateSTOP(HW_I2C, ENABLE);
        return 0;
    }
    /* 清 ADDR 标志 (读 SR1 + SR2) */
    (void)HW_I2C->SR1;
    (void)HW_I2C->SR2;
    return 1;
}

void HW_I2C_Ack(void)
{
    I2C_AcknowledgeConfig(HW_I2C, ENABLE);
}

void HW_I2C_NAck(void)
{
    I2C_AcknowledgeConfig(HW_I2C, DISABLE);
}

uint8_t HW_I2C_SendByte(uint8_t data)
{
    I2C_SendData(HW_I2C, data);
    /* 等 TXE (发送缓冲区空) */
    if (!HW_I2C_WaitFlag(I2C_FLAG_TXE, 1)) {
        return 0;
    }
    return 1;
}

uint8_t HW_I2C_ReadByte(uint8_t ack)
{
    uint8_t data;

    /* 设置 ACK/NACK */
    if (ack) {
        I2C_AcknowledgeConfig(HW_I2C, ENABLE);
    } else {
        I2C_AcknowledgeConfig(HW_I2C, DISABLE);
    }

    /* 等 RXNE (接收缓冲区非空) */
    if (!HW_I2C_WaitFlag(I2C_FLAG_RXNE, 1)) {
        return 0xFF; /* 超时返回 0xFF */
    }

    data = I2C_ReceiveData(HW_I2C);
    return data;
}


/* ================================================================
 *  高级操作
 * ================================================================ */

uint8_t HW_I2C_Write(uint8_t dev_addr, const uint8_t *data, uint16_t len)
{
    uint16_t i;

    if (!HW_I2C_Start()) return 0;

    /* 发送设备地址 + 写 */
    I2C_Send7bitAddress(HW_I2C, dev_addr << 1, I2C_Direction_Transmitter);
    if (!HW_I2C_Wait_Ack()) return 0;

    /* 逐字节发送 */
    for (i = 0; i < len; i++) {
        if (!HW_I2C_SendByte(data[i])) {
            HW_I2C_Stop();
            return 0;
        }
    }

    HW_I2C_Stop();
    return 1;
}

uint8_t HW_I2C_Read(uint8_t dev_addr, uint8_t *buf, uint16_t len)
{
    uint16_t i;

    if (!HW_I2C_Start()) return 0;

    /* 发送设备地址 + 读 */
    I2C_Send7bitAddress(HW_I2C, dev_addr << 1, I2C_Direction_Receiver);
    if (!HW_I2C_Wait_Ack()) return 0;

    /* 逐字节接收 */
    for (i = 0; i < len; i++) {
        buf[i] = HW_I2C_ReadByte((i < len - 1) ? 1 : 0); /* 最后一字节回 NACK */
    }

    HW_I2C_Stop();
    return 1;
}

uint8_t HW_I2C_WriteReg(uint8_t dev_addr, uint8_t reg_addr,
                         const uint8_t *data, uint16_t len)
{
    uint16_t i;

    if (!HW_I2C_Start()) return 0;

    /* 设备地址 + 写 */
    I2C_Send7bitAddress(HW_I2C, dev_addr << 1, I2C_Direction_Transmitter);
    if (!HW_I2C_Wait_Ack()) return 0;

    /* 寄存器地址 */
    if (!HW_I2C_SendByte(reg_addr)) {
        HW_I2C_Stop();
        return 0;
    }

    /* 数据 */
    for (i = 0; i < len; i++) {
        if (!HW_I2C_SendByte(data[i])) {
            HW_I2C_Stop();
            return 0;
        }
    }

    HW_I2C_Stop();
    return 1;
}

uint8_t HW_I2C_ReadReg(uint8_t dev_addr, uint8_t reg_addr,
                        uint8_t *buf, uint16_t len)
{
    uint16_t i;

    /* Phase 1: 写寄存器地址 */
    if (!HW_I2C_Start()) return 0;

    I2C_Send7bitAddress(HW_I2C, dev_addr << 1, I2C_Direction_Transmitter);
    if (!HW_I2C_Wait_Ack()) return 0;

    if (!HW_I2C_SendByte(reg_addr)) {
        HW_I2C_Stop();
        return 0;
    }

    /* Phase 2: 重复 START, 读数据 */
    if (!HW_I2C_Start()) return 0;

    I2C_Send7bitAddress(HW_I2C, dev_addr << 1, I2C_Direction_Receiver);
    if (!HW_I2C_Wait_Ack()) return 0;

    for (i = 0; i < len; i++) {
        buf[i] = HW_I2C_ReadByte((i < len - 1) ? 1 : 0);
    }

    HW_I2C_Stop();
    return 1;
}


/* ================================================================
 *  调试工具
 * ================================================================ */

uint32_t HW_I2C_GetRecoveryCount(void)
{
    return i2c_bus_recovery_count;
}




#ifdef SCAN_I2C_DEVICE
uint8_t HW_I2C_TestDevice(uint8_t dev_addr)
{
    uint8_t result;

    if (!HW_I2C_Start()) return 0;

    I2C_Send7bitAddress(HW_I2C, dev_addr << 1, I2C_Direction_Transmitter);

    /* 检查 ADDR 标志 */
    if (HW_I2C_WaitFlag(I2C_FLAG_ADDR, 1)) {
        result = 1; /* 有设备应答 */
        (void)HW_I2C->SR1; /* 清 ADDR */
        (void)HW_I2C->SR2;
    } else {
        result = 0; /* 无应答 */
        /* 清 AF 标志 */
        I2C_ClearFlag(HW_I2C, I2C_FLAG_AF);
    }

    HW_I2C_Stop();
    return result;
}

void HW_I2C_ScanDevices(void)
{
    uint8_t addr;
    uint8_t count = 0;

    /* 需要用户自行实现 log_i 或替换为 printf */
    hw_i2c_output("HW I2C scanning 0x08~0x77...");

    for (addr = 0x08; addr <= 0x77; addr++) {
        if (HW_I2C_TestDevice(addr)) {
            hw_i2c_output("  Found: 0x%02X (W=0x%02X, R=0x%02X)", 
                   addr, addr << 1, (addr << 1) | 1); 
            count++;
        }
    }

    hw_i2c_output("Scan done, %d device(s) found.", count); 
    (void)count; /* 避免未使用警告 */
}
#endif
