/*
    Driver for GPIO

    Copyright (C) 2015 Mikhail Grigorev (CHERTS)
	Modified vad7

    Pin number = GPIOx:

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

#ifndef __GPIO16_H__
#define __GPIO16_H__

#include "gpio.h"

#define GPIO_PIN_NUM 17
#define GPIO_INTERRUPT_ENABLE 1

#define GPIO_FLOAT 0
#define GPIO_PULLUP 1
#define GPIO_PULLDOWN 2

#define GPIO_INPUT 0
#define GPIO_OUTPUT 1
#define GPIO_INT 2

/* GPIO interrupt handler */
#ifdef GPIO_INTERRUPT_ENABLE
typedef void (* gpio_intr_handler)(unsigned pin, unsigned level);
#endif

void gpio16_output_conf(void);
void gpio16_output_set(uint8 value);
void gpio16_input_conf(void);
uint8 gpio16_input_get(void);
int set_gpio_mode(unsigned pin, unsigned mode, unsigned pull);
int gpio_write(unsigned pin, unsigned level);
int gpio_read(unsigned pin);
#ifdef GPIO_INTERRUPT_ENABLE
void gpio_intr_attach(gpio_intr_handler cb);
int gpio_intr_deattach(unsigned pin);
int gpio_intr_init(unsigned pin, GPIO_INT_TYPE type);
#endif

#endif
