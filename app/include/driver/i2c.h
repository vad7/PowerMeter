/*
    I2C driver for the ESP8266 
    Copyright (C) 2014 Rudy Hardeman (zarya) 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __I2C_H__
#define __I2C_H__

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"

#define I2C_SLEEP_TIME 3 //3 // 3 = 333KHz // 10 = 100KHz

// SDA on GPIO2
#define I2C_SDA_PIN 2

// SCK on GPIO0
#define I2C_SCL_PIN 0

uint32	I2C_EEPROM_Error;

#define I2C_WRITE			0
#define I2C_READ			1
#define I2C_NOACK			1
#define I2C_ACK				0

void 	i2c_Init(uint32 delay_us) ICACHE_FLASH_ATTR;
uint8_t i2c_Start(uint8_t addr);
void 	i2c_Stop(void);
uint8_t i2c_WriteBit(uint8_t bit);
uint8_t i2c_ReadBit(void);
uint8_t i2c_Write(uint8_t data);
uint8_t i2c_Read(uint8_t ack);
uint8_t i2c_eeprom_read_block(uint8_t addr, uint32_t pos, uint8_t *buffer, uint32_t cnt) ICACHE_FLASH_ATTR;
uint8_t i2c_eeprom_write_block(uint8_t addr, uint32_t pos, uint8_t *buffer, uint32_t cnt) ICACHE_FLASH_ATTR;

#endif
