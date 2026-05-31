#include "delay.h"
#include "led.h"
#include "usart.h"
#include "rtc.h"
#include "elog.h"
#include "soft_i2c.h"
#include "eeprom.h"
#include "eeprom_data.h"
#include "dwt_timer.h"

#include "qfplib-m3.h"

static float a,b,c;
static float sinA,cosB,tanC;


int main(void)
{

    
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);// 设置中断优先级分组2
	
	LED_Init();        //LED初始化
	delay_init();      //延时初始化
	uart_init(115200);	 	//串口初始化为115200
	RTC_Init();
	elog_init();

	elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_LVL | ELOG_FMT_TIME);                          
	elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TIME); 
	elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TIME);  
	elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TIME);  
	elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_LVL | ELOG_FMT_TIME | ELOG_FMT_DIR | ELOG_FMT_FUNC | ELOG_FMT_LINE); 
//    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL); 
	elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_LVL | ELOG_FMT_TIME); 	
//	elog_set_text_color_enabled(true); 
	elog_start();	
//    log_a("Hello EasyLogger!");
//    log_e("Hello EasyLogger!");
//    log_w("Hello EasyLogger!");
//    log_i("Hello EasyLogger!");
//    log_d("Hello EasyLogger!");

	RTC_Set(2025,9,10,15,0,0);	

	SoftI2C_Init();
#ifdef SCAN_I2C_DEVICE
    log_i("=== I2C Device Scan Program ===");   
//    I2C_ScanDevices();
    I2C_AdvancedScan();    
//    I2C_QuickScan();
    
    log_i("Testing specific devices:");
    if(I2C_TestDevice(0x50)) {
        log_i("AT24C04 EEPROM present (Address 0x50)");
    } else {
        log_i("AT24C04 EEPROM not present (Address 0x50)");
    }
    
    if(I2C_TestDevice(0x68)) {
        log_i("RTC present (Address 0x68)");
    } else {
        log_i("RTC not present (Address 0x68)");
    }


    
#endif
    
	uint8_t test_result = AT24C02_Test();	
	if(test_result == 0) {
		log_i("AT24C02 is ok.");
	} else {
		log_e("AT24C02 test fail!");
	}
	
    // 写入数据
    uint8_t write_data[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
    AT24C02_Write_NBytes(0x10, write_data, 8);
    
    // 读取数据
    uint8_t read_data[10]={0};
    AT24C02_Read_NBytes(0x10, read_data, 8);
    log_i("eeprom data:%s", read_data);
    
    
    a = 1.1;
    b = 0.6;
    c = 3.1415926;
    sinA = qfp_fsin(a);
    cosB = qfp_fcos(b);
    tanC = qfp_ftan(c);
    printf("sinA:%0.3f\r\n", sinA);
    printf("cosB:%0.3f\r\n", cosB);
    printf("tanC:%0.3f\r\n", tanC);
	while(1)
	{
        dwt_start_timer();
		LED = !LED;
		delay_ms(1500);
        dwt_stop_timer();

		log_d("time=%ldus,%ldms", dwt_get_elapsed_time_us(),dwt_get_elapsed_time_ms());

	}

}

