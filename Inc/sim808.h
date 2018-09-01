/*
 * sim808.h
 *
 *  Created on: 31. kol 2018.
 *      Author: User
 */

#ifndef SIM808_H_
#define SIM808_H_

#include "stm32l1xx.h"
#include "stm32l1xx_hal.h"
#include "stm32l1xx_hal_def.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "usbd_def.h"
#include "usbd_core.h"
#include "usbd_cdc_if.h"
#include "task.h"



// received message
char sim808MessageRecieve[MAX_COMMAND_INPUT_LENGTH];

void MX_USART1_UART_Init(void);
HAL_StatusTypeDef sim808_sendMsg(char *data);
void sim808RecieveTask(void *arg);
void sim808_rx_uartInit(void);
void sim808_rx_uartDeInit(void);


//HAL_StatusTypeDef CommunicationParser								(char *Message);
//HAL_StatusTypeDef	CommunicationRespond							(char *Message);
//HAL_StatusTypeDef CommunicationParameterReadWrite		(char *Name, char *GetSet, char *Value, char *Response);

#endif /* SIM808_H_ */
