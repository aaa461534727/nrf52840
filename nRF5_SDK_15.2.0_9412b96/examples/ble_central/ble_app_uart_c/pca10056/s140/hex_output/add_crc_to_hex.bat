srec_cat.exe .\app.hex -intel -crop 0x08000000 0x0803FFFC -fill 0xFF 0x08000000 0x0803FFFC -STM32_Little_Endian 0x0803FFFC -o .\app_crc.hex -intel

hex2bin .\app_crc.hex
