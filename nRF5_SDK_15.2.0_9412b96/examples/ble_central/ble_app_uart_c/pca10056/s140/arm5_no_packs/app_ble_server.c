/**
 * Copyright (c) 2013 - 2018, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
#include "sdk_common.h"
#include "app_ble_server.h"
#include "ble_srv_common.h"
#include "nrf_log.h"
//#include "at_wrapper.h"

/*激活状态*/
uint32_t g_active = 1;
/*复位状态*/
uint32_t g_reset = 0;

uint32_t g_test = 0;

/**@brief Function for handling the Write event.
 *
 * @param[in] p_lbs      LED Button Service structure.
 * @param[in] p_ble_evt  Event received from the BLE stack.
 */
static void on_write(ble_vent_t * p_vent, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    NRF_LOG_INFO("len=%d,hand=%x\r\n",p_evt_write->len, p_vent->vent_write_handler);
    if ((p_evt_write->len > 0) && (p_vent->vent_write_handler != NULL))
    {
        /*调用vent_write_handler*/
        p_vent->vent_write_handler(p_ble_evt->evt.gap_evt.conn_handle, p_vent, p_evt_write);
    }
}

/*gatt事件回调*/
void ble_vent_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_vent_t * p_vent = (ble_vent_t *)p_context;

    //NRF_LOG_INFO("BLE_GATTS_EVT:%d\r\n",p_ble_evt->header.evt_id);
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE:/*收到主机数据*/
            NRF_LOG_INFO("BLE_GATTS_EVT_WRITE\r\n");

            on_write(p_vent, p_ble_evt);
            break;

        default:
            //NRF_LOG_INFO("BLE_GATTS_EVT %x",p_ble_evt->header.evt_id);
            // No implementation needed.
            break;
    }
}

/**@brief Function for adding the LED Characteristic.
 *
 * @param[in] p_lbs      LED Button Service structure.
 * @param[in] p_lbs_init LED Button Service initialization structure.
 *
 * @retval NRF_SUCCESS on success, else an error value from the SoftDevice
 */
static uint32_t vent_char_add(ble_vent_t * p_vent, const ble_vent_init_t * p_vent_init , uint16_t uuid, ble_gatts_char_handles_t * handle)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;
    //uint32_t  tmp;


    memset(&char_md, 0, sizeof(char_md));
    /*设置读写属性*/
    char_md.char_props.read  = 1;
    char_md.char_props.write = 1;
    //char_md.char_props.write_wo_resp = 1;

    char_md.p_char_user_desc = NULL;
    char_md.p_char_pf        = NULL;
    char_md.p_user_desc_md   = NULL;
    char_md.p_cccd_md        = NULL;
    char_md.p_sccd_md        = NULL;

    /*设置属性UUID*/
    ble_uuid.type = p_vent->uuid_type;
    ble_uuid.uuid = uuid;

    memset(&attr_md, 0, sizeof(attr_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    attr_md.vloc    = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth = 0;
    attr_md.wr_auth = 0;
    attr_md.vlen    = 0;

    /*设置属性特征*/
    memset(&attr_char_value, 0, sizeof(attr_char_value));
    attr_char_value.p_uuid    = &ble_uuid; /*UUID*/
    attr_char_value.p_attr_md = &attr_md; /**/

    if(VENT_UUID_MV_GAP_CHAR == uuid || VENT_UUID_ST_GAP_CHAR == uuid || VENT_UUID_CLPW_GAP_CHAR == uuid )
    {
        attr_char_value.init_len  = sizeof(uint32_t);
        attr_char_value.max_len   = sizeof(uint32_t);
    }

    else if(VENT_UUID_ACT_BUTTON_CHAR == uuid || VENT_UUID_RST_BUTTON_CHAR == uuid || VENT_UUID_TEST_BUTTON_CHAR == uuid)
    {
        attr_char_value.init_len  = sizeof(uint32_t);
        attr_char_value.max_len   = sizeof(uint32_t);
    }
    else if(VENT_UUID_BOX_VAL_CHAR == uuid)
    {
        attr_char_value.init_len  = BLE_BOX_VAL_LEN;
        attr_char_value.max_len   = BLE_BOX_VAL_LEN;
    }

    else if(VENT_UUID_APN == uuid)
    {
        attr_char_value.init_len  = BLE_APN_MAX_LEN;
        attr_char_value.max_len   = BLE_APN_MAX_LEN;
    }

    attr_char_value.init_offs = 0;

    /*设置属性默认值*/
    if(VENT_UUID_MV_GAP_CHAR == uuid)
    {
        attr_char_value.p_value   = (uint8_t*)&"1";
    }
    else if(VENT_UUID_ST_GAP_CHAR == uuid )
    {
        attr_char_value.p_value   = (uint8_t*)&"1";
    }

    else if(VENT_UUID_ACT_BUTTON_CHAR == uuid)
    {
        attr_char_value.p_value   = (uint8_t*)&g_active;
    }
    else if(VENT_UUID_RST_BUTTON_CHAR == uuid)
    {
        attr_char_value.p_value   = (uint8_t*)&g_reset;
    }
    else if(VENT_UUID_BOX_VAL_CHAR == uuid)
    {
        attr_char_value.p_value   = (uint8_t*)&"1";
    }

    else if(VENT_UUID_APN == uuid)
    {
        attr_char_value.p_value   = (uint8_t*)&"1";
    }
    else if(VENT_UUID_TEST_BUTTON_CHAR == uuid)
    {
        attr_char_value.p_value   = (uint8_t*)&g_test;
    }
    else if(VENT_UUID_CLPW_GAP_CHAR == uuid)
    {
        attr_char_value.p_value   = (uint8_t*)&"1";
    }


    return sd_ble_gatts_characteristic_add(p_vent->service_handle, &char_md, &attr_char_value, handle);
}

/*服务初始化*/
uint32_t ble_vent_init(ble_vent_t * p_vent, const ble_vent_init_t *p_vent_init)
{
    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure.
    p_vent->vent_write_handler = p_vent_init->vent_write_handler;

    /*增加服务UUID*/
    ble_uuid128_t base_uuid = {VENT_UUID_BASE};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_vent->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_vent->uuid_type;
    ble_uuid.uuid = VENT_UUID_SERVICE;

    /*增加服务*/
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_vent->service_handle);
    VERIFY_SUCCESS(err_code);

    /*增加属性*/
    err_code = vent_char_add(p_vent, p_vent_init, VENT_UUID_MV_GAP_CHAR, &p_vent->mv_gap_char_handles);
    VERIFY_SUCCESS(err_code);
    err_code = vent_char_add(p_vent, p_vent_init, VENT_UUID_ST_GAP_CHAR, &p_vent->st_gap_char_handles);
    VERIFY_SUCCESS(err_code);
    //err_code = vent_char_add(p_vent, p_vent_init, VENT_UUID_RFS_VAL_CHAR, &p_vent->rfs_val_char_handles);
    //VERIFY_SUCCESS(err_code);
    err_code = vent_char_add(p_vent, p_vent_init, VENT_UUID_BOX_VAL_CHAR, &p_vent->box_id_char_handles);
    VERIFY_SUCCESS(err_code);
    err_code = vent_char_add(p_vent, p_vent_init, VENT_UUID_ACT_BUTTON_CHAR, &p_vent->act_button_char_handles);
    VERIFY_SUCCESS(err_code);
    err_code = vent_char_add(p_vent, p_vent_init, VENT_UUID_RST_BUTTON_CHAR, &p_vent->rst_button_char_handles);
    VERIFY_SUCCESS(err_code);
    //err_code = vent_char_add(p_vent, p_vent_init, VENT_UUID_GSENSOR_W_GAP, &p_vent->gs_gap_char_handles);
    //VERIFY_SUCCESS(err_code);
    //err_code = vent_char_add(p_vent, p_vent_init, VENT_UUID_GSENSOR_W_TIME, &p_vent->gs_time_char_handles);
    //VERIFY_SUCCESS(err_code);
    err_code = vent_char_add(p_vent, p_vent_init, VENT_UUID_APN, &p_vent->apn_char_handles);
    VERIFY_SUCCESS(err_code);
    err_code = vent_char_add(p_vent, p_vent_init, VENT_UUID_TEST_BUTTON_CHAR, &p_vent->test_button_char_handles);
    VERIFY_SUCCESS(err_code);
    err_code = vent_char_add(p_vent, p_vent_init, VENT_UUID_CLPW_GAP_CHAR, &p_vent->clpw_gap_char_handles);
    VERIFY_SUCCESS(err_code);

    return NRF_SUCCESS;
}


