#ifndef _power_meter_
#define _power_meter_

#define SENSOR_PIN		3
#define SENSOR_FRONT_EDGE 	GPIO_PIN_INTR_NEGEDGE
#define SENSOR_BACK_EDGE 	GPIO_PIN_INTR_POSEDGE
#define SENSOR_TASK_PRIO	USER_TASK_PRIO_2 // Hi prio, _0,1 - may be used

void power_meter_init(uint8 index) ICACHE_FLASH_ATTR;
void user_idle(void) ICACHE_FLASH_ATTR;

void uart_wait_tx_fifo_empty(void) ICACHE_FLASH_ATTR;

// GPIO_PIN_INTR_NEGEDGE - down
// GPIO_PIN_INTR_POSEDGE - up
// GPIO_PIN_INTR_ANYEDGE - both
// GPIO_PIN_INTR_LOLEVEL - low level
// GPIO_PIN_INTR_HILEVEL - high level
// GPIO_PIN_INTR_DISABLE - disable interrupt

#endif
