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
#include "hw/gpio_register.h"

#define GPIO_Tasks 		1
#define GPIO_Int_Signal 1
ETSEvent GPIO_TaskQueue[GPIO_Tasks] DATA_IRAM_ATTR;

uint32 PowerCnt = 0;
uint32 PowerCntTime = 0;
uint8  FRAM_Status = 1; 	// 0 - Ok, 1 - Not initialized, 2 - Error
uint8  Sensor_Edge; 		// 0 - Front pulse edge, 1 - Back

uint8	FRAM_STORE_Readed	= 0;

typedef struct __attribute__((packed)) {
	uint8 Cnt1; // if = 0, Cnt2 = minutes of zero; otherwise pulses in minute (max = 255)
	uint8 Cnt2;
	uint8 Cnt3;
	uint8 Cnt4;
} CNT_CURRENT;
CNT_CURRENT CntCurrent = {0, 0, 0, 0};

void NextPtrCurrent(uint8 cnt)
{
	fram_store.PtrCurrent += cnt;
	if(fram_store.PtrCurrent >= cfg_meter.Fram_Size - StartArrayOfCnts)
		fram_store.PtrCurrent = fram_store.PtrCurrent - cfg_meter.Fram_Size - StartArrayOfCnts;
}

void FRAM_Store_Init(void);

void ICACHE_FLASH_ATTR update_cnts(time_t time) // 1 minute passed
{
	if(FRAM_Status) {
		#if DEBUGSOO > 2
			os_printf("FRAM Reinit\n");
		#endif
		FRAM_Store_Init();
		//i2c_init();
		if(FRAM_Status) return;
	}
	ets_intr_lock();
	uint32 pcnt = fram_store.PowerCnt;
	if(pcnt > 1) {
		time_t tt = time - fram_store.LastTime;
		if(tt >= 60 * 2) pcnt /= tt / 60;
		if(pcnt > 255) pcnt = 255;
	}
	fram_store.PowerCnt -= pcnt;
	ets_intr_unlock();
	#if DEBUGSOO > 2
		os_printf("NPCur: %u\n", pcnt);
	#endif
	uint8 to_save = 0; // bytes
	uint8 to_add = 0;  // bytes after write
	if(pcnt == 0) { // new zero
		if(CntCurrent.Cnt2 == 255) { // overflow
			// next packed
			NextPtrCurrent(2);
			*(uint32 *)&CntCurrent = 0;
			CntCurrent.Cnt2 = 1;
			to_save = 2;
		} else {
			CntCurrent.Cnt2++;
			if(i2c_eeprom_write_block(I2C_FRAM_ID, StartArrayOfCnts + fram_store.PtrCurrent + 1, &CntCurrent.Cnt2, 1) == 0) {
				#if DEBUGSOO > 2
			   		os_printf("EW Curr0\n");
				#endif
		   		FRAM_Status = 2;
			   	return;
			}
		}
	} else {
		if(CntCurrent.Cnt2) { // packed exist - next +2
			if(CntCurrent.Cnt2 == 1) { // if 0,1 : next +2
				*(uint32 *)&CntCurrent = 0;
				CntCurrent.Cnt1 = 1;
				CntCurrent.Cnt2 = pcnt;
				to_save = 4;
				to_add = 2;
			} else {
				NextPtrCurrent(2);
				*(uint32 *)&CntCurrent = 0;
				CntCurrent.Cnt1 = pcnt;
				to_save = 3;
				to_add = 1;
			}
		} else {
			CntCurrent.Cnt1 = pcnt;
			to_save = 3;
			to_add = 1;
		}
	}
	if(to_save) {
		uint32 cnt = cfg_meter.Fram_Size - (StartArrayOfCnts + fram_store.PtrCurrent);
		if(cnt > to_save) cnt = to_save;
		if(!i2c_eeprom_write_block(I2C_FRAM_ID, StartArrayOfCnts + fram_store.PtrCurrent, (uint8 *)&CntCurrent, cnt)) {
			#if DEBUGSOO > 2
		   		os_printf("EW curr\n");
			#endif
	   		FRAM_Status = 2;
		   	return;
		}
		if(cnt < to_save) { // overflow
			if(!i2c_eeprom_write_block(I2C_FRAM_ID, StartArrayOfCnts, ((uint8 *)&CntCurrent) + cnt, to_save - cnt)) {
				#if DEBUGSOO > 2
			   		os_printf("EW curr2\n");
				#endif
			   	FRAM_Status = 2;
			   	return;
			}
		}
	}
	NextPtrCurrent(to_add);
	fram_store.TotalCnt += pcnt;
	fram_store.LastTime += 60;  // seconds
	if(!i2c_eeprom_write_block(I2C_FRAM_ID, 0, (uint8 *)&fram_store, sizeof(fram_store))) {
		#if DEBUGSOO > 2
	   		os_printf("EW f_s\n");
		#endif
   		FRAM_Status = 2;
	   	return;
	}
}

#if DEBUGSOO > 2
uint8 print_i2c_page = 0;
#endif

void ICACHE_FLASH_ATTR user_idle(void) // idle function for ets_run
{
	//FRAM_speed_test();
	time_t time = get_sntp_time();
	if(time && (time - fram_store.LastTime >= 60)) { // Passed 1 min
		ets_set_idle_cb(NULL, NULL);
		ets_intr_unlock();
		if(fram_store.LastTime == 0) { // first time run
			fram_store.LastTime = time;
		} else {
			update_cnts(time);
		}
		ets_set_idle_cb(user_idle, NULL);
	}
#if DEBUGSOO > 2
	else if(print_i2c_page) { // 1..n
		ets_set_idle_cb(NULL, NULL);
		ets_intr_unlock();
		#define READSIZE 256
		uint8 *buf = os_malloc(READSIZE);
		if(buf == NULL) {
			os_printf("Error malloc %d bytes", READSIZE);
		} else {
			uint16 pos = (print_i2c_page-1) * READSIZE;
			os_printf("Read fram: %6d", pos);
			uint32 mt = system_get_time();
			if(i2c_eeprom_read_block(I2C_FRAM_ID, pos, buf, READSIZE) == 0) {
				os_printf(" - Error!");
				goto x1;
			}
			mt = system_get_time() - mt;
			os_printf(", time: %6d us: ", mt);
			for(pos = 0; pos < READSIZE; pos++) {
				os_printf("%02x ", buf[pos]);
			}
x1:			os_printf("\n");
			if(++print_i2c_page > cfg_meter.Fram_Size / READSIZE) print_i2c_page = 0;
		}
		ets_set_idle_cb(user_idle, NULL);
	}
#endif

}

static void ICACHE_FLASH_ATTR GPIO_Task_NewData(os_event_t *e)
{
    switch(e->sig) {
    	case GPIO_Int_Signal:
    		if(e->par == SENSOR_PIN) { // new data
#if DEBUGSOO > 2
   				os_printf("*T: %u, n=%d\n", PowerCntTime, PowerCnt);
#endif
   				// update current PowerCnt
   				ets_intr_lock();
   				fram_store.PowerCnt += PowerCnt;
   				PowerCnt = 0;
   				ets_intr_unlock();
   				if(i2c_eeprom_write_block(I2C_FRAM_ID, (uint8 *)&fram_store.PowerCnt - (uint8 *)&fram_store, (uint8 *)&fram_store.PowerCnt, sizeof(fram_store.PowerCnt)) == 0) {
#if DEBUGSOO > 2
   					os_printf("EW PrCnt\n");
#endif
   					FRAM_Status = 2;
   				}
    		}
    		break;
    }
}

static void gpio_int_handler(void)
{
	uint32 gpio_status = GPIO_STATUS;
	GPIO_STATUS_W1TC = gpio_status;
	if(gpio_status & (1<<SENSOR_PIN)) {
		uint32 tm = system_get_time();
		if(tm - PowerCntTime > 20000) { // skip if interval less than 20ms
			PowerCntTime = tm;
			if(!Sensor_Edge) { // Front edge
				PowerCnt++;
				system_os_post(SENSOR_TASK_PRIO, GPIO_Int_Signal, SENSOR_PIN);
			}
		    Sensor_Edge ^= 1; // next edge
		}
	    gpio_pin_intr_state_set(SENSOR_PIN, Sensor_Edge ? SENSOR_BACK_EDGE : SENSOR_FRONT_EDGE);
	}
}

void FRAM_Store_Init(void)
{
	if(flash_read_cfg(&cfg_meter, ID_CFG_METER, sizeof(cfg_meter)) != sizeof(cfg_meter)) {
		// defaults
		cfg_meter.Fram_Size = FRAM_SIZE_DEFAULT;
		cfg_meter.PulsesPer0_01KWt = DEFAULT_PULSES_PER_0_01_KWT;
	}
	i2c_init();
	// restore workspace from FRAM
	if(!i2c_eeprom_read_block(I2C_FRAM_ID, 0, (uint8 *)&fram_store, sizeof(fram_store))) {
		#if DEBUGSOO > 2
			os_printf("ER f_s\n");
		#endif
		return;
	}
	if(fram_store.LastTime == 0xFFFFFFFF) { // new memory
		os_memset(&fram_store, 0, sizeof(fram_store));
		if(!i2c_eeprom_write_block(I2C_FRAM_ID, 0, (uint8 *)&fram_store, sizeof(fram_store))) {
			#if DEBUGSOO > 2
				os_printf("EW init f_s\n");
			#endif
			return;
		}
	} else if(fram_store.LastTime) { // LastTime must be filled
		uint8 cnt = cfg_meter.Fram_Size - (StartArrayOfCnts + fram_store.PtrCurrent);
		if(cnt > 2) cnt = 2;
		if(!i2c_eeprom_read_block(I2C_FRAM_ID, StartArrayOfCnts + fram_store.PtrCurrent, (uint8 *)&CntCurrent, cnt)) {
			#if DEBUGSOO > 2
				os_printf("ER curr\n");
			#endif
			return;
		} else if(cnt < 2) {
			if(!i2c_eeprom_read_block(I2C_FRAM_ID, StartArrayOfCnts, (uint8 *)&CntCurrent.Cnt2, 1)) {
				#if DEBUGSOO > 2
					os_printf("ER curr2\n");
				#endif
				return;
			}
		}
	}
	FRAM_Status = 0;
	#if DEBUGSOO > 4
		os_printf("FSize=%u, PCnt= %u, TCnt= %u, Ptr= %u, LTime= %u\n", cfg_meter.Fram_Size, fram_store.PowerCnt, fram_store.TotalCnt, fram_store.PtrCurrent, fram_store.LastTime);
		uint8 *buf = os_malloc(20);
		if(buf != NULL) {
			uint8 cnt = fram_store.PtrCurrent < 20 ? fram_store.PtrCurrent : 20;
			if(fram_store.PtrCurrent) {
				if(!i2c_eeprom_read_block(I2C_FRAM_ID, StartArrayOfCnts + fram_store.PtrCurrent - cnt, buf, cnt)) {
					os_printf("ER 20\n");
				} else {
					uint8 i;
					for(i = 0; i < cnt; i++) {
						os_printf("%02x ", buf[i]);
					}
					os_printf("\n");
				}
			}
		}
		os_printf("Cnt1= %u, Cnt2= %u\n", CntCurrent.Cnt1, CntCurrent.Cnt2);
	#endif
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
		gpio_pin_intr_state_set(SENSOR_PIN, SENSOR_FRONT_EDGE);
		Sensor_Edge = 0; // Front
		GPIO_STATUS_W1TC = pins_mask; // разрешить прерывания GPIOs
	}
	if(index & 2) {
		system_os_task(GPIO_Task_NewData, SENSOR_TASK_PRIO, GPIO_TaskQueue, GPIO_Tasks);
	}
	ets_isr_unmask(1 << ETS_GPIO_INUM);
	if(index & 1) {
		FRAM_Store_Init();
	}
#if DEBUGSOO > 2
	os_printf("PCnt = %d\n", PowerCnt);
	uint8 p3 = GPIO_INPUT_GET(GPIO_ID_PIN(3));
	os_printf("Systime: %d, io3=%x\n", system_get_time(), p3);
	if(p3 == 0) {
		uart_wait_tx_fifo_empty();
		#define blocklen 256
		uint8 *buf = os_malloc(blocklen);
		if(buf == NULL) {
			os_printf("Err malloc!\n");
		} else {
			wifi_set_opmode_current(WIFI_DISABLED);
			ets_isr_mask(0xFFFFFFFF); // mask all interrupts
			//i2c_init();
			os_memset(buf, 0, blocklen);
			WDT_FEED = WDT_FEED_MAGIC; // WDT
			uint16 i, j;
			for(i = 0; i < cfg_meter.Fram_Size / blocklen; i++) {
//				if(i2c_eeprom_write_block(I2C_FRAM_ID, i * blocklen, buf, blocklen) == 0) {
//					os_printf("EW Bl: %d\n", i);
//					break;
//				}
//				WDT_FEED = WDT_FEED_MAGIC; // WDT
//				break;
//				continue;
				uint32 mt = system_get_time();
				if(i2c_eeprom_read_block(I2C_FRAM_ID, i * blocklen, buf, blocklen) == 0) {
					os_printf("ER Bl: %d\n", i);
					break;
				}
				mt = system_get_time() - mt;
				WDT_FEED = WDT_FEED_MAGIC; // WDT
				os_printf("%3d: ", i);
				for(j = 0; j < blocklen; j++) {
					os_printf("%02x ", buf[j]);
					WDT_FEED = WDT_FEED_MAGIC; // WDT
				}
				os_printf(" = time: %d\n", mt);
			}
			os_printf("\nHalt\n");
			while(1) WDT_FEED = WDT_FEED_MAGIC; // halt
		}
	}
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
