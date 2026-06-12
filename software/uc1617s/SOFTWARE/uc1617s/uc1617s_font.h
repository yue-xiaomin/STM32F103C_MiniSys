/**
 * @file    uc1617s_font.h
 * @brief   字体定义 — ASCII + 中文
 */
#ifndef __UC1617S_FONT_H
#define __UC1617S_FONT_H

#include <stdint.h>

/* ASCII 字体描述符 */
typedef struct {
    const uint8_t *data;
    uint8_t width;
    uint8_t height;
    uint8_t first;
    uint8_t count;
} uc_font_t;

/* 中文16×16字模条目 */
typedef struct {
    uint16_t       code;
    const uint8_t  data[32];
} uc_font_cn_t;

/* ASCII 字体指针 */
extern const uc_font_t *uc_font_5x7;
extern const uc_font_t *uc_font_6x8;
extern const uc_font_t *uc_font_8x8;
extern const uc_font_t *uc_font_8x16;
/* 中文字库 */
extern const uc_font_cn_t _cn_font[];
extern const uint16_t CN_FONT_COUNT;


extern const uint8_t image_data[];

#endif /* __UC1617S_FONT_H */
