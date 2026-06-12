#include "delay.h"
#include "led.h"
#include "usart.h"
#include "rtc.h"
#include "elog.h"
#include "soft_i2c.h"
#include "eeprom.h"
#include "eeprom_data.h"
#include "dwt_timer.h"
#include "lcd_spi.h"
#include "font.h"
#define delay_ms_main delay_ms

/* 外部引用字库数据 (从数据手册中的 text[] 等数组导入) */
//extern const uint8_t text[];
/* extern const uint8_t bmp1[];  -- 如需显示图片 */


/* ============================================================
 *  测试图片1: 128×96 渐变 (直接生成 IC 格式, 无需转换)
 *
 *  从左到右: 白→浅灰→深灰→黑
 *  每32列一个灰度级
 *  格式: LSB-first (bit[1:0]=最左像素)
 * ============================================================ */
static uint8_t img_gradient_ic[3072];   /* 128×96/4 = 3072 */

static void generate_gradient_ic(void)
{
    uint16_t i = 0;
    uint8_t row, col;
    uint8_t px0, px1, px2, px3;

    for (row = 0; row < 96; row++)
    {
        for (col = 0; col < 128; col += 4)
        {
            px0 = (col + 0) / 32; if (px0 > 3) px0 = 3;
            px1 = (col + 1) / 32; if (px1 > 3) px1 = 3;
            px2 = (col + 2) / 32; if (px2 > 3) px2 = 3;
            px3 = (col + 3) / 32; if (px3 > 3) px3 = 3;

            /* ★ LSB-first: bit[1:0]=px0(最左) */
            img_gradient_ic[i++] = (px3 << 6) | (px2 << 4) | (px1 << 2) | px0;
        }
    }
}

/* ============================================================
 *  测试图片2: 64×64 棋盘格 (IC 格式)
 * ============================================================ */
static uint8_t img_checker_ic[1024];    /* 64×64/4 = 1024 */

static void generate_checker_ic(void)
{
    uint16_t i = 0;
    uint8_t row, col;
    uint8_t px0, px1, px2, px3;

    for (row = 0; row < 64; row++)
    {
        for (col = 0; col < 64; col += 4)
        {
            px0 = ((col + 0) / 8 + row / 8) % 4;
            px1 = ((col + 1) / 8 + row / 8) % 4;
            px2 = ((col + 2) / 8 + row / 8) % 4;
            px3 = ((col + 3) / 8 + row / 8) % 4;

            img_checker_ic[i++] = (px3 << 6) | (px2 << 4) | (px1 << 2) | px0;
        }
    }
}

void test(void)
{

    LCD_Init();
   

    generate_gradient_ic();
    generate_checker_ic();

    /* ===== 测试1: 全屏渐变 (IC格式, 无需转换) ===== */
    LCD_ShowImage(0, 0, 128, 96, img_gradient_ic, IMG_FMT_GRAY4_LSB);
    delay_ms_main(5000);

    /* ===== 测试2: 棋盘格 (IC格式) ===== */
    LCD_Clear();
    LCD_Refresh();
    LCD_ShowImage(0, 0, 64, 64, img_checker_ic, IMG_FMT_GRAY4_LSB);
    delay_ms_main(5000);

    /* ===== 测试3: 棋盘格 + 文字混合 ===== */
    /* 先刷背景 */
    LCD_Clear();
    LCD_ShowString(68, 4, "LCD", GRAY_BLACK, GRAY_WHITE);
    LCD_ShowString(68, 20, "128x96", GRAY_DARK, GRAY_WHITE);
    LCD_ShowString(68, 36, "4Gray", GRAY_DARK, GRAY_WHITE);
    LCD_Refresh();
    /* 再覆盖图片区域 */
    LCD_ShowImage(0, 0, 64, 64, img_checker_ic, IMG_FMT_GRAY4_LSB);
    delay_ms_main(5000);

    /* ===== 测试5: 显示真实图片 (示例: 用Img2Lcd生成) ===== */
    /*
     * Img2Lcd 操作步骤:
     * 1. 打开图片文件 (建议 128×96 或更小)
     * 2. 设置:
     *    扫描方式: 水平扫描
     *    最大宽度: 128, 最大高度: 96
     *    灰度: 4灰度
     *    输出格式: C语言数组
     *    不包含头信息
     *    MSB first
     * 3. 生成 .c 文件
     * 4. 把数组复制到工程中
     * 5. 调用:
     *    LCD_Clear();
     *    LCD_ShowImage(0, 0, 128, 96, your_image_data, IMG_FMT_GRAY4_MSB);
     *    LCD_Refresh();
     */
//    LCD_Clear();
//    LCD_ShowImage(0, 0, 128, 96, gImage_ameng, IMG_FMT_GRAY4_MSB);
//    LCD_Refresh();
//    delay_ms_main(5000);
//    /* ===== 主循环 ===== */
//    while (1)
//    {
//        LCD_Clear();
//        LCD_ShowString(0, 0, "UC1617S LCD", GRAY_BLACK, GRAY_WHITE);
//        LCD_ShowString(0, 16, "128x96 4Gray", GRAY_DARK, GRAY_WHITE);
//        LCD_Refresh();
//        delay_ms_main(2000);

//        LCD_Clear();
//        LCD_ShowImage(0, 0, 128, 96, img_gradient, IMG_FMT_GRAY4_MSB);
//        LCD_Refresh();
//        delay_ms_main(2000);

//        LCD_Clear();
//        LCD_ShowImage(32, 16, 64, 64, img_checker, IMG_FMT_GRAY4_MSB);
//        LCD_Refresh();
//        delay_ms_main(2000);
//    }



}





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
    
    
    
    test();

    
    
	while(1)
	{
        dwt_start_timer();
		LED = !LED;
		delay_ms(1500);
        dwt_stop_timer();

		//log_d("time=%ldus,%ldms", dwt_get_elapsed_time_us(),dwt_get_elapsed_time_ms());

	}

}

