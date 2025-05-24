/**
 * Copyright (c) 2016 - 2018, Nordic Semiconductor ASA
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

#include <stddef.h>
#include <string.h>
#include "nrf_error.h"
#include "dn_dfu_flash.h"
#include "nrf_soc.h"
#include "crc32.h"
#include "nrf_dfu_types.h"
#include "nrf_log.h"
#include "dn_dfu_settings.h"

#define DFU_SETTINGS_INIT_COMMAND_OFFSET      offsetof(nrf_dfu_settings_t, init_command)          //<! Offset in the settings struct where the InitCommand is located.


/**@brief   This variable reserves a page in flash for bootloader settings
 *          to ensure the linker doesn't place any code or variables at this location.
 */
#if defined (__CC_ARM )

    /*dfu FLASH存储区变量*/
    uint8_t m_dfu_settings_buffer[BOOTLOADER_SETTINGS_PAGE_SIZE]
    __attribute__((at(BOOTLOADER_SETTINGS_ADDRESS)))
    __attribute__((used));

#elif defined ( __GNUC__ ) || defined ( __SES_ARM )

    uint8_t m_dfu_settings_buffer[BOOTLOADER_SETTINGS_PAGE_SIZE]
    __attribute__((section(".bootloader_settings_page")))
    __attribute__((used));

#elif defined ( __ICCARM__ )

    __no_init __root uint8_t m_dfu_settings_buffer[BOOTLOADER_SETTINGS_PAGE_SIZE]
        @ BOOTLOADER_SETTINGS_ADDRESS;

#else
    #error Not a valid compiler/linker for m_dfu_settings placement.
#endif // Compiler specific
	
/*DFU数据RAM缓冲*/
dn_dfu_settings_t s_dfu_settings;


uint32_t nrf_dfu_settings_crc_get(void)
{
    // The crc is calculated from the s_dfu_settings struct, except the crc itself and the init command
    return crc32_compute((uint8_t*)&s_dfu_settings + 4, DFU_SETTINGS_INIT_COMMAND_OFFSET-4, NULL);
}


ret_code_t nrf_dfu_settings_init(bool sd_initialized)
{
    /*初始化DFU FLASH*/
    dfu_flashInit();

    // Copy the DFU settings out of flash and into a buffer in RAM.
    memcpy((void*)&s_dfu_settings, m_dfu_settings_buffer, sizeof(dn_dfu_settings_t));
    if (s_dfu_settings.crc != 0xFFFFFFFF)
    {
        // CRC is set. Content must be valid
        uint32_t crc = nrf_dfu_settings_crc_get();
        if (crc == s_dfu_settings.crc)
        {
            NRF_LOG_INFO("dfu init ok\r\n");
						printf("dfu init ok\r\n");
            return NRF_SUCCESS;
        }	
    }

    // Reached if the page is erased or CRC is wrong.
    NRF_LOG_INFO("Resetting bootloader settings.\r\n");
		printf("Resetting bootloader settings.\r\n");
    memset(&s_dfu_settings, 0x00, sizeof(dn_dfu_settings_t));
    s_dfu_settings.settings_version = 1;

    ret_code_t rc = nrf_dfu_settings_write(NULL);
    if (rc != NRF_SUCCESS)
    {
        NRF_LOG_INFO("nrf_dfu_flash_write() failed with error: %x", rc);
        return NRF_ERROR_INTERNAL;
    }
    NRF_LOG_INFO("dfu init2 ok\r\n");
		printf("dfu init2 ok\r\n");
    return NRF_SUCCESS;
}

ret_code_t nrf_dfu_settings_write(nrf_dfu_flash_callback_t callback)
{
    ret_code_t err_code;
    static dn_dfu_settings_t temp_dfu_settings;

    if (memcmp(&s_dfu_settings, m_dfu_settings_buffer, sizeof(dn_dfu_settings_t)) == 0)
    {
        NRF_LOG_INFO("New settings are identical to old, write not needed. Skipping.\r\n");
        if (callback != NULL)
        {
            callback(NULL);
        }
        return NRF_SUCCESS;
    }

    NRF_LOG_INFO("Writing settings...\r\n");
    NRF_LOG_INFO("Erasing old settings at: 0x%08x\r\n", (uint32_t)m_dfu_settings_buffer);
    printf("Writing settings...\r\n");
    printf("Erasing old settings at: 0x%08x\r\n", (uint32_t)m_dfu_settings_buffer);
    // Not setting the callback function because ERASE is required before STORE
    // Only report completion on successful STORE.
    err_code = dfu_flash_erase((uint32_t)m_dfu_settings_buffer, 1, NULL);

    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_INFO("Could not erase the settings page!\r\n");
        return NRF_ERROR_INTERNAL;
    }
  
    s_dfu_settings.crc = nrf_dfu_settings_crc_get();

    //s_dfu_settings.update_para_crc = crc32_compute((uint8_t*)&s_dfu_settings.update_para_crc + 4, DFU_SETTINGS_INIT_APP_PARA_SIZE, NULL);
    memcpy(&temp_dfu_settings, &s_dfu_settings, sizeof(dn_dfu_settings_t));

#if 0
    uint8_t *ptmp = (uint8_t*)&temp_dfu_settings;
    NRF_LOG_INFO("ram data:");
    for(uint16_t i = 0; i < DFU_SETTINGS_INIT_COMMAND_OFFSET;i++)
    {
        NRF_LOG_INFO("%x ",ptmp[i]);
    }
    NRF_LOG_INFO("\r\n")
#endif

    err_code = dfu_flash_store((uint32_t)m_dfu_settings_buffer,
                                                   &temp_dfu_settings,
                                                   sizeof(dn_dfu_settings_t),
                                                   callback);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_INFO("Could not write the DFU settings page!\r\n");
        return NRF_ERROR_INTERNAL;
    }

#if 0
    NRF_LOG_INFO("flash data:");
    for(uint16_t i = 0; i < DFU_SETTINGS_INIT_COMMAND_OFFSET;i++)
    {
        NRF_LOG_INFO("%x ",m_dfu_settings_buffer[i]);
    }
    NRF_LOG_INFO("\r\n")
#endif

    NRF_LOG_INFO("write DFU settings ok\r\n");
		printf("write DFU settings ok\r\n");
    return NRF_SUCCESS;
}

__WEAK ret_code_t nrf_dfu_settings_additional_erase(void)
{
    NRF_LOG_WARNING("No additional data erased");
    return NRF_SUCCESS;
}
