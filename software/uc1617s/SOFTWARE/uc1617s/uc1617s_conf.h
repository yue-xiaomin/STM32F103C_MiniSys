/**
 * @file    uc1617s_conf.h
 * @brief   UC1617S 驱动配置文件
 *
 *  用户只需修改此文件来选择:
 *    1. 传输方式 (软I2C / 硬I2C / SPI / DMA SPI)
 *    2. 屏幕尺寸
 *    3. 引脚定义 (SPI 模式)
 *    4. 显示方向
 */

#ifndef __UC1617S_CONF_H
#define __UC1617S_CONF_H

/* ================================================================
 *  选择传输方式 (取消注释你需要的那个, 注释掉其他)
 * ================================================================ */
/* #define UC_USE_SW_I2C     */  /* 软件 I2C (PB6=SCL, PB7=SDA) */
/* #define UC_USE_HW_I2C     */  /* 硬件 I2C (PB6=SCL, PB7=SDA) */
/* #define UC_USE_SPI        */  /* 普通 SPI  (PA5=SCK, PA7=MOSI, PA4=CS, PA3=DC) */
/* #define UC_USE_DMA_SPI    */  /* DMA SPI   (同上, 数据走 DMA) */

//#define UC_USE_SW_I2C
//#define UC_USE_HW_I2C
//#define UC_USE_SPI
#define UC_USE_DMA_SPI





#define LCD_COM_LINES       128      /* COM方向行数 */
#define LCD_SEG_PIXELS      128     /* SEG方向像素数 */
#define LCD_PAGES                   (LCD_SEG_PIXELS / 4)    /* = 32 */
#define LCD_ROWS                    LCD_COM_LINES            /* = 96 */
#define LCD_BUF_SIZE                (LCD_ROWS * LCD_PAGES)   /* = 3072 */



/* ================================================================
 *  默认显示方向
 * ================================================================ */
#define UC_DEFAULT_MAP      UC_MAP_NORMAL   /* 可选: UC_MAP_NORMAL / UC_MAP_FLIP_X /
                                               UC_MAP_FLIP_Y / UC_MAP_ROTATE180 */

/* ================================================================
 *  默认对比度
 * ================================================================ */
#define UC_DEFAULT_CONTRAST UC_CONTRAST_DEFAULT  /* 78, 范围 0~193 */

/* ================================================================
 *  默认灰度配置
 * ================================================================ */
#define UC_DEFAULT_GRAY1    UC_GRAY1_L1     /* 浅灰强度 */
#define UC_DEFAULT_GRAY2    UC_GRAY2_L3     /* 深灰强度 */

/* ================================================================
 *  SPI 引脚配置 (仅 SPI/DMA SPI 模式需要, 在 uc1617s_bus_spi.c 中使用)
 *  如果你的接线不同, 修改 uc1617s_bus_spi.c 头部的宏定义
 * ================================================================ */

#endif /* __UC1617S_CONF_H */
