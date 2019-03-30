/*
 * adc.c
 *
 *  Created on: Sep 1, 2018
 *      Author: Mislav
 */
#include "adc.h"

ADC_HandleTypeDef hadc;
DMA_HandleTypeDef hdma_adc;

void ADC_Init(void) {

	ADC_ChannelConfTypeDef sConfig;

	ADC_GPIOInit();

	/* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

  /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
	hadc.Instance = ADC1;
	hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
	hadc.Init.Resolution = ADC_RESOLUTION_12B;
	hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
	hadc.Init.LowPowerAutoWait = ADC_AUTOWAIT_DISABLE;
	hadc.Init.LowPowerAutoPowerOff = ADC_AUTOPOWEROFF_DISABLE;
	hadc.Init.ChannelsBank = ADC_CHANNELS_BANK_A;
	hadc.Init.ContinuousConvMode = ENABLE;
	hadc.Init.NbrOfConversion = 2;
	hadc.Init.DiscontinuousConvMode = DISABLE;
	hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc.Init.DMAContinuousRequests = DISABLE;
	if (HAL_ADC_Init(&hadc) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

		/**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
		*/
	sConfig.Channel = ADC_CHANNEL_1;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_384CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

		/**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
		*/
	sConfig.Channel = ADC_CHANNEL_2;
	sConfig.Rank = 2;
	if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}
}
void ADC_GPIOInit(void) {

	GPIO_InitTypeDef GPIO_InitStruct;

	/*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, ADC_PWR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED_STAT_Pin SIM_PWR_Pin SIM_DTR_Pin ADC_PWR_Pin */
  GPIO_InitStruct.Pin = ADC_PWR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}
void ADC_DeInit(void) {

	ADC_GPIODeInit();

	HAL_ADC_DeInit(&hadc);
}
void ADC_GPIODeInit(void) {
	HAL_GPIO_DeInit(ADC_PWR_GPIO_Port, ADC_PWR_Pin);
}
