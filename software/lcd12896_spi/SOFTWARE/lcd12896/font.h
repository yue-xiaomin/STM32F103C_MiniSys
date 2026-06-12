#ifndef __FONT_H
#define __FONT_H

#include <stdint.h>



//第1步: 取模 (PCtoLCD2002)
//  阴码, 逐行, 顺向(MSB-first), C51, 16×16
//  生成32字节 → 添加到 font.c

//第2步: 查 Unicode 码 (百度 "X的Unicode码")
//  例: "数" → U+6570

//第3步: 查 GB2312 码 (百度 "X的GB2312码")
//  例: "数" → 0xCAFD

//第4步: 在 font.c 的 FONT_CN16[] 中注册
//  {0xCAFD, CN_SHU},

//第5步: 在 LCD_ShowStringCN 的 UNI2GB[] 中注册
//  {0x6570, 0xCAFD},   /* 数 */

/* ============================================================
 *  ASCII 8×16 字库
 *  覆盖: 0x20(空格) ~ 0x7E(~), 共95个字符
 *  格式: 每字符16字节, 逐行, MSB-first (bit7=最左)
 *  索引: FONT_8x16[ch - 0x20]
 * ============================================================ */
extern const uint8_t FONT_8x16[][16];

/* ============================================================
 *  16×16 中文字库
 *  格式: 每字符32字节, 逐行, MSB-first
 *  每行2字节: byte0=左8像素, byte1=右8像素
 *  取模工具: PCtoLCD2002
 *    设置: 阴码, 逐行, 顺向(MSB-first), C51格式
 * ============================================================ */
typedef struct {
    uint16_t code;          /* GB2312 内码 (如 0xC4E3 = "你") */
    const uint8_t *data;    /* 32字节点阵 */
} FontCN16_t;

extern const FontCN16_t FONT_CN16[];
extern const uint8_t FONT_CN16_COUNT;
extern const unsigned char gImage_ameng[];

#endif
