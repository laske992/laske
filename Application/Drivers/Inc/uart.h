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

#define UART_TIMEOUT_MS					500
#define UART_TRANS_TIMEOUT_MS		100
#define SIM808_BUFFER_SIZE			1024

#define TX_EN										1
#define TX_DIS									0
#define RX_EN										1
#define RX_DIS									0

typedef ErrorType_t (SIM808_checkResp)(char *, uint8_t);

void UART_Init(void);
void UART_DeInit(void);
void UART_Enable ( uint8_t xRxEnable, uint8_t xTxEnable );
void UART_ByteReceivedHandler(void);
void UART_TransmissionCompleteHandler(void);
void UART_TxQueueFill(uint8_t *data);
uint8_t UART_TxQueueGetByteFromISR(uint8_t *data);
uint8_t UART_TxQueueGetByte(void);
void UART_TxQueuePutByte(uint8_t data);
void UART_RxQeuePutByte(uint8_t data);
uint8_t UART_RxQueueGetByte(void);
void UART_Timer35Enable(void);
void UART_Timer35Disable(void);
ErrorType_t UART_TakeMutex(uint16_t timeout);
void UART_GiveMutex(void);
ErrorType_t UART_SendByte(uint8_t dataToSend, uint16_t timeout);
void UART_GetData(char *);
ErrorType_t UART_Send(uint8_t *, uint16_t, uint8_t, SIM808_checkResp *);
void USART1_IRQHandler(void);

#endif /* DRIVERS_INC_UART_H_ */
