#ifndef _power_meter_
#define _power_meter_

#define SENSOR_PIN		3

volatile uint32 PowerCnt = 0;
volatile uint8 	PowerCntNeedSave = 0;

void power_meter_init(void);

#endif
