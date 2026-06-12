#ifndef __LCD_SPI_H
#define __LCD_SPI_H

#include "stm32f10x.h"
#include "stdint.h"

/* ============================================================
 *  屏幕尺寸
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
 *  DMA 配置
 *
 *  STM32F103 SPI1_TX = DMA1 Channel 3
 * ============================================================ */
#define LCD_DMA             DMA1
#define LCD_DMA_CHANNEL     DMA1_Channel3
#define LCD_DMA_TC_FLAG     DMA1_FLAG_TC3       /* 传输完成标志 */
#define LCD_DMA_GL_FLAG     DMA1_FLAG_GL3       /* 全局标志     */
#define LCD_DMA_HT_FLAG     DMA1_FLAG_HT3       /* 半传标志     */

/* ============================================================
 *  UC1617S 寄存器 / 命令定义
 * ============================================================ */

/* ---- 寄存器: CA (Page Column Address) ---- */
#define UC_SET_CA           0x00

/* ---- 寄存器: RA (Row Address) ---- */
#define UC_SET_RA_LSB       0x60
#define UC_SET_RA_MSB       0x70

/* ---- 寄存器: SL (Scroll Line) ---- */
#define UC_SET_SL_LSB       0x40
#define UC_SET_SL_MSB       0x60

/* ---- 寄存器: TC (Temperature Compensation) ---- */
#define UC_SET_TC           0x29
#define UC_TC_000           0x00
#define UC_TC_011           0x01
#define UC_TC_102           0x02
#define UC_TC_113           0x03

/* ---- 寄存器: PC (Panel Loading / Pump Control) ---- */
#define UC_SET_PANEL_LOAD   0x28
#define UC_PL_LE6           0x00
#define UC_PL_6TO9          0x01
#define UC_PL_9TO13         0x02
#define UC_PL_13TO18        0x03

#define UC_SET_PUMP         0x2F
#define UC_PUMP_EXT         0x00
#define UC_PUMP_INT         0x03

/* ---- 寄存器: PM (VBIAS Potentiometer) ---- */
#define UC_SET_VBIAS        0x81

/* ---- 寄存器: AC (RAM Address Control) ---- */
#define UC_SET_AC           0x88
#define UC_AC_WA            0x01
#define UC_AC_RA_FIRST      0x02
#define UC_AC_CA_FIRST      0x00
#define UC_AC_RA_DEC        0x04
#define UC_AC_RA_INC        0x00
#define UC_AC_WIN_EN        0x08

/* ---- 寄存器: DC (Display Control) ---- */
#define UC_DC_INVERSE       0xA7
#define UC_DC_NORMAL        0xA6
#define UC_DC_ALLPIXEL_ON   0xA5
#define UC_DC_ALLPIXEL_OFF  0xA4
#define UC_DC_DISP_OFF      0xAE
#define UC_DC_BW_ON         0xAF
#define UC_DC_GRAY_ON       0x2F

/* ---- 寄存器: LC (LCD Control) ---- */
#define UC_SET_LC0          0xC0
#define UC_LC_MY            0x04
#define UC_LC_MX            0x02

#define UC_SET_GRAY1        0xD0
#define UC_GRAY1_L1         0x00
#define UC_GRAY1_L2         0x01
#define UC_GRAY1_L3         0x02
#define UC_GRAY1_L4         0x03

#define UC_SET_GRAY2        0xD4
#define UC_GRAY2_L3         0x00
#define UC_GRAY2_L4         0x01
#define UC_GRAY2_L5         0x02
#define UC_GRAY2_L6         0x03

#define UC_SET_LINERATE     0xA8
#define UC_LR_14K           0x00
#define UC_LR_17K           0x01
#define UC_LR_21K           0x02
#define UC_LR_26K           0x03

#define UC_SET_PARTIAL      0x84
#define UC_PARTIAL_DIS      0x00
#define UC_PARTIAL_EN       0x03

/* ---- 寄存器: NIV (N-Line Inversion) ---- */
#define UC_SET_NIV          0xC8
#define UC_NIV_9L           0x00
#define UC_NIV_13L          0x01
#define UC_NIV_17L          0x02
#define UC_NIV_23L          0x03
#define UC_NIV_XOR          0x04
#define UC_NIV_EN           0x08

/* ---- 寄存器: BR (LCD Bias Ratio) ---- */
#define UC_SET_BR           0xE8
#define UC_BR_6             0x00
#define UC_BR_9             0x01
#define UC_BR_10            0x02
#define UC_BR_11            0x03

/* ---- 寄存器: CEN (COM End) ---- */
#define UC_SET_CEN          0xF1

/* ---- 寄存器: DST / DEN (Partial Display Start/End) ---- */
#define UC_SET_DST          0xF2
#define UC_SET_DEN          0xF3

/* ---- 寄存器: FLT/FLB (Fixed Lines) ---- */
#define UC_SET_FL           0x90

/* ---- 寄存器: WPP/WPC (Window Program) ---- */
#define UC_SET_WPC0         0xF4
#define UC_SET_WPP0         0xF5
#define UC_SET_WPC1         0xF6
#define UC_SET_WPP1         0xF7
#define UC_SET_WIN_EN       0xF9
#define UC_SET_WIN_DIS      0xF8

/* ---- 系统命令 ---- */
#define UC_SYS_RESET        0xE2
#define UC_NOP              0xE3

/* ============================================================
 *  组合显示宏
 * ============================================================ */
#define UC_DISP_GRAY_ON     UC_DC_GRAY_ON
#define UC_DISP_BW_ON       UC_DC_BW_ON
#define UC_DISP_OFF         UC_DC_DISP_OFF

#define UC_MAP_NORMAL       (UC_SET_LC0 | 0x00)
#define UC_MAP_FLIP_Y       (UC_SET_LC0 | UC_LC_MY)
#define UC_MAP_FLIP_X       (UC_SET_LC0 | UC_LC_MX)
#define UC_MAP_ROTATE180    (UC_SET_LC0 | UC_LC_MY | UC_LC_MX)

/* 4级灰度 */
#define GRAY_WHITE          0
#define GRAY_LIGHT          1
#define GRAY_DARK           2
#define GRAY_BLACK          3

#define COLOR_WHITE         GRAY_WHITE
#define COLOR_BLACK         GRAY_BLACK

/* ============================================================
 *  对比度默认值
 * ============================================================ */
#define UC_CONTRAST_DEFAULT 78

/* ============================================================
 *  背光控制
 * ============================================================ */
void LCD_BacklightOn(void);
void LCD_BacklightOff(void);

/* ============================================================
 *  初始化 / 清屏 / 刷新
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
 * ============================================================ */
#define IMG_FMT_GRAY4_LSB  0
#define IMG_FMT_GRAY4_MSB  1
#define IMG_FMT_MONO       2

void LCD_ShowImage(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                   const uint8_t *img, uint8_t fmt);

#endif
