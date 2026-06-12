#ifndef __LCD_SPI_H
#define __LCD_SPI_H

#include "stm32f10x.h"
#include "stdint.h"

/* ============================================================
 *  屏幕参数
 * ============================================================ */
#define LCD_WIDTH           128
#define LCD_HEIGHT          96

/* ============================================================
 *  引脚定义
 * ============================================================ */
#define LCD_CS_PORT         GPIOA
#define LCD_CS_PIN          GPIO_Pin_4

#define LCD_RS_PORT         GPIOA
#define LCD_RS_PIN          GPIO_Pin_3

#define LCD_RST_PORT        GPIOA
#define LCD_RST_PIN         GPIO_Pin_2

#define LCD_BL_PORT         GPIOA
#define LCD_BL_PIN          GPIO_Pin_1

/* ============================================================
 *  UC1617S 显示数据 RAM 格式
 *  ┌──────────────────────────────────────────────────────┐
 *  │  行地址 RA: 0~127   (128行)                          │
 *  │  列地址 CA: 0~31    (32页, 每页4个SEG=4像素列)       │
 *  │  每字节 = 4像素, 每像素2bit                          │
 *  │  bit[7:6]=像素0  bit[5:4]=像素1                      │
 *  │  bit[3:2]=像素2  bit[1:0]=像素3                      │
 *  │  00=白(最亮) 01=浅灰 10=深灰 11=黑(最暗)             │
 *  └──────────────────────────────────────────────────────┘
 * ============================================================ */

/* ============================================================
 *  UC1617S 寄存器 / 命令定义 (来自数据手册 v1.2)
 * ============================================================ */

/* ---- 寄存器: CA (Page Column Address) ---- */
#define UC_SET_CA           0x00    /* 命令4: Set Page_C Address        */
                                    /* D[7:5]=000, D[4:0]=CA[4:0]       */
                                    /* 范围: 0~31                        */
/* ---- 寄存器: RA (Row Address) ---- */
#define UC_SET_RA_LSB       0x60    /* 命令11: Set Row Address LSB       */
                                    /* D[7:4]=0110, D[3:0]=RA[3:0]      */
#define UC_SET_RA_MSB       0x70    /* 命令12: Set Row Address MSB       */
                                    /* D[7:4]=0111, D[2:0]=RA[6:4]      */
                                    /* RA 范围: 0~127                    */

/* ---- 寄存器: SL (Scroll Line) ---- */
#define UC_SET_SL_LSB       0x40    /* 命令9: Set Scroll Line LSB        */
                                    /* D[7:6]=01, D[3:0]=SL[3:0]        */
#define UC_SET_SL_MSB       0x60    /* 命令10: Set Scroll Line MSB       */
                                    /* D[7:4]=0110, D[2:0]=SL[6:4]      */
                                    /* SL 范围: 0~127                    */
                                    /* 功能: 滚动显示图像, 0=无滚动      */

/* ---- 寄存器: CR (Return Page_C Address) ---- */
                                    /* D[7:5]=000, D[4:0]=CR[4:0]       */
                                    /* 范围: 0~31, 用于光标实现          */

/* ---- 寄存器: TC (Temperature Compensation) ---- */
#define UC_SET_TC           0x29    /* 命令5: Set Temp. Compensation     */
                                    /* D[7:3]=00101, D[1:0]=TC[1:0]     */
#define UC_TC_000           0x00    /*   TC=00: -0.00%/°C (无补偿)      */
#define UC_TC_011           0x01    /*   TC=01: -0.10%/°C               */
#define UC_TC_102           0x02    /*   TC=10: -0.15%/°C               */
#define UC_TC_113           0x03    /*   TC=11: -0.05%/°C               */

/* ---- 寄存器: PC (Panel Loading / Pump Control) ---- */
#define UC_SET_PANEL_LOAD   0x28    /* 命令6: Set Panel Loading          */
                                    /* D[7:3]=00101, D[1:0]=PC[1:0]     */
#define UC_PL_LE6           0x00    /*   PC[1:0]=00: LCD≤6nF            */
#define UC_PL_6TO9          0x01    /*   PC[1:0]=01: LCD 6~9nF          */
#define UC_PL_9TO13         0x02    /*   PC[1:0]=10: LCD 9~13nF         */
#define UC_PL_13TO18        0x03    /*   PC[1:0]=11: LCD 13~18nF        */

#define UC_SET_PUMP         0x2F    /* 命令7: Set Pump Control           */
                                    /* D[7:3]=00101, D[2:1]=PC[3:2]     */
#define UC_PUMP_EXT         0x00    /*   PC[3:2]=00: 外部 VLCO           */
#define UC_PUMP_INT         0x03    /*   PC[3:2]=11: 内部 VLCO (标准)   */

/* ---- 寄存器: PM (VBIAS Potentiometer) ---- */
#define UC_SET_VBIAS        0x81    /* 命令13: Set VMAX Potentiometer    */
                                    /* D[7]=1, D[6:0]=PM[7:0]           */
                                    /* (双字节命令, 第2字节为PM值)        */
                                    /* 范围: 0~193                       */
                                    /* 对比度 = (CF0 + CPM×PM) × 温补系数 */

/* ---- 寄存器: APC (Advanced Program Control) ---- */
                                    /* 命令8: Set Adv. Program Control   */
                                    /* D[7:5]=000, D[4:0]=R (0~2)       */
                                    /* (双字节命令, 第2字节=APC[R])      */

/* ---- 寄存器: AC (RAM Address Control) ---- */
#define UC_SET_AC           0x88    /* 命令15: Set RAM Address Control   */
                                    /* D[7:3]=10001, D[2:0]=AC[2:0]     */
#define UC_AC_WA            0x01    /*   AC[0] WA: 自动换行 ON           */
#define UC_AC_RA_FIRST      0x02    /*   AC[1]=1: RA先增(纵向)           */
#define UC_AC_CA_FIRST      0x00    /*   AC[1]=0: CA先增(横向/页优先)    */
#define UC_AC_RA_DEC        0x04    /*   AC[2]=1: RA递减                 */
#define UC_AC_RA_INC        0x00    /*   AC[2]=0: RA递增                 */
#define UC_AC_WIN_EN        0x08    /*   AC[3]=1: 窗口编程使能           */
                                    /* (命令34中AC[3]独立控制)            */

/* ---- 寄存器: DC (Display Control) ---- */
#define UC_DC_INVERSE       0xA7    /* 命令19: 像素取反 DC[0]=1          */
                                    /* D[7:2]=101001, D[0]=DC[0]        */
#define UC_DC_NORMAL        0xA6    /*   DC[0]=0: 正常显示               */
#define UC_DC_ALLPIXEL_ON   0xA5    /* 命令18: 全像素开 DC[1]=1          */
                                    /* D[7:2]=101001, D[1]=DC[1]        */
#define UC_DC_ALLPIXEL_OFF  0xA4    /*   DC[1]=0: 关闭全像素             */
#define UC_DC_DISP_OFF      0xAE    /* 命令20: 关显示 (睡眠) DC[2]=0     */
                                    /* D[7:2]=101011, D[1:0]=DC[3:2]    */
#define UC_DC_BW_ON         0xAF    /* 命令20: B/W模式开显示              */
                                    /* DC[3:2]=10: B/W + Display ON      */
#define UC_DC_GRAY_ON       0x2F    /* 命令20: 4灰度模式开显示            */
                                    /* DC[3:2]=11: 4-Shade + Display ON  */
                                    /* DC[2]=0 时 IC 进入 Sleep 模式     */

/* ---- 寄存器: LC (LCD Control) ---- */
#define UC_SET_LC0          0xC0    /* 命令21: Set LCD Mapping Control   */
                                    /* D[7:3]=11000, D[2:0]=LC[2:0]     */
                                    /* D[2]=MY, D[1]=MX, D[0]=LC0       */
#define UC_LC_MY            0x04    /*   LC[2] MY=1: COM行方向镜像       */
                                    /*   MY=0: 正向, MY=1: 上下翻转     */
#define UC_LC_MX            0x02    /*   LC[1] MX=1: SEG列方向镜像       */
                                    /*   MX=0: 正向, MX=1: 左右翻转     */
                                    /*   MX改变CA写入映射, 需重写RAM生效  */
                                    /*   MY改变COM扫描顺序, 立即生效     */

#define UC_SET_GRAY1        0xD0    /* 命令23: Set LCD Gray Shade 1      */
                                    /* D[7:2]=110100, D[1:0]=LC[6:5]    */
                                    /* 控制灰度"01"和"10"的电压分离       */
#define UC_GRAY1_L1         0x00    /*   LC[6:5]=00: Level 1, 强度=9    */
#define UC_GRAY1_L2         0x01    /*   LC[6:5]=01: Level 2, 强度=12   */
#define UC_GRAY1_L3         0x02    /*   LC[6:5]=10: Level 3, 强度=15   */
#define UC_GRAY1_L4         0x03    /*   LC[6:5]=11: Level 4, 强度=21   */

#define UC_SET_GRAY2        0xD4    /* 命令24: Set LCD Gray Shade 2      */
                                    /* D[7:2]=110101, D[1:0]=LC[8:7]    */
#define UC_GRAY2_L3         0x00    /*   LC[8:7]=00: Level 3, 强度=15   */
#define UC_GRAY2_L4         0x01    /*   LC[8:7]=01: Level 4, 强度=21   */
#define UC_GRAY2_L5         0x02    /*   LC[8:7]=10: Level 5, 强度=24   */
#define UC_GRAY2_L6         0x03    /*   LC[8:7]=11: Level 6, 强度=27   */

                                    /* 灰度强度参考 (0~36):              */
                                    /*   数据 00→白, 01→浅灰(灰1)       */
                                    /*   数据 10→深灰(灰2), 11→黑       */
                                    /*   LC[6:5] 控制灰1的亮度           */
                                    /*   LC[8:7] 控制灰2的亮度           */

#define UC_SET_LINERATE     0xA8    /* 命令17: Set Line Rate             */
                                    /* D[7:2]=101010, D[1:0]=LC[4:3]    */
#define UC_LR_14K           0x00    /*   00: 14.2 Klines/s              */
#define UC_LR_17K           0x01    /*   01: 17.3 Klines/s              */
#define UC_LR_21K           0x02    /*   10: 21.1 Klines/s              */
#define UC_LR_26K           0x03    /*   11: 25.7 Klines/s              */

#define UC_SET_PARTIAL      0x84    /* 命令14: Set Partial Display Ctrl  */
                                    /* D[7:2]=100001, D[1:0]=LC[10:9]   */
#define UC_PARTIAL_DIS      0x00    /*   0x: 禁用, MuxRate=CEN+1        */
#define UC_PARTIAL_EN       0x03    /*   11: 启用, MuxRate=DEN-DST+1    */

/* ---- 寄存器: NIV (N-Line Inversion) ---- */
#define UC_SET_NIV          0xC8    /* 命令22: Set N-Line Inversion      */
                                    /* D[7:3]=11001 (双字节命令)         */
#define UC_NIV_9L           0x00    /*   NIV[1:0]=00: 9 lines           */
#define UC_NIV_13L          0x01    /*   NIV[1:0]=01: 13 lines          */
#define UC_NIV_17L          0x02    /*   NIV[1:0]=10: 17 lines          */
#define UC_NIV_23L          0x03    /*   NIV[1:0]=11: 23 lines          */
#define UC_NIV_XOR          0x04    /*   NIV[2]=1: XOR模式              */
#define UC_NIV_EN           0x08    /*   NIV[3]=1: 使能N线反转           */

/* ---- 寄存器: BR (LCD Bias Ratio) ---- */
#define UC_SET_BR           0xE8    /* 命令28: Set LCD Bias Ratio        */
                                    /* D[7:3]=11101, D[1:0]=BR[1:0]     */
#define UC_BR_6             0x00    /*   00: Bias Ratio = 6             */
#define UC_BR_9             0x01    /*   01: Bias Ratio = 9             */
#define UC_BR_10            0x02    /*   10: Bias Ratio = 10            */
#define UC_BR_11            0x03    /*   11: Bias Ratio = 11 (默认)     */

/* ---- 寄存器: CEN (COM End) ---- */
#define UC_SET_CEN          0xF1    /* 命令27: Set COM End               */
                                    /* D[7:0]=11110001 (双字节命令)      */
                                    /* 第2字节: CEN[6:0]                 */
                                    /* COM扫描结束行 (0-based)           */
                                    /* 应等于 实际像素行数 - 1           */

/* ---- 寄存器: DST / DEN (Partial Display Start/End) ---- */
#define UC_SET_DST          0xF2    /* 命令28: Set Partial Display Start */
                                    /* 第2字节: DST[6:0]                 */
#define UC_SET_DEN          0xF3    /* 命令29: Set Partial Display End   */
                                    /* 第2字节: DEN[6:0]                 */
                                    /* DST/DEN: 0-based COM索引          */
                                    /* 约束: CEN ≥ DEN > DST + 8        */

/* ---- 寄存器: FLT/FLB (Fixed Lines) ---- */
#define UC_SET_FL           0x90    /* 命令16: Set Fixed Lines           */
                                    /* D[7:4]=1001 (双字节命令)          */
                                    /* 第2字节: D[7:4]=FLT[3:0]         */
                                    /*          D[3:0]=FLB[3:0]         */
                                    /* 固定行: 顶部2×FLT行, 底部2×FLB行  */

/* ---- 寄存器: WPP/WPC (Window Program) ---- */
#define UC_SET_WPC0         0xF4    /* 命令30: 窗口起始页列地址           */
                                    /* D[7:0]=11110100 (双字节)          */
                                    /* 第2字节: WPC0[4:0], 范围 0~31     */
#define UC_SET_WPP0         0xF5    /* 命令31: 窗口起始行地址             */
                                    /* 第2字节: WPP0[6:0], 范围 0~127    */
#define UC_SET_WPC1         0xF6    /* 命令32: 窗口结束页列地址           */
                                    /* 第2字节: WPC1[4:0], 范围 0~31     */
#define UC_SET_WPP1         0xF7    /* 命令33: 窗口结束行地址             */
                                    /* 第2字节: WPP1[6:0], 范围 0~127    */
#define UC_SET_WIN_EN       0xF9    /* 命令34: 窗口编程使能 AC[3]=1      */
#define UC_SET_WIN_DIS      0xF8    /* 命令34: 窗口编程禁用 AC[3]=0      */

/* ---- 系统命令 ---- */
#define UC_SYS_RESET        0xE2    /* 命令25: System Reset              */
                                    /* 所有寄存器恢复默认值, RAM不受影响  */
#define UC_NOP              0xE3    /* 命令24: No Operation              */

/* ---- 寄存器: OM (Operating Mode, 只读) ---- */
                                    /*   OM=00: Reset                    */
                                    /*   OM=01: (未使用)                 */
                                    /*   OM=10: Sleep                    */
                                    /*   OM=11: Normal                   */

/* ============================================================
 *  常用组合宏
 * ============================================================ */

/* 开显示: 4灰度 + 显示ON */
#define UC_DISP_GRAY_ON     UC_DC_GRAY_ON       /* 0x2F: DC[3:2]=11 */

/* 开显示: B/W + 显示ON */
#define UC_DISP_BW_ON       UC_DC_BW_ON         /* 0xAF: DC[3:2]=10 */

/* 关显示 (进入Sleep模式, 寄存器保留) */
#define UC_DISP_OFF         UC_DC_DISP_OFF      /* 0xAE: DC[3:2]=00 */

/* 正向显示 MY=0 MX=0 */
#define UC_MAP_NORMAL       (UC_SET_LC0 | 0x00) /* 0xC0: MY=0 MX=0  */

/* 上下翻转 MY=1 */
#define UC_MAP_FLIP_Y       (UC_SET_LC0 | UC_LC_MY) /* 0xC4: MY=1 MX=0 */

/* 左右镜像 MX=1 */
#define UC_MAP_FLIP_X       (UC_SET_LC0 | UC_LC_MX) /* 0xC2: MY=0 MX=1 */

/* 180度旋转 MY=1 MX=1 */
#define UC_MAP_ROTATE180    (UC_SET_LC0 | UC_LC_MY | UC_LC_MX) /* 0xC6 */

/* 4级灰度 */
#define GRAY_WHITE          0       /* 00: 白 (最亮) */
#define GRAY_LIGHT          1       /* 01: 浅灰      */
#define GRAY_DARK           2       /* 10: 深灰      */
#define GRAY_BLACK          3       /* 11: 黑 (最暗)  */

/* 兼容旧代码 */
#define COLOR_WHITE         GRAY_WHITE
#define COLOR_BLACK         GRAY_BLACK

/* ============================================================
 *  对比度宏 (VBIAS Potentiometer)
 *  PM 值越大对比度越高, 范围 0~193
 *  默认 78 (0x4E), 推荐调范围 60~120
 * ============================================================ */
#define UC_CONTRAST_DEFAULT 78

/* ============================================================
 *  背光控制
 * ============================================================ */
void LCD_BacklightOn(void);
void LCD_BacklightOff(void);

/* ============================================================
 *  初始化 / 清屏 / 刷屏
 * ============================================================ */
void LCD_Init(void);
void LCD_Clear(void);
void LCD_FillScreen(uint8_t data);
void LCD_Refresh(void);

/* ============================================================
 *  像素操作
 * ============================================================ */
void LCD_DrawPoint(uint8_t x, uint8_t y, uint8_t gray);
uint8_t LCD_ReadPoint(uint8_t x, uint8_t y);

/* ============================================================
 *  基本图形
 * ============================================================ */
void LCD_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t gray);
void LCD_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t gray);
void LCD_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t gray);
void LCD_DrawCircle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t gray);
void LCD_FillCircle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t gray);
void LCD_DrawEllipse(uint8_t cx, uint8_t cy, uint8_t rx, uint8_t ry, uint8_t gray);

/* ============================================================
 *  字符显示
 *  fg/bg: GRAY_WHITE(0) / GRAY_LIGHT(1) / GRAY_DARK(2) / GRAY_BLACK(3)
 * ============================================================ */
void LCD_ShowChar(uint8_t x, uint8_t y, uint8_t ch, uint8_t fg, uint8_t bg);
void LCD_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t fg, uint8_t bg);
void LCD_ShowChinese(uint8_t x, uint8_t y, uint16_t gb2312, uint8_t fg, uint8_t bg);
void LCD_ShowStringCN(uint8_t x, uint8_t y, const char *str, uint8_t fg, uint8_t bg);
void LCD_ShowNum(uint8_t x, uint8_t y, int32_t num, uint8_t fg, uint8_t bg);
void LCD_ShowCharAA(uint8_t x, uint8_t y, uint8_t ch, uint8_t fg, uint8_t bg, uint8_t edge);
void LCD_ShowStringAA(uint8_t x, uint8_t y, const char *str, uint8_t fg, uint8_t bg, uint8_t edge);

/* ============================================================
 *  图片显示
 *
 *  4灰度图片: 每像素2bit, 每字节4像素
 *    总字节 = w × h / 4
 *    128×96 全屏 = 3072 字节
 *
 *  1bit 单色位图: 每像素1bit, 每字节8像素
 *    总字节 = w × h / 8
 *    128×96 全屏 = 1536 字节
 * ============================================================ */

/* 图片数据格式标志 */
#define IMG_FMT_GRAY4_LSB  0    /* 4灰度, 已是LCD格式 (LSB first) */
#define IMG_FMT_GRAY4_MSB  1    /* 4灰度, Img2Lcd格式 (MSB first) */
#define IMG_FMT_MONO       2    /* 1bit单色, MSB first */

void LCD_ShowImage(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                   const uint8_t *img, uint8_t fmt);


#endif
