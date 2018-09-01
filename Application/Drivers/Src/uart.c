/*
 * uart.c
 *
 *  Created on: Sep 1, 2018
 *      Author: Mislav
 */
#include "uart.h"

UART_HandleTypeDef huart1;


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

}

void UART_DeInit(void) {
	HAL_UART_DeInit(&huart1);
}

void USART1_IRQHandler(void)
{
	if(__HAL_UART_GET_IT_SOURCE(&huart1, UART_IT_RXNE) != RESET)
	{
	        //RXCallback();
	}
	HAL_NVIC_ClearPendingIRQ(USART1_IRQn);

  /* USER CODE END USART1_IRQn 1 */
}

