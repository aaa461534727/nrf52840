// ˵����
// main�ļ�
//******************************************************************************

//******************************************************************
// ͷ�ļ�����
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
// brief : ��ʼ��log��ӡ
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
// brief : ��ʼ����Դ����
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
// brief : �������״̬�Ĺ��ܣ�������ѭ����
// details : �����κι������־������Ȼ������ֱ����һ���¼�����
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
// brief : ������
//
// param : none
//
// return : none
int main(void)
{
    // ��ʼ��
    log_init(); //��ʼ��LOG��ӡ����RTT����
		UART_Init();//��ʼ������
    //����RTT�ն���ʾ��������ɫΪ��ɫ������������ "\033[32m"
    NRF_LOG_ERROR("\033[32m Text Color: Green\r\n");
    // ��ӡ����
    NRF_LOG_INFO("ble_central_scan_all is running...");
		printf("ble_central_scan_all is running...\r\n");
		ota_flash_init();
    power_management_init();// ��ʼ����Դ����      
		ble_init();// ��ʼ��BLEջ�� 
		wdt_start();
    // ������ѭ��
    for (;;)
    {
				printf("-------------duf test-------10210---------\r\n");
				OTA_Receive_Data();
        idle_state_handle();   // ����״̬����
				wdt_feed();
    }
}
