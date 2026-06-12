#include "sw_i2c.h"



// 初始化软件I2C
void SW_I2C_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(SW_I2C_RCC_APB2Periph, ENABLE);
    
    // 配置SCL和SDA为开漏输出模式
    GPIO_InitStructure.GPIO_Pin = SW_I2C_SCL_PIN | SW_I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SW_I2C_PORT, &GPIO_InitStructure);
    
    // 初始状态拉高
    SCL_HIGH();
    SDA_HIGH();
    
    i2c_delay_ms(10); // 等待EEPROM上电稳定
}

// I2C起始信号
void SW_I2C_Start(void) {
    SDA_HIGH();
    SCL_HIGH();
    i2c_delay_us(5);
    SDA_LOW();
    i2c_delay_us(5);
    SCL_LOW();
    i2c_delay_us(5);
}

// I2C停止信号
void SW_I2C_Stop(void) {
    SDA_LOW();
    SCL_LOW();
    i2c_delay_us(5);
    SCL_HIGH();
    i2c_delay_us(5);
    SDA_HIGH();
    i2c_delay_us(5);
}

// 等待ACK应答
uint8_t SW_I2C_Wait_Ack(void) {
    uint16_t timeout = 1000; // 超时计数器
    
    SDA_HIGH(); // 释放SDA
    i2c_delay_us(2);
    SCL_HIGH();
    i2c_delay_us(2);
    
    // 检查SDA是否为低电平(ACK)
    while(SDA_READ() == Bit_SET) {
        timeout--;
        if(timeout == 0) {
            SCL_LOW();
            return 0; // 超时，未收到ACK
        }
        i2c_delay_us(1);
    }
    
    SCL_LOW();
    i2c_delay_us(2);
    return 1; // 收到ACK
}

// 发送ACK
void SW_I2C_Ack(void) {
    SCL_LOW();
    i2c_delay_us(2);
    SDA_LOW();
    i2c_delay_us(2);
    SCL_HIGH();
    i2c_delay_us(5);
    SCL_LOW();
    i2c_delay_us(2);
    SDA_HIGH(); // 释放SDA
    i2c_delay_us(2);
}

// 发送NACK
void SW_I2C_NAck(void) {
    SCL_LOW();
    i2c_delay_us(2);
    SDA_HIGH();
    i2c_delay_us(2);
    SCL_HIGH();
    i2c_delay_us(5);
    SCL_LOW();
    i2c_delay_us(2);
}

// 发送一个字节
void SW_I2C_SendByte(uint8_t data) {
    uint8_t i;
    
    for(i = 0; i < 8; i++) {
        SCL_LOW();
        i2c_delay_us(2);
        
        if(data & 0x80) {
            SDA_HIGH();
        } else {
            SDA_LOW();
        }
        i2c_delay_us(2);
        
        SCL_HIGH();
        i2c_delay_us(5);
        
        data <<= 1;
    }
    
    SCL_LOW();
    i2c_delay_us(2);
    SDA_HIGH(); // 释放SDA
    i2c_delay_us(2);
}

// 接收一个字节
uint8_t SW_I2C_ReadByte(uint8_t ack) {
    uint8_t i, data = 0;
    
    SDA_HIGH(); // 释放SDA
    
    for(i = 0; i < 8; i++) {
        data <<= 1;
        
        SCL_LOW();
        i2c_delay_us(2);
        SCL_HIGH();
        i2c_delay_us(2);
        
        if(SDA_READ()) {
            data |= 0x01;
        }
        
        i2c_delay_us(2);
    }
    
    SCL_LOW();
    i2c_delay_us(2);
    
    if(ack) {
        SW_I2C_Ack();
    } else {
        SW_I2C_NAck();
    }
    
    return data;
}


#ifdef SCAN_I2C_DEVICE
// 扫描I2C设备
void SW_I2C_ScanDevices(void) {
    uint8_t i, j;
    uint8_t deviceCount = 0;
    uint8_t foundDevices[128] = {0}; // 存储找到的设备地址
    
    i2c_output("Starting I2C device scan...");
    i2c_output("Address range: 0x08 - 0x77");
    i2c_output("Scanning");
    
    // 扫描所有可能的I2C地址 (0x08 - 0x77)
    for(i = 0x08; i <= 0x77; i++) {
        SW_I2C_Start();
        SW_I2C_SendByte(i << 1); // 发送写地址(地址左移1位，写位=0)
        
        if(SW_I2C_Wait_Ack()) {
            // 收到ACK，设备存在
            foundDevices[deviceCount] = i;
            deviceCount++;
            i2c_output("Device found: 0x%02X", i);
        }
        
        SW_I2C_Stop();
        i2c_delay_us(10); // 额外延时
    }
    
    i2c_output("Scan completed!");
    i2c_output("Total found %d I2C devices:", deviceCount);
    
    for(j = 0; j < deviceCount; j++) {
        i2c_output("Device %d: Address 0x%02X", j+1, foundDevices[j]);
        SW_I2C_Scan_ShowDeviceInfo(foundDevices[j]);
    }
    
    if(deviceCount == 0) {
        i2c_output("No I2C devices found");
    }
}

// 显示找到的I2C设备信息
void SW_I2C_Scan_ShowDeviceInfo(uint8_t addr) {
    switch(addr) {
        case 0x50: case 0x51: case 0x52: case 0x53:
        case 0x54: case 0x55: case 0x56: case 0x57:
            i2c_output(" (EEPROM - 24Cxx series)");
            break;
            
        case 0x68:
            i2c_output(" (RTC - DS1307/DS3231)");
            break;
            
        case 0x27: case 0x3F:
            i2c_output(" (LCD - PCF8574 expansion board)");
            break;
            
        case 0x48: case 0x49: case 0x4A: case 0x4B:
            i2c_output(" (ADC - PCF8591)");
            break;
            
        case 0x1D:
            i2c_output(" (Accelerometer - ADXL345)");
            break;
            
        case 0x5A:
            i2c_output(" (Temperature sensor - MLX90614)");
            break;
            
        case 0x76: case 0x77:
            i2c_output(" (Temp/Humi/Pressure - BME280)");
            break;
            
        case 0x23:
            i2c_output(" (Light sensor - BH1750)");
            break;
            
        case 0x40:
            i2c_output(" (Temp/Humi sensor - HTU21D/SHT21)");
            break;
            
        case 0x60:
            i2c_output(" (FM radio - TEA5767)");
            break;
            
        case 0x29:
            i2c_output(" (Color sensor - TCS34725)");
            break;
            
        case 0x19:
            i2c_output(" (Accelerometer - LIS3DH)");
            break;
            
        case 0x6B:
            i2c_output(" (Gyroscope - L3G4200D)");
            break;
            
        case 0x0A:
            i2c_output(" (OLED display - SSD1306)");
            break;
            
        default:
            i2c_output(" (Unknown device)");
            break;
    }
}

// 快速扫描（只扫描常见地址）
void SW_I2C_QuickScan(void) {
    uint8_t commonAddrs[] = {
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, // IO扩展
        0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, // LCD
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, // 温湿度
        0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, // ADC
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, // EEPROM
        0x68,                                           // RTC
        0x76, 0x77,                                     // BME280
        0x00 // 结束标记
    };
    
    uint8_t i = 0;
    uint8_t deviceCount = 0;
    
    i2c_output("Quick scan for common I2C devices...");
    
    while(commonAddrs[i] != 0x00) {
        SW_I2C_Start();
        SW_I2C_SendByte(commonAddrs[i] << 1);
        
        if(SW_I2C_Wait_Ack()) {
            i2c_output("Device found: 0x%02X", commonAddrs[i]);
            SW_I2C_Scan_ShowDeviceInfo(commonAddrs[i]);
            deviceCount++;
        }
        
        SW_I2C_Stop();
        i2c_delay_us(10);
        i++;
    }
    
    i2c_output("Quick scan completed, found %d devices", deviceCount);
}

// 测试特定设备地址
uint8_t SW_I2C_TestDevice(uint8_t addr) {
    uint8_t writeAddr = addr << 1;
    uint8_t result = 0;
    
    SW_I2C_Start();
    SW_I2C_SendByte(writeAddr);
    
    if(SW_I2C_Wait_Ack()) {
        result = 1; // 设备存在
    } else {
        result = 0; // 设备不存在
    }
    
    SW_I2C_Stop();
    return result;
}

// 高级扫描功能（测试设备是否应答读写）
void SW_I2C_AdvancedScan(void) {
    uint8_t i;
    uint8_t writeAddr, readAddr;
    
    i2c_output("Advanced scan - testing read/write addresses:");
    
    for(i = 0x08; i <= 0x77; i++) {
        writeAddr = i << 1;      // 写地址
        readAddr = (i << 1) | 1; // 读地址
        
        // 测试写地址
        SW_I2C_Start();
        SW_I2C_SendByte(writeAddr);
        if(SW_I2C_Wait_Ack()) {
            SW_I2C_Stop();
            
            // 测试读地址
            SW_I2C_Start();
            SW_I2C_SendByte(readAddr);
            if(SW_I2C_Wait_Ack()) {
                i2c_output("Device 0x%02X: Write=0x%02X, Read=0x%02X [Full]", 
                       i, writeAddr, readAddr);
            } else {
                i2c_output("Device 0x%02X: Write=0x%02X, Read=No response [Write only?]", 
                       i, writeAddr);
            }
            SW_I2C_Stop();
        } else {
            SW_I2C_Stop();
        }
        
        i2c_delay_us(50);
    }
}

// 检查I2C总线状态
void SW_I2C_Bus_Check(void) {
    i2c_output("I2C Bus check:\r\n");
    
    if(SDA_READ()==0) i2c_output("SDA line low - bus busy or shorted");
    if(SCL_READ()==0) i2c_output("SCL line low - bus busy or shorted");
}

#endif
