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
 *  发送单字节命令 (C/D=0, CS独立事务)
 *  等价于 I2C 的 iic_send_cmd()
 * ============================================================ */
static void LCD_Cmd1(uint8_t cmd)
{
    GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN);    /* CS↓, 选中芯片     */
    GPIO_ResetBits(LCD_RS_PORT, LCD_RS_PIN);    /* RS=0, 本次为命令  */
    SPI_WriteByte(cmd);                          /* 发送命令字节       */
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
    GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN);      /* CS↑, 释放芯片     */
}

/* ============================================================
 *  发送双字节命令 (C/D=0, 一次CS事务内发完)
 *  适用于 Set VBIAS、Set APC、Set CEN 等双字节命令
 * ============================================================ */
static void LCD_Cmd2(uint8_t cmd, uint8_t param)
{
    GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN);
    GPIO_ResetBits(LCD_RS_PORT, LCD_RS_PIN);
    SPI_WriteByte(cmd);          /* 第1字节: 命令码 */
    SPI_WriteByte(param);        /* 第2字节: 参数   */
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
    GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN);
}

/* ============================================================
 *  设置 RAM 写入地址
 *
 *  UC1617S 地址寄存器:
 *    RA[6:0] = 行地址 0~127   (对应128个COM电极的RAM行)
 *    CA[4:0] = 列地址 0~31    (每页4个SEG电极 = 4像素列)
 *
 *  命令顺序 (与 I2C 参考代码一致):
 *    ① 0x60|RA[3:0]  行地址低4位
 *    ② 0x70|RA[6:4]  行地址高3位
 *    ③ 0x00|CA[4:0]  列地址 (页地址)
 * ============================================================ */
static void LCD_SetAddress(uint8_t row, uint8_t col)
{
    LCD_Cmd1(UC_SET_RA_LSB | (row & 0x0F));        /* 0x6X: RA[3:0]  */
    LCD_Cmd1(UC_SET_RA_MSB | ((row >> 4) & 0x07)); /* 0x7X: RA[6:4]  */
    LCD_Cmd1(UC_SET_CA     | (col & 0x1F));             /* 0x0X: CA[4:0]  */
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

    GPIO_SetBits(GPIOA, GPIO_Pin_4);       /* CS  = 1 (不选中)   */
    GPIO_SetBits(GPIOA, GPIO_Pin_2);       /* RST = 1 (释放复位) */
    GPIO_ResetBits(GPIOA, GPIO_Pin_1);     /* BL  = 0 (背光关)   */
}

/* ============================================================
 *  SPI1 初始化
 *  仅发送模式, Mode0 (CPOL=0, CPHA=0), MSB first
 *  时钟: 72MHz / 16 = 4.5MHz (满足 UC1617S ≤7.14MHz 要求)
 * ============================================================ */
static void LCD_SPI_Init(void)
{
    SPI_InitTypeDef spi;

    spi.SPI_Direction         = SPI_Direction_1Line_Tx;  /* 仅发送    */
    spi.SPI_Mode              = SPI_Mode_Master;          /* 主机模式  */
    spi.SPI_DataSize          = SPI_DataSize_8b;          /* 8位数据   */
    spi.SPI_CPOL              = SPI_CPOL_Low;             /* 空闲低电平*/
    spi.SPI_CPHA              = SPI_CPHA_1Edge;           /* 第1边沿采样 */
    spi.SPI_NSS               = SPI_NSS_Soft;             /* 软件控制CS */
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; /* 4.5MHz    */
    spi.SPI_FirstBit          = SPI_FirstBit_MSB;         /* 高位先发  */
    spi.SPI_CRCPolynomial     = 7;
    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);
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
 *
 *  参考 I2C 能工作版本的命令序列, 用宏重写并加详细注释
 *
 *  执行顺序:
 *    1. 硬件复位 (RST脉冲)
 *    2. 模拟配置 (APC/温度/灰度/对比度)
 *    3. 显示方向 (MY/MX/SL)
 *    4. 显示窗口 (WPC/WPP)
 *    5. 清屏后开显示
 * ============================================================ */
void LCD_Init(void)
{
    LCD_GPIO_Init();
    LCD_SPI_Init();

    /* ======================================================
     *  硬件复位
     *
     *  UC1617S 复位要求 (手册 Power-Up Sequence):
     *    RST 先高 → 拉低 ≥3μs → 拉高 → 等待 ≥150ms
     *    复位后 IC 自动执行 Power-ON Reset
     *    所有寄存器恢复默认值, RAM 内容不受影响
     * ====================================================== */
    GPIO_SetBits(GPIOA, GPIO_Pin_2);       /* RST = 1 (确保初始高)  */
    delay_us(100);                          /* 保持 100μs            */
    GPIO_ResetBits(GPIOA, GPIO_Pin_2);     /* RST = 0 (触发复位)    */
    delay_us(10);                           /* 低电平 ≥ 10μs (>3μs) */
    GPIO_SetBits(GPIOA, GPIO_Pin_2);       /* RST = 1 (释放复位)    */
    delay_ms(170);                          /* 等待 ≥ 170ms (>150ms) */

    /* ======================================================
     *  模拟参数配置
     * ====================================================== */

    /*
     * 0x31 0x00 — Set APC (Advanced Program Control)
     *   命令8: 双字节命令
     *   D[7:5]=001, D[4:0]=R (寄存器选择 0~2)
     *   0x31: D[7:0]=00110001 → R=0, 设置 APC[0] 寄存器
     *   0x00: APC[0]=0x00
     *   功能: 内部模拟性能配置 (厂商推荐值)
     *   参考: I2C 能工作版本使用相同参数
     */
    LCD_Cmd2(0x31, 0x00);

    /*
     * 0x24 — Set Temperature Compensation
     *   命令5: 单字节命令
     *   D[7:0]=00100100 → D[3:2]=00(固定), D[1:0]=TC[1:0]=00
     *   TC[1:0]=00: 补偿系数 -0.00%/°C (无温度补偿)
     *   手册: 四种可选系数:
     *     00: -0.00%/°C   01: -0.10%/°C
     *     10: -0.15%/°C   11: -0.05%/°C
     */
    LCD_Cmd1(0x24);

    /*
     * 0xD2 0xEB — Set LCD Gray Shade (灰度等级)
     *   命令23+24 的组合值, 控制4级灰度的电压分离
     *
     *   0xD2 = 0b11010010 → 命令23: Set Gray Shade 1
     *     D[7:2]=110100(命令码), D[1:0]=LC[6:5]=10
     *     LC[6:5]=10: Gray1 Level 3, 强度=15 (0~36)
     *     控制灰度"01"(浅灰)和"10"(深灰)之间的电压分离
     *
     *   0xEB = 0b11101011 → 命令24: Set Gray Shade 2
     *     D[7:2]=111010(命令码), D[1:0]=LC[8:7]=11
     *     LC[8:7]=11: Gray2 Level 6, 强度=27 (0~36)
     *
     *   手册灰度数据映射:
     *     数据 00(0x00) → 白 (最亮)
     *     数据 01(0x01) → 浅灰 (由LC[6:5]控制)
     *     数据 10(0x02) → 深灰 (由LC[8:7]控制)
     *     数据 11(0x03) → 黑 (最暗)
     */
    LCD_Cmd2(0xD2, 0xEB);

    /*
     * 0x81 0x50 — Set VBIAS Potentiometer (对比度)
     *   命令13: 双字节命令
     *   0x81 = 0b10000001 → 命令码 (Set VMAX Potentiometer)
     *   0x50 = 80 → PM[7:0] = 80
     *
     *   手册公式: VLCD = (CF0 + CPM × PM) × 温补系数
     *   PM 范围: 0~193, 值越大 VLCD 越高, 对比度越强
     *
     *   BR=6 时 (CF0=6.027V, CPM=21.0mV):
     *     PM=0   → VLCD ≈ 6.03V  (最小)
     *     PM=80  → VLCD ≈ 7.71V  (当前)
     *     PM=193 → VLCD ≈ 10.08V (最大)
     *
     *   调试: 改此值可调节对比度, 偏大则过黑, 偏小则偏淡
     */
    LCD_Cmd2(0x81, 80);

    /* ======================================================
     *  显示方向与滚动配置
     * ====================================================== */

    /*
     * 0xC4 — Set LCD Mapping Control
     *   命令21: 单字节命令
     *   D[7:0] = 11000100
     *     D[7:3] = 11000 (命令码)
     *     D[2] = MY = 1  → COM行方向镜像 (上下翻转)
     *     D[1] = MX = 0  → SEG列方向正常 (不左右镜像)
     *     D[0] = LC0 = 0 → 部分显示模式中固定行区域不显示
     *
     *   MY=1 时 (手册 RAM ADDRESS GENERATION):
     *     COM扫描方向反转, RAM地址从高到低递减
     *     效果: 显示行0(上)与行95(下)互换
     *     MY 立即生效, 不需要重写RAM
     *
     *   MX=0 时:
     *     CA写入映射正常: CA → CA
     *     MX=1时: CA → 31-CA (左右镜像, 需重写RAM才生效)
     *
     *   可选值:
     *     0xC0: MY=0 MX=0 (正向)
     *     0xC2: MY=0 MX=1 (左右镜像)
     *     0xC4: MY=1 MX=0 (上下翻转) ← 当前
     *     0xC6: MY=1 MX=1 (180°旋转)
     */
    LCD_Cmd1(0xC4);

    /*
     * 0x40 — Set Scroll Line LSB
     *   命令9: 单字节命令
     *   D[7:0] = 01000000
     *     D[7:6] = 01 (命令前缀)
     *     D[5:4] = 00 (固定)
     *     D[3:0] = SL[3:0] = 0000
     *   SL 低4位 = 0
     */
    LCD_Cmd1(0x40);

    /*
     * 0x56 — Set Scroll Line MSB (粘合玻璃偏移)
     *   命令10: 单字节命令
     *   D[7:0] = 01010110
     *     D[7:4] = 0101 → 注意: 实际命令码应为 0110 (0x60)
     *     这里用 0x56, 低3位与SL高3位组合使用
     *
     *   实际效果: SL 高位与该模块玻璃偏移粘合
     *   与 0xC4 (MY=1) 配合, 使 RAM行0 映射到正确的 COM 位置
     *
     *   手册: SL 寄存器控制显示滚动
     *     SL=0: 不滚动, 从RAM行0开始显示
     *     SL>0: 整体向上滚动 SL 行
     *     范围: 0~127
     */
    LCD_Cmd1(0x56);

    /* ======================================================
     *  显示窗口设置
     *
     *  手册 "Window Program":
     *    定义 RAM 写入的有效范围
     *    CA/RA 在窗口内自动循环
     *    配合 AC[3]=1 (0xF9) 使能窗口编程
     *
     *  当前设置: 全屏窗口
     *    列: Page 0~31 = 128像素列 (每页4个SEG = 4像素)
     *    行: 0~127 = 全部128行RAM (玻璃只显示96行)
     * ====================================================== */

    /*
     * 0xF4 0x00 — Set WPC0 (窗口起始页列地址)
     *   命令30: 双字节命令
     *   D[7:0] = 11110100 (命令码)
     *   第2字节: WPC0[4:0] = 0x00 = 0
     *   页列起始: 第0页 (SEG1~SEG4, 像素列0~3)
     */
    LCD_Cmd2(0xF4, 0);

    /*
     * 0xF6 0x1F — Set WPC1 (窗口结束页列地址)
     *   命令32: 双字节命令
     *   D[7:0] = 11110110 (命令码)
     *   第2字节: WPC1[4:0] = 31 (0x1F)
     *   页列结束: 第31页 (SEG125~SEG128, 像素列124~127)
     *   覆盖: 32页 × 4像素/页 = 128列
     */
    LCD_Cmd2(0xF6, 31);

    /*
     * 0xF5 0x00 — Set WPP0 (窗口起始行地址)
     *   命令31: 双字节命令
     *   D[7:0] = 11110101 (命令码)
     *   第2字节: WPP0[6:0] = 0
     *   行起始: 第0行
     */
    LCD_Cmd2(0xF5, 0);

    /*
     * 0xF7 0x7F — Set WPP1 (窗口结束行地址)
     *   命令33: 双字节命令
     *   D[7:0] = 11110111 (命令码)
     *   第2字节: WPP1[6:0] = 127 (0x7F)
     *   行结束: 第127行
     *   覆盖全部128行RAM (玻璃只有96行, 多余的不显示)
     */
    LCD_Cmd2(0xF7, 127);

    /*
     * 0xF9 — Enable Window Program
     *   命令34: 单字节命令
     *   D[7:0] = 11111001 → AC[3] = 1
     *   手册: 使能后, CA/RA 自增和换行被限制在
     *         WPC0~WPC1, WPP0~WPP1 范围内
     *         超出窗口边界时根据 WA/RA方向 自动回绕
     *
     *   禁用: 0xF8 (AC[3]=0)
     *   修改窗口前应先禁用, 改完再使能
     */
    LCD_Cmd1(0xF9);

    /* ======================================================
     *  清屏 + 开显示
     * ====================================================== */

    /*
     * 清屏: 缓冲区全填 0x00
     *   每字节 00-00-00-00 = 4个白色像素 (灰度值0)
     *   96行 × 32字节 = 3072字节, 全白
     */
    LCD_Clear();

    /*
     * 刷屏: 将缓冲区数据写入 IC RAM
     *   格式完全一致 (2bit/像素, MSB first)
     *   零转换直接搬运, 利用 WA 自动换行
     */
    LCD_Refresh();

    /*
     * 0xAF — Set Display Enable
     *   命令20: 单字节命令
     *   D[7:0] = 10101111
     *     D[7:2] = 101011 (命令码)
     *     D[1:0] = DC[3:2] = 10
     *       DC[2] = 1: Display ON (退出Sleep, 开启COM/SEG驱动)
     *       DC[3] = 0: B/W 模式
     *
     *   手册: DC[2]=1后 IC 启动内部电荷泵
     *     产生 5~10ms 浪涌电流
     *     此期间不要发命令或数据
     *
     *   可选值:
     *     0xAE (10101110): DC[3:2]=00 → 关显示 (Sleep)
     *     0xAF (10101111): DC[3:2]=10 → B/W模式 + 开显示 ← 当前
     *     0x2F (00101111): DC[3:2]=11 → 4灰度模式 + 开显示
     */
    LCD_Cmd1(0x2F);
    delay_ms(12);                           /* 等电荷泵稳定 */

    /* 打开背光 */
    LCD_BacklightOn();
}


/* ============================================================
 *  清屏 (缓冲区 → 全白)
 * ============================================================ */
void LCD_Clear(void)
{
    uint16_t i;
    for (i = 0; i < sizeof(g_framebuf); i++)
        ((uint8_t *)g_framebuf)[i] = 0x00;  /* 00-00-00-00 = 4白色 */
}

/* ============================================================
 *  直接填 RAM (绕过缓冲区, 用于快速测试)
 *
 *  参数 data 为填充字节, 每字节4像素:
 *    0x00 = 00-00-00-00 = 全白
 *    0x55 = 01-01-01-01 = 全浅灰
 *    0xAA = 10-10-10-10 = 全深灰
 *    0xFF = 11-11-11-11 = 全黑
 * ============================================================ */
void LCD_FillScreen(uint8_t data)
{
    uint8_t row;
    uint16_t i;

    LCD_Cmd1(0xAE);                         /* 关显示 */

    for (row = 0; row < LCD_HEIGHT; row++)  /* ★ 逐行 */
    {
        LCD_SetAddress(row, 0);             /* ★ 每行重设地址 */

        GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN);
        GPIO_SetBits(LCD_RS_PORT, LCD_RS_PIN);

        for (i = 0; i < (LCD_WIDTH / 4); i++)   /* 32 字节 */
            SPI_WriteByte(data);

        while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
        GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN);
    }

    LCD_Cmd1(0xAF);                         /* ★ 0xAF 不是 0x2F */
}

/* ============================================================
 *  画点 (操作缓冲区, 2bit灰度)
 *
 *  计算该像素在缓冲区中的字节地址和位位置:
 *    byte_addr = x / 4          第几个字节 (0~31)
 *    pixel_idx = x % 4          字节内第几个像素 (0~3)
 *    bit_pos   = 6 - pixel×2    该像素的起始bit位置
 *                pixel0→bit[7:6], pixel1→bit[5:4],
 *                pixel2→bit[3:2], pixel3→bit[1:0]
 *
 *  写入步骤:
 *    1. 清除目标2bit: AND ~(0x03 << bit_pos)
 *    2. 写入灰度值:   OR  (gray  << bit_pos)
 * ============================================================ */
void LCD_DrawPoint(uint8_t x, uint8_t y, uint8_t gray)
{
    uint8_t byte_addr, bit_pos, mask;

    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (gray > 3) gray = 3;

    byte_addr = x / 4;
    bit_pos   = 2 * (x % 4);       /* ★ LSB first: 0/2/4/6 */
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
    return (g_framebuf[y][x / 4] >> (2 * (x % 4))) & 0x03;
}


/* ============================================================
 *  刷屏: 缓冲区 → IC RAM
 *
 *  缓冲区格式与 UC1617S RAM 完全一致:
 *    2bit/像素, MSB first, 行优先
 *  因此可以零转换直接搬运
 *
 *  流程:
 *    1. 关显示 (避免刷新中的闪烁/撕裂)
 *    2. 设起始地址 RA=0, CA=0
 *    3. 利用 WA 自动换行, 连续发送 3072 字节
 *    4. 开显示
 * ============================================================ */
void LCD_Refresh(void)
{
    uint8_t row;
    uint16_t i;

    LCD_Cmd1(0xAE);

    for (row = 0; row < LCD_HEIGHT; row++)  /* ★ 逐行 */
    {
        LCD_SetAddress(row, 0);             /* ★ 每行重设地址 */

        GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN);
        GPIO_SetBits(LCD_RS_PORT, LCD_RS_PIN);

        for (i = 0; i < (LCD_WIDTH / 4); i++)
            SPI_WriteByte(g_framebuf[row][i]);

        while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
        GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN);
    }

    LCD_Cmd1(0xAF);                         /* ★ 0xAF 不是 0x2F */
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
    LCD_DrawLine(x, y, x+w-1, y,         gray);       /* 上边 */
    LCD_DrawLine(x, y+h-1, x+w-1, y+h-1, gray);       /* 下边 */
    LCD_DrawLine(x, y, x, y+h-1,         gray);       /* 左边 */
    LCD_DrawLine(x+w-1, y, x+w-1, y+h-1, gray);       /* 右边 */
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
 *
 *  坐标: (x, y) = 左上角
 *  fg/bg: GRAY_WHITE(0), GRAY_LIGHT(1), GRAY_DARK(2), GRAY_BLACK(3)
 *
 *  字库: MSB-first (bit7=左), 每行1字节, 共16行
 *  取位: data & (0x80 >> col), col=0→bit7(最左)
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
 *  显示 ASCII 字符串 (自动换行)
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
 *
 *  gb2312: GB2312内码 (如 0xC4E3 = "你")
 *  字库: MSB-first (bit7=左), 每行2字节(左8+右8), 共16行
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
 *  ASCII (< 0x80): 8像素宽
 *  GB2312 (>= 0x80): 与下一字节组成内码, 16像素宽
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
 *  fg=前景, bg=背景, edge=边缘过渡灰度
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
 *
 *  Img2Lcd 输出: bit[7:6]=px0, bit[5:4]=px1, bit[3:2]=px2, bit[1:0]=px3
 *  IC RAM 格式:  bit[1:0]=px0, bit[3:2]=px1, bit[5:4]=px2, bit[7:6]=px3
 *
 *  例: 0xE4 (11-10-01-00) → 0x1B (00-01-10-11)
 * ============================================================ */
static uint8_t gray4_msb_to_lsb(uint8_t b)
{
    return ((b & 0x03) << 6) |
           ((b & 0x0C) << 2) |
           ((b & 0x30) >> 2) |
           ((b & 0xC0) >> 6);
}

/* ============================================================
 *  显示图片 — 直接写 IC RAM
 *
 *  (x, y): 左上角坐标, x 必须是 4 的倍数
 *  w × h:  图片尺寸, w 必须是 4 的倍数
 *  img:    图片数据
 *  fmt:    数据格式
 *    IMG_FMT_GRAY4_LSB(0): 已是IC格式 (LSB first)
 *    IMG_FMT_GRAY4_MSB(1): Img2Lcd格式 (MSB first), 自动转换
 *    IMG_FMT_MONO(2):      1bit单色 (MSB first), 自动转为4灰度
 *
 *  UC1617S RAM 格式 (LSB first):
 *    bit[1:0] = 像素0 (最左), bit[3:2] = 像素1,
 *    bit[5:4] = 像素2,        bit[7:6] = 像素3 (最右)
 * ============================================================ */
void LCD_ShowImage(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                   const uint8_t *img, uint8_t fmt)
{
    uint8_t row, col;
    uint16_t idx = 0;
    uint8_t byte_val;
    uint8_t col_start = x / 4;
    uint8_t col_count = w / 4;
    uint8_t mono_hi, mono_lo;

    /* 边界检查 */
    if (x % 4 != 0) return;
    if (w % 4 != 0) return;
    if (x + w > LCD_WIDTH) return;
    if (y + h > LCD_HEIGHT) return;

    LCD_Cmd1(0xAE);     /* 关显示 */

    for (row = 0; row < h; row++)
    {
        LCD_SetAddress(y + row, col_start);

        GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN);
        GPIO_SetBits(LCD_RS_PORT, LCD_RS_PIN);

        if (fmt == IMG_FMT_MONO)
        {
            /*
             * MONO: 1字节单色(8像素) → 2字节灰度(8像素)
             * 用 while 手动控制步进, 避免 for 循环额外 col++
             */
            col = 0;
            while (col < col_count)
            {
                byte_val = img[idx++];

                /* 高半字节: px0~px3 → 第1个灰度字节 */
                mono_hi = 0;
                if (byte_val & 0x80) mono_hi |= 0x03;
                if (byte_val & 0x40) mono_hi |= 0x0C;
                if (byte_val & 0x20) mono_hi |= 0x30;
                if (byte_val & 0x10) mono_hi |= 0xC0;
                SPI_WriteByte(mono_hi);
                col++;

                if (col < col_count)
                {
                    /* 低半字节: px4~px7 → 第2个灰度字节 */
                    mono_lo = 0;
                    if (byte_val & 0x08) mono_lo |= 0x03;
                    if (byte_val & 0x04) mono_lo |= 0x0C;
                    if (byte_val & 0x02) mono_lo |= 0x30;
                    if (byte_val & 0x01) mono_lo |= 0xC0;
                    SPI_WriteByte(mono_lo);
                    col++;
                }
            }
        }
        else
        {
            /* GRAY4: 逐字节发送 */
            for (col = 0; col < col_count; col++)
            {
                byte_val = img[idx++];

                if (fmt == IMG_FMT_GRAY4_MSB)
                    byte_val = gray4_msb_to_lsb(byte_val);

                SPI_WriteByte(byte_val);
            }
        }

        while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
        GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN);
    }

    LCD_Cmd1(0xAF);     /* 开显示 */
}





