#include "user_config.h"
#include "bios.h"
#include "sdk/add_func.h"
#include "hw/esp8266.h"
#include "user_interface.h"
#include "flash_eep.h"
#include "webfs.h"
#include "sdk/libmain.h"
#include "driver/gpio16.h"
#include "power_meter.h"

// GPIO_PIN_INTR_NEGEDGE - down
// GPIO_PIN_INTR_POSEDGE - up
// GPIO_PIN_INTR_ANYEDGE - both
// GPIO_PIN_INTR_LOLEVEL - low level
// GPIO_PIN_INTR_HILEVEL - high level
// GPIO_PIN_INTR_DISABLE - disable interrupt

void ICACHE_FLASH_ATTR gpio_intr_callback(unsigned pin, unsigned level)
{
	if(pin == SENSOR_PIN) {
		PowerCnt++;
		PowerCntNeedSave = 1;
	}
}

void power_meter_init(void)
{
	set_gpio_mode(SENSOR_PIN, GPIO_FLOAT, GPIO_INT);
	gpio_intr_init(SENSOR_PIN, GPIO_PIN_INTR_POSEDGE);
	gpio_intr_attach(gpio_intr_callback);

}
