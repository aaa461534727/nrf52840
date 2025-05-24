#ifndef _APP_OTA_FLASH_H
#define _APP_OTA_FLASH_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/*新固件存储FLASH的截止地址*/
#define FIRMWARE_BLE_AREA_END_ADDR            (0x58000UL-1)/*100KB*/
/*新固件存储FLASH的起始地址*/
#define FIRMWARE_BLE_AREA_BEIGN_ADDR        (0x3F000UL)/*252K~352K合计100KB空间*/

typedef struct
{
    uint32_t                image_size;         /**< Size of the image in the bank. */
    uint32_t                image_crc;          /**< CRC of the image. If set to 0, the CRC is ignored. */
    uint32_t                bank_code;          /**< Identifier code for the bank. */
		uint32_t 								update_start_address; /**< Value indicating the start address of the new firmware (before copy). It's always used, but it's most important for an SD/SD+BL update where the SD changes size or if the DFU process had a power loss when updating a SD with changed size. */       
} user_flash_data_item_t;

void ota_flash_init(void);
int32_t ota_flash_block_write(uint16_t block_offset, uint8_t * src, uint16_t srclen);
int32_t ota_flash_block_read(uint16_t block_offset, uint8_t * dst);
int32_t ota_flash_earse_all(void);
void ota_flash_blk_earse(uint8_t blkno);
int32_t ota_flash_write_buffer(uint8_t* buffer, uint32_t data_length);
void flash_ota_init(void);
#endif
