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

typedef enum {
    NTP_SUCCESS = 1,
    NTP_NET_ERROR = 61,
    NTP_DNS_ERROR = 62,
    NTP_CONNECT_ERR = 63,
    NTP_TIMEOUT = 64,
    NTP_SERVER_ERROR = 65,
    NTP_OPERATION_NOT_ALLOWED = 66
} NTP_status_t;

typedef enum {
    GSM_LOC_SUCCESS = 0,
    GSM_LOC_FAILED,
    GSM_LOC_TIMEOUT,
    GSM_LOC_NET_ERROR,
    GSM_LOC_DNS_ERROR,
    GSM_LOC_SERVICE_OVERDUE,
    GSM_LOC_AUTH_FAILED,
    GSM_LOC_OTHER_ERROR
} gsm_loc_status_t;

void SIM808_handleSMS();
void SIM808_handleCall();
void SIM808_handleUnsolicited();
void SIM808_GPIOInit();
void SIM808_GPIODeInit();
bool SIM808_RI_active();
ErrorType_t SIM808_Init(void);
ErrorType_t SIM808_SendSMS(char *);
void SIM808_send_GET_request(char *, char *);
void SIM808_send_POST_request(char *, char *);
void SIM808_send_TCP_request(char *, char *, char *);

#endif /* DRIVERS_INC_SIM808_H_ */
