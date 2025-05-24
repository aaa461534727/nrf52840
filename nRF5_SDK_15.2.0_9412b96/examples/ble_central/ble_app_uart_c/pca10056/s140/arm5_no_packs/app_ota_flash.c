#include "nrf_fstorage_sd.h"
#include "crc32.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "app_ota_flash.h"
#include "app_wdt_handle.h"
/**********************************************define**********************************************************/

#define __ENABLE_IRQ                        __enable_irq
#define __DISABLE_IRQ                       __disable_irq


/*FLASH页大小*/
#define FLASH_PAGE_SIZE  (0x1000UL)

/**********************************************global**********************************************************/
static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);

/*创建FLASH存储参数fstorage_ble_firmware*/
NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage_ble_firmware) =
{
    /* Set a handler for fstorage events. 事件处理*/
    .evt_handler = fstorage_evt_handler,

    /* These below are the boundaries of the flash space assigned to this instance of fstorage.
     * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
     * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
     * last page of flash available to write data. */

    /*设置FLASH存储区地址*/
    .start_addr = FIRMWARE_BLE_AREA_BEIGN_ADDR,
    .end_addr   = FIRMWARE_BLE_AREA_END_ADDR,
	
};


/**********************************************function**********************************************************/

/*存储事件处理*/
static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
       NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }
    switch (p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
            NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",
                         p_evt->len, p_evt->addr);
            break;
        case NRF_FSTORAGE_EVT_ERASE_RESULT:
            NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
                         p_evt->len, p_evt->addr);
            break;
        default:
            break;
    }
}

/*等待FLASH操作完成*/
static void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
{
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        sd_app_evt_wait();
    }
}

/*初始化系统FLASH接口*/
void ota_flash_init(void)
{
    ret_code_t rc;
    nrf_fstorage_api_t * p_ble_fs_api = &nrf_fstorage_sd;
    rc = nrf_fstorage_init(&fstorage_ble_firmware, p_ble_fs_api, NULL);   
    APP_ERROR_CHECK(rc);		
}  

void write_test()
{		
		uint8_t buf[2048];
		ret_code_t err_code;
		memset(buf, 0x77, sizeof(buf));
	  err_code = nrf_fstorage_write(&fstorage_ble_firmware, 0x3F000, buf, 2048, NULL);//向0x3e000写入数据m_data数据
    APP_ERROR_CHECK(err_code);
    wait_for_flash_ready(&fstorage_ble_firmware);//等待写完
	        NRF_LOG_INFO("success\r\n");
				printf("success\r\n");
	
}


/*块写数据*/
int32_t ota_flash_block_write(uint16_t block_offset, uint8_t * src, uint16_t srclen)
{
    ret_code_t err_code;

    /*FLASH目标地址*/
    uint32_t addr = fstorage_ble_firmware.start_addr + block_offset * srclen;
    if(addr > fstorage_ble_firmware.end_addr)
    {
        NRF_LOG_INFO("flash write addr:%d error\r\n", addr);
				printf("flash write addr:%d error\r\n", addr);
        return -1;
    }
    NRF_LOG_INFO("write flash addr:%x,len:%d\r\n",addr, srclen);
		printf("write flash addr:%x,len:%d\r\n",addr, srclen);
    /*数据写入FLASH块*/
    err_code = nrf_fstorage_write(&fstorage_ble_firmware, addr, src, srclen, NULL);
    APP_ERROR_CHECK(err_code);
    wait_for_flash_ready(&fstorage_ble_firmware);
    if(err_code != 0)
    {
        NRF_LOG_INFO("flash block write failed\r\n");
				printf("flash block write failed\r\n");
        return -1;
    }
    return 0;
}

int32_t ota_flash_block_read(uint16_t block_offset, uint8_t * dst)
{
    ret_code_t err_code;

    /*FLASH目标地址*/
    uint32_t addr = fstorage_ble_firmware.start_addr + block_offset * 1024;
    if(addr > fstorage_ble_firmware.end_addr)
    {
        NRF_LOG_INFO("flash read addr:%d error\r\n", addr);
        return -1;
    }

    NRF_LOG_INFO("read flash addr:%x\r\n",addr);
    err_code = nrf_fstorage_read(&fstorage_ble_firmware, addr, dst, 1024);
    //APP_ERROR_CHECK(err_code);
    wait_for_flash_ready(&fstorage_ble_firmware);
    if(err_code != 0)
    {
        NRF_LOG_INFO("flash block read failed\r\n");
        return -1;
    }
    return 0;
}


/*下载固件存储区域全擦*/
int32_t ota_flash_earse_all(void)
{
    ret_code_t err_code;

    err_code = nrf_fstorage_erase(&fstorage_ble_firmware, fstorage_ble_firmware.start_addr, (fstorage_ble_firmware.end_addr - fstorage_ble_firmware.start_addr)/FLASH_PAGE_SIZE, NULL);
    //APP_ERROR_CHECK(err_code);
    wait_for_flash_ready(&fstorage_ble_firmware);
    if(err_code != 0)
    {
        NRF_LOG_INFO("flash block erease failed\r\n");
        //return -1;
    }

    uint32_t max= (fstorage_ble_firmware.end_addr - fstorage_ble_firmware.start_addr)/FLASH_PAGE_SIZE;

    for(uint32_t i=0 ; i < max; i++)
    {
        for(uint32_t j=0;j<3;j++)
        {
            /*判断是否擦除过*/
            if(((*(uint8_t*)(fstorage_ble_firmware.start_addr+i*FLASH_PAGE_SIZE))!=0xff)
                ||((*(uint8_t*)(fstorage_ble_firmware.start_addr+i*FLASH_PAGE_SIZE+0x4ff))!=0xff)
                ||((*(uint8_t*)(fstorage_ble_firmware.start_addr+i*FLASH_PAGE_SIZE+0xeff))!=0xff))		
            {
                err_code = nrf_fstorage_erase(&fstorage_ble_firmware, fstorage_ble_firmware.start_addr+i*FLASH_PAGE_SIZE,1, NULL);
                //APP_ERROR_CHECK(err_code);
                wait_for_flash_ready(&fstorage_ble_firmware);
                nrf_delay_ms(150);
            }
            else
            {
                break;
            }
        }
    }
    NRF_LOG_INFO("flash erase all\r\n");
		printf("flash erase all\r\n");
    return 0;
}

void ota_flash_blk_earse(uint8_t blkno)
{
    ret_code_t err_code;
    err_code = nrf_fstorage_erase(&fstorage_ble_firmware, fstorage_ble_firmware.start_addr+blkno*FLASH_PAGE_SIZE, 1 , NULL);
    wait_for_flash_ready(&fstorage_ble_firmware);
    if(err_code != 0)
    {
        NRF_LOG_INFO("erase blk%d err\r\n", blkno);
    }
}

#define OTA_FLASH_PAGE_SIZE (2048)

/**
 * @brief 将缓冲区数据分块写入Flash
 * @param buffer 源数据缓冲区指针
 * @param data_length 需要写入的总数据长度（字节）
 * @return 执行结果：
 *         - 0：写入成功
 *         - -1：参数错误
 *         - -2：Flash操作失败
 */
int32_t ota_flash_write_buffer(uint8_t* buffer, uint32_t data_length)
{
    /* 参数检查 */
    if (!buffer || data_length == 0) {
        NRF_LOG_INFO("Invalid parameters\n");
				printf("Invalid parameters\n");
        return -1;
    }

    uint32_t total_blocks = data_length / OTA_FLASH_PAGE_SIZE;
    uint32_t remaining_bytes = data_length % OTA_FLASH_PAGE_SIZE;

    /* 处理完整块 */
    for (uint32_t block = 0; block < total_blocks; block++) {
        /* 擦除目标块 */
//        ota_flash_blk_earse(block);
        
        /* 执行块写入 */
        if (ota_flash_block_write(block, 
                                buffer + (block * OTA_FLASH_PAGE_SIZE),
                                OTA_FLASH_PAGE_SIZE) != 0) {
            NRF_LOG_INFO("Block %d write failed\n", block);
						printf("Block %d write failed\n", block);									
            return -2;
        }
				wdt_feed();												
				nrf_delay_ms(1000);
    }

    /* 处理剩余字节（非完整块）*/
    if (remaining_bytes > 0) {
        uint8_t padding_buffer[OTA_FLASH_PAGE_SIZE] = {0xFF};
        uint32_t last_block = total_blocks;

        /* 擦除最后一个块 */
//        ota_flash_blk_earse(last_block);

        /* 复制剩余数据并填充0xFF */
        memcpy(padding_buffer, 
              buffer + (total_blocks * OTA_FLASH_PAGE_SIZE),
              remaining_bytes);

        /* 写入填充后的块 */
        if (ota_flash_block_write(last_block, 
                                padding_buffer,
                                OTA_FLASH_PAGE_SIZE) != 0) {
            NRF_LOG_INFO("Last block write failed\n");
						printf("Last block write failed\n");
            return -2;
        }
    }

    NRF_LOG_INFO("Total wrote %d blocks + %d bytes\n", 
              total_blocks, remaining_bytes);
		printf("Total wrote %d blocks + %d bytes\n", 
              total_blocks, remaining_bytes);
    return 0;
}


//ota_flag
NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,

    /* These below are the boundaries of the flash space assigned to this instance of fstorage.
     * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
     * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
     * last page of flash available to write data. */
    .start_addr = 0x58000,
    .end_addr   = 0x59000,
};

/* Dummy data to write to flash. */
static uint32_t m_data          = 0xAADC0FFE;
static uint32_t m_data2;  

static uint32_t nrf5_flash_end_addr_get()
{
    uint32_t const bootloader_addr = NRF_UICR->NRFFW[0];
    uint32_t const page_sz         = NRF_FICR->CODEPAGESIZE;
    uint32_t const code_sz         = NRF_FICR->CODESIZE;

    return (bootloader_addr != 0xFFFFFFFF ?
            bootloader_addr : (code_sz * page_sz));
}


/*FLASH用户数据,RAM*/
user_flash_data_item_t  user_flash_data_item;

void flash_ota_init(void)
{
    ret_code_t rc;
		uint8_t buf[256];
		uint8_t TXbuf=0;
    nrf_fstorage_api_t * p_fs_api;
	  p_fs_api = &nrf_fstorage_sd;//处理操作类型设置	
	  rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);//flash处理的开始地址和结束地址初始化
    APP_ERROR_CHECK(rc);
	
	 /*flash data init*/
		rc = nrf_fstorage_erase(&fstorage, fstorage.start_addr, 1, NULL);  
		APP_ERROR_CHECK(rc);
		wait_for_flash_ready(&fstorage);
		rc = nrf_fstorage_write(&fstorage, fstorage.start_addr, &user_flash_data_item, sizeof(user_flash_data_item), NULL);
		APP_ERROR_CHECK(rc);
		wait_for_flash_ready(&fstorage);
		//read
    rc = nrf_fstorage_read(&fstorage, fstorage.start_addr, &user_flash_data_item, sizeof(user_flash_data_item));
    APP_ERROR_CHECK(rc);
		printf(" size:%02X  crc:%02X bank_code:%02X  address:%02x \r\n",user_flash_data_item.image_size,user_flash_data_item.image_crc,
						user_flash_data_item.bank_code,user_flash_data_item.update_start_address);
}

