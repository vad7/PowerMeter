/*
    Driver for the serial EEPROM from ATMEL
    Base on https://github.com/husio-org/AT24C512C and https://code.google.com/p/arduino-at24c1024/
    This driver depends on the I2C driver https://github.com/zarya/esp8266_i2c_driver/
    Some modifications...

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

#ifndef __I2C_EEPROM_H__
#define __I2C_EEPROM_H__

#include "ets_sys.h"
#include "osapi.h"
#include "i2c.h"

#define WORD_MASK 0xFFFF
uint32	I2C_EEPROM_Error;

uint8 i2c_eeprom_read_byte(uint8 address, uint32_t location) ICACHE_FLASH_ATTR; // RETURN 0 if an error!
uint8 i2c_eeprom_read_block(uint8 address, uint32_t location, uint8 *data, uint32_t len) ICACHE_FLASH_ATTR;
uint8 i2c_eeprom_write_byte(uint8 address, uint32_t location, uint8 data) ICACHE_FLASH_ATTR;
uint8 i2c_eeprom_write_block(uint8 address, uint32_t location, uint8 *data, uint32_t len) ICACHE_FLASH_ATTR;

#endif
