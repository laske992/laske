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

#define SMSC_ADDRESS	(char *)"+385910401"

void SIM808_Init(void);
void SIM808_GPIOInit(void);
void SIM808_DeInit(void);
void SIM808_GPIODeInit(void);
ErrorType_t SIM808_SendCheckReply(uint8_t *send, uint8_t *reply, uint16_t timeout);
ErrorType_t SIM808_SendString(uint8_t *send, uint16_t timeout);
ErrorType_t SIM808_SendByte(uint8_t send, uint16_t timeout);
ErrorType_t SIM808_DisableEcho(void);
ErrorType_t SIM808_SendSMS(uint8_t *number, uint8_t *message);


#endif /* DRIVERS_INC_SIM808_H_ */
