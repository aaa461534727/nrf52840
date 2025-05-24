
#include "app_timer.h"
#include "nrf_drv_wdt.h"
#include "nrf_log.h"
#include "nrf_drv_wdt.h"
#include "nrf_drv_clock.h"
#include "app_wdt_handle.h"

nrf_drv_wdt_channel_id m_channel_id;


/*wdt在CPU休眠的时候不工作，唤醒后开始倒计时，时间记到0复位，*/

void wdt_event_handler(void)
{
    /*nothing*/
}

/**/
void wdt_feed(void)
{
    nrf_drv_wdt_channel_feed(m_channel_id);
}

/*WDT初始化并启动*/
void wdt_start(void)
{
    uint32_t err_code = NRF_SUCCESS;

    /*wdt init,timeout define in WDT_CONFIG_RELOAD_VALUE(sdk_config.h),超时时间2000 = 2s*/
    nrf_drv_wdt_config_t config = NRF_DRV_WDT_DEAFULT_CONFIG;
    err_code = nrf_drv_wdt_init(&config, wdt_event_handler);
    APP_ERROR_CHECK(err_code);

    /*启动wdt*/
    err_code = nrf_drv_wdt_channel_alloc(&m_channel_id);
    APP_ERROR_CHECK(err_code);
    nrf_drv_wdt_enable();

    wdt_feed();
}

