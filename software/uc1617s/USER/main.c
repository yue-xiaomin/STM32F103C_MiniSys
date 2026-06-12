#include "delay.h"
#include "led.h"
#include "usart.h"
#include "rtc.h"
#include "elog.h"
//#include "soft_i2c.h"
#include "eeprom.h"
#include "eeprom_data.h"
#include "dwt_timer.h"


//#include "sw_i2c.h"
//#include "hw_i2c.h"
#include "uc1617s_conf.h"
#include "uc1617s_lcd.h"
#include "uc1617s_font.h"


//void LCD_GPIO_Init(void)
//{
//    GPIO_InitTypeDef gpio;

//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

//    /* 控制引脚: PA1-BL, PA2-RST, PA3-RS(悬空), PA4-CS (推挽输出) */
//    gpio.GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
//    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
//    gpio.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &gpio);

//    /* ★ I2C 模式: CS 必须拉低, 否则芯片不响应 I2C 总线 */
//    GPIO_ResetBits(GPIOA, GPIO_Pin_4);   /* CS = 0 (始终选中) */
//    GPIO_SetBits(GPIOA, GPIO_Pin_2);           /* RST = 1 (释放复位) */
//    GPIO_ResetBits(GPIOA, GPIO_Pin_1);         /* BL  = 0 (背光关)   */
//}


/*===========================================================================
 * 字体测试 — 四种字号全部展示, 128×96 屏幕
 *
 * 布局:
 *   y= 0~7   5×7  第1行  (8字高含间距)
 *   y= 8~14  5×7  第2行
 *   y= 16~23 6×8  第1行
 *   y= 24~31 6×8  第2行
 *   y= 33~40 8×8  第1行
 *   y= 41~48 8×8  第2行
 *   y= 50~65 8×16 第1行
 *   y= 66~81 8×16 第2行
 *   y= 84~95 分隔线 + 综合测试
 *===========================================================================*/
void uc1617s_font_test(void)
{
    uc1617s_clear(GRAY_WHITE);

    /* ── 5×7 ── */
    uc1617s_draw_string(0,  0, "5x7: ABCDEFGHIJKLM",   GRAY_BLACK, uc_font_5x7);
    uc1617s_draw_string(0,  8, "5x7: NOPQRSTUVWXYZ",   GRAY_DARK,  uc_font_5x7);
    uc1617s_draw_string(0, 16, "5x7: 0123456789 !@#", GRAY_BLACK, uc_font_5x7);

    /* ── 6×8 ── */
    uc1617s_draw_string(0, 25, "6x8: Hello!",           GRAY_BLACK, uc_font_6x8);
    uc1617s_draw_string(0, 34, "6x8: 0123456789",      GRAY_DARK,  uc_font_6x8);

    /* ── 8×8 ── */
    uc1617s_draw_string(0, 44, "8x8: Hello!",           GRAY_BLACK, uc_font_8x8);
    uc1617s_draw_string(0, 53, "8x8: 01234",            GRAY_DARK,  uc_font_8x8);

    /* ── 8×16 ── */
    uc1617s_draw_string(0, 63, "8x16 Hi!",              GRAY_BLACK, uc_font_8x16);
    uc1617s_draw_string(0, 80, "T=25.6C",               GRAY_DARK,  uc_font_8x16);

    /* ── 分隔线 ── */
    uc1617s_draw_line(0, 23, 127, 23, GRAY_LIGHT);
    uc1617s_draw_line(0, 42, 127, 42, GRAY_LIGHT);
    uc1617s_draw_line(0, 61, 127, 61, GRAY_LIGHT);

    uc1617s_refresh();
}

void uc1617s_refresh_test(void);
void uc1617s_cn_test(void);

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


#if USE_EEPROM
	SoftI2C_Init();
#ifdef SCAN_I2C_DEVICE
    log_i("=== I2C Device Scan Program ===");   
    I2C_ScanDevices();
//    I2C_AdvancedScan();    
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
#endif    
//    LCD_GPIO_Init();
    
//    SW_I2C_Init();    
//    log_i("=== I2C Device Scan Program ===");   
//    SW_I2C_ScanDevices();
//    SW_I2C_AdvancedScan();    
//    SW_I2C_QuickScan();

//    HW_I2C_Init();
//    log_i("=== I2C Device Scan Program ==="); 
//    HW_I2C_ScanDevices();
    
    /* 1. 选择传输方式 (与 uc1617s_conf.h 中的宏对应) */
#ifdef UC_USE_SW_I2C
    uc1617s_gfx_init(&uc_bus_sw_i2c);
#elif defined(UC_USE_HW_I2C)
    uc1617s_gfx_init(&uc_bus_hw_i2c);
#elif defined(UC_USE_SPI)
    uc1617s_gfx_init(&uc_bus_spi);
#elif defined(UC_USE_DMA_SPI)
    uc1617s_gfx_init(&uc_bus_dma_spi);
#else
    #error "请在 uc1617s_conf.h 中选择一种传输方式!"
#endif



    //uc1617s_cn_test();
    uc1617s_clear(GRAY_WHITE);
    uc1617s_draw_image_raw(11, 0, image_data, 105, 96);
	while(1)
	{
        dwt_start_timer();
		LED = !LED;
		delay_ms(1500);
        dwt_stop_timer();

		//log_d("time=%ldus,%ldms", dwt_get_elapsed_time_us(),dwt_get_elapsed_time_ms());

	}

}

