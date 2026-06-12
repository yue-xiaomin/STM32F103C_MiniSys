#include "eeprom_data.h"
#include "eeprom.h"
#include "string.h"
#include "math.h"  // 用于NaN定义

// 字节序处理（STM32是小端模式）
#define LITTLE_ENDIAN

// uint16_t 转字节数组
void uint16_to_bytes(uint16_t value, uint8_t *bytes) {
    #ifdef LITTLE_ENDIAN
    bytes[0] = (uint8_t)(value & 0xFF);
    bytes[1] = (uint8_t)((value >> 8) & 0xFF);
    #else
    bytes[1] = (uint8_t)(value & 0xFF);
    bytes[0] = (uint8_t)((value >> 8) & 0xFF);
    #endif
}

// 字节数组转 uint16_t
uint16_t bytes_to_uint16(uint8_t *bytes) {
    #ifdef LITTLE_ENDIAN
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
    #else
    return (uint16_t)bytes[1] | ((uint16_t)bytes[0] << 8);
    #endif
}

// uint32_t 转字节数组
void uint32_to_bytes(uint32_t value, uint8_t *bytes) {
    #ifdef LITTLE_ENDIAN
    bytes[0] = (uint8_t)(value & 0xFF);
    bytes[1] = (uint8_t)((value >> 8) & 0xFF);
    bytes[2] = (uint8_t)((value >> 16) & 0xFF);
    bytes[3] = (uint8_t)((value >> 24) & 0xFF);
    #else
    bytes[3] = (uint8_t)(value & 0xFF);
    bytes[2] = (uint8_t)((value >> 8) & 0xFF);
    bytes[1] = (uint8_t)((value >> 16) & 0xFF);
    bytes[0] = (uint8_t)((value >> 24) & 0xFF);
    #endif
}

// 字节数组转 uint32_t
uint32_t bytes_to_uint32(uint8_t *bytes) {
    #ifdef LITTLE_ENDIAN
    return (uint32_t)bytes[0] | 
           ((uint32_t)bytes[1] << 8) | 
           ((uint32_t)bytes[2] << 16) | 
           ((uint32_t)bytes[3] << 24);
    #else
    return (uint32_t)bytes[3] | 
           ((uint32_t)bytes[2] << 8) | 
           ((uint32_t)bytes[1] << 16) | 
           ((uint32_t)bytes[0] << 24);
    #endif
}

// int16_t 转字节数组
void int16_to_bytes(int16_t value, uint8_t *bytes) {
    uint16_to_bytes((uint16_t)value, bytes);
}

// 字节数组转 int16_t
int16_t bytes_to_int16(uint8_t *bytes) {
    return (int16_t)bytes_to_uint16(bytes);
}

// int32_t 转字节数组
void int32_to_bytes(int32_t value, uint8_t *bytes) {
    uint32_to_bytes((uint32_t)value, bytes);
}

// 字节数组转 int32_t
int32_t bytes_to_int32(uint8_t *bytes) {
    return (int32_t)bytes_to_uint32(bytes);
}

// float 转字节数组
void float_to_bytes(float value, uint8_t *bytes) {
    // 使用memcpy避免类型转换的严格别名问题
    memcpy(bytes, &value, sizeof(float));
}

// 字节数组转 float
float bytes_to_float(uint8_t *bytes) {
    float value;
    memcpy(&value, bytes, sizeof(float));
    return value;
}

// double 转字节数组 (8字节)
void double_to_bytes(double value, uint8_t *bytes) {
    // 使用memcpy避免类型转换的严格别名问题
    memcpy(bytes, &value, sizeof(double));
}

// 字节数组转 double
double bytes_to_double(uint8_t *bytes) {
    double value;
    memcpy(&value, bytes, sizeof(double));
    return value;
}



// EEPROM存储函数
bool eeprom_write_uint16(uint16_t addr, uint16_t value) {
    uint8_t bytes[2];
    uint16_to_bytes(value, bytes);
    return (AT24C02_Write_NBytes(addr, bytes, 2) == 0);
}

uint16_t eeprom_read_uint16(uint16_t addr) {
    uint8_t bytes[2];
    if(AT24C02_Read_NBytes(addr, bytes, 2) == 0) {
        return bytes_to_uint16(bytes);
    }
    return 0xFFFF; // 读取失败返回特定值
}

bool eeprom_write_uint32(uint16_t addr, uint32_t value) {
    uint8_t bytes[4];
    uint32_to_bytes(value, bytes);
    return (AT24C02_Write_NBytes(addr, bytes, 4) == 0);
}

uint32_t eeprom_read_uint32(uint16_t addr) {
    uint8_t bytes[4];
    if(AT24C02_Read_NBytes(addr, bytes, 4) == 0) {
        return bytes_to_uint32(bytes);
    }
    return 0xFFFFFFFF; // 读取失败返回特定值
}

bool eeprom_write_int16(uint16_t addr, int16_t value) {
    return eeprom_write_uint16(addr, (uint16_t)value);
}

int16_t eeprom_read_int16(uint16_t addr) {
    return (int16_t)eeprom_read_uint16(addr);
}

bool eeprom_write_int32(uint16_t addr, int32_t value) {
    return eeprom_write_uint32(addr, (uint32_t)value);
}

int32_t eeprom_read_int32(uint16_t addr) {
    return (int32_t)eeprom_read_uint32(addr);
}

bool eeprom_write_float(uint16_t addr, float value) {
    uint8_t bytes[4];
    float_to_bytes(value, bytes);
    return (AT24C02_Write_NBytes(addr, bytes, 4) == 0);
}

float eeprom_read_float(uint16_t addr) {
    uint8_t bytes[4];
    if(AT24C02_Read_NBytes(addr, bytes, 4) == 0) {
        return bytes_to_float(bytes);
    }
    return nan(""); // 返回NaN表示读取失败
}

bool eeprom_write_double(uint16_t addr, double value) {
    uint8_t bytes[8];
    double_to_bytes(value, bytes);
    return (AT24C02_Write_NBytes(addr, bytes, 8) == 0);
}

double eeprom_read_double(uint16_t addr) {
    uint8_t bytes[8];
    if(AT24C02_Read_NBytes(addr, bytes, 8) == 0) {
        return bytes_to_double(bytes);
    }
    return nan(""); // 返回NaN表示读取失败
}

// 数组存储函数
bool eeprom_write_uint16_array(uint16_t addr, uint16_t *array, uint16_t length) {
    uint8_t *bytes = (uint8_t*)array; // 直接利用内存布局
    return (AT24C02_Write_NBytes(addr, bytes, length * 2) == 0);
}

bool eeprom_read_uint16_array(uint16_t addr, uint16_t *array, uint16_t length) {
    uint8_t *bytes = (uint8_t*)array;
    return (AT24C02_Read_NBytes(addr, bytes, length * 2) == 0);
}

bool eeprom_write_uint32_array(uint16_t addr, uint32_t *array, uint16_t length) {
    uint8_t *bytes = (uint8_t*)array;
    return (AT24C02_Write_NBytes(addr, bytes, length * 4) == 0);
}

bool eeprom_read_uint32_array(uint16_t addr, uint32_t *array, uint16_t length) {
    uint8_t *bytes = (uint8_t*)array;
    return (AT24C02_Read_NBytes(addr, bytes, length * 4) == 0);
}

bool eeprom_write_float_array(uint16_t addr, float *array, uint16_t length) {
    uint8_t *bytes = (uint8_t*)array;
    return (AT24C02_Write_NBytes(addr, bytes, length * sizeof(float)) == 0);
}

bool eeprom_read_float_array(uint16_t addr, float *array, uint16_t length) {
    uint8_t *bytes = (uint8_t*)array;
    return (AT24C02_Read_NBytes(addr, bytes, length * sizeof(float)) == 0);
}

bool eeprom_write_double_array(uint16_t addr, double *array, uint16_t length) {
    uint8_t *bytes = (uint8_t*)array;
    return (AT24C02_Write_NBytes(addr, bytes, length * sizeof(double)) == 0);
}

bool eeprom_read_double_array(uint16_t addr, double *array, uint16_t length) {
    uint8_t *bytes = (uint8_t*)array;
    return (AT24C02_Read_NBytes(addr, bytes, length * sizeof(double)) == 0);
}



// 检查double值是否为有效数字（非NaN、非无穷大）
static bool is_valid_double(double value) {
    return !isnan(value) && !isinf(value);
}

// 安全写入double，检查有效性
bool eeprom_write_double_safe(uint16_t addr, double value) {
    if(!is_valid_double(value)) {
        return false;
    }
    return eeprom_write_double(addr, value);
}

// 安全读取double，检查有效性
bool eeprom_read_double_safe(uint16_t addr, double *value) {
    double result = eeprom_read_double(addr);
    if(is_valid_double(result)) {
        *value = result;
        return true;
    }
    return false;
}

// 批量写入double数组（带边界检查）
bool eeprom_write_double_bulk(uint16_t addr, double *array, uint16_t length) {
    if(addr + length * sizeof(double) > 256) {
        return false; // 超出EEPROM范围
    }
    
    for(uint16_t i = 0; i < length; i++) {
        if(!eeprom_write_double_safe(addr + i * sizeof(double), array[i])) {
            return false;
        }
    }
    return true;
}

// 批量读取double数组（带边界检查）
bool eeprom_read_double_bulk(uint16_t addr, double *array, uint16_t length) {
    if(addr + length * sizeof(double) > 256) {
        return false; // 超出EEPROM范围
    }
    
    for(uint16_t i = 0; i < length; i++) {
        if(!eeprom_read_double_safe(addr + i * sizeof(double), &array[i])) {
            return false;
        }
    }
    return true;
}

// 比较两个double值（考虑浮点数精度）
bool double_equals(double a, double b, double epsilon) {
    return fabs(a - b) < epsilon;
}

// 验证写入的double值是否正确
bool verify_double_write(uint16_t addr, double expected, double epsilon) {
    double actual = eeprom_read_double(addr);
    return double_equals(expected, actual, epsilon);
}




static uint16_t calculate_checksum(uint8_t *data, uint16_t length) {
    uint16_t sum = 0;
    for(uint16_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

// 附带版本信息的数据
bool eeprom_write_with_checksum(uint16_t addr, uint32_t data, uint16_t version) {
    DataWithChecksum packet;
    packet.version = version;
    packet.data = data;
    packet.checksum = calculate_checksum((uint8_t*)&packet, 6); // 前6字节的校验和
    
    return eeprom_write_uint32_array(addr, (uint32_t*)&packet, 2); // 2个uint32_t
}

bool eeprom_read_with_checksum(uint16_t addr, uint32_t *data, uint16_t expected_version) {
    DataWithChecksum packet;
    if(!eeprom_read_uint32_array(addr, (uint32_t*)&packet, 2)) {
        return false;
    }
    
    if(packet.version != expected_version) {
        return false; // 版本不匹配
    }
    
    uint16_t calculated_checksum = calculate_checksum((uint8_t*)&packet, 6);
    if(calculated_checksum != packet.checksum) {
        return false; // 校验和错误
    }
    
    *data = packet.data;
    return true;
}


// 结构体序列化
void sensor_data_to_bytes(SensorData *data, uint8_t *bytes) {
    uint16_to_bytes(data->id, &bytes[0]);
    uint32_to_bytes(data->timestamp, &bytes[2]);
    float_to_bytes(data->temperature, &bytes[6]);
    double_to_bytes(data->pressure, &bytes[10]);
    bytes[18] = data->status;
}

// 结构体反序列化
void bytes_to_sensor_data(uint8_t *bytes, SensorData *data) {
    data->id = bytes_to_uint16(&bytes[0]);
    data->timestamp = bytes_to_uint32(&bytes[2]);
    data->temperature = bytes_to_float(&bytes[6]);
    data->pressure = bytes_to_double(&bytes[10]);
    data->status = bytes[18];
}

/*
void data_test()
{
// 定义EEPROM存储地址
#define ADDR_DOUBLE_VALUE   0x00
#define ADDR_DOUBLE_ARRAY   0x08
#define ADDR_SENSOR_DATA    0x30    
    printf("Testing double storage...\r\n");
    
    // 1. 存储和读取单个double值
    double test_value = 3.141592653589793;
    if(eeprom_write_double(ADDR_DOUBLE_VALUE, test_value)) {
        printf("Double written successfully\r\n");
    }
    
    double read_value = eeprom_read_double(ADDR_DOUBLE_VALUE);
    printf("Original: %.15f, Read: %.15f\r\n", test_value, read_value);
    
    // 2. 安全写入和读取
    double safe_value = 2.718281828459045;
    if(eeprom_write_double_safe(ADDR_DOUBLE_VALUE + 8, safe_value)) {
        printf("Safe double written\r\n");
    }
    
    double read_safe;
    if(eeprom_read_double_safe(ADDR_DOUBLE_VALUE + 8, &read_safe)) {
        printf("Safe read: %.15f\r\n", read_safe);
    }
    
    // 3. 存储和读取double数组
    double double_array[3] = {1.23456789, 9.87654321, 6.62607015e-34};
    if(eeprom_write_double_array(ADDR_DOUBLE_ARRAY, double_array, 3)) {
        printf("Double array written\r\n");
    }
    
    double read_array[3];
    if(eeprom_read_double_array(ADDR_DOUBLE_ARRAY, read_array, 3)) {
        printf("Array read: [%.8f, %.8f, %.3e]\r\n", 
               read_array[0], read_array[1], read_array[2]);
    }
    
    // 4. 使用包含double的结构体
    SensorData sensor_data = {
        .id = 0x1234,
        .timestamp = 1640995200,
        .temperature = 23.75648932,      // 高精度温度
        .pressure = 1013.250000000001,   // 高精度压力
        .status = 0x01
    };
    
    uint8_t bytes[23];
    sensor_data_to_bytes(&sensor_data, bytes);
    AT24C02_Write_NBytes(ADDR_SENSOR_DATA, bytes, 23);
    
    SensorData read_data;
    uint8_t read_bytes[23];
    AT24C02_Read_NBytes(ADDR_SENSOR_DATA, read_bytes, 23);
    bytes_to_sensor_data(read_bytes, &read_data);
    
    printf("Sensor data - Temp: %.6f, Pressure: %.6f\r\n", 
           read_data.temperature, read_data.pressure);
    
    // 5. 验证写入是否正确
    if(verify_double_write(ADDR_DOUBLE_VALUE, test_value, 1e-12)) {
        printf("Double verification passed!\r\n");
    } else {
        printf("Double verification failed!\r\n");
    }
}
*/














