#include "user_config.h"
#include "bios.h"
#include "sdk/add_func.h"
#include "hw/esp8266.h"
#include "user_interface.h"
#include "web_iohw.h"
#include "flash_eep.h"
#include "webfs.h"
#include "sdk/libmain.h"
#include "power_meter.h"
#include "driver/i2c_eeprom.h"

#define GPIO_Tasks 		1
#define GPIO_Int_Signal 1
ETSEvent GPIO_TaskQueue[GPIO_Tasks] DATA_IRAM_ATTR;

uint32 PowerCnt = 0;
uint32 PowerCntTime = 0;

typedef struct __attribute__((packed)) {
	uint32 PowerCnt;
	uint32 TotalCnt;
	time_t LastTime;
	uint32 PtrLastTime;
	uint32 PtrCurrent;
	//uint8 Reserved[12];
} FRAM_STORE;
FRAM_STORE fram_store;
#define ArrayOfCnts			32 // Start pos, packed: if [cell] = 0 and [cell+1] > 1, then [cell+1] = How much minutes was 0.
#define	FRAM_SIZE_DEFAULT	= 32768;
uint8	FRAM_STORE_Readed	= 0;

typedef struct __attribute__((packed)) {
	uint32 Fram_Size;
//	char sntp_server[20];
} CFG_METER;
CFG_METER cfg_meter;
typedef struct __attribute__((packed)) {
	uint8 Cnt1_1;
	uint8 Cnt1_2;
	uint8 Cnt2_1;
	uint8 Cnt2_2;
} CNT_CURRENT;

void ICACHE_FLASH_ATTR update_cnts(void) // 1 minute passed
{
	CNT_CURRENT CntCurrent;
	uint8 CntLastTime;
	uint32 pcnt, CntCurrentPos2;
	//CntLastTime = i2c_eeprom_read_byte(I2C_FRAM_ID, ArrayOfCnts + fram_store.PtrLastTime);
	//if(I2C_EEPROM_Error) goto xEpromReadError;
	if(i2c_eeprom_read_block(I2C_FRAM_ID, ArrayOfCnts + fram_store.PtrCurrent, &CntCurrent, 2)) {
		#if DEBUGSOO > 2
			os_printf("Error read FRAM at: %u\n", 0);
		#endif
		return;
	}
	ets_intr_lock();
	pcnt = fram_store.PowerCnt;
	fram_store.PowerCnt = 0;
	ets_intr_unlock();
	if(pcnt == 0) { // zero
		CntCurrent.Cnt1_2++;
	} else {


	}
	if(CntCurrent.Cnt1 == 0) { // may be packed
	} else {



	}
	CntCurrentPos2 = fram_store.PtrCurrent + 1;
	if(CntCurrentPos2 + 4 > cfg_meter.Fram_Size - ArrayOfCnts) CntCurrentPos2 = 0;







}

void ICACHE_FLASH_ATTR user_idle(void) // idle function for ets_run
{
	//FRAM_speed_test();
	time_t time = get_sntp_time();
	if(time && time - fram_store.LastTime >= 60) {
		ets_set_idle_cb(NULL, NULL);
		update_cnts();
		ets_set_idle_cb(user_idle, NULL);
	}
}

static void ICACHE_FLASH_ATTR GPIO_Task_NewData(os_event_t *e)
{
    switch(e->sig) {
    	case GPIO_Int_Signal:
    		if(e->par == SENSOR_PIN) { // new data
#if DEBUGSOO > 2
   				os_printf("* INT GPIO%d, PowerCnt=%d\n", SENSOR_PIN, PowerCnt);
#endif
   				// update current PowerCnt
   				ets_intr_lock();
   				fram_store.PowerCnt += PowerCnt;
   				PowerCnt = 0;
   				ets_intr_unlock();
   				if(i2c_eeprom_write_block(I2C_FRAM_ID, &fram_store.PowerCnt - &fram_store, &fram_store.PowerCnt, sizeof(fram_store.PowerCnt)) == 0) {
#if DEBUGSOO > 2
   					os_printf("Error write PowerCnt\n");
#endif
   				}
    		}
    		break;
    }
}

void ICACHE_FLASH_ATTR FRAM_speed_test(void)
{
	#define eblen 1024
	#define Test_KB 32

	uint8 *buf = os_malloc(eblen);
	if(buf == NULL) {
		os_printf("Error malloc some bytes!\n");
	} else {
		os_printf("Test %dKB FRAM\n", Test_KB);
		uart_wait_tx_fifo_empty();
		wifi_set_opmode_current(WIFI_DISABLED);
		ets_isr_mask(0xFFFFFFFF); // mask all interrupts
		i2c_init();
		//os_memset(buf, 0, eblen);
		WDT_FEED = WDT_FEED_MAGIC; // WDT
		uint16 i;
		uint32 mt = system_get_time();
		for(i = 0; i < Test_KB; i++) {
			if(i2c_eeprom_read_block(I2C_FRAM_ID, i * eblen, buf, eblen) == 0) {
				os_printf("Error read block: %d\n", i);
				break;
			}
			WDT_FEED = WDT_FEED_MAGIC; // WDT
		}
		mt = system_get_time() - mt;
		os_printf("Reading time: %d us\n", mt);
//		for(i = 0; i < eblen; i++) {
//			os_printf("%x ", buf[i]);
//		}
//		os_printf("\n");
		mt = system_get_time(); //get_mac_time();
		uint32 pos = 0x2000 + (mt & 0xFFF);
		spi_flash_read(pos, buf, eblen);
		for(i = 0; i < Test_KB; i++) {
			if(i2c_eeprom_write_block(I2C_FRAM_ID, i * eblen, buf, eblen) == 0) {
				os_printf("Error write block: %d\n", i);
				break;
			}
			WDT_FEED = WDT_FEED_MAGIC; // WDT
		}
		mt = system_get_time() - mt; //get_mac_time();
		os_printf("Write time: %d us\n", mt);
		uint8 *buf2 = os_malloc(eblen);
		if(buf2 == NULL) {
			os_printf("Error malloc some bytes! 2\n");
		} else {
			mt = system_get_time();
			uint8 eq = 0;
			for(i = 0; i < Test_KB; i++) {
				if(i2c_eeprom_read_block(I2C_FRAM_ID, i * eblen, buf2, eblen) == 0) {
					os_printf("Error read block: %d\n", i);
					break;
				}
				WDT_FEED = WDT_FEED_MAGIC; // WDT
				eq = os_memcmp(buf, buf2, eblen) == 0;
				if(!eq) break;
			}
			mt = system_get_time() - mt;
			os_printf("Compare: %d us - %s\n", mt, eq ? "ok!" : "not equal!");
			uart_wait_tx_fifo_empty();
			os_free(buf2);
		}
		os_free(buf);
		while(1) WDT_FEED = WDT_FEED_MAGIC; // WDT
	}
}

static void gpio_int_handler(void)
{
	uint32 gpio_status = GPIO_STATUS;
	GPIO_STATUS_W1TC = gpio_status;
	if(gpio_status & (1<<SENSOR_PIN)) {
		uint32 tm = system_get_time();
		if(tm - PowerCntTime > 1000) { // skip if interval less than 1ms
			PowerCnt++;
			PowerCntTime = tm;
			system_os_post(USER_TASK_PRIO_1, GPIO_Int_Signal, SENSOR_PIN);
		}
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
	if(index & 1) {
		if(flash_read_cfg(CFG_METER, ID_CFG_METER, sizeof(CFG_METER)) != sizeof(CFG_METER)) {
			// defaults
			cfg_meter.Fram_Size = FRAM_SIZE_DEFAULT;
		}
		i2c_init();
		if(i2c_eeprom_read_block(I2C_FRAM_ID, 0, &fram_store, sizeof(fram_store)) == 0) {
#if DEBUGSOO > 2
			os_printf("Error read FRAM at: %u\n", 0);
#endif
		}
	}
#if DEBUGSOO > 0
	os_printf("PowerCnt = %d\n", PowerCnt);

	os_printf("time_t size: %u", sizeof(time_t));

#endif

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
