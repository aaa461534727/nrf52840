#ifndef APP_BLE_SERVER_H__
#define APP_BLE_SERVER_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"

#ifdef __cplusplus
extern "C" {
#endif


//******************************************************************************
// fn :BLE_LED_DEF
//
// brief : 初始化LED服务实例
//
// param : _name -> 实例的名称
//
// return : none
#define BLE_LED_DEF(_name)                                                                          \
static ble_t _name;                                                                             \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_LBS_BLE_OBSERVER_PRIO,                                                     \
                     ble_on_ble_evt, &_name)


// 定义LED服务的UUID，以及特征值相关参数
#define UUID_BASE        {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, \
                              0xDE, 0xEF, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00}

#define UUID_SERVICE     0xFFF0
#define UUID_CHAR        0xFFF1

#define UUID_CHAR_LEN    4


// 申明ble_t类型
typedef struct ble_s ble_t;

// 定义一个回调函数
typedef void (*ble_write_handler_t) (uint16_t conn_handle, ble_t * p_vent, ble_gatts_evt_write_t const * p_evt_write);

//服务初始化结构体
typedef struct
{
    uint8_t *p_led_value;
    ble_write_handler_t write_handler; /**< Event handler to be called when the LED Characteristic is written. */
} ble_server_init_t;

// 服务结构体
struct ble_s
{
    uint8_t                     uuid_type;           /**< UUID type for the LED Button Service. */
    uint16_t                    service_handle;      /**< Handle of LED Button Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t    led_char_handles;    /**< Handles related to the LED Characteristic. */
    ble_write_handler_t     write_handler;   /**< Event handler to be called when the LED Characteristic is written. */
};


//添加新的自定义服务
// 新服务观察者宏（新增）
#define BLE_CUS_DEF(_name) \
static ble_cus_t _name; \
NRF_SDH_BLE_OBSERVER(_name ## _obs, \
                     APP_BLE_OBSERVER_PRIO, \
                     ble_on_ble_evt, &_name)

#define CUSTOM_SERVICE_UUID        0xFF01       // 自定义服务UUID
#define CUSTOM_VALUE_CHAR_UUID     0xFF02       // 特征值UUID（读写+通知）
#define MAX_DATA_LEN               20           // 最大数据长度

typedef struct ble_cus_s ble_cus_t;

// 回调函数类型定义
typedef void (*ble_cus_write_handler_t)(uint16_t conn_handle, ble_cus_t *p_cus, const uint8_t *data, uint16_t length);

typedef struct {
    ble_cus_write_handler_t write_handler;     // 写操作回调
} ble_cus_init_t;

// 服务结构体
struct ble_cus_s {
    uint16_t                    service_handle;
    ble_gatts_char_handles_t    value_handles;
    uint8_t                     uuid_type;
    uint16_t                    conn_handle;
    ble_cus_write_handler_t     write_handler;
    bool                        notify_enabled;
	  uint8_t                     read_buf[MAX_DATA_LEN];  // 新增读取缓冲区
    uint16_t                    read_len;                     // 当前数据长度
};

// 初始化函数声明
uint32_t ble_cus_init(ble_cus_t *p_cus, const ble_cus_init_t *p_cus_init);

// 数据发送函数
uint32_t ble_cus_send_data(ble_cus_t *p_cus, uint8_t *data, uint16_t length);

////////////////////////////共用函数///////////////////////////////
//******************************************************************************
// fn :ble_on_ble_evt
//
// brief : BLE事件处理函数
//
// param : p_ble_evt -> ble事件
//         p_context -> ble事件处理程序的参数（暂时理解应该是不同的服务，注册时所携带的结构体参数）
//
// return : none
void ble_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

void ble_init(void);
#ifdef __cplusplus
}
#endif

#endif // APP_BLE_SERVER_H__

/** @} */
