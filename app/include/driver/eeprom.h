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
#define eeprom_read_block(pos, buffer, cnt)  i2c_eeprom_read_block(I2C_ID, pos, buffer, cnt)
#define eeprom_write_block(pos, buffer, cnt) i2c_eeprom_read_block(I2C_ID, pos, buffer, cnt)
#define eeprom_init(freq) i2c_Init(freq)
#else
#ifdef USE_HSPI

#include "spi.h"
#define eeprom_read_block(pos, buffer, cnt)  i2c_eeprom_read_block(I2C_FRAM_ID, pos, buffer, cnt)
#define eeprom_write_block(pos, buffer, cnt) i2c_eeprom_read_block(I2C_FRAM_ID, pos, buffer, cnt)

#endif
#endif

#endif
