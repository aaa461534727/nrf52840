nrfutil settings generate --family NRF52840 --application app.hex --application-version 1 --bootloader-version 1 --bl-settings-version 2 settings.hex
mergehex --merge bootloader.hex settings.hex --output bl_temp.hex
mergehex --merge bl_temp.hex app.hex s140_nrf52_6.1.0_softdevice.hex --output whole.hex
python cal_crc.py


