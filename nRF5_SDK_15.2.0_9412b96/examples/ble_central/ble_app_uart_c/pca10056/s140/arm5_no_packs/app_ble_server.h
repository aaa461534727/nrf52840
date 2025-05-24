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
// brief : ��ʼ��LED����ʵ��
//
// param : _name -> ʵ��������
//
// return : none
#define BLE_LED_DEF(_name)                                                                          \
static ble_t _name;                                                                             \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_LBS_BLE_OBSERVER_PRIO,                                                     \
                     ble_on_ble_evt, &_name)


// ����LED�����UUID���Լ�����ֵ��ز���
#define UUID_BASE        {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, \
                              0xDE, 0xEF, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00}

#define UUID_SERVICE     0xFFF0
#define UUID_CHAR        0xFFF1

#define UUID_CHAR_LEN    4


// ����ble_t����
typedef struct ble_s ble_t;

// ����һ���ص�����
typedef void (*ble_write_handler_t) (uint16_t conn_handle, ble_t * p_vent, ble_gatts_evt_write_t const * p_evt_write);

//�����ʼ���ṹ��
typedef struct
{
    uint8_t *p_led_value;
    ble_write_handler_t write_handler; /**< Event handler to be called when the LED Characteristic is written. */
} ble_server_init_t;

// ����ṹ��
struct ble_s
{
    uint8_t                     uuid_type;           /**< UUID type for the LED Button Service. */
    uint16_t                    service_handle;      /**< Handle of LED Button Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t    led_char_handles;    /**< Handles related to the LED Characteristic. */
    ble_write_handler_t     write_handler;   /**< Event handler to be called when the LED Characteristic is written. */
};


//����µ��Զ������
// �·���۲��ߺ꣨������
#define BLE_CUS_DEF(_name) \
static ble_cus_t _name; \
NRF_SDH_BLE_OBSERVER(_name ## _obs, \
                     APP_BLE_OBSERVER_PRIO, \
                     ble_on_ble_evt, &_name)

#define CUSTOM_SERVICE_UUID        0xFF01       // �Զ������UUID
#define CUSTOM_VALUE_CHAR_UUID     0xFF02       // ����ֵUUID����д+֪ͨ��
#define MAX_DATA_LEN               20           // ������ݳ���

typedef struct ble_cus_s ble_cus_t;

// �ص��������Ͷ���
typedef void (*ble_cus_write_handler_t)(uint16_t conn_handle, ble_cus_t *p_cus, const uint8_t *data, uint16_t length);

typedef struct {
    ble_cus_write_handler_t write_handler;     // д�����ص�
} ble_cus_init_t;

// ����ṹ��
struct ble_cus_s {
    uint16_t                    service_handle;
    ble_gatts_char_handles_t    value_handles;
    uint8_t                     uuid_type;
    uint16_t                    conn_handle;
    ble_cus_write_handler_t     write_handler;
    bool                        notify_enabled;
	  uint8_t                     read_buf[MAX_DATA_LEN];  // ������ȡ������
    uint16_t                    read_len;                     // ��ǰ���ݳ���
};

// ��ʼ����������
uint32_t ble_cus_init(ble_cus_t *p_cus, const ble_cus_init_t *p_cus_init);

// ���ݷ��ͺ���
uint32_t ble_cus_send_data(ble_cus_t *p_cus, uint8_t *data, uint16_t length);

////////////////////////////���ú���///////////////////////////////
//******************************************************************************
// fn :ble_on_ble_evt
//
// brief : BLE�¼�������
//
// param : p_ble_evt -> ble�¼�
//         p_context -> ble�¼��������Ĳ�������ʱ���Ӧ���ǲ�ͬ�ķ���ע��ʱ��Я���Ľṹ�������
//
// return : none
void ble_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

void ble_init(void);
#ifdef __cplusplus
}
#endif

#endif // APP_BLE_SERVER_H__

/** @} */
