/*
 * led.c
 *
 *  Created on: Sep 1, 2018
 *      Author: Mislav
 */
#include "led.h"

void LED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, LED_STAT_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pins : LED_STAT_Pin SIM_PWR_Pin SIM_DTR_Pin ADC_PWR_Pin */
	GPIO_InitStruct.Pin = LED_STAT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void LED_DeInit(void)
{
	HAL_GPIO_DeInit(LED_STAT_GPIO_Port, LED_STAT_Pin);
}

void LED_Set(Led_State_t LedState)
{
	if (LedState == LED_On)
		HAL_GPIO_WritePin(LED_STAT_GPIO_Port, LED_STAT_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(LED_STAT_GPIO_Port, LED_STAT_Pin, GPIO_PIN_RESET);
}

void LED_Toggle(void)
{
	HAL_GPIO_TogglePin(LED_STAT_GPIO_Port, LED_STAT_Pin);
}

