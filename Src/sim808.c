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

/* USART1 init function */
void MX_USART1_UART_Init(void)
{

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
		Error_Handler();

	xReturned = xTaskCreate(sim808RecieveTask, (const char *)"sim808RecTask", 100, NULL, 9, &recieveTaskHandle);
	if(xReturned != pdPASS)
		Error_Handler();
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
