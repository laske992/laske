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

static HAL_StatusTypeDef sim808RecvParser(char *msg);

void SIM808_Init(void) {
	SIM808_GPIOInit();

	UART_Init();
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


HAL_StatusTypeDef sim808_sendMsg(char *data) {
	HAL_StatusTypeDef status;

	status = HAL_UART_Transmit(&huart1, (uint8_t *)data, strlen(data), 500);
	return status;
}

void sim808RecieveTask(void *arg) {
	HAL_StatusTypeDef Status = HAL_OK;

  for(;;){
    if(xQueueReceive( inputQueue,&sim808MessageRecieve, 0xFFFFFFFF ) == pdTRUE){
			Status = sim808RecvParser(sim808MessageRecieve);
			if (Status == HAL_OK) {
				CDC_Transmit_FS(sim808MessageRecieve, strlen(sim808MessageRecieve));
			} else {
				strcpy(sim808MessageRecieve,"[ERROR] Unknown error occured#");
				CDC_Transmit_FS(sim808MessageRecieve, strlen(sim808MessageRecieve));
			}
		}
	}
}

void sim808_rx_uartInit(void) {
	BaseType_t xReturned;
	CDCMutex = xSemaphoreCreateMutex();
	inputQueue = xQueueCreate(2, sizeof(sim808MessageRecieve));

	if(!inputQueue)
		Error_Handler(FreeRTOS_Error);

	xReturned = xTaskCreate(sim808RecieveTask, (const char *)"sim808RecTask", 100, NULL, 9, &recieveTaskHandle);
	if(xReturned != pdPASS)
		Error_Handler(FreeRTOS_Error);
}

void sim808_rx_uartDeInit(void) {

	if (CDCMutex) {
		vSemaphoreDelete(CDCMutex);
		CDCMutex = NULL;
	}
	if (recieveTaskHandle) {
		vTaskDelete(recieveTaskHandle);
		recieveTaskHandle = NULL;
	}
	if (inputQueue) {
		vQueueDelete(inputQueue);
		inputQueue=NULL;
	}
}

static HAL_StatusTypeDef sim808RecvParser(char *msg) {
	return HAL_OK;
}
