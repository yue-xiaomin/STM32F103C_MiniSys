/**
 * @file osal_main.c
 * @brief osal操作系统运行主函数，添加任务在此文件中添加
 * @version 0.1
 * @date 
 * @author 
 */
#include "sys.h"
#include "led.h"

#include "osal_main.h"


uint8 led_task_id;                    //记录打印任务的任务ID

/**
 * @brief 任务初始化
 * @param task_id [初始化时分配给当前任务的任务ID，标记区分每一个任务]
 */
void led_task_init(uint8 task_id)
{
    led_task_id = task_id;
    // 1. 初始化LED
    LED_Init();        //LED初始化
    // 2. 启动自动重装的定时器，每 1000 毫秒产生 LED_TASK_FLASH_EVT 事件
    // 详细说明：osal_start_reload_timer(任务ID, 事件ID, 毫秒时间)
    osal_start_reload_timer(led_task_id, LED_FLASH_EVT, 1000 / TICK_PERIOD_MS);
}

/**
 * @brief 当前任务的事件回调处理函数
 * @param task_id       [任务ID]
 * @param task_event    [收到的本任务事件]
 * @return uint16       [未处理的事件]
 */
uint16 led_task_event_process(uint8 task_id, uint16 task_event)
{
    // 安全检查：如果收到的任务ID与当前任务不匹配，直接退出
    if (task_id != led_task_id)
    {
        return 0;
    }
    
    if(task_event & SYS_EVENT_MSG)       //判断是否为系统消息事件
    {
        osal_sys_msg_t *msg_pkt;
        msg_pkt = (osal_sys_msg_t *)osal_msg_receive(task_id);      //从消息队列获取一条消息

        while(msg_pkt)
        {
            switch(msg_pkt->hdr.event)      //判断该消息事件类型
            {
                default:
                    break;
            }

            osal_msg_deallocate((uint8 *)msg_pkt);                  //释放消息内存
            msg_pkt = (osal_sys_msg_t *)osal_msg_receive(task_id);  //读取下一条消息
        }

        // return unprocessed events
        return (task_event ^ SYS_EVENT_MSG);
    }

    if(task_event & LED_FLASH_EVT)
    {
        LED = !LED;

        return task_event ^ LED_FLASH_EVT; //处理完后需要清除事件位
    }

    return 0;
}



void osal_main(void)
{
    //系统硬件、外设等初始化

    //禁止中断
    HAL_DISABLE_INTERRUPTS();

    //osal操作系统初始化
    osal_init_system();

    //添加任务
    osal_add_Task(print_task_init, print_task_event_process, 1);
    osal_add_Task(statistics_task_init, statistics_task_event_process, 2);
    osal_add_Task(led_task_init, led_task_event_process, 3);

    //添加的任务统一进行初始化
    osal_Task_init();

    osal_mem_kick();

    //允许中断
    HAL_ENABLE_INTERRUPTS();

    //设置初始任务事件，上电就需要自动轮询的任务事件可在此添加

    //启动osal系统，不会再返回
    osal_start_system();
}
