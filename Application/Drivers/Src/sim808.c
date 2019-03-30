/*
 * sim808.c
 *
 *  Created on: 31. kol 2018.
 *      Author: User
 */

#include "sim808.h"

UART_HandleTypeDef huart1;

xQueueHandle inputQueue;
xTaskHandle  recieveTaskHandle;
xSemaphoreHandle CDCMutex;

static char *smscAddress = SMSC_ADDRESS;

void SIM808_Init(void) {
	SIM808_GPIOInit();

	UART_Init();

	//SIM808_DisableEcho();
}
void SIM808_GPIOInit(void) {

	GPIO_InitTypeDef GPIO_InitStruct;

	/*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SIM_PWR_Pin|SIM_DTR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED_STAT_Pin SIM_PWR_Pin SIM_DTR_Pin ADC_PWR_Pin */
  GPIO_InitStruct.Pin = SIM_PWR_Pin|SIM_DTR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SIM_RI_Pin */
  GPIO_InitStruct.Pin = SIM_RI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SIM_RI_GPIO_Port, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
void SIM808_DeInit(void) {

	SIM808_GPIODeInit();
	UART_DeInit();
}
void SIM808_GPIODeInit(void) {

	HAL_GPIO_DeInit(GPIOB, SIM_PWR_Pin|SIM_DTR_Pin);
	HAL_GPIO_DeInit(SIM_RI_GPIO_Port, SIM_RI_Pin);

	HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
}

ErrorType_t SIM808_SendCheckReply(uint8_t *send, uint8_t *reply, uint16_t timeout) {
	uint8_t dataToSend[50];
	sprintf((char *)dataToSend, "%s\r\n", (char *)send);
	return UART_SendCheckReply(dataToSend, reply, timeout);
}

ErrorType_t SIM808_SendString(uint8_t *send, uint16_t timeout) {
	uint8_t dataToSend[50];
	sprintf((char *)dataToSend, "%s\r\n", (char *)send);
	return UART_SendString(dataToSend, timeout);
}

ErrorType_t SIM808_SendByte(uint8_t send, uint16_t timeout) {
	return UART_SendByte(send, timeout);
}

ErrorType_t SIM808_DisableEcho(void) {
	return SIM808_SendString((uint8_t *)"ATE0", 100);
}

ErrorType_t SIM808_SendSMS(uint8_t *number, uint8_t *message) {
	char dataToSend[50];
	if(SIM808_SendCheckReply((uint8_t *)"AT+CMGF=1", (uint8_t *)"OK", 1000) != Ok) return UART_Error;
  vTaskDelay(1000);
  sprintf(dataToSend, "AT+CSCA=\"%s\"", smscAddress);
  if(SIM808_SendCheckReply((uint8_t *)dataToSend, (uint8_t *)"OK", 1000) != Ok) return UART_Error;
  vTaskDelay(1000);
  sprintf(dataToSend, "AT+CMGS=\"%s\"", number);
  if(SIM808_SendString((uint8_t *)dataToSend, 1000) != Ok) return UART_Error;
  vTaskDelay(1000);
  if(UART_SendString(message, 1000) != Ok) return UART_Error;
  vTaskDelay(1000);
  if(SIM808_SendByte((char)26, 1000) != Ok) return UART_Error;

  return Ok;
}
