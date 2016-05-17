/*
 * eeprom read/write
 *
 * Created: 12.05.2016
 * Written by Vadim Kulakov, vad7 @ yahoo.com
 *
 */

#ifndef __EEPROM_H__
#define __EEPROM_H__

#ifdef USE_I2C

#include "i2c.h"
#define eeprom_read_block(addr, buffer, len)  i2c_eeprom_read_block(I2C_ID, addr, buffer, len)
#define eeprom_write_block(addr, buffer, len) i2c_eeprom_read_block(I2C_ID, addr, buffer, len)
#else
#ifdef USE_HSPI

#include "spi.h"
#define eeprom_read_block(addr, buffer, len)  spi_write_read_block(SPI_RECEIVE, addr, buffer, len)
#define eeprom_write_block(addr, buffer, len) spi_write_read_block(SPI_SEND, addr, buffer, len)

#endif
#endif

#endif
