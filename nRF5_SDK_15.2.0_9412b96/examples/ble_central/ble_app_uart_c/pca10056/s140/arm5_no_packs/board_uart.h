#ifndef _BOARD_UART_H_
#define _BOARD_UART_H_

/*********************************************************************
 * INCLUDES
 */
#include "nrf_gpio.h"

/*********************************************************************
 * DEFINITIONS
 */
#define UART_TX_BUF_SIZE                256                     // UART TX buffer size
#define UART_RX_BUF_SIZE                256                     // UART RX buffer size

// UART0
#define BOARD_UART0_TX_IO               6                       // 发送引脚  
#define BOARD_UART0_RX_IO               8                       // 接收引脚
// UART1
#define BOARD_UART1_TX_IO               NRF_GPIO_PIN_MAP(1, 2) //NRF_GPIO_PIN_MAP(1, 10)  					// 发送引脚  
#define BOARD_UART1_RX_IO               NRF_GPIO_PIN_MAP(1, 4) //NRF_GPIO_PIN_MAP(1, 11)   					// 接收引脚

#define UART0                           0
#define UART1                           1

#define MAX_RECV_BUF_SIZE               32//用于接收多少个字节然后进行清缓存
/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * API FUNCTION
 */
void UART_Init(void);
void UART_WriteData(uint8_t uartNum, uint8_t *pData, uint8_t dataLen);
void OTA_Receive_Data();
#endif /* _BOARD_UART_H_ */
