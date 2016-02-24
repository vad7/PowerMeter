#include "user_config.h"
#include "bios.h"
#include "sdk/add_func.h"
#include "hw/esp8266.h"
#include "user_interface.h"
#include "web_iohw.h"
#include "flash_eep.h"
#include "webfs.h"
#include "sdk/libmain.h"
//#include "driver/gpio16.h"
#include "power_meter.h"

#define GPIO_Tasks 		1
#define GPIO_Int_Signal 1
ETSEvent GPIO_TaskQueue[GPIO_Tasks] DATA_IRAM_ATTR;

static void ICACHE_FLASH_ATTR GPIO_Task_NewData(os_event_t *e)
{
    switch(e->sig) {
    	case GPIO_Int_Signal:
    		if(e->par == SENSOR_PIN) { // new data
				#if DEBUGSOO > 2
    				os_printf("* INT GPIO%d, PowerCnt=%d\n", SENSOR_PIN, PowerCnt);
				#endif



    		}
    		break;
    }
}

static void gpio_int_handler(void)
{
	uint32 gpio_status = GPIO_STATUS;
	GPIO_STATUS_W1TC = gpio_status;
	if(gpio_status & (1<<SENSOR_PIN)) {
		PowerCnt++;
		system_os_post(USER_TASK_PRIO_1, GPIO_Int_Signal, SENSOR_PIN);
	    gpio_pin_intr_state_set(SENSOR_PIN, SENSOR_EDGE); // GPIO_PIN_INTR_ANYEDGE GPIO_PIN_INTR_POSEDGE | GPIO_PIN_INTR_NEGEDGE ?
	}
}

void ICACHE_FLASH_ATTR power_meter_init(uint8 index)
{
	ets_isr_mask(1 << ETS_GPIO_INUM); // запрет прерываний GPIOs
	if(index & 1) {
		// setup interrupt and os_task
		uint32 pins_mask = (1<<SENSOR_PIN);
		gpio_output_set(0,0,0, pins_mask); // настроить GPIOx на ввод
		set_gpiox_mux_func_ioport(SENSOR_PIN); // установить функцию GPIOx в режим порта i/o
	//	GPIO_ENABLE_W1TC = pins_mask; // GPIO OUTPUT DISABLE отключить вывод в портах
		ets_isr_attach(ETS_GPIO_INUM, gpio_int_handler, NULL);
		gpio_pin_intr_state_set(SENSOR_PIN, SENSOR_EDGE); // GPIO_PIN_INTR_NEGEDGE
		// разрешить прерывания GPIOs
		GPIO_STATUS_W1TC = pins_mask;
	}
	if(index & 2) {
		system_os_task(GPIO_Task_NewData, USER_TASK_PRIO_1, GPIO_TaskQueue, GPIO_Tasks);
	}
	ets_isr_unmask(1 << ETS_GPIO_INUM);

//	os_printf("-------- read cfg  ------------\n");
//	char buf[50];
//	uint32 faddr = FMEMORY_SCFG_BASE_ADDR;
//	fobj_head fobj;
//	do {
//		if(flash_read(faddr, &fobj, fobj_head_size)) break;
//		if(fobj.x == fobj_x_free && fobj.n.size > MAX_FOBJ_SIZE) break;
//		if(flash_read(faddr + fobj_head_size, buf, align(fobj.n.size))) break;
//		buf[fobj.n.size] = 0;
//		os_printf("Read id: %d, len: %d = %s\n", fobj.n.id, fobj.n.size, buf);
//		faddr += align(fobj.n.size + fobj_head_size);
//	} while(faddr < FMEMORY_SCFG_BASE_ADDR + FMEMORY_SCFG_BANK_SIZE - align(fobj_head_size));
//	os_printf("--------- read cfg END --------\n");
}
