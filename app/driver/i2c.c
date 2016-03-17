/*
 * I2C.c
 *
 * Created: 17.01.2014 14:00:10
 *  Author: Vadim Kulakov, vad7@yahoo.com
 */

//#define IC2_MULTI_MASTER	// If other master(s) available, if omitted - only ONE master on I2C bus

#include "os_type.h"
#include "hw/esp8266.h"
#include "bios.h"
#include "user_interface.h"
#include "driver/i2c.h"

#define I2C_DELAY_US		5 // 4.7us - 100kb, 1.3us - 400kb

#define SET_SDA_LOW			GPIO_OUT_W1TC = (1<<I2C_SDA_PIN)
#define SET_SDA_HI			GPIO_OUT_W1TS = (1<<I2C_SDA_PIN)
#define GET_SDA				(GPIO_IN & (1<<I2C_SDA_PIN))
#define SET_SCL_LOW			GPIO_OUT_W1TC = (1<<I2C_SCL_PIN)
#define SET_SCL_HI			GPIO_OUT_W1TS = (1<<I2C_SCL_PIN)

uint32 i2c_delay_time;
#define GET_CCOUNT(x) __asm__ __volatile__("rsr.ccount %0" : "=r"(x))
void i2c_delay(void)	{ uint32 t1,t2; GET_CCOUNT(t1); do GET_CCOUNT(t2); while(t2-t1 <= i2c_delay_time); }

void ICACHE_FLASH_ATTR i2c_Init(uint32 delay_us)
{
	if(delay_us) { // re-calc delay
		#if DEBUGSOO > 2
			ets_intr_lock();
			i2c_delay_time = 1000;
			uint32 mt = system_get_time();
			i2c_delay();
			mt = system_get_time() - mt;
			i2c_delay_time = 2000;
			uint32 mt2 = system_get_time();
			i2c_delay();
			mt2 = system_get_time() - mt2;
			ets_intr_unlock();
			os_printf("i2c_delay(10): %d us\n", mt); // 13
			os_printf("i2c_delay(20): %d us\n", mt2); // 26
		#endif
		i2c_delay_time = 250; //=143Khz //500; //= 75KHz ets_get_cpu_frequency();
	}
	ets_intr_lock();
	GPIOx_PIN(I2C_SDA_PIN) = GPIO_PIN_DRIVER;
	GPIOx_PIN(I2C_SCL_PIN) = GPIO_PIN_DRIVER;
	SET_PIN_FUNC(I2C_SDA_PIN, (MUX_FUN_IO_PORT(I2C_SDA_PIN) | (1 << GPIO_MUX_PULLUP_BIT)));
	SET_PIN_FUNC(I2C_SCL_PIN, (MUX_FUN_IO_PORT(I2C_SCL_PIN) | (1 << GPIO_MUX_PULLUP_BIT)));
	GPIO_OUT_W1TS = (1<<I2C_SDA_PIN) | (1<<I2C_SCL_PIN); // Set HI
	GPIO_ENABLE_W1TS = (1<<I2C_SDA_PIN) | (1<<I2C_SCL_PIN);
	ets_intr_unlock();
	#if DEBUGSOO > 2
	#endif
#ifndef IC2_MULTI_MASTER
	i2c_delay();
	if(GET_SDA == 0) { // some problem here
		i2c_Stop();
	}
#endif
}

void i2c_Stop(void)
{
	SET_SDA_LOW;
	SET_SCL_LOW;
	i2c_delay();
	SET_SCL_HI; // release
	i2c_delay();
	SET_SDA_HI; // release
	i2c_delay();
}

// return: 1 - if write failed, 0 - ok
uint8_t i2c_WriteBit(uint8_t bit)
{
	//while(I2C_IN & I2C_SCL) ;
	if(bit)
		SET_SDA_HI;
	else
		SET_SDA_LOW;
	i2c_delay();
	SET_SCL_HI;
	i2c_delay();
	#ifdef IC2_MULTI_MASTER
		if(bit && (GET_SDA) == 0) return 1; // other master active
	#endif
	SET_SCL_LOW;
	return 0;
}

uint8_t i2c_ReadBit(void)
{
	SET_SDA_HI;
	i2c_delay();
	SET_SCL_HI;
	i2c_delay();
	uint8_t bit = (GET_SDA) != 0;
	SET_SCL_LOW;
	return bit;
}

// return: ACK, 1 - if write failed, 0 - ok
uint8_t i2c_Write(uint8_t data)
{
	uint8_t i;
	for(i = 0; i < 8; i++)
	{
		#ifdef IC2_MULTI_MASTER
			if(i2c_WriteBit(data & 0x80)) return 2;
		#else
			i2c_WriteBit(data & 0x80);
		#endif
		data <<= 1;
	}
	return i2c_ReadBit();
}

uint8_t i2c_Read(uint8_t ack)
{
	uint8_t i, data = 0;
	for(i = 0; i < 8; i++)
	{
		data = (data << 1) | i2c_ReadBit();
	}
	i2c_WriteBit(ack);
	return data;
}

// Return: 1 - failed, 0 - ok,
uint8_t i2c_Start(uint8_t addr)
{
#ifdef IC2_MULTI_MASTER
	uint8_t i;
	for(i = 1; i != 0; i++)
	{
		// Restart
		SET_SDA_HI;
		i2c_delay();
		SET_SCL_HI;
		i2c_delay();
		if((GET_SDA) == 0) {
			continue; // other master active
		};
		SET_SDA_LOW;
		i2c_delay();
		SET_SCL_LOW;
		i2c_delay();
		if(i2c_Write(addr) == 0) return 0;
		i2c_Stop();
	}
	return 1;
#else
	SET_SDA_HI;
	i2c_delay();
	SET_SCL_HI;
	i2c_delay();
	if(GET_SDA == 0) return 1; // other master active
	SET_SDA_LOW;
	i2c_delay();
	SET_SCL_LOW;
	i2c_delay();
	if(i2c_Write(addr) == 0) return 0;
	i2c_Stop();
	return 1;
#endif
}

// return 0 - ok
uint8_t ICACHE_FLASH_ATTR i2c_eeprom_read_block(uint8_t addr, uint32_t pos, uint8_t *buffer, uint32_t cnt)
{
	addr <<= 1;
	if(i2c_Start(addr + I2C_WRITE) == 0) {
		if (i2c_Write(pos / 256) == 0 && i2c_Write(pos & 255) == 0) {
			if(i2c_Start(addr + I2C_READ) == 0) {
				do {
					cnt--;
					*buffer++ = i2c_Read(cnt ? I2C_ACK : I2C_NOACK);
				} while(cnt);
			}
		}
	}
	i2c_Stop();
	if(cnt) I2C_EEPROM_Error++;
	return cnt;
}

// return 0 - ok
uint8_t ICACHE_FLASH_ATTR i2c_eeprom_write_block(uint8_t addr, uint32_t pos, uint8_t *buffer, uint32_t cnt)
{
	addr <<= 1;
	if(i2c_Start(addr + I2C_WRITE) == 0) {
		if (i2c_Write(pos / 256) == 0 && i2c_Write(pos & 255) == 0) {
			for(; cnt; cnt--) {
				if(i2c_Write(*buffer++)) break;
			}
		}
	}
	i2c_Stop();
	if(cnt) I2C_EEPROM_Error++;
	return cnt;
}

