/**
 * @file    uc1617s_bus.h
 * @brief   UC1617S 传输层抽象接口
 *
 *  统一软I2C、硬I2C、普通SPI、DMA SPI 四种驱动的接口,
 *  上层 LCD 驱动通过函数指针调用, 不关心底层总线类型.
 */

#ifndef __UC1617S_BUS_H
#define __UC1617S_BUS_H

#include "stdint.h"

/* ================================================================
 *  传输类型枚举
 * ================================================================ */
typedef enum {
    UC_BUS_SOFT_I2C = 0,
    UC_BUS_HW_I2C,
    UC_BUS_SPI,
    UC_BUS_DMA_SPI,
} uc_bus_type_t;

/* ================================================================
 *  传输层操作接口 (虚函数表)
 *
 *  write_cmd     : 写命令 (DC=0)
 *  write_cmd2    : 写双字节命令 (cmd + param)
 *  write_data    : 批量写数据 (DC=1)
 *  write_fill    : 批量填充同一字节
 *  init          : 初始化总线硬件
 *  deinit        : 反初始化 (可选)
 * ================================================================ */
typedef struct {
    void     (*write_cmd)(uint8_t cmd);
    void     (*write_cmd2)(uint8_t cmd, uint8_t param);
    uint8_t  (*write_data)(const uint8_t *data, uint16_t len);
    uint8_t  (*write_fill)(uint8_t val, uint16_t count);
    void     (*init)(void);
    void     (*deinit)(void);
    uc_bus_type_t type;
} uc1617s_bus_t;

/* ================================================================
 *  四种总线的全局实例 (由各 .c 文件定义)
 * ================================================================ */
extern const uc1617s_bus_t uc_bus_sw_i2c;
extern const uc1617s_bus_t uc_bus_hw_i2c;
extern const uc1617s_bus_t uc_bus_spi;
extern const uc1617s_bus_t uc_bus_dma_spi;

#endif /* __UC1617S_BUS_H */
