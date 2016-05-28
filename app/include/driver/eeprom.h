/*
 * eeprom read/write
 *
 * Created: 12.05.2016
 * Written by Vadim Kulakov, vad7 @ yahoo.com
 *
 */
#include "user_config.h"

#ifndef __EEPROM_H__
#define __EEPROM_H__

#ifdef USE_I2C

#include "i2c.h"
#define eeprom_read_block(addr, buffer, len)  i2c_eeprom_read_block(I2C_ID, addr, buffer, len)
#define eeprom_write_block(addr, buffer, len) i2c_eeprom_write_block(I2C_ID, addr, buffer, len)
#else
#ifdef USE_HSPI

#include "spi.h"

#define EEPROM_ADDR_BITS (SPI_ADDR_BITS - 8)
#define EEPROM_WREN 	0b00000110 // Set write enable latch
#define EEPROM_READ 	0b00000011
#define EEPROM_WRITE	0b00000010
#define EEPROM_SLEEP	0b10111001
#define EEPROM_RDID		0b10011111 // Read device ID

uint8_t eeprom_read_block(uint32_t addr, uint8_t *buffer, uint32_t len);
uint8_t eeprom_write_block(uint32_t addr, uint8_t *buffer, uint32_t len);
#endif
#endif

#endif
