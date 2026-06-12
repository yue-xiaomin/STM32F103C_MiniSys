/**
 * @file    uc1617s_bus_i2c.c
 * @brief   I2C 传输层封装 (软I2C + 硬件I2C)
 *
 *  将 sw_i2c / hw_i2c 通用驱动适配到 uc1617s_bus_t 接口.
 *  UC1617S 特有的 I2C 协议 (控制字节、地址切换) 在此层处理.
 */

#include "uc1617s_bus.h"
#include "uc1617s.h"

#include "uc1617s_conf.h"


/* ---- 根据编译宏选择底层 I2C 驱动 ---- */
#ifdef UC_USE_SW_I2C
  #include "sw_i2c.h"
#elif defined(UC_USE_HW_I2C)
  #include "hw_i2c.h"
#endif


#if defined(UC_USE_SW_I2C) || defined(UC_USE_HW_I2C)
#define LCD_GPIO            GPIOA
#define LCD_GPIO_RCC        RCC_APB2Periph_GPIOA
#define LCD_CS_PIN          GPIO_Pin_4
#define LCD_DC_PIN          GPIO_Pin_3
#define LCD_RST_PIN         GPIO_Pin_2   /* 硬件复位引脚 (可选) */
#define LCD_BL_PIN          GPIO_Pin_1

#define CS_LOW()    GPIO_ResetBits(LCD_GPIO, LCD_CS_PIN)
#define CS_HIGH()   GPIO_SetBits(LCD_GPIO, LCD_CS_PIN)

#define RST_LOW()   GPIO_ResetBits(LCD_GPIO, LCD_RST_PIN)
#define RST_HIGH()  GPIO_SetBits(LCD_GPIO, LCD_RST_PIN)

#define BL_OFF()    GPIO_ResetBits(LCD_GPIO, LCD_BL_PIN)
#define BL_ON()     GPIO_SetBits(LCD_GPIO, LCD_BL_PIN)


/* ============================================================
 *  延时 (使用 delay.h，这里提供兼容包装)
 *  如果你的工程中 lcd_delay_ms/lcd_delay_us 已在别处实现，
 *  可以删除这两个函数，直接调用即可。
 * ============================================================ */
//static void lcd_delay_us(uint32_t us)
//{
//    uint32_t i;
//    while (us--)
//        for (i = 0; i < 8; i++);
//}

static void lcd_delay_ms(uint32_t ms)
{
    uint32_t i;
    while (ms--)
        for (i = 0; i < 7200; i++);
}

static void lcd_gpio_init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(LCD_GPIO_RCC, ENABLE);

    /* 控制引脚: PA1-BL, PA2-RST, PA3-RS(悬空), PA4-CS (推挽输出) */
    gpio.GPIO_Pin   = LCD_BL_PIN | LCD_RST_PIN | LCD_DC_PIN | LCD_CS_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LCD_GPIO, &gpio);

    /* ★ I2C 模式: CS 必须拉低, 否则芯片不响应 I2C 总线 */
    GPIO_ResetBits(LCD_GPIO, LCD_CS_PIN);         /* CS = 0 (始终选中) */
    GPIO_SetBits(LCD_GPIO, LCD_RST_PIN);          /* RST = 1 (释放复位) */
    GPIO_SetBits(LCD_GPIO, LCD_BL_PIN);           /* BL  = 1 (背光开)   */ 
    
}


/* ================================================================
 *  硬件复位 (可选)
 * ================================================================ */
static void device_hw_reset(void)
{
    RST_LOW();
    lcd_delay_ms(20);       /* 复位低脉宽 >= 10ms */
    RST_HIGH();
    lcd_delay_ms(120);      /* 复位后等待 >= 100ms */
}

#endif


/* ================================================================
 *  软件 I2C 传输层
 * ================================================================ */
#ifdef UC_USE_SW_I2C

static void si2c_init(void)
{
    SW_I2C_Init();
    lcd_gpio_init();
    device_hw_reset();    
}

static void si2c_write_cmd(uint8_t cmd)
{
    SW_I2C_Start();
    SW_I2C_SendByte(UC1617S_ADDR_CMD << 1);  /* 0x78 */
    SW_I2C_Wait_Ack();
    SW_I2C_SendByte(cmd);
    SW_I2C_Wait_Ack();
    SW_I2C_Stop();
}

static void si2c_write_cmd2(uint8_t cmd, uint8_t param)
{
    SW_I2C_Start();
    SW_I2C_SendByte(UC1617S_ADDR_CMD << 1);
    SW_I2C_Wait_Ack();
    SW_I2C_SendByte(cmd);
    SW_I2C_Wait_Ack();
    SW_I2C_SendByte(param);
    SW_I2C_Wait_Ack();
    SW_I2C_Stop();
}

static uint8_t si2c_write_data(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    SW_I2C_Start();
    SW_I2C_SendByte(UC1617S_ADDR_DATA << 1);  /* 0x7A */
    if (!SW_I2C_Wait_Ack()) { SW_I2C_Stop(); return 0; }
    for (i = 0; i < len; i++) {
        SW_I2C_SendByte(data[i]);
        if (!SW_I2C_Wait_Ack()) { SW_I2C_Stop(); return 0; }
    }
    SW_I2C_Stop();
    return 1;
}

static uint8_t si2c_write_fill(uint8_t val, uint16_t count)
{
    uint16_t i;
    SW_I2C_Start();
    SW_I2C_SendByte(UC1617S_ADDR_DATA << 1);
    if (!SW_I2C_Wait_Ack()) { SW_I2C_Stop(); return 0; }
    for (i = 0; i < count; i++) {
        SW_I2C_SendByte(val);
        if (!SW_I2C_Wait_Ack()) { SW_I2C_Stop(); return 0; }
    }
    SW_I2C_Stop();
    return 1;
}

static void si2c_deinit(void) { /* 无特殊操作 */ }

const uc1617s_bus_t uc_bus_sw_i2c = {
    .write_cmd  = si2c_write_cmd,
    .write_cmd2 = si2c_write_cmd2,
    .write_data = si2c_write_data,
    .write_fill = si2c_write_fill,
    .init       = si2c_init,
    .deinit     = si2c_deinit,
    .type       = UC_BUS_SOFT_I2C,
};

#endif /* UC_USE_SW_I2C */


/* ================================================================
 *  硬件 I2C 传输层
 * ================================================================ */
#ifdef UC_USE_HW_I2C

static void hi2c_init(void)
{
    HW_I2C_Init();
    lcd_gpio_init();
    device_hw_reset();
}

/**
 * @brief  发送命令 (控制字节 + 命令字节)
 *         [START] [0x78] [0x00] [cmd] [STOP]
 */
static void hi2c_write_cmd(uint8_t cmd)
{
    
    HW_I2C_Write(UC1617S_ADDR_CMD, &cmd, 1);
}

/**
 * @brief  发送双字节命令
 *         [START] [0x78] [0x00] [cmd] [param] [STOP]
 */
static void hi2c_write_cmd2(uint8_t cmd, uint8_t param)
{
    uint8_t buf[2];
    buf[0] = cmd;
    buf[1] = param;
    HW_I2C_Write(UC1617S_ADDR_CMD, buf, 2);
}

/**
 * @brief  批量写数据
 *         [START] [0x7A] [0x40] [data0] [data1] ... [STOP]
 * @note   I2C 单次传输有 256 字节限制 (UC1617S 页大小),
 *         超长数据需分包. 此处分 128 字节/包.
 */
#define I2C_BULK_CHUNK  128

static uint8_t hi2c_write_data(const uint8_t *data, uint16_t len)
{
    uint16_t sent = 0;
    uint16_t chunk;

    while (sent < len) {
        chunk = len - sent;
        if (chunk > I2C_BULK_CHUNK) chunk = I2C_BULK_CHUNK;

        /* 发送地址头: 设备地址(写) + 控制字节 */
        if (!HW_I2C_Start()) return 0;
        HW_I2C_SendByte(UC1617S_ADDR_DATA << 1);
        if (!HW_I2C_Wait_Ack()) { HW_I2C_Stop(); return 0; }

        /* 发送数据块 */
        {
            uint16_t i;
            for (i = 0; i < chunk; i++) {
                if (!HW_I2C_SendByte(data[sent + i])) {
                    HW_I2C_Stop();
                    return 0;
                }
            }
        }

        HW_I2C_Stop();
        sent += chunk;
    }
    return 1;
}

/**
 * @brief  批量填充同一字节
 */
static uint8_t hi2c_write_fill(uint8_t val, uint16_t count)
{
    uint16_t sent = 0;
    uint16_t chunk;

    while (sent < count) {
        chunk = count - sent;
        if (chunk > I2C_BULK_CHUNK) chunk = I2C_BULK_CHUNK;

        if (!HW_I2C_Start()) return 0;
        HW_I2C_SendByte(UC1617S_ADDR_DATA << 1);
        if (!HW_I2C_Wait_Ack()) { HW_I2C_Stop(); return 0; }

        {
            uint16_t i;
            for (i = 0; i < chunk; i++) {
                if (!HW_I2C_SendByte(val)) {
                    HW_I2C_Stop();
                    return 0;
                }
            }
        }

        HW_I2C_Stop();
        sent += chunk;
    }
    return 1;
}

static void hi2c_deinit(void) { /* 无特殊操作 */ }

const uc1617s_bus_t uc_bus_hw_i2c = {
    .write_cmd  = hi2c_write_cmd,
    .write_cmd2 = hi2c_write_cmd2,
    .write_data = hi2c_write_data,
    .write_fill = hi2c_write_fill,
    .init       = hi2c_init,
    .deinit     = hi2c_deinit,
    .type       = UC_BUS_HW_I2C,
};

#endif /* UC_USE_HW_I2C */
