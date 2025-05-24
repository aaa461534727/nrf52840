//******************************************************************************
//                                www.ghostyu.com
//
//                 Copyright (c) 2019-2020, WUXI Ghostyu Co.,Ltd.
//                                All rights reserved.
//
// FileName : mian.c
// Date     : 2019-06-24 10:34
// Version  : V0001
// 历史记录 : 1.第一次创建
//
// 说明：
// main文件
//******************************************************************************

//******************************************************************
// 头文件定义
//
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
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
#include "ble_nus.h"
#include "ble_nus_c.h"
#include "nrf_ble_gatt.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_ble_scan.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "common.h"
#include "ble_advertising.h"
#include "nrf_ble_qwr.h"
#include "board_uart.h"
#include "ble_srv_common.h"
#include "app_ble_server.h"

//******************************************************************
// 常量定义
//
#define APP_BLE_CONN_CFG_TAG    1   // 
#define APP_BLE_OBSERVER_PRIO   3   // BLE应用程序的观察者优先级,无需修改此值。
#define APP_TIME_INTERVAL 							APP_TIMER_TICKS( 1000 )  //每隔1000ms进入一次

//主机
APP_TIMER_DEF (m_app_timer_id); 	//定义软件定时器实例名称
NRF_BLE_SCAN_DEF (m_scan);           // 定义扫描实例的名称
// 定义扫描参数
static ble_gap_scan_params_t m_scan_params = 
{
    .active        = 1,                            // 1为主动扫描，可获得广播数据及扫描回调数据
    .interval      = NRF_BLE_SCAN_SCAN_INTERVAL,   // 扫描间隔：160*0.625 = 100ms  
    .window        = NRF_BLE_SCAN_SCAN_WINDOW,     // 扫描窗口：80*0.625 = 50ms   
    .timeout       = NRF_BLE_SCAN_SCAN_DURATION,   // 扫描持续的时间：设置为0，一直扫描，直到明确的停止扫描
    .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,   // 扫描所有BLE设备，不做限制
    .scan_phys     = BLE_GAP_PHY_1MBPS,            // 扫描1MBPS的PHY
};

//定义结构体
typedef struct rid{
	//数据域 
	uint8_t msg_type; //报文类型
	uint8_t base_msg[31];//基本ID报文0x0
	uint8_t pos_msg[31];//位置向量报文0x1
	uint8_t diy_msg[31];//自定义保留报文0x2
	uint8_t run_msg[31];//运行描述报文0x3
	uint8_t sys_msg[31];//系统报文0x4
	uint8_t ope_msg[31];//操作人员报文0x5
	uint8_t mac[6];//蓝牙mac地址
}RID;

//从机
BLE_LED_DEF(m_led);   // LED实例
BLE_CUS_DEF(m_cus);
NRF_BLE_QWR_DEF(m_qwr);             // 队列实例
NRF_BLE_GATT_DEF(m_gatt);           // GATT实例
BLE_ADVERTISING_DEF(m_advertising); // 广播实例
#define DEVICE_NAME  "GY-NRF52832"  // 定义设备名称

// 定义广播的UUID
static ble_uuid_t m_adv_uuids[] = {{UUID_SERVICE, BLE_UUID_TYPE_BLE},{CUSTOM_SERVICE_UUID, BLE_UUID_TYPE_BLE}};
// 定义连接的handle
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;

//共用
//定时器回调函数
void app_timeout_handler( void *p_context )
{
	uint8_t sample_data[] = {0x01, 0x02, 0x03};
	if (m_cus.notify_enabled && 
    m_cus.conn_handle != BLE_CONN_HANDLE_INVALID) {
    // 允许发送通知
		uint32_t err_code = ble_cus_send_data(&m_cus, sample_data, strlen(sample_data));
		if (err_code != NRF_SUCCESS) {
				NRF_LOG_ERROR("send fail: 0x%X", err_code);
				// 资源不足时可延时重试
		}
	}

}

//******************************************************************
// 定时器初始化函数
//	m_app_timer_id，定时器事件ID号，对特定ID触发事件
//	APP_TIMER_MODE_REPEATED，定时器重复触发模式
//	app_timeout_handler 定时器中断函数，一定要确保和上面的函数名称一致！
// return : none
static void timers_init(void)
{
		ret_code_t err_code;
	
    // Initialize timer module, making it use the scheduler
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_app_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                app_timeout_handler); 
    APP_ERROR_CHECK(err_code);    
		// Start application timers.
    err_code = app_timer_start(m_app_timer_id, APP_TIME_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}


//******************************************************************
// fn : assert_nrf_callback
//
// brief : 在SoftDevice中用于处理asserts的函数
// details : 如果SoftDevice中存在assert，则调用此函数
// warning : 这个处理程序只是一个例子，不适用于最终产品。 您需要分析产品在assert时应该如何反应
// warning : 如果assert来自SoftDevice，则系统只能在复位时恢复
//
// param : line_num     失败的assert的行号
//         p_file_name  识别的assert的文件名
//
// return : none
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}


///////////////////主机//////////////////////
//******************************************************************
// fn : scan_start
//
// brief : 开始扫描
//
// param : none
//
// return : none
static void scan_start(void)
{
    ret_code_t ret;

    ret = nrf_ble_scan_start(&m_scan);
    APP_ERROR_CHECK(ret);
}


//******************************************************************
// fn : scan_evt_handler
//
// brief : 处理扫描回调事件
//
// param : scan_evt_t  扫描事件结构体
//
// return : none
static void scan_evt_handler(scan_evt_t const * p_scan_evt)
{
 
  // 0x1E16FAFF0D 每个rid报文都会携带这个参数
    RID rid_info;//创建节点结构体准备保存数据添加进链表
		uint8_t buf[41]={0xAA,0xBB};//存放最终数据
		int i=0;
    switch(p_scan_evt->scan_evt_id)
    {
        // 未过滤的扫描数据
        case NRF_BLE_SCAN_EVT_NOT_FOUND:
        {
          // 判断是否为扫描回调数据
          if(p_scan_evt->params.p_not_found->type.scan_response)
          {
            if(p_scan_evt->params.p_not_found->data.len)    // 存在扫描回调数据
            {
//              NRF_LOG_INFO("ScanRsp Data:%s",Util_convertHex2Str(p_scan_evt->params.p_not_found->data.p_data,p_scan_evt->params.p_not_found->data.len));
            }
            else
            {
              NRF_LOG_INFO("ScanRsp Data:%s","NONE");
            }
            
          }
          else  // 否则为广播数据
          {
//            //打印设备RSSI信号
//            NRF_LOG_INFO("Device RSSI: %ddBm",p_scan_evt->params.p_not_found->rssi);
//            // 打印扫描的设备MAC
//            NRF_LOG_INFO("Device MAC:  %s",
//                         Util_convertBdAddr2Str((uint8_t*)p_scan_evt->params.p_not_found->peer_addr.addr));           
						//UART_WriteData(UART0,(uint8_t *)&run_buf,sizeof(run_buf));
						//NRF_LOG_INFO("nrf52840 adv is running!\n");
            if(p_scan_evt->params.p_not_found->data.len&&p_scan_evt->params.p_not_found->data.len>6)    // 存在广播数据,过滤空数据
            {
							//打印广播数据
             NRF_LOG_INFO("Adv Data: %s",Util_convertHex2Str(p_scan_evt->params.p_not_found->data.p_data,p_scan_evt->params.p_not_found->data.len));
              //RID数据头部比对成功
              if(p_scan_evt->params.p_not_found->data.p_data[0] == 0x1E && p_scan_evt->params.p_not_found->data.p_data[1] == 0x16 &&\
                  p_scan_evt->params.p_not_found->data.p_data[2] == 0xFA && p_scan_evt->params.p_not_found->data.p_data[3] == 0xFF &&\
                   p_scan_evt->params.p_not_found->data.p_data[4] == 0x0D)
              {
                // 打印扫描的设备MAC,信道
                NRF_LOG_INFO("---------Device MAC:  %s  Channel Index : %d   Typre : %X ------------",Util_convertBdAddr2Str((uint8_t*)p_scan_evt->params.p_not_found->peer_addr.addr),p_scan_evt->params.p_not_found->ch_index,p_scan_evt->params.p_not_found->data.p_data[6]>>4);
                //打印广播数据
                NRF_LOG_INFO("Adv Data:    %s \r\n",Util_convertHex2Str(p_scan_evt->params.p_not_found->data.p_data,p_scan_evt->params.p_not_found->data.len));   
                if(p_scan_evt->params.p_not_found->data.p_data[6]>>4 == 0x0)//基本ID报文
                {
									//提取广播数据
									rid_info.msg_type = 0x0;
									//将解析的数据存到数组转成16进制字节方式发送
                  memcpy(rid_info.mac ,p_scan_evt->params.p_not_found->peer_addr.addr,sizeof(p_scan_evt->params.p_not_found->peer_addr.addr));
                  memcpy(rid_info.base_msg,p_scan_evt->params.p_not_found->data.p_data,31);										
									//printf("%s ",Util_convertHex2Str(p_scan_evt->params.p_not_found->data.p_data,p_scan_evt->params.p_not_found->data.len));								
//									UART_WriteData(UART1,(uint8_t *)&data_head,2);
//									UART_WriteData(UART1,(uint8_t *)&rid_info.mac,sizeof(p_scan_evt->params.p_not_found->peer_addr.addr));
//									UART_WriteData(UART1,(uint8_t *)&rid_info.base_msg,p_scan_evt->params.p_not_found->data.len);
//									UART_WriteData(UART1,(uint8_t *)&data_end,2);
									for(i=0;i<sizeof(p_scan_evt->params.p_not_found->peer_addr.addr)+p_scan_evt->params.p_not_found->data.len+2;i++)
									{									
										if(i==8||i>8) buf[i] = rid_info.base_msg[i-8];
											else buf[i+2]= rid_info.mac[i];
									}
									buf[8+p_scan_evt->params.p_not_found->data.len] = 0xCC;
									buf[8+p_scan_evt->params.p_not_found->data.len+1] = 0xDD;
									UART_WriteData(UART1,(uint8_t *)&buf,sizeof(buf));
									UART_WriteData(UART0,(uint8_t *)&buf,sizeof(buf));
                }
                else if(p_scan_evt->params.p_not_found->data.p_data[6]>>4 == 0x1)//位置报文
                {
                  //提取广播数据
									rid_info.msg_type = 0x1;
                  memcpy(rid_info.mac ,p_scan_evt->params.p_not_found->peer_addr.addr,sizeof(p_scan_evt->params.p_not_found->peer_addr.addr));
                  memcpy(rid_info.pos_msg,p_scan_evt->params.p_not_found->data.p_data,31);	
									//printf("%s ",Util_convertHex2Str(p_scan_evt->params.p_not_found->data.p_data,p_scan_evt->params.p_not_found->data.len));
//									UART_WriteData(UART1,(uint8_t *)&data_head,2);
//									UART_WriteData(UART1,(uint8_t *)&rid_info.mac,sizeof(p_scan_evt->params.p_not_found->peer_addr.addr));
//									UART_WriteData(UART1,(uint8_t *)&rid_info.pos_msg,p_scan_evt->params.p_not_found->data.len);
//									UART_WriteData(UART1,(uint8_t *)&data_end,2);			
									for(i=0;i<sizeof(p_scan_evt->params.p_not_found->peer_addr.addr)+p_scan_evt->params.p_not_found->data.len+2;i++)
									{									
										if(i==8||i>8) buf[i] = rid_info.pos_msg[i-8];
											else buf[i+2]= rid_info.mac[i];
									}
									buf[8+p_scan_evt->params.p_not_found->data.len] = 0xCC;
									buf[8+p_scan_evt->params.p_not_found->data.len+1] = 0xDD;
									UART_WriteData(UART1,(uint8_t *)&buf,sizeof(buf));		
									UART_WriteData(UART0,(uint8_t *)&buf,sizeof(buf));									
                }
                else if(p_scan_evt->params.p_not_found->data.p_data[6]>>4 == 0x2)//自定义报文
                {
                  //提取广播数据
									rid_info.msg_type = 0x2;
                  memcpy(rid_info.mac ,p_scan_evt->params.p_not_found->peer_addr.addr,sizeof(p_scan_evt->params.p_not_found->peer_addr.addr));
                  memcpy(rid_info.diy_msg,p_scan_evt->params.p_not_found->data.p_data,31);	
									//printf("%s ",Util_convertHex2Str(p_scan_evt->params.p_not_found->data.p_data,p_scan_evt->params.p_not_found->data.len));
//									UART_WriteData(UART1,(uint8_t *)&data_head,2);
//									UART_WriteData(UART1,(uint8_t *)&rid_info.mac,sizeof(p_scan_evt->params.p_not_found->peer_addr.addr));
//									UART_WriteData(UART1,(uint8_t *)&rid_info.diy_msg,p_scan_evt->params.p_not_found->data.len);
//									UART_WriteData(UART1,(uint8_t *)&data_end,2);				
									for(i=0;i<sizeof(p_scan_evt->params.p_not_found->peer_addr.addr)+p_scan_evt->params.p_not_found->data.len+2;i++)
									{									
										if(i==8||i>8) buf[i] = rid_info.diy_msg[i-8];
											else buf[i+2]= rid_info.mac[i];
									}
									buf[8+p_scan_evt->params.p_not_found->data.len] = 0xCC;
									buf[8+p_scan_evt->params.p_not_found->data.len+1] = 0xDD;
									UART_WriteData(UART1,(uint8_t *)&buf,sizeof(buf));
									UART_WriteData(UART0,(uint8_t *)&buf,sizeof(buf));									
                }
                else if(p_scan_evt->params.p_not_found->data.p_data[6]>>4 == 0x3)//运行报文
                {
                  //提取广播数据
									rid_info.msg_type = 0x3;
                  memcpy(rid_info.mac ,p_scan_evt->params.p_not_found->peer_addr.addr,sizeof(p_scan_evt->params.p_not_found->peer_addr.addr));
                  memcpy(rid_info.run_msg,p_scan_evt->params.p_not_found->data.p_data,31);	
									//printf("%s ",Util_convertHex2Str(p_scan_evt->params.p_not_found->data.p_data,p_scan_evt->params.p_not_found->data.len));
//									UART_WriteData(UART1,(uint8_t *)&data_head,2);
//									UART_WriteData(UART1,(uint8_t *)&rid_info.mac,sizeof(p_scan_evt->params.p_not_found->peer_addr.addr));
//									UART_WriteData(UART1,(uint8_t *)&rid_info.run_msg,p_scan_evt->params.p_not_found->data.len);
//									UART_WriteData(UART1,(uint8_t *)&data_end,2);
									for(i=0;i<sizeof(p_scan_evt->params.p_not_found->peer_addr.addr)+p_scan_evt->params.p_not_found->data.len+2;i++)
									{									
										if(i==8||i>8) buf[i] = rid_info.run_msg[i-8];
											else buf[i+2]= rid_info.mac[i];
									}
									buf[8+p_scan_evt->params.p_not_found->data.len] = 0xCC;
									buf[8+p_scan_evt->params.p_not_found->data.len+1] = 0xDD;
									UART_WriteData(UART1,(uint8_t *)&buf,sizeof(buf));	
									UART_WriteData(UART0,(uint8_t *)&buf,sizeof(buf));									
                } 
                else if(p_scan_evt->params.p_not_found->data.p_data[6]>>4 == 0x4)//系统报文
                {
                  //提取广播数据
									rid_info.msg_type = 0x4;
                  memcpy(rid_info.mac ,p_scan_evt->params.p_not_found->peer_addr.addr,sizeof(p_scan_evt->params.p_not_found->peer_addr.addr));
                  memcpy(rid_info.sys_msg,p_scan_evt->params.p_not_found->data.p_data,31);	
									//printf("%s ",Util_convertHex2Str(p_scan_evt->params.p_not_found->data.p_data,p_scan_evt->params.p_not_found->data.len));
//									UART_WriteData(UART1,(uint8_t *)&data_head,2);
//									UART_WriteData(UART1,(uint8_t *)&rid_info.mac,sizeof(p_scan_evt->params.p_not_found->peer_addr.addr));
//									UART_WriteData(UART1,(uint8_t *)&rid_info.sys_msg,p_scan_evt->params.p_not_found->data.len);
//									UART_WriteData(UART1,(uint8_t *)&data_end,2);
									for(i=0;i<sizeof(p_scan_evt->params.p_not_found->peer_addr.addr)+p_scan_evt->params.p_not_found->data.len+2;i++)
									{									
										if(i==8||i>8) buf[i] = rid_info.sys_msg[i-8];
											else buf[i+2]= rid_info.mac[i];
									}
									buf[8+p_scan_evt->params.p_not_found->data.len] = 0xCC;
									buf[8+p_scan_evt->params.p_not_found->data.len+1] = 0xDD;
									UART_WriteData(UART1,(uint8_t *)&buf,sizeof(buf));	
									UART_WriteData(UART0,(uint8_t *)&buf,sizeof(buf));
                }
                else if(p_scan_evt->params.p_not_found->data.p_data[6]>>4 == 0x5)//操作人报文
                {
                  //提取广播数据
									rid_info.msg_type = 0x5;
                  memcpy(rid_info.mac ,p_scan_evt->params.p_not_found->peer_addr.addr,sizeof(p_scan_evt->params.p_not_found->peer_addr.addr));
                  memcpy(rid_info.ope_msg,p_scan_evt->params.p_not_found->data.p_data,31);	
									//printf("%s ",Util_convertHex2Str(p_scan_evt->params.p_not_found->data.p_data,p_scan_evt->params.p_not_found->data.len));
//									UART_WriteData(UART1,(uint8_t *)&data_head,2);
//									UART_WriteData(UART1,(uint8_t *)&rid_info.mac,sizeof(p_scan_evt->params.p_not_found->peer_addr.addr));
//									UART_WriteData(UART1,(uint8_t *)&rid_info.ope_msg,p_scan_evt->params.p_not_found->data.len);
//									UART_WriteData(UART1,(uint8_t *)&data_end,2);
									for(i=0;i<sizeof(p_scan_evt->params.p_not_found->peer_addr.addr)+p_scan_evt->params.p_not_found->data.len+2;i++)
									{									
										if(i==8||i>8) buf[i] = rid_info.ope_msg[i-8];
											else buf[i+2]= rid_info.mac[i];
									}
									buf[8+p_scan_evt->params.p_not_found->data.len] = 0xCC;
									buf[8+p_scan_evt->params.p_not_found->data.len+1] = 0xDD;
									UART_WriteData(UART1,(uint8_t *)&buf,sizeof(buf));										
									UART_WriteData(UART0,(uint8_t *)&buf,sizeof(buf));									
                }               
              }
            }
            else
            {
              NRF_LOG_INFO("Adv Data:  %s","NONE");
            }
          }
        } break;

        default:
           break;
    }
}

//******************************************************************
// fn : scan_init
//
// brief : 初始化扫描（未设置扫描数据限制）
//
// param : none
//
// return : none
static void scan_init(void)
{
    ret_code_t          err_code;
    nrf_ble_scan_init_t init_scan;

    // 清空扫描结构体参数
    memset(&init_scan, 0, sizeof(init_scan));
    
    // 配置扫描的参数
    init_scan.p_scan_param = &m_scan_params;
    
    // 初始化扫描
    err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK(err_code);
}

/////////////////////////从机/////////////////////////

//******************************************************************
// fn : write_handler
//
// brief : 处理主机(GATTC)Write事件的处理函数，用于接收数据。
// details : 
//
// param : new_state 四字节灯控数据。单字节0x00表示点亮LED，0x01表示关闭LED，(led为共阳驱动)
//
// return : none

static void write_handler(uint16_t conn_handle, ble_t * p_evt, ble_gatts_evt_write_t const * p_evt_write)
{	
	  if(p_evt_write->handle == p_evt->led_char_handles.value_handle)
    {
				NRF_LOG_INFO("Received data (len=%d):", p_evt_write->len);
        for (int i = 0; i < p_evt_write->len; i++) {
            NRF_LOG_INFO("data %02X ", p_evt_write->data[i]); // 以16进制打印每个字节
        }
		}
}

//******************************************************************************
// fn :on_write
//
// brief : 处理Write事件的函数。该事件来自主机(GATTC)的Write写特征值
//
// param : p_led -> led服务结构体
//         p_ble_evt -> ble事件
//
// return : none
static void on_write(ble_t * p_evt, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    //参数判断和筛选
    if ((p_evt_write->len > 0) && (p_evt->write_handler != NULL))
    {
        //调用mainc中service_init函数设置的处理函数。并传递从无线端接收到的数据。
        p_evt->write_handler(p_ble_evt->evt.gap_evt.conn_handle, p_evt, p_evt_write);
    }
}

//自定义服务写属性回调
// 写操作回调实现
static void on_cus_write(uint16_t conn_handle, ble_cus_t *p_cus,uint8_t *data, uint16_t length) 
{
		NRF_LOG_INFO("Received data (len=%d):", length);
		for (int i = 0; i < length; i++) {
				NRF_LOG_INFO("data %02X ", data[i]); // 以16进制打印每个字节
		}
    // 处理接收数据逻辑
}
												
//******************************************************************************
// fn :ble_on_ble_evt
//
// brief : BLE事件处理函数
//
// param : p_ble_evt -> ble事件
//         p_context -> ble事件处理程序的参数（暂时理解应该是不同的功能，注册时所携带的结构体参数）
//
// return : none
// 在app_ble_server.h中添加服务上下文判断
void ble_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    // 通过p_context判断事件来源
    if (p_context == &m_led) 
    {
        // 原有LED服务事件处理
        switch (p_ble_evt->header.evt_id)
        {
            case BLE_GATTS_EVT_WRITE:
                on_write((ble_t *)p_context, p_ble_evt);
                break;
            default:
                break;
        }
    }
    else if (p_context == &m_cus)
    {
        // 新自定义服务事件处理
        switch (p_ble_evt->header.evt_id)
        {
            case BLE_GAP_EVT_CONNECTED:
                m_cus.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
                break;

            case BLE_GATTS_EVT_WRITE: 
							{
                ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
                
                if (p_evt_write->handle == m_cus.value_handles.cccd_handle) {
                    m_cus.notify_enabled = ble_srv_is_notification_enabled(p_evt_write->data);
                }
                else if (p_evt_write->handle == m_cus.value_handles.value_handle) {
                    if (m_cus.write_handler != NULL) {
                        m_cus.write_handler(p_ble_evt->evt.gap_evt.conn_handle,
                                          &m_cus,
                                          p_evt_write->data,
                                          p_evt_write->len);
                    }
                }
                break;
							}
						
        }
    }
}

//******************************************************************************
// fn :ble_server_init
//
// brief : 初始化LED服务
//
// param : p_led -> led服务结构体
//         p_led_init -> led服务初始化结构体
//
// return : uint32_t -> 成功返回SUCCESS，其他返回ERR NO.
uint32_t ble_server_init(ble_t * p_led, const ble_server_init_t * p_led_init)
{
    uint32_t              err_code;
    ble_uuid_t            ble_uuid;
    ble_add_char_params_t add_char_params;

    // 初始化服务结构体
    p_led->write_handler = p_led_init->write_handler;
    
    // 添加服务（128bit UUID），uuid_type由此判断是128位
    ble_uuid128_t base_uuid = {UUID_BASE};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_led->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_led->uuid_type;  //type value = 2(BLE_UUID_TYPE_VENDOR_BEGIN), is 128bit uuid;  value = 1(BLE_UUID_TYPE_BLE), is 16bit uuid
    ble_uuid.uuid = UUID_SERVICE;
    
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_led->service_handle);
    VERIFY_SUCCESS(err_code);

    // 添加LED特征值（属性是Write和Read、长度是4）
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid             = UUID_CHAR;
    add_char_params.uuid_type        = p_led->uuid_type;
    add_char_params.init_len         = UUID_CHAR_LEN;
    add_char_params.max_len          = UUID_CHAR_LEN;
    add_char_params.p_init_value     = p_led_init->p_led_value;
    add_char_params.char_props.read  = 1;
    add_char_params.char_props.write = 1;

    add_char_params.read_access  = SEC_OPEN;
    add_char_params.write_access = SEC_OPEN;

    return characteristic_add(p_led->service_handle, &add_char_params, &p_led->led_char_handles);
}

//添加自定义服务
static uint8_t buf[] = {0x11, 0x22, 0x33};  // 需要传输的预设数据
uint32_t ble_cus_init(ble_cus_t *p_cus, const ble_cus_init_t *p_cus_init) {
    uint32_t err_code;
    ble_uuid_t ble_uuid;
    ble_add_char_params_t add_char_params;
		
	  // 初始化服务结构体
    p_cus->write_handler = p_cus_init->write_handler;
		
	  // 将预设缓冲区赋值给服务实例
    memcpy(p_cus->read_buf, buf, sizeof(buf));  // 拷贝预设数据到服务缓冲区
    p_cus->read_len = sizeof(buf);              // 设置数据长度
	
    // 添加服务
    ble_uuid128_t base_uuid = {0x23,0xD1,0xBC,0xEA,0x5F,0x78,0x23,0x15,
                               0xDE,0xEF,0x12,0x12,0x00,0x00,0x00,0x00};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_cus->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = CUSTOM_SERVICE_UUID;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, 
                                       &ble_uuid,
                                       &p_cus->service_handle);
    VERIFY_SUCCESS(err_code);

    // 添加特征值（支持读写+通知）
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = CUSTOM_VALUE_CHAR_UUID;
    add_char_params.uuid_type         = p_cus->uuid_type;
    add_char_params.max_len           = MAX_DATA_LEN;
		add_char_params.init_len          = p_cus->read_len;  // 初始长度
    add_char_params.p_init_value      = p_cus->read_buf;          // 绑定读缓冲区
    add_char_params.char_props.read   = 1;
    add_char_params.char_props.write  = 1;
    add_char_params.char_props.notify = 1;
    add_char_params.read_access       = SEC_OPEN;
    add_char_params.write_access      = SEC_OPEN;
    add_char_params.cccd_write_access = SEC_OPEN;

    err_code = characteristic_add(p_cus->service_handle,
                                  &add_char_params,
                                  &p_cus->value_handles);
    return err_code;
}

//通知函数
uint32_t ble_cus_send_data(ble_cus_t *p_cus, uint8_t *data, uint16_t length) 
{
    if (length > MAX_DATA_LEN) return NRF_ERROR_INVALID_PARAM;

    ble_gatts_hvx_params_t hvx_params;
    memset(&hvx_params, 0, sizeof(hvx_params));
    
    hvx_params.handle = p_cus->value_handles.value_handle;
    hvx_params.p_data = data;
    hvx_params.p_len  = &length;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;

    return sd_ble_gatts_hvx(p_cus->conn_handle, &hvx_params);
}

//******************************************************************
// fn : advertising_start
//
// brief : 用于开启广播
//
// param : none
//
// return : none
static void advertising_start(void)
{
    uint32_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}

//******************************************************************
// fn : advertising_init
//
// brief : 用于初始化广播
//
// param : none
//
// return : none
static void advertising_init(void)
{
    uint32_t               err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    // 广播数据包含所有设备名称（FULL NAME）
    init.advdata.name_type = BLE_ADVDATA_FULL_NAME;
//    // 广播数据只包含部分设备名称（SHORT NAME，长度为6）
//    init.advdata.name_type =  BLE_ADVDATA_SHORT_NAME;
//    init.advdata.short_name_len = 6;
    
    // 扫描回调数据中包含16bit UUID：0x0001
    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;
    
    // 扫描回调数据中包含设备MAC地址
    init.srdata.include_ble_device_addr = true;
    
    // 配置广播周期
    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = 64;     // 64*0.625 = 40ms
    init.config.ble_adv_fast_timeout  = 0;      // 设置为0，一直广播
    
    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

//******************************************************************
// fn : advertising_init
//
// brief : 初始化GAP
// details : 此功能将设置设备的所有必需的GAP（通用访问配置文件）参数。它还设置权限和外观。
//
// param : none
//
// return : none
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    // 设置设备名称
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);
}

//******************************************************************
// fn : gatt_evt_handler
//
// brief : GATT事件回调
//
// param : p_gatt  gatt类型结构体
//         p_evt  gatt事件
//
// return : none
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        NRF_LOG_INFO("Data len is set to 0x%X(%d)", 
                     p_evt->params.att_mtu_effective, 
                     p_evt->params.att_mtu_effective);
    }
}

//******************************************************************
// fn : gatt_init
//
// brief : 初始化GATT
//
// param : none
//
// return : none
void gatt_init(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);
}


//******************************************************************
// fn : services_init
//
// brief : 初始化复位（本例程展示NUS：Nordic Uart Service）
//
// param : none
//
// return : none
static void services_init(void)
{
    uint32_t           err_code;
    ble_server_init_t     server_init;
		ble_cus_init_t				cus_init;
    //led状态默认值
    uint8_t led_value[UUID_CHAR_LEN] = {1,1,1,1};

    //清除赋值
    memset(&server_init, 0, sizeof(server_init));
		memset(&cus_init, 0, sizeof(cus_init));
		
    //设置特征值初始取值。
    server_init.p_led_value = led_value;
    //设置处理主机(GATTC)Write事件的处理函数，用于接收数据。
    server_init.write_handler = write_handler;
    //函数参数是指针的形式，因此携带的参数要使用&符号，取变量地址。
    err_code = ble_server_init(&m_led, &server_init);
    APP_ERROR_CHECK(err_code);
		
		 //设置处理主机(GATTC)Write事件的处理函数，用于接收数据。
    cus_init.write_handler = on_cus_write;
    //函数参数是指针的形式，因此携带的参数要使用&符号，取变量地址。
    err_code = ble_cus_init(&m_cus, &cus_init);
    APP_ERROR_CHECK(err_code);
}


//********************************共用**********************************
// fn : ble_evt_handler
//
// brief : BLE事件回调
// details : 包含以下几种事件类型：COMMON、GAP、GATT Client、GATT Server、L2CAP
//
// param : ble_evt_t  事件类型
//         p_context  未使用
//
// return : none
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t            err_code;
    ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;
    ble_gap_evt_connected_t const * p_connected_evt = &p_gap_evt->params.connected;
    uint16_t role        = ble_conn_state_role(p_gap_evt->conn_handle);
    
    switch (p_ble_evt->header.evt_id)
    {
        // 连接
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected", sizeof("Connected"));
                  
            NRF_LOG_INFO("Connected. conn_DevAddr: %s\nConnected. conn_handle: 0x%04x\nConnected. conn_Param: %d,%d,%d,%d",
                         Util_convertBdAddr2Str((uint8_t*)p_connected_evt->peer_addr.addr),
                         p_gap_evt->conn_handle,
                         p_connected_evt->conn_params.min_conn_interval,
                         p_connected_evt->conn_params.max_conn_interval,
                         p_connected_evt->conn_params.slave_latency,
                         p_connected_evt->conn_params.conn_sup_timeout
                         );
            
            
            // 本机当前连接角色为从机
            if(role == BLE_GAP_ROLE_PERIPH)
            {
             
            }
            // 本机当前连接角色为主机
            else if(role == BLE_GAP_ROLE_CENTRAL)
            {

            }
            
            break;

        // 断开连接
        case BLE_GAP_EVT_DISCONNECTED:
          
            NRF_LOG_INFO("Disconnected. conn_handle: 0x%x, reason: 0x%04x",
                         p_gap_evt->conn_handle,
                         p_gap_evt->params.disconnected.reason);
            
            break;
          
        default:
            break;
    }
}

//******************************************************************
// fn : ble_stack_init
//
// brief : 用于初始化BLE协议栈
// details : 初始化SoftDevice、BLE事件中断
//
// param : none
//
// return : none
static void ble_stack_init(void)
{
    ret_code_t err_code;

    // SD使能请求，配置时钟，配置错误回调，中断（中断优先级栈堆默认设置）
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // SD默认配置
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // 使能BLE栈堆
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // 注册BLE事件的处理程序，所有BLE的事件都将分派ble_evt_handler回调
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}




void ble_init(void)
{
	
    ble_stack_init();       // 初始化BLE栈堆
    gatt_init();            // 初始化GATT
    // 主机
    //db_discovery_init();    // 初始化数据库发现模块（也就是扫描服务）
    scan_init();            // 初始化扫描
    // 从机
		gap_params_init();      // 初始化GAP
    services_init();        // 初始化Service
    advertising_init();     // 初始化广播
      
    scan_start();           // 开始扫描
    advertising_start();    // 开启广播
    timers_init();  			//初始化定时器
}
