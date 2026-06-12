/**
 * @file    uc1617s_lcd.h
 * @brief   UC1617S LCD 驱动层接口
 *
 *  提供完整的 LCD 操作函数:
 *    - 初始化 / 复位 / 批量绘制
 *    - 全屏刷新 / 局部刷新
 *    - 像素/区域操作
 *    - 字符串/整数/抗锯齿绘制
 *
 *  底层传输通过 uc1617s_bus_t 接口, 与具体总线无关.
 *
 *  字体接口分三类:
 *    基础系列:   高性能, 列式字模, 无背景色, 适合快速刷新
 *    扩展系列:   通用, 行式字模, 有背景色, 适合菜单/UI
 *    抗锯齿系列: 利用4灰度做边缘平滑, 适合大字显示
 */

#ifndef __UC1617S_LCD_H
#define __UC1617S_LCD_H

#include "uc1617s.h"
#include "uc1617s_bus.h"
#include "uc1617s_font.h"

/* ================================================================
 *  帧缓冲尺寸
 * ================================================================ */
#define FB_CA_MAX   (LCD_WIDTH / 4)             /* 每行字节数 (128/4 = 32) */
#define FB_SIZE     (LCD_HEIGHT * FB_CA_MAX)    /* 帧缓冲总字节 (96×32 = 3072) */


//┌─────────────────────────────────┐
//│  PCtoLCD2002 设置               │
//│                                 │
//│  取模方式: ● 逐列式             │
//│  取模走向: ● 逆向 (低位在前)    │
//│  输出格式: ● C51                │
//│  宽度: 16  高度: 16             │
//└─────────────────────────────────┘

/*
函数                        列式  背景  中文  抗锯齿  换行  整数
──────────────────────────────────────────────────────────────
列式 ASCII:
  draw_char                   Y     N     N     N      N     N
  draw_string                 Y     N     N     N      N     N
  draw_int                    Y     N     N     N      N     Y

列式 抗锯齿:
  draw_char_aa                Y     Y     N     Y      N     N
  draw_string_aa              Y     Y     N     Y      Y     N

列式 中文:
  draw_chinese                Y     Y     Y     N      N     N
  draw_string_cn              Y     Y     Y     N      Y     N
*/

/* ================================================================
 *  初始化 / 批量绘制
 * ================================================================ */

/**
 * @brief  初始化 UC1617S 显示控制器
 * @param  bus  总线接口实例 (I2C/SPI)
 * @note   包含硬件复位、模拟参数、灰度、对比度、方向、滚动等完整配置
 */
void uc1617s_gfx_init(const uc1617s_bus_t *bus);

/**
 * @brief  批量绘制开始 — 禁止自动刷新, 所有操作只写本地 _fb
 */
void uc1617s_batch_begin(void);

/**
 * @brief  批量绘制结束 — 一次性全屏刷新到 SRAM
 * @note   内部调用 uc1617s_refresh(), 整个批次只闪一次
 */
void uc1617s_batch_end(void);

/* ================================================================
 *  帧缓冲操作
 * ================================================================ */

/**
 * @brief  清屏 — 将帧缓冲全部填为指定灰度
 * @param  gray  灰度值: GRAY_WHITE(3) / GRAY_LIGHT(2) / GRAY_DARK(1) / GRAY_BLACK(0)
 */
void uc1617s_clear(uint8_t gray);

/**
 * @brief  设置单个像素
 * @param  x,y   像素坐标
 * @param  gray  灰度值
 */
void uc1617s_set_pixel(uint8_t x, uint8_t y, uint8_t gray);

/**
 * @brief  读取单个像素灰度值 (从本地 _fb 读取)
 * @return 灰度值 (0~3)
 */
uint8_t uc1617s_get_pixel(uint8_t x, uint8_t y);

/**
 * @brief  全屏刷新 — 将本地 _fb 一次性写入 UC1617S SRAM
 */
void uc1617s_refresh(void);

/* ================================================================
 *  图元绘制
 * ================================================================ */

/**
 * @brief  Bresenham 直线
 */
void uc1617s_draw_line(int x0, int y0, int x1, int y1, uint8_t gray);

/**
 * @brief  矩形边框
 */
void uc1617s_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t gray);

/**
 * @brief  填充矩形 (字节对齐加速)
 */
void uc1617s_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t gray);

/**
 * @brief  中点画圆 (空心)
 */
void uc1617s_draw_circle(int cx, int cy, int r, uint8_t gray);

/**
 * @brief  填充圆 (扫描线填充)
 */
void uc1617s_fill_circle(int cx, int cy, int r, uint8_t gray);

/**
 * @brief  显示原始图像数据 (2bit/像素, 与 SRAM 格式一致)
 * @param  x,y    显示位置
 * @param  data   图像数据指针
 * @param  w,h    图像尺寸
 * @note   局刷到指定区域, 不影响屏幕其余部分
 */
void uc1617s_draw_image_raw(uint8_t x, uint8_t y,
                            const uint8_t *data,
                            uint8_t w, uint8_t h);

/* ================================================================
 *  列式 ASCII (列式字模, LSB-first, 透明背景)
 *
 *  字模格式: 列式, bit0=顶行
 *  特点: 只画前景像素, 不破坏背景内容
 *  适用: 覆盖在图形/图片上显示文字
 * ================================================================ */

/**
 * @brief  绘制单字符 (列式, 透明背景)
 * @param  x,y   左上角坐标
 * @param  c     ASCII 字符 (0x20~0x7E)
 * @param  gray  前景灰度值
 * @param  font  字体描述符
 */
void uc1617s_draw_char(uint8_t x, uint8_t y, char c,
                       uint8_t gray, const uc_font_t *font);

/**
 * @brief  绘制字符串 (列式, 透明背景)
 * @param  x,y   左上角坐标
 * @param  str   ASCII 字符串
 * @param  gray  前景灰度值
 * @param  font  字体描述符
 * @note   整行一次局刷, 不自动换行
 */
void uc1617s_draw_string(uint8_t x, uint8_t y, const char *str,
                         uint8_t gray, const uc_font_t *font);

/**
 * @brief  显示整数 (列式, 透明背景, 支持负数)
 * @param  x,y   左上角坐标
 * @param  num   有符号整数 (int32_t)
 * @param  gray  前景灰度值
 * @param  font  字体描述符
 */
void uc1617s_draw_int(uint8_t x, uint8_t y, int32_t num,
                      uint8_t gray, const uc_font_t *font);

/* ================================================================
 *  列式 ASCII 抗锯齿 (列式字模, 4灰度边缘平滑)
 *
 *  利用 UC1617S 的 4 级灰度做边缘过渡:
 *    前景像素 → fg   (如 GRAY_BLACK)
 *    边缘像素 → edge (如 GRAY_DARK, 过渡)
 *    背景像素 → bg   (如 GRAY_WHITE)
 *  适合大字体 (8×16) 场景
 * ================================================================ */

/**
 * @brief  绘制抗锯齿单字符 (列式)
 * @param  x,y   左上角坐标
 * @param  c     ASCII 字符
 * @param  fg    前景灰度值
 * @param  bg    背景灰度值
 * @param  edge  边缘过渡灰度值 (介于 fg 和 bg 之间)
 * @param  font  字体描述符
 */
void uc1617s_draw_char_aa(uint8_t x, uint8_t y, char c,
                          uint8_t fg, uint8_t bg, uint8_t edge,
                          const uc_font_t *font);

/**
 * @brief  绘制抗锯齿字符串 (列式, 自动换行)
 * @param  x,y   左上角坐标
 * @param  str   ASCII 字符串
 * @param  fg    前景灰度值
 * @param  bg    背景灰度值
 * @param  edge  边缘过渡灰度值
 * @param  font  字体描述符
 */
void uc1617s_draw_string_aa(uint8_t x, uint8_t y, const char *str,
                            uint8_t fg, uint8_t bg, uint8_t edge,
                            const uc_font_t *font);

/* ================================================================
 *  列式中文 (列式字模, LSB=顶, 有背景色)
 *
 *  字模格式: 逐列式 + 逆向(低位在前) + C51
 *           data[0~15]  = 列0~15, page0(行0~7),  bit0=行0(顶)
 *           data[16~31] = 列0~15, page1(行8~15), bit0=行8(顶)
 *  特点: 清晰, 不做抗锯齿 (16×16笔画细, AA会糊)
 * ================================================================ */

/**
 * @brief  绘制单个16×16中文字符 (列式, 有背景色)
 * @param  x,y     左上角坐标
 * @param  gb2312  GB2312 内码 (如 0xC4E3 = "你")
 * @param  fg      前景灰度值
 * @param  bg      背景灰度值
 * @param  font    中文字库数组 (_cn_font)
 * @param  count   字库条目数 (CN_FONT_COUNT)
 */
void uc1617s_draw_chinese(uint8_t x, uint8_t y, uint16_t gb2312,
                          uint8_t fg, uint8_t bg,
                          const uc_font_cn_t *font, uint16_t count);

/**
 * @brief  绘制中英文混合字符串 (列式, 自动换行)
 * @param  x,y        左上角坐标
 * @param  str        GB2312 编码字符串
 * @param  fg         前景灰度值
 * @param  bg         背景灰度值
 * @param  cn_font    中文字库数组 (_cn_font)
 * @param  cn_count   中文字库条目数 (CN_FONT_COUNT)
 * @param  ascii_font ASCII 字体描述符 (uc_font_8x16)
 *
 * @note   ASCII (< 0x80): 使用 ascii_font, 透明背景
 *         GB2312 (>= 0x80): 与下一字节组成内码, 16像素宽, 有背景色
 *         超出屏幕宽度自动换行
 */
void uc1617s_draw_string_cn(uint8_t x, uint8_t y, const char *str,
                            uint8_t fg, uint8_t bg,
                            const uc_font_cn_t *cn_font, uint16_t cn_count,
                            const uc_font_t *ascii_font);

                            
void uc1617s_draw_image_col(uint8_t x, uint8_t y,
                            const uint8_t *data,
                            uint8_t width, uint8_t height,
                            uint8_t fg, uint8_t bg);                            
                            
                            
                            
#endif /* __UC1617S_LCD_H */
