/*
 * communication.h
 *
 *  Created on: Jan 3, 2018
 *      Author: Mislav
 */

#ifndef DRIVERS_INC_COMMUNICATION_H_
#define DRIVERS_INC_COMMUNICATION_H_

#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "usbd_def.h"
#include "usbd_core.h"
#include "usbd_cdc_if.h"
#include "task.h"



// received message
char communicationMessageRecieve[MAX_COMMAND_INPUT_LENGTH];



void communicationRecieveTask	(void *argument);
void communicationInit				(void);
void communicationDeInit			(void);


HAL_StatusTypeDef CommunicationParser								(char *Message);
HAL_StatusTypeDef	CommunicationRespond							(char *Message);
HAL_StatusTypeDef CommunicationParameterReadWrite		(char *Name, char *GetSet, char *Value, char *Response);

#endif /* DRIVERS_INC_COMMUNICATION_H_ */
