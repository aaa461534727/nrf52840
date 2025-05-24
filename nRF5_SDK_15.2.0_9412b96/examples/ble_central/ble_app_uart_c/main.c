// 说明：
// main文件
//******************************************************************************

//******************************************************************
// 头文件定义
//
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "nordic_common.h"
#include "app_error.h"
#include "app_uart.h"
#include "ble_db_discovery.h"
#include "app_timer.h"
#include "app_util.h"
#include "bsp_btn_ble.h"
#include "ble.h"
#include "ble_gap.h"
#include "ble_hci.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "ble_nus_c.h"
#include "nrf_ble_gatt.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_ble_scan.h"
#include "app_timer.h"
#include "nrf_uart.h"
#include "nrf_uarte.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_delay.h"

#include "common.h"
#include "app_ota_flash.h"
#include "board_uart.h"
#include "nrf_drv_wdt.h"
#include "nrf_drv_clock.h"
#include "app_wdt_handle.h"
#include "app_ble_server.h"

//******************************************************************
// fn : log_init
//
// brief : 初始化log打印
//
// param : none
//
// return : none
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

//******************************************************************
// fn : power_management_init
//
// brief : 初始化电源管理
//
// param : none
//
// return : none
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

//******************************************************************
// fn : idle_state_handle
//
// brief : 处理空闲状态的功能（用于主循环）
// details : 处理任何挂起的日志操作，然后休眠直到下一个事件发生
//
// param : none
//
// return : none
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}

//******************************************************************
// fn : main
//
// brief : 主函数
//
// param : none
//
// return : none
int main(void)
{
    // 初始化
    log_init(); //初始化LOG打印，由RTT工作
		UART_Init();//初始化串口
    //设置RTT终端显示的字体颜色为绿色，控制命令是 "\033[32m"
    NRF_LOG_ERROR("\033[32m Text Color: Green\r\n");
    // 打印名称
    NRF_LOG_INFO("ble_central_scan_all is running...");
		printf("ble_central_scan_all is running...\r\n");
		ota_flash_init();
    power_management_init();// 初始化电源控制      
		ble_init();// 初始化BLE栈堆 
		wdt_start();
    // 进入主循环
    for (;;)
    {
				printf("-------------duf test-------10210---------\r\n");
				OTA_Receive_Data();
        idle_state_handle();   // 空闲状态处理
				wdt_feed();
    }
}
