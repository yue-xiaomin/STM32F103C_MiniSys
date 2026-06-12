#include "lcd_spi.h"
#include <stdlib.h>

/* ============================================================
 *  显存缓冲区
 *
 *  格式: 2bit/像素, 与 UC1617S RAM 格式完全一致
 *  每行 128像素 ÷ 4 = 32字节
 *  96行 × 32字节 = 3072字节
 *
 *  字节内像素排列 (MSB first):
 *    bit[7:6] = 像素0 (最左)
 *    bit[5:4] = 像素1
 *    bit[3:2] = 像素2
 *    bit[1:0] = 像素3 (最右)
 *
 *  灰度值:
 *    00=白   01=浅灰   10=深灰   11=黑
 * ============================================================ */
static uint8_t g_framebuf[LCD_HEIGHT][LCD_WIDTH / 4];  /* 96×32 = 3072B */


/* ============================================================
 *  延时
 * ============================================================ */
static void delay_us(uint32_t us)
{
    uint32_t i;
    while (us--)
        for (i = 0; i < 8; i++);
}

static void delay_ms(uint32_t ms)
{
    uint32_t i;
    while (ms--)
        for (i = 0; i < 7200; i++);
}


/* ============================================================
 *  SPI 发送 1 字节 (阻塞等待完成)
 *  SPI1 → PA5(SCK), PA7(MOSI), Mode0, MSB first
 * ============================================================ */
static void SPI_WriteByte(uint8_t dat)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, dat);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
}


/* ============================================================
 *  DMA 批量发送
 *
 *  SPI1_TX = DMA1 Channel 3
 *  配置 DMA 从内存 → SPI1->DR, 发送 len 字节
 *  发送期间 CPU 可以做其他事 (本驱动中选择等待完成)
 *
 *  适用场景:
 *    LCD_Refresh()    — 3072 字节
 *    LCD_FillScreen() — 3072 字节
 *    LCD_ShowImage()  — 最大 3072 字节
 *
 *  对比普通 SPI 轮询:
 *    每字节省 ~20 个 CPU 周期的 TXE+BSY 轮询
 *    3072 字节约节省 ~60000 周期 ≈ 0.83ms @72MHz
 *    实际效果取决于 SPI 时钟分频
 * ============================================================ */
static void SPI_WriteDMA(const uint8_t *buf, uint16_t len)
{
    DMA_InitTypeDef dma;

    if (len == 0) return;

    /* 等待上次 DMA 传输完成 (防止重入) */
    while (DMA_GetFlagStatus(LCD_DMA_TC_FLAG) == RESET);

    /* DMA 通道配置 */
    DMA_DeInit(LCD_DMA_CHANNEL);

    dma.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DR;       /* 外设地址: SPI数据寄存器 */
    dma.DMA_MemoryBaseAddr     = (uint32_t)buf;              /* 内存地址: 数据缓冲区   */
    dma.DMA_DIR                = DMA_DIR_PeripheralDST;      /* 方向: 内存→外设        */
    dma.DMA_BufferSize         = len;                        /* 传输数量               */
    dma.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;  /* 外设地址不自增         */
    dma.DMA_MemoryInc          = DMA_MemoryInc_Enable;       /* 内存地址自增           */
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;/* 外设数据宽度: 8bit     */
    dma.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;    /* 内存数据宽度: 8bit     */
    dma.DMA_Mode               = DMA_Mode_Normal;            /* 普通模式 (非循环)      */
    dma.DMA_Priority           = DMA_Priority_High;          /* 优先级: 高             */
    dma.DMA_M2M                = DMA_M2M_Disable;            /* 非内存到内存           */
    DMA_Init(LCD_DMA_CHANNEL, &dma);

    /* 清除所有 DMA 标志, 然后使能通道 */
    DMA_ClearFlag(LCD_DMA_TC_FLAG | LCD_DMA_GL_FLAG | LCD_DMA_HT_FLAG);
    DMA_Cmd(LCD_DMA_CHANNEL, ENABLE);

    /* 使能 SPI 的 TX DMA 请求 */
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);

    /* 等待 DMA 传输完成 */
    while (DMA_GetFlagStatus(LCD_DMA_TC_FLAG) == RESET);

    /* 等待 SPI 发送完毕 (最后字节从移位寄存器发出) */
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);

    /* 关闭 DMA 通道和 SPI DMA 请求 */
    DMA_Cmd(LCD_DMA_CHANNEL, DISABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
}


/* ============================================================
 *  发送单字节命令 (C/D=0, CS独立事务)
 * ============================================================ */
static void LCD_Cmd1(uint8_t cmd)
{
    GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN);    /* CS↓ */
    GPIO_ResetBits(LCD_RS_PORT, LCD_RS_PIN);    /* RS=0, 命令 */
    SPI_WriteByte(cmd);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
    GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN);      /* CS↑ */
}


/* ============================================================
 *  发送双字节命令 (C/D=0, 一次CS事务内发完)
 * ============================================================ */
static void LCD_Cmd2(uint8_t cmd, uint8_t param)
{
    GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN);
    GPIO_ResetBits(LCD_RS_PORT, LCD_RS_PIN);
    SPI_WriteByte(cmd);
    SPI_WriteByte(param);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
    GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN);
}


/* ============================================================
 *  设置 RAM 写入地址
 * ============================================================ */
static void LCD_SetAddress(uint8_t row, uint8_t col)
{
    LCD_Cmd1(UC_SET_RA_LSB | (row & 0x0F));        /* 0x6X */
    LCD_Cmd1(UC_SET_RA_MSB | ((row >> 4) & 0x07)); /* 0x7X */
    LCD_Cmd1(UC_SET_CA     | (col & 0x1F));        /* 0x0X */
}


/* ============================================================
 *  GPIO 初始化
 * ============================================================ */
static void LCD_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    /* SPI 引脚: PA5-SCK, PA7-MOSI (复用推挽) */
    gpio.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_7;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    /* 控制引脚: PA1-BL, PA2-RST, PA3-RS, PA4-CS (推挽输出) */
    gpio.GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    GPIO_SetBits(GPIOA, GPIO_Pin_4);       /* CS  = 1 */
    GPIO_SetBits(GPIOA, GPIO_Pin_2);       /* RST = 1 */
    GPIO_ResetBits(GPIOA, GPIO_Pin_1);     /* BL  = 0 */
}


/* ============================================================
 *  SPI1 初始化
 *  仅发送模式, Mode0, MSB first
 *  时钟: 72MHz / 16 = 4.5MHz (满足 UC1617S ≤7.14MHz)
 * ============================================================ */
static void LCD_SPI_Init(void)
{
    SPI_InitTypeDef spi;

    spi.SPI_Direction         = SPI_Direction_1Line_Tx;
    spi.SPI_Mode              = SPI_Mode_Master;
    spi.SPI_DataSize          = SPI_DataSize_8b;
    spi.SPI_CPOL              = SPI_CPOL_Low;
    spi.SPI_CPHA              = SPI_CPHA_1Edge;
    spi.SPI_NSS               = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; /* 4.5MHz */
    spi.SPI_FirstBit          = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial     = 7;
    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);
}


/* ============================================================
 *  DMA 时钟使能
 *
 *  STM32F103 DMA1 挂在 AHB 总线上, 默认使能
 *  这里显式调用确保可靠
 * ============================================================ */
static void LCD_DMA_Init(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
}


/* ============================================================
 *  背光控制
 * ============================================================ */
void LCD_BacklightOn(void)
{
    GPIO_SetBits(LCD_BL_PORT, LCD_BL_PIN);
}

void LCD_BacklightOff(void)
{
    GPIO_ResetBits(LCD_BL_PORT, LCD_BL_PIN);
}


/* ============================================================
 *  ★ LCD 初始化
 * ============================================================ */
void LCD_Init(void)
{
    LCD_GPIO_Init();
    LCD_SPI_Init();
    LCD_DMA_Init();

    /* 硬件复位 */
    GPIO_SetBits(GPIOA, GPIO_Pin_2);
    delay_us(100);
    GPIO_ResetBits(GPIOA, GPIO_Pin_2);
    delay_us(10);
    GPIO_SetBits(GPIOA, GPIO_Pin_2);
    delay_ms(170);

    /* 模拟参数配置 */
    LCD_Cmd2(0x31, 0x00);      /* Set APC */
    LCD_Cmd1(0x24);            /* Temperature Compensation */
    LCD_Cmd2(0xD2, 0xEB);      /* Gray Shade */
    LCD_Cmd2(0x81, 80);        /* VBIAS (对比度) */

    /* 显示方向 */
    LCD_Cmd1(0xC4);            /* MY=1, MX=0 */
    LCD_Cmd1(0x40);            /* Scroll Line LSB */
    LCD_Cmd1(0x56);            /* Scroll Line MSB */

    /* 显示窗口 (全屏) */
    LCD_Cmd2(0xF4, 0);         /* WPC0 */
    LCD_Cmd2(0xF6, 31);        /* WPC1 */
    LCD_Cmd2(0xF5, 0);         /* WPP0 */
    LCD_Cmd2(0xF7, 127);       /* WPP1 */
    LCD_Cmd1(0xF9);            /* Enable Window */

    /* 清屏 + 开显示 */
    LCD_Clear();
    LCD_Refresh();

    LCD_Cmd1(0x2F);            /* 4灰度 + 开显示 */
    delay_ms(12);

    LCD_BacklightOn();
}


/* ============================================================
 *  清屏 (缓冲区 → 全白)
 * ============================================================ */
void LCD_Clear(void)
{
    uint16_t i;
    for (i = 0; i < sizeof(g_framebuf); i++)
        ((uint8_t *)g_framebuf)[i] = 0x00;
}


/* ============================================================
 *  直接填 RAM (DMA 版本)
 *
 *  逐行: 设地址 → DMA 发送 32 字节
 *  每行只需一次 DMA 触发, 比轮询快约 30%
 * ============================================================ */
void LCD_FillScreen(uint8_t data)
{
    uint8_t row;
    uint8_t line_buf[LCD_WIDTH / 4];  /* 32 字节行缓冲 */

    /* 预填充行缓冲 */
    for (row = 0; row < LCD_WIDTH / 4; row++)
        line_buf[row] = data;

    LCD_Cmd1(UC_DC_DISP_OFF);

    for (row = 0; row < LCD_HEIGHT; row++)
    {
        LCD_SetAddress(row, 0);

        /* 开启 CS + RS=1 (数据), 然后 DMA 发送 */
        GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN);
        GPIO_SetBits(LCD_RS_PORT, LCD_RS_PIN);

        SPI_WriteDMA(line_buf, LCD_WIDTH / 4);  /* 32B DMA */

        GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN);
    }

    LCD_Cmd1(UC_DC_BW_ON);
}


/* ============================================================
 *  画点 (操作缓冲区, 2bit灰度)
 * ============================================================ */
void LCD_DrawPoint(uint8_t x, uint8_t y, uint8_t gray)
{
    uint8_t byte_addr, bit_pos, mask;

    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (gray > 3) gray = 3;

    byte_addr = x / 4;
    bit_pos   = 6 - 2 * (x % 4);
    mask      = 0x03 << bit_pos;

    g_framebuf[y][byte_addr] &= ~mask;
    g_framebuf[y][byte_addr] |= (gray << bit_pos);
}


/* ============================================================
 *  读点
 * ============================================================ */
uint8_t LCD_ReadPoint(uint8_t x, uint8_t y)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return 0;
    return (g_framebuf[y][x / 4] >> (6 - 2 * (x % 4))) & 0x03;
}


/* ============================================================
 *  刷屏: 缓冲区 → IC RAM (DMA 版本)
 *
 *  逐行 DMA 传输:
 *    设地址 → CS↓ + RS=1 → DMA 发 32B → CS↑
 *    重复 96 行
 *
 *  与旧版轮询对比:
 *    旧版: 每字节 TXE轮询 + BSY等待 ≈ 20周期/字节
 *    DMA:  配置DMA + 等TC中断 ≈ 5周期/字节 + 一次配置开销
 *    3072 字节净省约 45000 周期 ≈ 0.6ms
 * ============================================================ */
void LCD_Refresh(void)
{
    uint8_t row;

    LCD_Cmd1(UC_DC_DISP_OFF);

    for (row = 0; row < LCD_HEIGHT; row++)
    {
        LCD_SetAddress(row, 0);

        GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN);
        GPIO_SetBits(LCD_RS_PORT, LCD_RS_PIN);

        SPI_WriteDMA(g_framebuf[row], LCD_WIDTH / 4);  /* 32B DMA */

        GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN);
    }

    LCD_Cmd1(UC_DC_BW_ON);
}


/* ============================================================
 *  画直线 (Bresenham 算法)
 * ============================================================ */
void LCD_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t gray)
{
    int16_t dx  = abs((int)x1 - (int)x0);
    int16_t dy  = -abs((int)y1 - (int)y0);
    int16_t sx  = (x0 < x1) ? 1 : -1;
    int16_t sy  = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy;
    int16_t e2;

    for (;;)
    {
        LCD_DrawPoint(x0, y0, gray);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}


/* ============================================================
 *  矩形边框
 * ============================================================ */
void LCD_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t gray)
{
    LCD_DrawLine(x, y, x+w-1, y,         gray);
    LCD_DrawLine(x, y+h-1, x+w-1, y+h-1, gray);
    LCD_DrawLine(x, y, x, y+h-1,         gray);
    LCD_DrawLine(x+w-1, y, x+w-1, y+h-1, gray);
}


/* ============================================================
 *  填充矩形
 * ============================================================ */
void LCD_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t gray)
{
    uint8_t i, j;
    for (j = y; j < y + h; j++)
        for (i = x; i < x + w; i++)
            LCD_DrawPoint(i, j, gray);
}


/* ============================================================
 *  画圆 (中点圆算法, 8对称)
 * ============================================================ */
void LCD_DrawCircle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t gray)
{
    int16_t x = 0, y = r, d = 3 - 2 * r;
    while (x <= y)
    {
        LCD_DrawPoint(cx+x, cy+y, gray);
        LCD_DrawPoint(cx-x, cy+y, gray);
        LCD_DrawPoint(cx+x, cy-y, gray);
        LCD_DrawPoint(cx-x, cy-y, gray);
        LCD_DrawPoint(cx+y, cy+x, gray);
        LCD_DrawPoint(cx-y, cy+x, gray);
        LCD_DrawPoint(cx+y, cy-x, gray);
        LCD_DrawPoint(cx-y, cy-x, gray);
        if (d < 0) d += 4*x + 6;
        else { d += 4*(x-y) + 10; y--; }
        x++;
    }
}


/* ============================================================
 *  填充圆
 * ============================================================ */
void LCD_FillCircle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t gray)
{
    int16_t x = 0, y = r, d = 3 - 2 * r;
    while (x <= y)
    {
        LCD_DrawLine(cx-x, cy+y, cx+x, cy+y, gray);
        LCD_DrawLine(cx-x, cy-y, cx+x, cy-y, gray);
        LCD_DrawLine(cx-y, cy+x, cx+y, cy+x, gray);
        LCD_DrawLine(cx-y, cy-x, cx+y, cy-x, gray);
        if (d < 0) d += 4*x + 6;
        else { d += 4*(x-y) + 10; y--; }
        x++;
    }
}


/* ============================================================
 *  椭圆 (中点椭圆算法)
 * ============================================================ */
void LCD_DrawEllipse(uint8_t cx, uint8_t cy, uint8_t rx, uint8_t ry, uint8_t gray)
{
    int32_t x = 0, y = ry;
    int32_t rx2 = (int32_t)rx*rx, ry2 = (int32_t)ry*ry;
    int32_t tworx2 = 2*rx2, twory2 = 2*ry2;
    int32_t px = 0, py = tworx2 * y;
    int32_t p;

    p = ry2 - rx2*ry + rx2/4;
    while (px < py)
    {
        LCD_DrawPoint(cx+x, cy+y, gray);
        LCD_DrawPoint(cx-x, cy+y, gray);
        LCD_DrawPoint(cx+x, cy-y, gray);
        LCD_DrawPoint(cx-x, cy-y, gray);
        x++; px += twory2;
        if (p < 0) p += ry2 + px;
        else { y--; py -= tworx2; p += ry2 + px - py; }
    }

    p = ry2*(2*x+1)*(2*x+1)/4 + rx2*(y-1)*(y-1) - rx2*ry2;
    while (y >= 0)
    {
        LCD_DrawPoint(cx+x, cy+y, gray);
        LCD_DrawPoint(cx-x, cy+y, gray);
        LCD_DrawPoint(cx+x, cy-y, gray);
        LCD_DrawPoint(cx-x, cy-y, gray);
        y--; py -= tworx2;
        if (p > 0) p += rx2 - py;
        else { x++; px += twory2; p += rx2 - py + px; }
    }
}


#include "font.h"

/* ============================================================
 *  显示 8×16 ASCII 字符
 * ============================================================ */
void LCD_ShowChar(uint8_t x, uint8_t y, uint8_t ch,
                  uint8_t fg, uint8_t bg)
{
    uint8_t row, col, data;

    if (ch < 0x20 || ch > 0x7E) return;
    if (x > LCD_WIDTH - 8 || y > LCD_HEIGHT - 16) return;

    const uint8_t *p = FONT_8x16[ch - 0x20];

    for (row = 0; row < 16; row++)
    {
        data = p[row];
        for (col = 0; col < 8; col++)
        {
            LCD_DrawPoint(x + col, y + row,
                          (data & (0x80 >> col)) ? fg : bg);
        }
    }
}


/* ============================================================
 *  显示 ASCII 字符串
 * ============================================================ */
void LCD_ShowString(uint8_t x, uint8_t y, const char *str,
                    uint8_t fg, uint8_t bg)
{
    while (*str)
    {
        if (x > LCD_WIDTH - 8) { x = 0; y += 16; }
        if (y > LCD_HEIGHT - 16) break;
        LCD_ShowChar(x, y, *str, fg, bg);
        x += 8;
        str++;
    }
}


/* ============================================================
 *  显示 16×16 中文字符
 * ============================================================ */
void LCD_ShowChinese(uint8_t x, uint8_t y, uint16_t gb2312,
                     uint8_t fg, uint8_t bg)
{
    uint8_t row, col, idx, data;

    if (x > LCD_WIDTH - 16 || y > LCD_HEIGHT - 16) return;

    for (idx = 0; idx < FONT_CN16_COUNT; idx++)
        if (FONT_CN16[idx].code == gb2312) break;
    if (idx >= FONT_CN16_COUNT) return;

    const uint8_t *p = FONT_CN16[idx].data;

    for (row = 0; row < 16; row++)
    {
        for (col = 0; col < 16; col++)
        {
            data = (col < 8) ? p[row * 2] : p[row * 2 + 1];
            LCD_DrawPoint(x + col, y + row,
                          (data & (0x80 >> (col % 8))) ? fg : bg);
        }
    }
}


/* ============================================================
 *  中英文混合字符串
 * ============================================================ */
void LCD_ShowStringCN(uint8_t x, uint8_t y, const char *str,
                      uint8_t fg, uint8_t bg)
{
    uint16_t code;
    while (*str)
    {
        if (x > LCD_WIDTH - 16) { x = 0; y += 16; }
        if (y > LCD_HEIGHT - 16) break;

        if ((uint8_t)*str >= 0x80)
        {
            code = ((uint8_t)*str << 8) | (uint8_t)*(str + 1);
            LCD_ShowChinese(x, y, code, fg, bg);
            x += 16;
            str += 2;
        }
        else
        {
            LCD_ShowChar(x, y, *str, fg, bg);
            x += 8;
            str += 1;
        }
    }
}


/* ============================================================
 *  显示整数 (支持负数)
 * ============================================================ */
void LCD_ShowNum(uint8_t x, uint8_t y, int32_t num,
                 uint8_t fg, uint8_t bg)
{
    char buf[12];
    uint8_t i = 0;
    uint32_t n;

    if (num < 0) { LCD_ShowChar(x, y, '-', fg, bg); x += 8; n = -num; }
    else          n = num;

    do { buf[i++] = '0' + (n % 10); n /= 10; } while (n > 0);
    while (i > 0) { LCD_ShowChar(x, y, buf[--i], fg, bg); x += 8; }
}


/* ============================================================
 *  抗锯齿字符 (4灰度边缘平滑)
 * ============================================================ */
void LCD_ShowCharAA(uint8_t x, uint8_t y, uint8_t ch,
                    uint8_t fg, uint8_t bg, uint8_t edge)
{
    uint8_t row, col, rows[16];

    if (ch < 0x20 || ch > 0x7E) return;
    if (x > LCD_WIDTH - 8 || y > LCD_HEIGHT - 16) return;

    const uint8_t *p = FONT_8x16[ch - 0x20];
    for (row = 0; row < 16; row++) rows[row] = p[row];

    for (row = 0; row < 16; row++)
    {
        for (col = 0; col < 8; col++)
        {
            if (rows[row] & (0x80 >> col))
            {
                LCD_DrawPoint(x + col, y + row, fg);
            }
            else
            {
                uint8_t aa = 0;
                if (col > 0 && (rows[row]     & (0x80 >> (col-1)))) aa = 1;
                if (col < 7 && (rows[row]     & (0x80 >> (col+1)))) aa = 1;
                if (row > 0 && (rows[row - 1] & (0x80 >> col)))     aa = 1;
                if (row <15 && (rows[row + 1] & (0x80 >> col)))     aa = 1;
                LCD_DrawPoint(x + col, y + row, aa ? edge : bg);
            }
        }
    }
}


/* ============================================================
 *  抗锯齿字符串
 * ============================================================ */
void LCD_ShowStringAA(uint8_t x, uint8_t y, const char *str,
                      uint8_t fg, uint8_t bg, uint8_t edge)
{
    while (*str)
    {
        if (x > LCD_WIDTH - 8) { x = 0; y += 16; }
        if (y > LCD_HEIGHT - 16) break;
        LCD_ShowCharAA(x, y, *str, fg, bg, edge);
        x += 8;
        str++;
    }
}


/* ============================================================
 *  MSB-first → LSB-first 2bit块反转
 * ============================================================ */
static uint8_t gray4_msb_to_lsb(uint8_t b)
{
    return ((b & 0x03) << 6) |
           ((b & 0x0C) << 2) |
           ((b & 0x30) >> 2) |
           ((b & 0xC0) >> 6);
}


/* ============================================================
 *  显示图片 — DMA 版本
 *
 *  (x, y): 左上角坐标, x 必须是 4 的倍数
 *  w × h:  图片尺寸, w 必须是 4 的倍数
 *  img:    图片数据
 *  fmt:    IMG_FMT_GRAY4_LSB / IMG_FMT_GRAY4_MSB / IMG_FMT_MONO
 *
 *  UC1617S RAM 格式 (LSB first):
 *    bit[1:0] = 像素0 (最左), bit[3:2] = 像素1,
 *    bit[5:4] = 像素2,        bit[7:6] = 像素3 (最右)
 * ============================================================ */
void LCD_ShowImage(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                   const uint8_t *img, uint8_t fmt)
{
    uint8_t row;
    uint16_t idx = 0;
    uint8_t byte_val;
    uint8_t col_start = x / 4;
    uint8_t col_count = w / 4;
    uint8_t mono_hi, mono_lo;
    uint8_t line_buf[LCD_WIDTH / 4];  /* 行转换缓冲, 最大 32B */
    uint8_t buf_idx;
    uint8_t col;

    if (x % 4 != 0) return;
    if (w % 4 != 0) return;
    if (x + w > LCD_WIDTH) return;
    if (y + h > LCD_HEIGHT) return;

    LCD_Cmd1(UC_DC_DISP_OFF);

    for (row = 0; row < h; row++)
    {
        LCD_SetAddress(y + row, col_start);

        GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN);
        GPIO_SetBits(LCD_RS_PORT, LCD_RS_PIN);

        if (fmt == IMG_FMT_GRAY4_LSB)
        {
            /* 已是IC格式, 直接 DMA 发送 */
            SPI_WriteDMA(&img[idx], col_count);
            idx += col_count;
        }
        else
        {
            /*
             * 需要格式转换, 先转到行缓冲区, 再 DMA
             *
             * 注意: GRAY4 每字节 = 4像素 = 1列 (col++)
             *       MONO   每字节 = 8像素 = 2列 (col += 2)
             *       所以 MONO 用 while 循环手动控制步进
             */
            buf_idx = 0;

            if (fmt == IMG_FMT_MONO)
            {
                /* MONO: 1字节单色(8像素) → 2字节灰度(8像素) */
                col = 0;
                while (col < col_count)
                {
                    byte_val = img[idx++];

                    /* 高半字节: px0~px3 → 第1个灰度字节 */
                    mono_hi = 0;
                    if (byte_val & 0x80) mono_hi |= 0x03;  /* px0 → bit[1:0] */
                    if (byte_val & 0x40) mono_hi |= 0x0C;  /* px1 → bit[3:2] */
                    if (byte_val & 0x20) mono_hi |= 0x30;  /* px2 → bit[5:4] */
                    if (byte_val & 0x10) mono_hi |= 0xC0;  /* px3 → bit[7:6] */
                    line_buf[buf_idx++] = mono_hi;
                    col++;

                    if (col < col_count)
                    {
                        /* 低半字节: px4~px7 → 第2个灰度字节 */
                        mono_lo = 0;
                        if (byte_val & 0x08) mono_lo |= 0x03;  /* px4 → bit[1:0] */
                        if (byte_val & 0x04) mono_lo |= 0x0C;  /* px5 → bit[3:2] */
                        if (byte_val & 0x02) mono_lo |= 0x30;  /* px6 → bit[5:4] */
                        if (byte_val & 0x01) mono_lo |= 0xC0;  /* px7 → bit[7:6] */
                        line_buf[buf_idx++] = mono_lo;
                        col++;
                    }
                }
            }
            else
            {
                /* GRAY4_MSB: 逐字节转换 */
                for (col = 0; col < col_count; col++)
                {
                    byte_val = img[idx++];
                    line_buf[buf_idx++] = gray4_msb_to_lsb(byte_val);
                }
            }

            SPI_WriteDMA(line_buf, buf_idx);
        }

        GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN);
    }

    LCD_Cmd1(UC_DC_BW_ON);
}
