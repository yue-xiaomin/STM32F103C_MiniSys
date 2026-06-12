/**
 * @file    uc1617s_bus_spi.c
 * @brief   SPI 传输层实现 (普通SPI + DMA SPI)
 *
 *  适用于 STM32F103, SPI1 + DMA1_Channel3
 *  引脚: PA5=SCK, PA7=MOSI, PA4=CS, PA3=DC
 *
 *  ★ UC1617S SPI 模式:
 *    - CS 拉低后, DC 引脚区分命令/数据
 *    - DC=0: 命令字节
 *    - DC=1: 数据字节
 *    - 连续传输, 无需像 I2C 那样分包
 */

#include "uc1617s_bus.h"

#include "uc1617s_conf.h"


#if defined(UC_USE_SPI) || defined(UC_USE_DMA_SPI)

#include "uc1617s.h"
#include "stm32f10x.h"
#include "delay.h"

/* ================================================================
 *  SPI 引脚 & 外设配置 (根据实际硬件修改)
 * ================================================================ */
#define LCD_SPI             SPI1
#define LCD_SPI_RCC_APB2    RCC_APB2Periph_SPI1
#define LCD_SPI_RCC_APB1    0   /* SPI1 挂在 APB2, 无 APB1 时钟 */

/* GPIO */
#define LCD_GPIO            GPIOA
#define LCD_GPIO_RCC        RCC_APB2Periph_GPIOA
#define LCD_SCK_PIN         GPIO_Pin_5
#define LCD_MOSI_PIN        GPIO_Pin_7
#define LCD_CS_PIN          GPIO_Pin_4
#define LCD_DC_PIN          GPIO_Pin_3
#define LCD_RST_PIN         GPIO_Pin_2   /* 硬件复位引脚 (可选) */
#define LCD_BL_PIN          GPIO_Pin_1

/* DMA */
#define LCD_DMA             DMA1
#define LCD_DMA_RCC         RCC_AHBPeriph_DMA1
#define LCD_DMA_CHANNEL     DMA1_Channel3   /* SPI1_TX = Channel3 */
#define LCD_DMA_FLAG_TC     DMA1_FLAG_TC3
#define LCD_DMA_FLAG_GL     DMA1_FLAG_GL3

/* ================================================================
 *  引脚操作宏
 * ================================================================ */
#define CS_LOW()    GPIO_ResetBits(LCD_GPIO, LCD_CS_PIN)
#define CS_HIGH()   GPIO_SetBits(LCD_GPIO, LCD_CS_PIN)
#define DC_CMD()    GPIO_ResetBits(LCD_GPIO, LCD_DC_PIN)
#define DC_DATA()   GPIO_SetBits(LCD_GPIO, LCD_DC_PIN)
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



/* ================================================================
 *  SPI 硬件初始化
 * ================================================================ */
static void spi_hw_init(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef spi;

    /* 时钟使能 */
    RCC_APB2PeriphClockCmd(LCD_GPIO_RCC | LCD_SPI_RCC_APB2 | RCC_APB2Periph_AFIO, ENABLE);

    /* SCK, MOSI: 复用推挽输出 */
    gpio.GPIO_Pin   = LCD_SCK_PIN | LCD_MOSI_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LCD_GPIO, &gpio);

    /* CS, DC, RST: 推挽输出 */
    gpio.GPIO_Pin   = LCD_CS_PIN | LCD_DC_PIN | LCD_RST_PIN | LCD_BL_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LCD_GPIO, &gpio);

    CS_HIGH();  /* 默认不选中 */
    DC_CMD();
    BL_ON();
    
    /* SPI 配置 */
    SPI_StructInit(&spi);
    spi.SPI_Direction         = SPI_Direction_1Line_Tx;  /* 只发不收 */
    spi.SPI_Mode              = SPI_Mode_Master;
    spi.SPI_DataSize          = SPI_DataSize_8b;
    spi.SPI_CPOL              = SPI_CPOL_High;           /* UC1617S: CPOL=1 */
    spi.SPI_CPHA              = SPI_CPHA_2Edge;          /* UC1617S: CPHA=1 */
    spi.SPI_NSS               = SPI_NSS_Soft;            /* 软件 CS */
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; /* 72MHz/4=18MHz */
    spi.SPI_FirstBit          = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial     = 7;
    SPI_Init(LCD_SPI, &spi);

    SPI_Cmd(LCD_SPI, ENABLE);
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


/* ================================================================
 *  SPI 阻塞发送一个字节
 * ================================================================ */
static inline void spi_send_byte(uint8_t data)
{
    while (SPI_I2S_GetFlagStatus(LCD_SPI, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(LCD_SPI, data);
}

/* ================================================================
 *  SPI 等待最后一字节发完 (BSY 清除)
 * ================================================================ */
static inline void spi_wait_bsy(void)
{
    while (SPI_I2S_GetFlagStatus(LCD_SPI, SPI_I2S_FLAG_BSY) == SET);
}

static void spi_write_cmd(uint8_t cmd)
{
    CS_LOW();
    DC_CMD();
    spi_send_byte(cmd);
    spi_wait_bsy();
    CS_HIGH();
}

static void spi_write_cmd2(uint8_t cmd, uint8_t param)
{
    CS_LOW();
    DC_CMD();
    spi_send_byte(cmd);
    spi_send_byte(param);
    spi_wait_bsy();
    CS_HIGH();
}

/* ================================================================
 *  普通 SPI 传输层
 * ================================================================ */
#ifdef UC_USE_SPI

static void spi_init(void)
{
    spi_hw_init();
    device_hw_reset();
}



static uint8_t spi_write_data(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    CS_LOW();
    DC_DATA();
    for (i = 0; i < len; i++) {
        spi_send_byte(data[i]);
    }
    spi_wait_bsy();
    CS_HIGH();
    return 1;
}

static uint8_t spi_write_fill(uint8_t val, uint16_t count)
{
    uint16_t i;
    CS_LOW();
    DC_DATA();
    for (i = 0; i < count; i++) {
        spi_send_byte(val);
    }
    spi_wait_bsy();
    CS_HIGH();
    return 1;
}

static void spi_deinit(void)
{
    SPI_Cmd(LCD_SPI, DISABLE);
}

const uc1617s_bus_t uc_bus_spi = {
    .write_cmd  = spi_write_cmd,
    .write_cmd2 = spi_write_cmd2,
    .write_data = spi_write_data,
    .write_fill = spi_write_fill,
    .init       = spi_init,
    .deinit     = spi_deinit,
    .type       = UC_BUS_SPI,
};

#endif /* UC_USE_SPI */


/* ================================================================
 *  DMA SPI 传输层
 * ================================================================ */
#ifdef UC_USE_DMA_SPI

/** @brief DMA 传输完成标志 (由中断置位) */
static volatile uint8_t dma_transfer_done = 1;

/**
 * @brief  DMA1_Channel3 中断处理 (传输完成)
 * @note   需要在 stm32f10x_it.c 中调用, 或在此定义
 */
void DMA1_Channel3_IRQHandler(void)
{
    if (DMA_GetITStatus(LCD_DMA_FLAG_TC)) {
        DMA_ClearITPendingBit(LCD_DMA_FLAG_GL);
        /* 等 SPI 最后一字节发完 */
        spi_wait_bsy();
        CS_HIGH();
        dma_transfer_done = 1;
    }
}

/**
 * @brief  等待 DMA 传输完成
 * @param  timeout_ms 超时毫秒 (近似)
 * @return 1=成功, 0=超时
 */
static uint8_t dma_wait_complete(uint32_t timeout_ms)
{
    uint32_t t = timeout_ms * 1000; /* 粗略计数 */
    while (!dma_transfer_done && t--);
    return dma_transfer_done;
}

/**
 * @brief  DMA 传输配置 (内存 → SPI->DR)
 * @param  mem_addr  源地址
 * @param  len       传输长度 (字节)
 */
static void dma_start_transfer(const uint8_t *mem_addr, uint16_t len)
{
    dma_transfer_done = 0;

    /* 确保上一次 DMA 完成 */
    DMA_Cmd(LCD_DMA_CHANNEL, DISABLE);

    /* 配置 DMA */
    DMA_InitTypeDef dma;
    dma.DMA_PeripheralBaseAddr = (uint32_t)&LCD_SPI->DR;
    dma.DMA_MemoryBaseAddr     = (uint32_t)mem_addr;
    dma.DMA_DIR                = DMA_DIR_PeripheralDST;
    dma.DMA_BufferSize         = len;
    dma.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode               = DMA_Mode_Normal;
    dma.DMA_Priority           = DMA_Priority_High;
    dma.DMA_M2M                = DMA_M2M_Disable;
    DMA_Init(LCD_DMA_CHANNEL, &dma);

    /* 使能 DMA 发送请求 */
    SPI_I2S_DMACmd(LCD_SPI, SPI_I2S_DMAReq_Tx, ENABLE);

    /* 使能 DMA 传输完成中断 */
    DMA_ITConfig(LCD_DMA_CHANNEL, DMA_IT_TC, ENABLE);

    /* 开始传输 */
    DMA_Cmd(LCD_DMA_CHANNEL, ENABLE);
}

/**
 * @brief  DMA 传输字节缓冲区 (SPI 只发不收, 需要 copy 到临时 buf)
 * @note   DMA 要求源地址在 SRAM 中, 帧缓冲区满足此条件
 */
static uint8_t dma_spi_write_data(const uint8_t *data, uint16_t len)
{
    CS_LOW();
    DC_DATA();

    /* 使用 SPI 阻塞发送前几个字节对齐 DMA (可选, 此处直接 DMA) */
    dma_start_transfer(data, len);
    if (!dma_wait_complete(100)) {
        /* 超时, 强制停止 */
        DMA_Cmd(LCD_DMA_CHANNEL, DISABLE);
        CS_HIGH();
        return 0;
    }
    return 1;
}

/**
 * @brief  DMA 填充 — 需要临时缓冲区
 * @note   DMA 从内存读数据, 不能直接填同一字节.
 *         这里使用一个小的填充缓冲区分块传输.
 */
#define FILL_BUF_SIZE   256  /* 分块大小, 可调整 */
static uint8_t fill_buf[FILL_BUF_SIZE];

static uint8_t dma_spi_write_fill(uint8_t val, uint16_t count)
{
    uint16_t sent = 0;

    /* 预填充临时缓冲区 */
    uint16_t chunk;
    uint16_t i;
    for (i = 0; i < FILL_BUF_SIZE; i++) fill_buf[i] = val;

    CS_LOW();
    DC_DATA();

    while (sent < count) {
        chunk = count - sent;
        if (chunk > FILL_BUF_SIZE) chunk = FILL_BUF_SIZE;

        dma_start_transfer(fill_buf, chunk);
        if (!dma_wait_complete(100)) {
            DMA_Cmd(LCD_DMA_CHANNEL, DISABLE);
            CS_HIGH();
            return 0;
        }
        sent += chunk;
    }
    return 1;
}

static void dma_spi_init(void)
{
    NVIC_InitTypeDef nvic;

    spi_hw_init();

    /* DMA 时钟 */
    RCC_AHBPeriphClockCmd(LCD_DMA_RCC, ENABLE);

    /* DMA 中断 */
    nvic.NVIC_IRQChannel                   = DMA1_Channel3_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority        = 0;
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);

    device_hw_reset();
}

static void dma_spi_deinit(void)
{
    DMA_Cmd(LCD_DMA_CHANNEL, DISABLE);
    SPI_I2S_DMACmd(LCD_SPI, SPI_I2S_DMAReq_Tx, DISABLE);
    SPI_Cmd(LCD_SPI, DISABLE);
}

const uc1617s_bus_t uc_bus_dma_spi = {
    .write_cmd  = spi_write_cmd,        /* 命令仍用阻塞, 数据量小 */
    .write_cmd2 = spi_write_cmd2,
    .write_data = dma_spi_write_data,
    .write_fill = dma_spi_write_fill,
    .init       = dma_spi_init,
    .deinit     = dma_spi_deinit,
    .type       = UC_BUS_DMA_SPI,
};

#endif /* UC_USE_DMA_SPI */

#endif /* UC_USE_SPI || UC_USE_DMA_SPI */
