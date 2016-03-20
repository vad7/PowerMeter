#ifndef _power_meter_
#define _power_meter_

#include "sntp.h"

#define I2C_SDA_PIN 			2
#define I2C_SCL_PIN 			0
#define SENSOR_PIN				3
#define SENSOR_FRONT_EDGE 		GPIO_PIN_INTR_NEGEDGE
#define SENSOR_BACK_EDGE 		GPIO_PIN_INTR_POSEDGE
#define	FRAM_SIZE_DEFAULT		32768
#define FRAM_MAX_BLOCK_AT_ONCE 	128
#define DEFAULT_PULSES_PER_0_01_KWT 6 // 600 per kWt
#define TIME_STEP_SEC			60 // 1 minute
#define SENSOR_TASK_PRIO			USER_TASK_PRIO_2 // Hi prio, _0,1 - may be used

typedef struct __attribute__((packed)) {
	uint32 Fram_Size;
	uint16 PulsesPer0_01KWt; // 6
//	char sntp_server[20];
} CFG_METER;
CFG_METER cfg_meter;

typedef struct __attribute__((packed)) {
	uint32 PowerCnt;
	uint32 TotalCnt;
	uint32 PtrCurrent;
	time_t LastTime;
	//uint8 Reserved[];
} FRAM_STORE;
FRAM_STORE fram_store;
#define StartArrayOfCnts	32 // Start pos, packed: if [cell] = 0 and [cell+1] > 1, then [cell+1] = How many minutes was 0.

typedef struct __attribute__((packed)) {
	uint8 Cnt1; // otherwise pulses in minute (max = 255)
	uint8 Cnt2; // if Cnt1 == 0, Cnt2 = minutes of zero;
	uint8 Cnt3; // always = 0
	uint8 Cnt4; // always = 0
} CNT_CURRENT;
CNT_CURRENT CntCurrent; // = {0, 0, 0, 0};

uint32 WebChart_MaxMinutes; // ~ChartMaxMin~

void power_meter_init(uint8 index) ICACHE_FLASH_ATTR;
void user_idle(void) ICACHE_FLASH_ATTR;
bool write_power_meter_cfg(void) ICACHE_FLASH_ATTR;
void power_meter_clear_all_data(void) ICACHE_FLASH_ATTR;

void uart_wait_tx_fifo_empty(void) ICACHE_FLASH_ATTR;
void _localtime(const time_t * tim_p, struct tm * res) ICACHE_FLASH_ATTR;

// GPIO_PIN_INTR_NEGEDGE - down
// GPIO_PIN_INTR_POSEDGE - up
// GPIO_PIN_INTR_ANYEDGE - both
// GPIO_PIN_INTR_LOLEVEL - low level
// GPIO_PIN_INTR_HILEVEL - high level
// GPIO_PIN_INTR_DISABLE - disable interrupt

#endif
