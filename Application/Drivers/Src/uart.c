/*
 * uart.c
 *
 *  Created on: Sep 1, 2018
 *      Author: Mislav
 */
#include "uart.h"
#include "string.h"
#include "led.h"

UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim3;
uint8_t rxByte;
static uint8_t sim808reply[SIM808_BUFFER_SIZE];

xSemaphoreHandle txBinarySemaphore = NULL;
xSemaphoreHandle rxBinarySemaphore = NULL;
xSemaphoreHandle UARTMutex = NULL;

xQueueHandle uartRxQueue;
xQueueHandle uartTxQueue;

void UART_Init(void) {

	huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  // Init timer
  __HAL_RCC_TIM3_CLK_ENABLE();

  htim3.Instance = TIM3;
	htim3.Init.Prescaler = 31; // TODO: Postaviti na 1us
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = 10000;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	HAL_TIM_Base_Init(&htim3);

	HAL_NVIC_SetPriority(TIM3_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(TIM3_IRQn);

  // Init mutex semaphore
  UARTMutex = xSemaphoreCreateMutex();
	// Init binary semaphores
  rxBinarySemaphore = xSemaphoreCreateBinary();
  txBinarySemaphore = xSemaphoreCreateBinary();

  if((UARTMutex == NULL) || (rxBinarySemaphore == NULL) || (txBinarySemaphore == NULL))
  	Error_Handler(FreeRTOS_Error);

  uartRxQueue = xQueueCreate(SIM808_BUFFER_SIZE, sizeof(rxByte));
  uartTxQueue = xQueueCreate(SIM808_BUFFER_SIZE, sizeof(rxByte));

  if((uartRxQueue == NULL) || (uartTxQueue))
  	Error_Handler(FreeRTOS_Error);

  UART_Enable(RX_EN, TX_EN);
}

void UART_DeInit(void) {
	HAL_UART_DeInit(&huart1);
}

void UART_ByteReceivedHandler(void) {
	uint8_t rxData;
	UART_Timer35Disable();
	UART_GetByte(&rxData);
	if((rxData != '\r') && (rxData != '\n') && (rxData != '\0'))
		UART_RxQeuePutByte(rxData);

	UART_Timer35Enable();
}
void UART_TransmissionCompleteHandler(void) {
	uint8_t byteToSend;
	if(UART_TxQueueGetByteFromISR(&byteToSend))
		UART_PutByte(byteToSend);
	else {
		// Tx queue empty
		BaseType_t xTaskWokenByReceive = pdFALSE;

		xSemaphoreGiveFromISR(txBinarySemaphore, &xTaskWokenByReceive);
		if(xTaskWokenByReceive != pdFALSE)
	 		taskYIELD ();
	}
}

void UART_QueueReset(void) {
	// Reset the queue
	xQueueReset(uartTxQueue);
	xQueueReset(uartRxQueue);
}

void UART_TxQueueFill(uint8_t *data) {

	uint16_t txSize = strlen((char *)data);
	uint16_t i;

	for(i = 0; i < txSize; i++) {
		UART_TxQueuePutByte(data[i]);
	}
}

void UART_RxDeQueue(uint8_t *data) {
	uint16_t i = 0;

	while(uxQueueMessagesWaiting(uartRxQueue) > 0) {
		data[i] = UART_RxQueueGetByte();
		i++;
	}
}

uint8_t UART_TxQueueGetByteFromISR(uint8_t *data) {

	BaseType_t xTaskWokenByReceive = pdFALSE;

	if(xQueueReceiveFromISR(uartTxQueue, data, &xTaskWokenByReceive) == pdFALSE)
		return 0;

	if(xTaskWokenByReceive != pdFALSE)
 		taskYIELD ();

	return 1;
}

uint8_t UART_TxQueueGetByte(void) {
	uint8_t data;

	xQueueReceive(uartTxQueue, &data, 10);

	return data;
}

void UART_TxQueuePutByte(uint8_t data) {
	xQueueSend(uartTxQueue, &data, 10);
}

void UART_RxQeuePutByte(uint8_t data) {
	BaseType_t xTaskWokenByReceive = pdFALSE;

	xQueueSendFromISR(uartRxQueue, &data, &xTaskWokenByReceive);

	if(xTaskWokenByReceive != pdFALSE)
 		taskYIELD ();
}

uint8_t UART_RxQueueGetByte(void) {
	uint8_t data;

	xQueueReceive(uartRxQueue, &data, 10);

	return data;
}

void UART_PutByte( uint8_t ucByte )
{
	huart1.Instance->DR = ucByte;
}

void UART_GetByte( uint8_t * pucByte )
{
	*pucByte=huart1.Instance->DR;
}

void UART_Enable ( uint8_t xRxEnable, uint8_t xTxEnable )
{
	uint32_t tmpreg = 0;

	tmpreg = huart1.Instance->CR1;
  if( xRxEnable ) {
		tmpreg |= (USART_CR1_RE);
		huart1.Instance->CR1 = (uint32_t)tmpreg;
		__HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
  }
  else {
		tmpreg &= ~(USART_CR1_RE);
		huart1.Instance->CR1 = (uint32_t)tmpreg;
		__HAL_UART_DISABLE_IT(&huart1, UART_IT_RXNE);
  }

	tmpreg = huart1.Instance->CR1;
  if( xTxEnable ) {
		tmpreg |= (USART_CR1_TE);
		huart1.Instance->CR1 = (uint32_t)tmpreg;
		__HAL_UART_ENABLE_IT(&huart1, UART_IT_TC);
  }
  else {
		tmpreg &= ~(USART_CR1_TE);
		huart1.Instance->CR1 = (uint32_t)tmpreg;
		__HAL_UART_DISABLE_IT(&huart1, UART_IT_TC);
  }
}

void UART_Timer35Enable(void) {
	htim3.Instance->CNT=0;
	HAL_TIM_Base_Start_IT(&htim3);
}

void UART_Timer35Disable(void) {
	HAL_TIM_Base_Stop_IT(&htim3);
}

void TIM3_IRQHandler(void)
{
	__HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
	__HAL_TIM_CLEAR_IT(&htim3, TIM_IT_UPDATE);
	HAL_NVIC_ClearPendingIRQ(TIM3_IRQn);

	HAL_TIM_Base_Stop_IT(&htim3);

	BaseType_t xTaskWokenByReceive = pdFALSE;

	xSemaphoreGiveFromISR(rxBinarySemaphore, &xTaskWokenByReceive);
	if(xTaskWokenByReceive != pdFALSE)
		taskYIELD ();
}

ErrorType_t UART_TakeMutex(uint16_t timeout) {
	if(xSemaphoreTake(UARTMutex, timeout) == pdFALSE)
		return UART_Error;
	else
		return Ok;
}
void UART_GiveMutex(void) {
	xSemaphoreGive(UARTMutex);
}

ErrorType_t UART_SendByte(uint8_t dataToSend, uint16_t timeout) {
	// Take mutex first
	if(UART_TakeMutex(timeout) != Ok)
		return UART_Error;

	// Reset the queues
	UART_QueueReset();

	// Fill the TX queue
	xQueueSend(uartTxQueue, &dataToSend, 10);

	// Enable UART transmitter
	UART_PutByte(UART_TxQueueGetByte());

	// Delay till the whole frame is transmitted
	if(xSemaphoreTake(txBinarySemaphore, timeout) == pdFALSE) {
		UART_GiveMutex();
		return UART_Error;
	}

	UART_GiveMutex();
	return Ok;
}

ErrorType_t UART_SendString(uint8_t *dataToSend, uint16_t timeout) {
	// Take mutex first
	if(UART_TakeMutex(timeout) != Ok)
		return UART_Error;

	// Reset the queues
	UART_QueueReset();

	// Fill the TX queue
	UART_TxQueueFill(dataToSend);

	// Enable UART transmitter
	UART_PutByte(UART_TxQueueGetByte());

	// Delay till the whole frame is transmitted
	if(xSemaphoreTake(txBinarySemaphore, timeout) == pdFALSE) {
		UART_GiveMutex();
		return UART_Error;
	}

	UART_GiveMutex();
	return Ok;
}

ErrorType_t UART_SendCheckReply(uint8_t *send, uint8_t *reply, uint16_t timeout) {


	// Take mutex first
	if(UART_TakeMutex(timeout) != Ok)
		return UART_Error;

	// Reset the queues
	UART_QueueReset();

	// Fill the TX queue
	UART_TxQueueFill(send);

	// Enable UART transmitter
	UART_PutByte(UART_TxQueueGetByte());

	// Delay till the whole frame is transmitted
	if(xSemaphoreTake(txBinarySemaphore, timeout) == pdFALSE) {
		UART_GiveMutex();
		return UART_Error;
	}

	// Get the answer from sim808

	// Delay till the whole frame is received
	if(xSemaphoreTake(rxBinarySemaphore, timeout) == pdFALSE) {
		UART_GiveMutex();
		return UART_Error;
	}

	// Get the received message
	UART_RxDeQueue(sim808reply);

	if(strcmp((char *)sim808reply, (char *)reply) != 0) {
		UART_GiveMutex();
		return UART_Error;
	}

	UART_GiveMutex();
	return Ok;
}

void USART1_IRQHandler(void)
{
	//HAL_UART_IRQHandler(&huart1);
	uint32_t tmp1 = 0, tmp2 = 0;

	tmp1 = __HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE);
	tmp2 = __HAL_UART_GET_IT_SOURCE(&huart1, UART_IT_RXNE);
	if((tmp1 != RESET) && (tmp2 != RESET))
	{
		__HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE);
		UART_ByteReceivedHandler();

	}

	tmp1 = __HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC);
	tmp2 = __HAL_UART_GET_IT_SOURCE(&huart1, UART_IT_TC);
	if((tmp1 != RESET) && (tmp2 != RESET))
	{
		__HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_TC);
		UART_TransmissionCompleteHandler();
	}

	tmp1 = __HAL_UART_GET_FLAG(&huart1, UART_FLAG_TXE);
	tmp2 = __HAL_UART_GET_IT_SOURCE(&huart1, UART_IT_TXE);
	if((tmp1 != RESET) && (tmp2 != RESET))
	{
		__HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_TXE);
	}
}
