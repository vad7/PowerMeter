/******************************************************************************
 * PV` FileName: user_main.c
 *******************************************************************************/

#include "user_config.h"
#include "bios.h"
#include "sdk/add_func.h"
#include "hw/esp8266.h"
#include "user_interface.h"
#include "tcp_srv_conn.h"
#include "flash_eep.h"
#include "wifi.h"
#include "hw/spi_register.h"
#include "sdk/rom2ram.h"
#include "web_iohw.h"
#include "tcp2uart.h"
#include "webfs.h"
#include "sdk/libmain.h"
#include "driver/i2c_eeprom.h"

void power_meter_init(uint8 index) ICACHE_FLASH_ATTR;
void uart_wait_tx_fifo_empty(void) ICACHE_FLASH_ATTR;

#ifdef USE_WEB
#include "web_srv.h"
#endif

#ifdef UDP_TEST_PORT
#include "udp_test_port.h"
#endif

#ifdef USE_NETBIOS
#include "netbios.h"
#endif

#ifdef USE_SNTP
#include "sntp.h"
#endif

#ifdef USE_WDRV
#include "driver/wdrv.h"
#endif

//#ifdef USE_MODBUS
//#include "modbustcp.h"
#ifdef USE_RS485DRV
#include "driver/rs485drv.h"
#include "mdbtab.h"
#endif
//#endif

#ifdef USE_WEB
extern void web_fini(const uint8 * fname);
static const uint8 sysinifname[] ICACHE_RODATA_ATTR = "protect/init.ini";
#endif

extern volatile uint32 PowerCnt;

uint32 PowerCntLast = 0;
uint8 user_idle_flag = 0;

void ICACHE_FLASH_ATTR user_idle(void) // idle function for ets_run_new
{
	uint16 i;
	if(PowerCnt > PowerCntLast) {

		#define eblen 1024
		uint32 mt = 0;
		uint8 *buf = os_malloc(eblen);
		if(buf == NULL) {
			os_printf("Error malloc some bytes!\n");
		} else {
			os_printf("Test 32KB FRAM - %x\n", user_idle_flag);
			ets_intr_lock();
			//ets_isr_mask(0xFFFFFFFF);
			if(user_idle_flag == 0) {
				WDT_FEED = WDT_FEED_MAGIC; // WDT
				uart_wait_tx_fifo_empty();
				i2c_init();

				os_memset(buf, 0, eblen);
				mt = system_get_time();
				for(i = 0; i < 32; i++) {
					if(i2c_eeprom_read_block(I2C_FRAM_ID, i * eblen, buf, eblen) == 0) {
						os_printf("Error read block: %d\n", i);
						break;
					}
					WDT_FEED = WDT_FEED_MAGIC; // WDT
				}
				mt = system_get_time() - mt;
				os_printf("Reading time: %d us\n", mt);

				for(i = 0; i < eblen; i++) {
					os_printf("%x ", buf[i]);
				}
				os_printf("\n");
				WDT_FEED = WDT_FEED_MAGIC; // WDT
			} else if(user_idle_flag == 1) {
				spi_flash_read(0x1000, buf, eblen);
				mt = system_get_time(); //get_mac_time();
				for(i = 0; i < 32; i++) {
					WDT_FEED = WDT_FEED_MAGIC; // WDT
					if(i2c_eeprom_write_block(I2C_FRAM_ID, i * eblen, buf, eblen) == 0) {
						os_printf("Error write block: %d\n", i);
						break;
					}
				}
				mt = system_get_time() - mt; //get_mac_time();
				os_printf("Write time: %d us\n", mt);
				WDT_FEED = WDT_FEED_MAGIC; // WDT
			} else {
				uint8 *buf2 = os_malloc(eblen);
				if(buf2 == NULL) {
					os_printf("Error malloc some bytes! 2\n");
				} else {
					spi_flash_read(0x1000, buf2, eblen);
					//ets_intr_lock();
					mt = system_get_time();
					uint8 eq = 0;
					for(i = 0; i < 32; i++) {
						WDT_FEED = WDT_FEED_MAGIC; // WDT
						if(i2c_eeprom_read_block(I2C_FRAM_ID, i * eblen, buf2, eblen) == 0) {
							os_printf("Error read block: %d\n", i);
							break;
						}
						eq = os_memcmp(buf, buf2, eblen) == 0;
					}
					mt = system_get_time() - mt;
					os_printf("Compare time: %d us - %s\n", mt, eq ? "ok!" : "not equal!");
					os_free(buf2);
				}
			}
			ets_intr_unlock();
			//ets_isr_unmask(0xFFFFFFFF);
			//os_memset(buf, 0, eblen);
			os_free(buf);
		}
		if(++user_idle_flag > 2) user_idle_flag = 0;

		PowerCntLast = PowerCnt;
	}
}

void ICACHE_FLASH_ATTR init_done_cb(void)
{
#if DEBUGSOO > 0
	os_printf("\nSDK Init - Ok\nCurrent 'heap' size: %d bytes\n", system_get_free_heap_size());
    os_printf("Current config size: %d bytes\n", current_cfg_length());
	struct ets_store_wifi_hdr whd;
	spi_flash_read(((flashchip->chip_size/flashchip->sector_size)-1)*flashchip->sector_size, &whd, sizeof(whd));
	os_printf("Last sectors rewrite count: %u\n\n", whd.wr_cnt);

	os_printf("PowerCnt = %d\n", PowerCnt);
#endif
	//
	power_meter_init(3); // init timer/tasks
	ets_set_idle_cb(user_idle, NULL);
	//
#ifdef USE_WEB
	web_fini(sysinifname);
#endif
	switch(system_get_rst_info()->reason) {
	case REASON_SOFT_RESTART:
	case REASON_DEEP_SLEEP_AWAKE:
		break;
	default:
		New_WiFi_config(WIFI_MASK_ALL);
		break;
	}
#ifdef USE_RS485DRV
	rs485_drv_start();
	init_mdbtab();
#endif
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR user_init(void) {
	sys_read_cfg();
	if(!syscfg.cfg.b.debug_print_enable) system_set_os_print(0);
//	GPIO0_MUX = VAL_MUX_GPIO0_SDK_DEF;
//	GPIO4_MUX = VAL_MUX_GPIO4_SDK_DEF;
//	GPIO5_MUX = VAL_MUX_GPIO5_SDK_DEF;
//	GPIO12_MUX = VAL_MUX_GPIO12_SDK_DEF;
//	GPIO13_MUX = VAL_MUX_GPIO13_SDK_DEF;
//	GPIO14_MUX = VAL_MUX_GPIO14_SDK_DEF;
//	GPIO15_MUX = VAL_MUX_GPIO15_SDK_DEF;
	// vad7
	//power_meter_init();
	//
	uart_init();
	system_timer_reinit();
#if (DEBUGSOO > 0 && defined(USE_WEB))
	os_printf("\nSimple WEB version: " WEB_SVERSION "\nOpenLoaderSDK v1.2\n");
#endif
	//if(syscfg.cfg.b.pin_clear_cfg_enable) test_pin_clr_wifi_config(); // сброс настроек, если замкнут пин RX
	set_cpu_clk(); // select cpu frequency 80 or 160 MHz
#ifdef USE_GDBSTUB
extern void gdbstub_init(void);
	gdbstub_init();
#endif
#if DEBUGSOO > 0
	if(eraminfo.size > 0) os_printf("Found free IRAM: base: %p, size: %d bytes\n", eraminfo.base,  eraminfo.size);
	os_printf("System memory:\n");
    system_print_meminfo();
    os_printf("Start 'heap' size: %d bytes\n", system_get_free_heap_size());
#endif
#if DEBUGSOO > 0
	os_printf("Set CPU CLK: %u MHz\n", ets_get_cpu_frequency());
#endif
	Setup_WiFi();
#ifdef USE_WDRV
    init_wdrv();
#endif

    WEBFSInit(); // файловая система

	system_deep_sleep_set_option(0);
	system_init_done_cb(init_done_cb);
}
