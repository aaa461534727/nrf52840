//******************************************************************************
//                                www.ghostyu.com
//
//                 Copyright (c) 2019-2020, WUXI Ghostyu Co.,Ltd.
//                                All rights reserved.
//
// FileName : mian.c
// Date     : 2019-06-24 10:34
// Version  : V0001
// ��ʷ��¼ : 1.��һ�δ���
//
// ˵����
// main�ļ�
//******************************************************************************

//******************************************************************
// ͷ�ļ�����
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
// ��������
//
#define APP_BLE_CONN_CFG_TAG    1   // 
#define APP_BLE_OBSERVER_PRIO   3   // BLEӦ�ó���Ĺ۲������ȼ�,�����޸Ĵ�ֵ��
#define APP_TIME_INTERVAL 							APP_TIMER_TICKS( 1000 )  //ÿ��1000ms����һ��

//����
APP_TIMER_DEF (m_app_timer_id); 	//���������ʱ��ʵ������
NRF_BLE_SCAN_DEF (m_scan);           // ����ɨ��ʵ��������
// ����ɨ�����
static ble_gap_scan_params_t m_scan_params = 
{
    .active        = 1,                            // 1Ϊ����ɨ�裬�ɻ�ù㲥���ݼ�ɨ��ص�����
    .interval      = NRF_BLE_SCAN_SCAN_INTERVAL,   // ɨ������160*0.625 = 100ms  
    .window        = NRF_BLE_SCAN_SCAN_WINDOW,     // ɨ�贰�ڣ�80*0.625 = 50ms   
    .timeout       = NRF_BLE_SCAN_SCAN_DURATION,   // ɨ�������ʱ�䣺����Ϊ0��һֱɨ�裬ֱ����ȷ��ֹͣɨ��
    .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,   // ɨ������BLE�豸����������
    .scan_phys     = BLE_GAP_PHY_1MBPS,            // ɨ��1MBPS��PHY
};

//����ṹ��
typedef struct rid{
	//������ 
	uint8_t msg_type; //��������
	uint8_t base_msg[31];//����ID����0x0
	uint8_t pos_msg[31];//λ����������0x1
	uint8_t diy_msg[31];//�Զ��屣������0x2
	uint8_t run_msg[31];//������������0x3
	uint8_t sys_msg[31];//ϵͳ����0x4
	uint8_t ope_msg[31];//������Ա����0x5
	uint8_t mac[6];//����mac��ַ
}RID;

//�ӻ�
BLE_LED_DEF(m_led);   // LEDʵ��
BLE_CUS_DEF(m_cus);
NRF_BLE_QWR_DEF(m_qwr);             // ����ʵ��
NRF_BLE_GATT_DEF(m_gatt);           // GATTʵ��
BLE_ADVERTISING_DEF(m_advertising); // �㲥ʵ��
#define DEVICE_NAME  "GY-NRF52832"  // �����豸����

// ����㲥��UUID
static ble_uuid_t m_adv_uuids[] = {{UUID_SERVICE, BLE_UUID_TYPE_BLE},{CUSTOM_SERVICE_UUID, BLE_UUID_TYPE_BLE}};
// �������ӵ�handle
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;

//����
//��ʱ���ص�����
void app_timeout_handler( void *p_context )
{
	uint8_t sample_data[] = {0x01, 0x02, 0x03};
	if (m_cus.notify_enabled && 
    m_cus.conn_handle != BLE_CONN_HANDLE_INVALID) {
    // ������֪ͨ
		uint32_t err_code = ble_cus_send_data(&m_cus, sample_data, strlen(sample_data));
		if (err_code != NRF_SUCCESS) {
				NRF_LOG_ERROR("send fail: 0x%X", err_code);
				// ��Դ����ʱ����ʱ����
		}
	}

}

//******************************************************************
// ��ʱ����ʼ������
//	m_app_timer_id����ʱ���¼�ID�ţ����ض�ID�����¼�
//	APP_TIMER_MODE_REPEATED����ʱ���ظ�����ģʽ
//	app_timeout_handler ��ʱ���жϺ�����һ��Ҫȷ��������ĺ�������һ�£�
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
// brief : ��SoftDevice�����ڴ���asserts�ĺ���
// details : ���SoftDevice�д���assert������ô˺���
// warning : ����������ֻ��һ�����ӣ������������ղ�Ʒ�� ����Ҫ������Ʒ��assertʱӦ����η�Ӧ
// warning : ���assert����SoftDevice����ϵͳֻ���ڸ�λʱ�ָ�
//
// param : line_num     ʧ�ܵ�assert���к�
//         p_file_name  ʶ���assert���ļ���
//
// return : none
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}


///////////////////����//////////////////////
//******************************************************************
// fn : scan_start
//
// brief : ��ʼɨ��
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
// brief : ����ɨ��ص��¼�
//
// param : scan_evt_t  ɨ���¼��ṹ��
//
// return : none
static void scan_evt_handler(scan_evt_t const * p_scan_evt)
{
 
  // 0x1E16FAFF0D ÿ��rid���Ķ���Я���������
    RID rid_info;//�����ڵ�ṹ��׼������������ӽ�����
		uint8_t buf[41]={0xAA,0xBB};//�����������
		int i=0;
    switch(p_scan_evt->scan_evt_id)
    {
        // δ���˵�ɨ������
        case NRF_BLE_SCAN_EVT_NOT_FOUND:
        {
          // �ж��Ƿ�Ϊɨ��ص�����
          if(p_scan_evt->params.p_not_found->type.scan_response)
          {
            if(p_scan_evt->params.p_not_found->data.len)    // ����ɨ��ص�����
            {
//              NRF_LOG_INFO("ScanRsp Data:%s",Util_convertHex2Str(p_scan_evt->params.p_not_found->data.p_data,p_scan_evt->params.p_not_found->data.len));
            }
            else
            {
              NRF_LOG_INFO("ScanRsp Data:%s","NONE");
            }
            
          }
          else  // ����Ϊ�㲥����
          {
//            //��ӡ�豸RSSI�ź�
//            NRF_LOG_INFO("Device RSSI: %ddBm",p_scan_evt->params.p_not_found->rssi);
//            // ��ӡɨ����豸MAC
//            NRF_LOG_INFO("Device MAC:  %s",
//                         Util_convertBdAddr2Str((uint8_t*)p_scan_evt->params.p_not_found->peer_addr.addr));           
						//UART_WriteData(UART0,(uint8_t *)&run_buf,sizeof(run_buf));
						//NRF_LOG_INFO("nrf52840 adv is running!\n");
            if(p_scan_evt->params.p_not_found->data.len&&p_scan_evt->params.p_not_found->data.len>6)    // ���ڹ㲥����,���˿�����
            {
							//��ӡ�㲥����
             NRF_LOG_INFO("Adv Data: %s",Util_convertHex2Str(p_scan_evt->params.p_not_found->data.p_data,p_scan_evt->params.p_not_found->data.len));
              //RID����ͷ���ȶԳɹ�
              if(p_scan_evt->params.p_not_found->data.p_data[0] == 0x1E && p_scan_evt->params.p_not_found->data.p_data[1] == 0x16 &&\
                  p_scan_evt->params.p_not_found->data.p_data[2] == 0xFA && p_scan_evt->params.p_not_found->data.p_data[3] == 0xFF &&\
                   p_scan_evt->params.p_not_found->data.p_data[4] == 0x0D)
              {
                // ��ӡɨ����豸MAC,�ŵ�
                NRF_LOG_INFO("---------Device MAC:  %s  Channel Index : %d   Typre : %X ------------",Util_convertBdAddr2Str((uint8_t*)p_scan_evt->params.p_not_found->peer_addr.addr),p_scan_evt->params.p_not_found->ch_index,p_scan_evt->params.p_not_found->data.p_data[6]>>4);
                //��ӡ�㲥����
                NRF_LOG_INFO("Adv Data:    %s \r\n",Util_convertHex2Str(p_scan_evt->params.p_not_found->data.p_data,p_scan_evt->params.p_not_found->data.len));   
                if(p_scan_evt->params.p_not_found->data.p_data[6]>>4 == 0x0)//����ID����
                {
									//��ȡ�㲥����
									rid_info.msg_type = 0x0;
									//�����������ݴ浽����ת��16�����ֽڷ�ʽ����
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
                else if(p_scan_evt->params.p_not_found->data.p_data[6]>>4 == 0x1)//λ�ñ���
                {
                  //��ȡ�㲥����
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
                else if(p_scan_evt->params.p_not_found->data.p_data[6]>>4 == 0x2)//�Զ��屨��
                {
                  //��ȡ�㲥����
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
                else if(p_scan_evt->params.p_not_found->data.p_data[6]>>4 == 0x3)//���б���
                {
                  //��ȡ�㲥����
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
                else if(p_scan_evt->params.p_not_found->data.p_data[6]>>4 == 0x4)//ϵͳ����
                {
                  //��ȡ�㲥����
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
                else if(p_scan_evt->params.p_not_found->data.p_data[6]>>4 == 0x5)//�����˱���
                {
                  //��ȡ�㲥����
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
// brief : ��ʼ��ɨ�裨δ����ɨ���������ƣ�
//
// param : none
//
// return : none
static void scan_init(void)
{
    ret_code_t          err_code;
    nrf_ble_scan_init_t init_scan;

    // ���ɨ��ṹ�����
    memset(&init_scan, 0, sizeof(init_scan));
    
    // ����ɨ��Ĳ���
    init_scan.p_scan_param = &m_scan_params;
    
    // ��ʼ��ɨ��
    err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK(err_code);
}

/////////////////////////�ӻ�/////////////////////////

//******************************************************************
// fn : write_handler
//
// brief : ��������(GATTC)Write�¼��Ĵ����������ڽ������ݡ�
// details : 
//
// param : new_state ���ֽڵƿ����ݡ����ֽ�0x00��ʾ����LED��0x01��ʾ�ر�LED��(ledΪ��������)
//
// return : none

static void write_handler(uint16_t conn_handle, ble_t * p_evt, ble_gatts_evt_write_t const * p_evt_write)
{	
	  if(p_evt_write->handle == p_evt->led_char_handles.value_handle)
    {
				NRF_LOG_INFO("Received data (len=%d):", p_evt_write->len);
        for (int i = 0; i < p_evt_write->len; i++) {
            NRF_LOG_INFO("data %02X ", p_evt_write->data[i]); // ��16���ƴ�ӡÿ���ֽ�
        }
		}
}

//******************************************************************************
// fn :on_write
//
// brief : ����Write�¼��ĺ��������¼���������(GATTC)��Writeд����ֵ
//
// param : p_led -> led����ṹ��
//         p_ble_evt -> ble�¼�
//
// return : none
static void on_write(ble_t * p_evt, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    //�����жϺ�ɸѡ
    if ((p_evt_write->len > 0) && (p_evt->write_handler != NULL))
    {
        //����mainc��service_init�������õĴ������������ݴ����߶˽��յ������ݡ�
        p_evt->write_handler(p_ble_evt->evt.gap_evt.conn_handle, p_evt, p_evt_write);
    }
}

//�Զ������д���Իص�
// д�����ص�ʵ��
static void on_cus_write(uint16_t conn_handle, ble_cus_t *p_cus,uint8_t *data, uint16_t length) 
{
		NRF_LOG_INFO("Received data (len=%d):", length);
		for (int i = 0; i < length; i++) {
				NRF_LOG_INFO("data %02X ", data[i]); // ��16���ƴ�ӡÿ���ֽ�
		}
    // ������������߼�
}
												
//******************************************************************************
// fn :ble_on_ble_evt
//
// brief : BLE�¼�������
//
// param : p_ble_evt -> ble�¼�
//         p_context -> ble�¼��������Ĳ�������ʱ���Ӧ���ǲ�ͬ�Ĺ��ܣ�ע��ʱ��Я���Ľṹ�������
//
// return : none
// ��app_ble_server.h����ӷ����������ж�
void ble_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    // ͨ��p_context�ж��¼���Դ
    if (p_context == &m_led) 
    {
        // ԭ��LED�����¼�����
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
        // ���Զ�������¼�����
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
// brief : ��ʼ��LED����
//
// param : p_led -> led����ṹ��
//         p_led_init -> led�����ʼ���ṹ��
//
// return : uint32_t -> �ɹ�����SUCCESS����������ERR NO.
uint32_t ble_server_init(ble_t * p_led, const ble_server_init_t * p_led_init)
{
    uint32_t              err_code;
    ble_uuid_t            ble_uuid;
    ble_add_char_params_t add_char_params;

    // ��ʼ������ṹ��
    p_led->write_handler = p_led_init->write_handler;
    
    // ��ӷ���128bit UUID����uuid_type�ɴ��ж���128λ
    ble_uuid128_t base_uuid = {UUID_BASE};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_led->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_led->uuid_type;  //type value = 2(BLE_UUID_TYPE_VENDOR_BEGIN), is 128bit uuid;  value = 1(BLE_UUID_TYPE_BLE), is 16bit uuid
    ble_uuid.uuid = UUID_SERVICE;
    
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_led->service_handle);
    VERIFY_SUCCESS(err_code);

    // ���LED����ֵ��������Write��Read��������4��
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

//����Զ������
static uint8_t buf[] = {0x11, 0x22, 0x33};  // ��Ҫ�����Ԥ������
uint32_t ble_cus_init(ble_cus_t *p_cus, const ble_cus_init_t *p_cus_init) {
    uint32_t err_code;
    ble_uuid_t ble_uuid;
    ble_add_char_params_t add_char_params;
		
	  // ��ʼ������ṹ��
    p_cus->write_handler = p_cus_init->write_handler;
		
	  // ��Ԥ�軺������ֵ������ʵ��
    memcpy(p_cus->read_buf, buf, sizeof(buf));  // ����Ԥ�����ݵ����񻺳���
    p_cus->read_len = sizeof(buf);              // �������ݳ���
	
    // ��ӷ���
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

    // �������ֵ��֧�ֶ�д+֪ͨ��
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = CUSTOM_VALUE_CHAR_UUID;
    add_char_params.uuid_type         = p_cus->uuid_type;
    add_char_params.max_len           = MAX_DATA_LEN;
		add_char_params.init_len          = p_cus->read_len;  // ��ʼ����
    add_char_params.p_init_value      = p_cus->read_buf;          // �󶨶�������
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

//֪ͨ����
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
// brief : ���ڿ����㲥
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
// brief : ���ڳ�ʼ���㲥
//
// param : none
//
// return : none
static void advertising_init(void)
{
    uint32_t               err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    // �㲥���ݰ��������豸���ƣ�FULL NAME��
    init.advdata.name_type = BLE_ADVDATA_FULL_NAME;
//    // �㲥����ֻ���������豸���ƣ�SHORT NAME������Ϊ6��
//    init.advdata.name_type =  BLE_ADVDATA_SHORT_NAME;
//    init.advdata.short_name_len = 6;
    
    // ɨ��ص������а���16bit UUID��0x0001
    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;
    
    // ɨ��ص������а����豸MAC��ַ
    init.srdata.include_ble_device_addr = true;
    
    // ���ù㲥����
    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = 64;     // 64*0.625 = 40ms
    init.config.ble_adv_fast_timeout  = 0;      // ����Ϊ0��һֱ�㲥
    
    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

//******************************************************************
// fn : advertising_init
//
// brief : ��ʼ��GAP
// details : �˹��ܽ������豸�����б����GAP��ͨ�÷��������ļ�����������������Ȩ�޺���ۡ�
//
// param : none
//
// return : none
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    // �����豸����
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);
}

//******************************************************************
// fn : gatt_evt_handler
//
// brief : GATT�¼��ص�
//
// param : p_gatt  gatt���ͽṹ��
//         p_evt  gatt�¼�
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
// brief : ��ʼ��GATT
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
// brief : ��ʼ����λ��������չʾNUS��Nordic Uart Service��
//
// param : none
//
// return : none
static void services_init(void)
{
    uint32_t           err_code;
    ble_server_init_t     server_init;
		ble_cus_init_t				cus_init;
    //led״̬Ĭ��ֵ
    uint8_t led_value[UUID_CHAR_LEN] = {1,1,1,1};

    //�����ֵ
    memset(&server_init, 0, sizeof(server_init));
		memset(&cus_init, 0, sizeof(cus_init));
		
    //��������ֵ��ʼȡֵ��
    server_init.p_led_value = led_value;
    //���ô�������(GATTC)Write�¼��Ĵ����������ڽ������ݡ�
    server_init.write_handler = write_handler;
    //����������ָ�����ʽ�����Я���Ĳ���Ҫʹ��&���ţ�ȡ������ַ��
    err_code = ble_server_init(&m_led, &server_init);
    APP_ERROR_CHECK(err_code);
		
		 //���ô�������(GATTC)Write�¼��Ĵ����������ڽ������ݡ�
    cus_init.write_handler = on_cus_write;
    //����������ָ�����ʽ�����Я���Ĳ���Ҫʹ��&���ţ�ȡ������ַ��
    err_code = ble_cus_init(&m_cus, &cus_init);
    APP_ERROR_CHECK(err_code);
}


//********************************����**********************************
// fn : ble_evt_handler
//
// brief : BLE�¼��ص�
// details : �������¼����¼����ͣ�COMMON��GAP��GATT Client��GATT Server��L2CAP
//
// param : ble_evt_t  �¼�����
//         p_context  δʹ��
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
        // ����
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
            
            
            // ������ǰ���ӽ�ɫΪ�ӻ�
            if(role == BLE_GAP_ROLE_PERIPH)
            {
             
            }
            // ������ǰ���ӽ�ɫΪ����
            else if(role == BLE_GAP_ROLE_CENTRAL)
            {

            }
            
            break;

        // �Ͽ�����
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
// brief : ���ڳ�ʼ��BLEЭ��ջ
// details : ��ʼ��SoftDevice��BLE�¼��ж�
//
// param : none
//
// return : none
static void ble_stack_init(void)
{
    ret_code_t err_code;

    // SDʹ����������ʱ�ӣ����ô���ص����жϣ��ж����ȼ�ջ��Ĭ�����ã�
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // SDĬ������
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // ʹ��BLEջ��
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // ע��BLE�¼��Ĵ����������BLE���¼���������ble_evt_handler�ص�
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}




void ble_init(void)
{
	
    ble_stack_init();       // ��ʼ��BLEջ��
    gatt_init();            // ��ʼ��GATT
    // ����
    //db_discovery_init();    // ��ʼ�����ݿⷢ��ģ�飨Ҳ����ɨ�����
    scan_init();            // ��ʼ��ɨ��
    // �ӻ�
		gap_params_init();      // ��ʼ��GAP
    services_init();        // ��ʼ��Service
    advertising_init();     // ��ʼ���㲥
      
    scan_start();           // ��ʼɨ��
    advertising_start();    // �����㲥
    timers_init();  			//��ʼ����ʱ��
}
