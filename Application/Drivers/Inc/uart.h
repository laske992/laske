/*
 * uart.h
 *
 *  Created on: Sep 1, 2018
 *      Author: Mislav
 */

#ifndef DRIVERS_INC_UART_H_
#define DRIVERS_INC_UART_H_

#include "config.h"
#include "util.h"

#define UART_TIMEOUT_MS				500
#define UART_TRANS_TIMEOUT_MS		100
#define SIM808_BUFFER_SIZE			1024

#define TX_EN									1
#define TX_DIS									0
#define RX_EN									1
#define RX_DIS									0

typedef ErrorType_t (SIM808_checkResp)(char *, uint8_t);

void UART_Init(void);
void UART_DeInit(void);
void UART_Enable (uint8_t, uint8_t);
ErrorType_t UART_Send(uint8_t *, uint16_t, uint8_t, SIM808_checkResp *);
void UART_GetData(char *);
void UART_FlushQueues();

#endif /* DRIVERS_INC_UART_H_ */
