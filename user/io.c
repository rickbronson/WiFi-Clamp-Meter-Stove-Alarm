
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <esp8266.h>
#include <time.h>
#include "main.h"


void ioInit() {
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
//	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3);  /* RX Pin */
//	PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0RXD_U);
	/* set, clear, enable, disable */
	gpio_output_set(1 << STOVE_ALARM_BEEP, 0 << STOVE_ALARM_BEEP, 1 << STOVE_ALARM_BEEP,	0);
	gpio_output_set(0, 0, 0, 1 << BTNGPIO);
}
