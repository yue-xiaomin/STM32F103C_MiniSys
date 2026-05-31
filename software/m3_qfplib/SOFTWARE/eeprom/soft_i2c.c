#include "soft_i2c.h"

// 初始化软件I2C
void SoftI2C_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(Soft_I2C_RCC_APB2Periph, ENABLE);
    
    // 配置SCL和SDA为开漏输出模式
    GPIO_InitStructure.GPIO_Pin = Soft_I2C_SCL_PIN | Soft_I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(Soft_I2C_PORT, &GPIO_InitStructure);
    
    // 初始状态拉高
    SCL_HIGH();
    SDA_HIGH();
    
    delay_ms(10); // 等待EEPROM上电稳定
}

// I2C起始信号
void I2C_Start(void) {
    SDA_HIGH();
    SCL_HIGH();
    delay_us(5);
    SDA_LOW();
    delay_us(5);
    SCL_LOW();
    delay_us(5);
}

// I2C停止信号
void I2C_Stop(void) {
    SDA_LOW();
    SCL_LOW();
    delay_us(5);
    SCL_HIGH();
    delay_us(5);
    SDA_HIGH();
    delay_us(5);
}

// 等待ACK响应
uint8_t I2C_Wait_Ack(void) {
    uint16_t timeout = 1000; // 超时计数器
    
    SDA_HIGH(); // 释放SDA
    delay_us(2);
    SCL_HIGH();
    delay_us(2);
    
    // 检测SDA是否为低电平(ACK)
    while(SDA_READ() == Bit_SET) {
        timeout--;
        if(timeout == 0) {
            SCL_LOW();
            return 0; // 超时，未收到ACK
        }
        delay_us(1);
    }
    
    SCL_LOW();
    delay_us(2);
    return 1; // 收到ACK
}

// 发送ACK
void I2C_Ack(void) {
    SCL_LOW();
    delay_us(2);
    SDA_LOW();
    delay_us(2);
    SCL_HIGH();
    delay_us(5);
    SCL_LOW();
    delay_us(2);
    SDA_HIGH(); // 释放SDA
    delay_us(2);
}

// 发送NACK
void I2C_NAck(void) {
    SCL_LOW();
    delay_us(2);
    SDA_HIGH();
    delay_us(2);
    SCL_HIGH();
    delay_us(5);
    SCL_LOW();
    delay_us(2);
}

// 发送一个字节
void I2C_SendByte(uint8_t data) {
    uint8_t i;
    
    for(i = 0; i < 8; i++) {
        SCL_LOW();
        delay_us(2);
        
        if(data & 0x80) {
            SDA_HIGH();
        } else {
            SDA_LOW();
        }
        delay_us(2);
        
        SCL_HIGH();
        delay_us(5);
        
        data <<= 1;
    }
    
    SCL_LOW();
    delay_us(2);
    SDA_HIGH(); // 释放SDA
    delay_us(2);
}

// 接收一个字节
uint8_t I2C_ReadByte(uint8_t ack) {
    uint8_t i, data = 0;
    
    SDA_HIGH(); // 释放SDA
    
    for(i = 0; i < 8; i++) {
        data <<= 1;
        
        SCL_LOW();
        delay_us(2);
        SCL_HIGH();
        delay_us(2);
        
        if(SDA_READ()) {
            data |= 0x01;
        }
        
        delay_us(2);
    }
    
    SCL_LOW();
    delay_us(2);
    
    if(ack) {
        I2C_Ack();
    } else {
        I2C_NAck();
    }
    
    return data;
}


#ifdef SCAN_I2C_DEVICE
// 扫描I2C设备
void I2C_ScanDevices(void) {
    uint8_t i, j;
    uint8_t deviceCount = 0;
    uint8_t foundDevices[128] = {0}; // 存储找到的设备地址
    
    log_i("Starting I2C device scan...");
    log_i("Address range: 0x08 - 0x77");
    log_i("Scanning");
    
    // 扫描所有可能的I2C地址 (0x08 - 0x77)
    for(i = 0x08; i <= 0x77; i++) {
        // 显示扫描进度
//        if(i % 16 == 0) {
//            log_i(".");
//        }
        
        I2C_Start();
        I2C_SendByte(i << 1); // 发送写地址(地址左移1位，最低位=0)
        
        if(I2C_Wait_Ack()) {
            // 收到ACK，设备存在
            foundDevices[deviceCount] = i;
            deviceCount++;
            log_i("Device found: 0x%02X", i);
            
            // 显示设备类型信息
//            I2C_Scan_ShowDeviceInfo(i);
        }
        
        I2C_Stop();
        delay_us(10); // 短暂延时
    }
    
    log_i("Scan completed!");
    log_i("Total found %d I2C devices:", deviceCount);
    
    for(j = 0; j < deviceCount; j++) {
        log_i("Device %d: Address 0x%02X", j+1, foundDevices[j]);
        I2C_Scan_ShowDeviceInfo(foundDevices[j]);
    }
    
    if(deviceCount == 0) {
        log_i("No I2C devices found");
    }
}

// 显示常见I2C设备信息
void I2C_Scan_ShowDeviceInfo(uint8_t addr) {
    switch(addr) {
        case 0x50: case 0x51: case 0x52: case 0x53:
        case 0x54: case 0x55: case 0x56: case 0x57:
            log_i(" (EEPROM - 24Cxx series)");
            break;
            
        case 0x68:
            log_i(" (RTC - DS1307/DS3231)");
            break;
            
        case 0x27: case 0x3F:
            log_i(" (LCD - PCF8574 expansion board)");
            break;
            
        case 0x48: case 0x49: case 0x4A: case 0x4B:
            log_i(" (ADC - PCF8591)");
            break;
            
        case 0x1D:
            log_i(" (Accelerometer - ADXL345)");
            break;
            
        case 0x5A:
            log_i(" (Temperature sensor - MLX90614)");
            break;
            
        case 0x76: case 0x77:
            log_i(" (Temp/Humi/Pressure - BME280)");
            break;
            
        case 0x23:
            log_i(" (Light sensor - BH1750)");
            break;
            
        case 0x40:
            log_i(" (Temp/Humi sensor - HTU21D/SHT21)");
            break;
            
        case 0x60:
            log_i(" (FM radio - TEA5767)");
            break;
            
        case 0x29:
            log_i(" (Color sensor - TCS34725)");
            break;
            
        case 0x19:
            log_i(" (Accelerometer - LIS3DH)");
            break;
            
        case 0x6B:
            log_i(" (Gyroscope - L3G4200D)");
            break;
            
        case 0x0A:
            log_i(" (OLED display - SSD1306)");
            break;
            
        default:
            log_i(" (Unknown device)");
            break;
    }
}

// 快速扫描（只扫描常见地址）
void I2C_QuickScan(void) {
    uint8_t commonAddrs[] = {
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, // IO扩展
        0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, // LCD
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, // 传感器
        0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, // ADC
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, // EEPROM
        0x68,                                           // RTC
        0x76, 0x77,                                     // BME280
        0x00 // 结束标记
    };
    
    uint8_t i = 0;
    uint8_t deviceCount = 0;
    
    log_i("Quick scan for common I2C devices...");
    
    while(commonAddrs[i] != 0x00) {
        I2C_Start();
        I2C_SendByte(commonAddrs[i] << 1);
        
        if(I2C_Wait_Ack()) {
            log_i("Device found: 0x%02X", commonAddrs[i]);
            I2C_Scan_ShowDeviceInfo(commonAddrs[i]);
            deviceCount++;
        }
        
        I2C_Stop();
        delay_us(10);
        i++;
    }
    
    log_i("Quick scan completed, found %d devices", deviceCount);
}

// 测试特定设备地址
uint8_t I2C_TestDevice(uint8_t addr) {
    uint8_t writeAddr = addr << 1;
    uint8_t result = 0;
    
    I2C_Start();
    I2C_SendByte(writeAddr);
    
    if(I2C_Wait_Ack()) {
        result = 1; // 设备存在
    } else {
        result = 0; // 设备不存在
    }
    
    I2C_Stop();
    return result;
}

// 高级扫描功能：检测设备是否响应读操作
void I2C_AdvancedScan(void) {
    uint8_t i;
    uint8_t writeAddr, readAddr;
    
    log_i("Advanced scan - testing read/write addresses:");
    
    for(i = 0x08; i <= 0x77; i++) {
        writeAddr = i << 1;      // 写地址
        readAddr = (i << 1) | 1; // 读地址
        
        // 测试写地址
        I2C_Start();
        I2C_SendByte(writeAddr);
        if(I2C_Wait_Ack()) {
            I2C_Stop();
            
            // 测试读地址
            I2C_Start();
            I2C_SendByte(readAddr);
            if(I2C_Wait_Ack()) {
                log_i("Device 0x%02X: Write=0x%02X, Read=0x%02X [Full]", 
                       i, writeAddr, readAddr);
            } else {
                log_i("Device 0x%02X: Write=0x%02X, Read=No response [Write only?]", 
                       i, writeAddr);
            }
            I2C_Stop();
        } else {
            I2C_Stop();
        }
        
        delay_us(50);
    }
}

// 检查I2C总线状态
void I2C_Bus_Check(void) {
    // 注意：这里需要读取SDA和SCL引脚状态
    // 由于您已实现引脚操作，这里需要根据您的具体实现来读取引脚状态
    
    log_i("I2C Bus check:\r\n");
    
    // 这里需要根据您的具体硬件实现来读取引脚电平

    if(SDA_READ()==0) log_i("SDA line low - bus busy or shorted");
    if(SCL_READ()==0) log_i("SCL line low - bus busy or shorted");

}


#endif

