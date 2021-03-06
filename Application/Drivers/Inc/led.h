/*
 * led.h
 *
 *  Created on: Sep 1, 2018
 *      Author: Mislav
 */

#ifndef DRIVERS_INC_LED_H_
#define DRIVERS_INC_LED_H_

#include "config.h"

typedef enum {
	LED_Off = 0,
	LED_On = 1
} Led_State_t;

void LED_Init();
void LED_DeInit();
void LED_Set(Led_State_t);
void LED_Toggle();

#endif /* DRIVERS_INC_LED_H_ */
