/*
 * Mercury power meter driver (http://www.incotexcom.ru/counters.htm)
 *
 * Use IrDa interface (UART0) at 9600b
 *
 * Written by vad7
 */
#include "user_config.h"
#ifdef USE_MERCURY
#include "mercury.h"

uint8		pwmt_buffer[20];
uint16 		crc_tab16[256];
#define		CRC_START_MODBUS	0xFFFF
#define		CRC_POLY_16			0xA001

uint16_t ICACHE_FLASH_ATTR crc_modbus(uint8 *input_str, size_t num_bytes ) {
	uint16_t crc, tmp, short_c;
	uint8 *ptr;
	size_t a;
	crc = CRC_START_MODBUS;
	ptr = input_str;
	if ( ptr != NULL ) for (a=0; a<num_bytes; a++) {
		short_c = 0x00ff & (uint16_t) *ptr;
		tmp     =  crc       ^ short_c;
		crc     = (crc >> 8) ^ crc_tab16[ tmp & 0xff ];
		ptr++;
	}
	return crc;
}

void ICACHE_FLASH_ATTR  init_crc16_tab( void ) {
	uint16_t i, j, crc, c;
	for (i=0; i<256; i++) {
		crc = 0;
		c   = i;
		for (j=0; j<8; j++) {
			if ( (crc ^ c) & 0x0001 ) crc = ( crc >> 1 ) ^ CRC_POLY_16;
			else                      crc =   crc >> 1;
			c = c >> 1;
		}
		crc_tab16[i] = crc;
	}
	crc_tab16_init = true;
}

void ICACHE_FLASH_ATTR uart_recvTask(os_event_t *events)
{
    if(events->sig == 0){
		#if DEBUGSOO > 4
    		os_printf("%s\n", UART_Buffer);
		#endif
    	if(UART_Buffer_idx >= MIN_Reponse_length) {
    		UART_Buffer[UART_Buffer_size-1] = 0;
    		uint8 *p = web_strnstr(UART_Buffer, AZ_7798_ResponseEnd, UART_Buffer_idx);
    		if(p == NULL) return;
   			*p = 0;
   			if((p = os_strchr(UART_Buffer, AZ_7798_TempStart)) == NULL) return;
   			p++;
			uint8 *p2 = os_strchr(p, AZ_7798_TempEnd);
			if(p2 == NULL) return;
			*p2 = 0;
			Temperature = atoi_z(p, 1);
			p = p2 + 3;
			if((p2 = os_strchr(p, AZ_7798_CO2End)) != NULL) {
				*p2 = 0;
				CO2level = atoi_z(p, 1);
				p = p2 + 5;
				Humidity = atoi_z(p, 1);
				ProcessIncomingValues();
				CO2_work_flag = 1;
				receive_timeout = 0;
			}
    	}
    }
}

void irda_init(void)
{
	uarts_init();
	pwmt_connect_status = 0;
}

void pwmt_read_current(void)
{
	if(pwmt_connect_status == 0) {
		pwmt_connect();
	}

}

void pwmt_connect(void)
{


}

#endif
