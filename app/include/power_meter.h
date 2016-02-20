#ifndef _power_meter_
#define _power_meter_

#define SENSOR_PIN		3
#define SENSOR_EDGE 	GPIO_PIN_INTR_POSEDGE // GPIO_PIN_INTR_NEGEDGE

volatile uint32 PowerCnt = 0;

void power_meter_init(uint8 index) ICACHE_FLASH_ATTR;

// GPIO_PIN_INTR_NEGEDGE - down
// GPIO_PIN_INTR_POSEDGE - up
// GPIO_PIN_INTR_ANYEDGE - both
// GPIO_PIN_INTR_LOLEVEL - low level
// GPIO_PIN_INTR_HILEVEL - high level
// GPIO_PIN_INTR_DISABLE - disable interrupt

#endif
