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

struct at_response {
    bool wait;                          /* Wait for response (true/false) */
    uint16_t rx_timeout;                /* Timeout for response waiting */
    char response[SIM808_BUFFER_SIZE];  /* AT response */
};

void UART_Init(void);
void UART_DeInit(void);
void UART_Enable (uint8_t, uint8_t);
ErrorType_t UART_Send(uint8_t *, uint16_t, struct at_response *);
void UART_Get_rxData(char *, uint32_t);
void UART_FlushQueues();

#endif /* DRIVERS_INC_UART_H_ */
