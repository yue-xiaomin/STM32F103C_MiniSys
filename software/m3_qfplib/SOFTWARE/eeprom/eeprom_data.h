#ifndef __EEPROM_DATA_H
#define __EEPROM_DATA_H

#include "stm32f10x.h"
#include <stdint.h>
#include <stdbool.h>




// 数据序列化和反序列化函数
void uint16_to_bytes(uint16_t value, uint8_t *bytes);
uint16_t bytes_to_uint16(uint8_t *bytes);
void uint32_to_bytes(uint32_t value, uint8_t *bytes);
uint32_t bytes_to_uint32(uint8_t *bytes);
void int16_to_bytes(int16_t value, uint8_t *bytes);
int16_t bytes_to_int16(uint8_t *bytes);
void int32_to_bytes(int32_t value, uint8_t *bytes);
int32_t bytes_to_int32(uint8_t *bytes);
void float_to_bytes(float value, uint8_t *bytes);
float bytes_to_float(uint8_t *bytes);
void double_to_bytes(double value, uint8_t *bytes);      
double bytes_to_double(uint8_t *bytes);                  

// EEPROM数据存储函数
bool eeprom_write_uint16(uint16_t addr, uint16_t value);
uint16_t eeprom_read_uint16(uint16_t addr);
bool eeprom_write_uint32(uint16_t addr, uint32_t value);
uint32_t eeprom_read_uint32(uint16_t addr);
bool eeprom_write_int16(uint16_t addr, int16_t value);
int16_t eeprom_read_int16(uint16_t addr);
bool eeprom_write_int32(uint16_t addr, int32_t value);
int32_t eeprom_read_int32(uint16_t addr);
bool eeprom_write_float(uint16_t addr, float value);
float eeprom_read_float(uint16_t addr);
bool eeprom_write_double(uint16_t addr, double value);  
double eeprom_read_double(uint16_t addr);                

// 数组存储函数
bool eeprom_write_uint16_array(uint16_t addr, uint16_t *array, uint16_t length);
bool eeprom_read_uint16_array(uint16_t addr, uint16_t *array, uint16_t length);
bool eeprom_write_uint32_array(uint16_t addr, uint32_t *array, uint16_t length);
bool eeprom_read_uint32_array(uint16_t addr, uint32_t *array, uint16_t length);
bool eeprom_write_float_array(uint16_t addr, float *array, uint16_t length);     
bool eeprom_read_float_array(uint16_t addr, float *array, uint16_t length);     
bool eeprom_write_double_array(uint16_t addr, double *array, uint16_t length);   
bool eeprom_read_double_array(uint16_t addr, double *array, uint16_t length);    


// 安全写入double，检查有效性
bool eeprom_write_double_safe(uint16_t addr, double value);
// 安全读取double，检查有效性
bool eeprom_read_double_safe(uint16_t addr, double *value);
// 批量写入double数组（带边界检查）
bool eeprom_write_double_bulk(uint16_t addr, double *array, uint16_t length);
// 批量读取double数组（带边界检查）
bool eeprom_read_double_bulk(uint16_t addr, double *array, uint16_t length);
// 比较两个double值（考虑浮点数精度）
bool double_equals(double a, double b, double epsilon);
// 验证写入的double值是否正确
bool verify_double_write(uint16_t addr, double expected, double epsilon);



typedef struct {
    uint16_t version;
    uint16_t checksum;
    uint32_t data;
} DataWithChecksum;

bool eeprom_write_with_checksum(uint16_t addr, uint32_t data, uint16_t version);
bool eeprom_read_with_checksum(uint16_t addr, uint32_t *data, uint16_t expected_version);


// 结构体序列化示例
typedef struct {
    uint16_t id;
    uint32_t timestamp;
    float temperature;          // 使用double替代float
    double pressure;             // 新增double字段
    uint8_t status;
} SensorData;

// 结构体序列化
void sensor_data_to_bytes(SensorData *data, uint8_t *bytes);
// 结构体反序列化
void bytes_to_sensor_data(uint8_t *bytes, SensorData *data);

#endif /* __EEPROM_DATA_H */

