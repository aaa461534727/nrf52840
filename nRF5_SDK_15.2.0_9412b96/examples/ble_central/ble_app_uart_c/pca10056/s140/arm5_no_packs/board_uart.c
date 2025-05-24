/*********************************************************************
 * INCLUDES
 */
#include "nrf_uart.h"
#include "app_uart.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#include "app_ota_flash.h"
#include "board_uart.h"
#include "dn_dfu_settings.h"
#include "crc32.h"
#include "nrf_ble_scan.h"
#include "app_wdt_handle.h"

extern uint8_t m_dfu_settings_buffer[BOOTLOADER_SETTINGS_PAGE_SIZE];

extern user_flash_data_item_t  user_flash_data_item;

static void uart0_handleEvent(app_uart_evt_t *pEvent);
static void uart1_handleEvent(app_uart_evt_t *pEvent);

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
#define MAX_PAYLOAD_SIZE          100*1024

APP_UART_DEF(uart0, 0, UART_RX_BUF_SIZE, uart0_handleEvent);
APP_UART_DEF(uart1, 1, UART_RX_BUF_SIZE, uart1_handleEvent);
static uint8_t s_uart0ReadDataBuffer[UART_RX_BUF_SIZE] __attribute__((aligned(4))) = {0};
static uint32_t s_index = 0;
static bool s_begin = false;
static uint8_t s_uart1ReadDataBuffer[MAX_PAYLOAD_SIZE] __attribute__((aligned(4))) = {0};
static uint8_t s_index2 = 0;
static bool s_begin2 = false;

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 @brief 串口驱动初始化
 @param 无
 @return 无
*/
void UART_Init(void)
{
    uint32_t errCode;
    
    /*------------------------ UART0 ------------------------*/
    app_uart_comm_params_t const commParams_0 =
    {
        .rx_pin_no    = BOARD_UART0_RX_IO,
        .tx_pin_no    = BOARD_UART0_TX_IO,
        .rts_pin_no   = NRF_UART_PSEL_DISCONNECTED,
        .cts_pin_no   = NRF_UART_PSEL_DISCONNECTED,                    
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,             // 关掉流控
        .use_parity   = false,
#if defined (UART_PRESENT)
        .baud_rate    = UART_BAUDRATE_BAUDRATE_Baud115200                    // 波特率
#else
        .baud_rate    = NRF_UARTE_BAUDRATE_115200
#endif
    };
    uart0.comm_params = &commParams_0;
    errCode = app_uart_init(&uart0, &uart0_buffers, APP_IRQ_PRIORITY_MID);
    APP_ERROR_CHECK(errCode);
    
    /*------------------------ UART1 ------------------------*/
    app_uart_comm_params_t const commParams_1 =
    {
        .rx_pin_no    = BOARD_UART1_RX_IO,
        .tx_pin_no    = BOARD_UART1_TX_IO,
        .rts_pin_no   = NRF_UART_PSEL_DISCONNECTED,
        .cts_pin_no   = NRF_UART_PSEL_DISCONNECTED,                    
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,             // 关掉流控
        .use_parity   = false,
#if defined (UART_PRESENT)
        .baud_rate    = UART_BAUDRATE_BAUDRATE_Baud115200                    // 波特率
#else
        .baud_rate    = NRF_UART_BAUDRATE_115200
#endif
    };
    uart1.comm_params = &commParams_1;
    errCode = app_uart_init(&uart1, &uart1_buffers, APP_IRQ_PRIORITY_LOWEST);
    APP_ERROR_CHECK(errCode);
}

/**
 @brief 串口写数据函数
 @param uartNum -[in] 串口号
 @param pData -[in] 写入数据
 @param dataLen -[in] 写入数据长度
 @return 无
*/
void UART_WriteData(uint8_t uartNum, uint8_t *pData, uint8_t dataLen)
{
    uint8_t i;
    if(uartNum == 0)
    {
        for(i = 0; i < dataLen; i++)
        {
            app_uart_put(&uart0, pData[i]);
        }
    }
    else if(uartNum == 1)
    {
        for(i = 0; i < dataLen; i++)
        {
            app_uart_put(&uart1, pData[i]);
        }
    }
}
//重定向printf
uint32_t _app_uart_get(uint8_t * p_byte)
{
    return app_uart_get(&uart0, p_byte);
}
uint32_t _app_uart_put(uint8_t byte)
{
    return app_uart_put(&uart0, byte);
}


/*********************************************************************
 * LOCAL FUNCTIONS
 */
/**
 @brief 串口0读取数据处理函数
 @param pEvent -[in] 串口事件
 @return 无
*/
static void uart0_handleEvent(app_uart_evt_t *pEvent)
{
    uint8_t dataChar = 0;
    switch(pEvent->evt_type)
    {
			        // 已接收到UART数据
        case APP_UART_DATA_READY:
        {
					/***可用这种方法接收任意字节然后进行判断操作，必须死等待直到有数据，不然就会一直执行回调
					while(app_uart_get(&uart1,&dataChar) != NRF_SUCCESS);
          if(dataChar==0x11)
          {
            UART_WriteData(UART1,(uint8_t *)&dataChar,1);
          }
					*****/
            UNUSED_VARIABLE(app_uart_get(&uart1, &dataChar));
            // 不是回车符或换行符则开始   
            if(dataChar != '\n' && dataChar != '\r')
            {
                s_begin2 = true;
            }
            if(s_begin2)
            {
                s_uart1ReadDataBuffer[s_index2] = dataChar;
                s_index2++;
						
            }
						
            // 遇到回车符或换行符结束,可用来做判断接收到特定字节然后进行对应操作      
            if((s_uart1ReadDataBuffer[s_index2 - 1] == '\n') ||
               (s_uart1ReadDataBuffer[s_index2 - 1] == '\r') ||
               (s_index2 >= MAX_RECV_BUF_SIZE))
            {
						NRF_LOG_HEXDUMP_INFO(s_uart1ReadDataBuffer, s_index2);   
                memset(s_uart1ReadDataBuffer, 0, s_index2); 
                s_index2 = 0;
                s_begin2 = false;                
            }
        } break;

        // 接收过程中发生通信错误
        case APP_UART_COMMUNICATION_ERROR:
//            APP_ERROR_HANDLER(pEvent->data.error_communication);
            break;

        // app_uart模块使用的FIFO模块中出现错误
        case APP_UART_FIFO_ERROR:
//            APP_ERROR_HANDLER(pEvent->data.error_code);
            break;

        default:
            break;

    }
}

void bootloader_start()
{
    // 清除所有挂起中断
    for (int i = 0; i < 8; i++) {
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    // 设置GPREGRET触发DFU
    sd_power_gpregret_set(0, 0xB1);
    NVIC_SystemReset(); // 触发系统复位进入DFU
}

void OTA_Receive_Data()
{
	ret_code_t err_code;
	uint32_t file_crc32=0xFFFFFFFF;   /*下载文件的CRC*/
	uint32_t cal_crc32=0xFFFFFFFF;
	uint32_t rxlen=s_index;	
	uint32_t i=0;
	uint8_t buf[256];
	nrf_delay_ms(50);		//等待10ms,连续超过10ms没有接收到一个数据,则认为接收结束
	if(rxlen==s_index&&rxlen)//接收到了数据,且接收完成了
	{
		wdt_feed();
		//获取固件的CRC校验值
		file_crc32 = s_uart1ReadDataBuffer[s_index-1]<<24|s_uart1ReadDataBuffer[s_index-2]<<16
									|s_uart1ReadDataBuffer[s_index-3]<<8|s_uart1ReadDataBuffer[s_index-4];
//		printf(" %02x %02x %02x %02x\r\n",s_uart0ReadDataBuffer[s_index-1],
//									s_uart0ReadDataBuffer[s_index-2],s_uart0ReadDataBuffer[s_index-3],s_uart0ReadDataBuffer[s_index-4]);
		//减去4个字节的CRC校验
		cal_crc32 = crc32_compute(s_uart1ReadDataBuffer, s_index-4, &cal_crc32);
		NRF_LOG_INFO("image size:%d, cal_crc:%x  file_crc:%x\r\n",s_index , cal_crc32,file_crc32);
		printf("image size:%d, cal_crc:%x  file_crc:%x\r\n",s_index , cal_crc32,file_crc32);
			
		if(s_index>10*1024)
		{
			//固件CRC校验通过
			if(file_crc32 == cal_crc32)
			{
					nrf_ble_scan_stop();
					ota_flash_earse_all();
					nrf_delay_ms(1000);
					ota_flash_write_buffer(s_uart1ReadDataBuffer,s_index-4);
					
					/*DFU配置初始化*/
					nrf_dfu_settings_init(false);
					/*配置下列参数，BOOT会根据这些参数更新固件*/
					memcpy((void*)&s_dfu_settings, m_dfu_settings_buffer, sizeof(nrf_dfu_settings_t));
					s_dfu_settings.bank_1.image_size = s_index-4;
					s_dfu_settings.bank_1.image_crc = cal_crc32;
					s_dfu_settings.bank_1.bank_code = NRF_DFU_BANK_VALID_APP;
					s_dfu_settings.progress.update_start_address = FIRMWARE_BLE_AREA_BEIGN_ADDR;
					//将参数写入flash
					user_flash_data_item.image_size = s_index-4;
					user_flash_data_item.image_crc = cal_crc32;
					user_flash_data_item.bank_code = NRF_DFU_BANK_VALID_APP;
					user_flash_data_item.update_start_address = FIRMWARE_BLE_AREA_BEIGN_ADDR;
					flash_ota_init();
					/*设置固件更新标志*/
					ret_code_t res = nrf_dfu_settings_write(NULL);
					if(res == NRF_SUCCESS)
					{
						NRF_LOG_INFO("firmware upload success:%x\r\n", m_dfu_settings_buffer);
						printf("firmware upload success:%x\r\n", m_dfu_settings_buffer);
						nrf_delay_ms(1000);
						dn_dfu_settings_t tmp_dfu_settings;
						memcpy((void*)&tmp_dfu_settings, m_dfu_settings_buffer, sizeof(nrf_dfu_settings_t));
						
						uint8_t *ptmp = (uint8_t *)(&s_dfu_settings.bank_1);
						printf("dfu %d:",sizeof(nrf_dfu_bank_t));
						for(uint16_t i = 0; i < sizeof(nrf_dfu_bank_t);i++)
						{
								printf("%x ",ptmp[i]);
						}
						printf("\r\n");
						ptmp = (uint8_t *)(&tmp_dfu_settings.bank_1);
						printf("dfu %d:",sizeof(nrf_dfu_bank_t));
						for(uint16_t i = 0; i < sizeof(nrf_dfu_bank_t);i++)
						{
								printf("%x ",ptmp[i]);
						}
						printf("\r\n");
						
						
						if(memcmp((void*)&tmp_dfu_settings.bank_1,(void*)&s_dfu_settings.bank_1,sizeof(nrf_dfu_bank_t)) ==0)
						{

								NRF_LOG_INFO("boot start!!!\r\n");
								printf("boot start!!!\r\n");
								nrf_delay_ms(1000);
								bootloader_start();
								//NVIC_SystemReset();
						}
					}
			}
		}
		s_index=0;		//清零
		
	}
}

/**
 @brief 串口1读取数据处理函数
 @param pEvent -[in] 串口事件
 @return 无
*/
static void uart1_handleEvent(app_uart_evt_t *pEvent)
{
    uint8_t dataChar = 0;

    switch(pEvent->evt_type)
    {
        // 已接收到UART数据
        case APP_UART_DATA_READY:
        {
            UNUSED_VARIABLE(app_uart_get(&uart1, &dataChar));
            s_uart1ReadDataBuffer[s_index] = dataChar;													
            s_index++;
				    if(s_index >= MAX_PAYLOAD_SIZE)
            {
                s_index = 0;
            }

        } break;

        // 接收过程中发生通信错误
        case APP_UART_COMMUNICATION_ERROR:
//            APP_ERROR_HANDLER(pEvent->data.error_communication);
            break;

        // app_uart模块使用的FIFO模块中出现错误
        case APP_UART_FIFO_ERROR:
//            APP_ERROR_HANDLER(pEvent->data.error_code);
            break;

        default:
            break;
    }
}

/****************************************************END OF FILE****************************************************/
