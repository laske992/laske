/*
 * sim808.h
 *
 *  Created on: 31. kol 2018.
 *      Author: User
 */

#ifndef DRIVERS_INC_SIM808_H_
#define DRIVERS_INC_SIM808_H_

#include "stm32l1xx.h"
#include "stm32l1xx_hal.h"
#include "stm32l1xx_hal_def.h"

#include "uart.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "usbd_def.h"
#include "usbd_core.h"
#include "usbd_cdc_if.h"
#include "task.h"
#include "util.h"
#include "storage.h"
#include "led.h"

typedef enum {
	NOT_REGISTERED = 0,
	REGISTERED,
	SEARCHING_OPERATOR,
	REGISTRATION_DENIED,
	UNKNOWN,
	ROAMING
} CREG_resp_t;

typedef enum {
	PDU_MODE = 0,
	TEXT_MODE
} CMGF_resp_t;

void SIM808_handleSMS();
void SIM808_handleCall();
void SIM808_GPIOInit();
void SIM808_GPIODeInit();
bool SIM808_RI_active();
ErrorType_t SIM808_Init(void);
ErrorType_t SIM808_SendSMS(char *);
void SIM808_send_GET_request(char *, char *);
void SIM808_send_POST_request(char *, char *);
void SIM808_send_TCP_request(char *, char *, char *);


#endif /* DRIVERS_INC_SIM808_H_ */
