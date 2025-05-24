#include "nrf_dfu_types.h"//
#include "sdk_errors.h"
#include "nrf_fstorage.h"  //
#include "nrf_fstorage_sd.h"//
#include "nrf_log.h"
#include "dn_dfu_flash.h"
//#include "nrf_delay.h"
#include "nrf_mbr.h"      //

//nrf_fstorage_sd.c

//#include "nrf_fstorage_nvmc.h"
static void dfu_fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);

/*初始化dfu flash*/
NRF_FSTORAGE_DEF(nrf_fstorage_t m_fs) =
{
    .evt_handler = dfu_fstorage_evt_handler,
    .start_addr  = MBR_SIZE,
    .end_addr    = BOOTLOADER_SETTINGS_ADDRESS + BOOTLOADER_SETTINGS_PAGE_SIZE,
    //.end_addr    = 0x100000,
};

static void dfu_fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        //NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
        NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation,errorcode=%d,errorid=%d",p_evt->result,p_evt->id);
        return;
    }

    switch (p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            //NRF_LOG_INFO("-->write %d  0x%x.",
            // p_evt->len, p_evt->addr);
        } break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            //NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
            //p_evt->len, p_evt->addr);
        } break;

        default:
            break;
    }
}


static uint32_t nrf5_flash_end_addr_get()
{
    uint32_t const bootloader_addr = NRF_UICR->NRFFW[0];
    uint32_t const page_sz         = NRF_FICR->CODEPAGESIZE;
    uint32_t const code_sz         = NRF_FICR->CODESIZE;

    return (bootloader_addr != 0xFFFFFFFF ? bootloader_addr : (code_sz * page_sz));
}

/*DFU FLASH初始化*/
uint32_t dfu_flashInit(void)
{
    uint32_t Writable_size;
    nrf_fstorage_api_t * p_fs_api;

    /*处理操作类型设置*/
    p_fs_api = &nrf_fstorage_sd;

    /*flash处理的开始地址和结束地址初始化*/
    nrf_fstorage_init(&m_fs, p_fs_api, NULL);

    /*获取地址，判断为可写地址大小*/
    Writable_size = nrf5_flash_end_addr_get();
    return Writable_size;
}

static void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
{
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
       sd_app_evt_wait();
    }
}

ret_code_t dfu_flash_erase(uint32_t page_addr,uint32_t num_pages,nrf_dfu_flash_callback_t callback)
{
    ret_code_t rc;

    rc = nrf_fstorage_erase(&m_fs, page_addr, num_pages, (void *)callback);
    APP_ERROR_CHECK(rc);
    wait_for_flash_ready(&m_fs);
    return rc;
}

ret_code_t dfu_flash_store(uint32_t dest,void const * p_src,uint32_t len,nrf_dfu_flash_callback_t callback)
{
    ret_code_t rc;

    //lint -save -e611 (Suspicious cast)
    rc = nrf_fstorage_write(&m_fs, dest, p_src, len, (void *)callback);
    APP_ERROR_CHECK(rc);
    wait_for_flash_ready(&m_fs);
    //nrf_delay_ms(2);
    return rc;
}

ret_code_t dfu_flash_Read(void * p_dest ,uint32_t src, uint32_t len,nrf_dfu_flash_callback_t callback)
{
    ret_code_t rc;
    rc = nrf_fstorage_read(&m_fs,src,p_dest,len);
    APP_ERROR_CHECK(rc);
    return rc;
}


