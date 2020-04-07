/*
 * adc.c
 *
 *  Created on: Sep 1, 2018
 *      Author: Mislav
 */
#include "adc.h"
#include "config.h"
#include "util.h"
#include "led.h"

#define ADC_FULL_SCALE 		4095	/* 0xFFF */
#define R37_OHM 			(28 * 1000.)
#define R38_OHM 			(20 * 1000.)
#define R39_OHM				(10 * 1000.)
#define AMP_GAIN			(1 + (100 * 1000) / (R39_OHM))
#define LOADCELL_MAX_SENSING 200	/* kg */
#define LOADCELL_SENSITIVITY 2		/* mV/V */
#define VREFINT_CAL_ADDR     0x1FF800F8
#define DMA_MAX_TIME         300    /* ms */

ADC_HandleTypeDef hadc;
DMA_HandleTypeDef hdma_adc;

xSemaphoreHandle ADC_conSemaphore = NULL;

static uint16_t ADC_ReadValue[3];
static double WeightVal;

static void ADC_Start();
static void ADC_Stop();
static void ADC_CreateSemaphore();
static double ADC_calculateWeight(double, double);
static void ADC_processData();

void ADC_Init(void) {

	ADC_ChannelConfTypeDef sConfig;

	ADC_GPIOInit();
	ADC_CreateSemaphore();

	/* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

  /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
	hadc.Instance = ADC1;
	hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
	hadc.Init.Resolution = ADC_RESOLUTION_12B;
	hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc.Init.ScanConvMode = ADC_SCAN_ENABLE;
	hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
	hadc.Init.LowPowerAutoWait = ADC_AUTOWAIT_DISABLE;
	hadc.Init.LowPowerAutoPowerOff = ADC_AUTOPOWEROFF_DISABLE;
	hadc.Init.ChannelsBank = ADC_CHANNELS_BANK_A;
	hadc.Init.ContinuousConvMode = ENABLE;
	hadc.Init.NbrOfConversion = 3;
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

	sConfig.Channel = ADC_CHANNEL_VREFINT;
	sConfig.Rank = 3;
	if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}
}

void ADC_GPIOInit(void) {

	GPIO_InitTypeDef GPIO_InitStruct;

	/*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ADC_PWR_GPIO_Port, ADC_PWR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED_STAT_Pin SIM_PWR_Pin SIM_DTR_Pin ADC_PWR_Pin */
  GPIO_InitStruct.Pin = ADC_PWR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ADC_PWR_GPIO_Port, &GPIO_InitStruct);

}

void ADC_startMeasurement(void)
{
    DEBUG_INFO(" ");
	/* Start conversion */
	ADC_Start();
	/* Wait for DMA conversion */
	if (xSemaphoreTake(ADC_conSemaphore, DMA_MAX_TIME) == pdFALSE)
	{
		return;
	}
	/* Stop conversion */
	ADC_Stop();
	/* Start processing data */
	ADC_processData();
}

static void ADC_Start(void)
{
	HAL_GPIO_WritePin(ADC_PWR_GPIO_Port, ADC_PWR_Pin, GPIO_PIN_SET);
	delay_ms(10);
	HAL_ADC_Start_DMA(&hadc, (uint32_t *)ADC_ReadValue, 3);
}

static void ADC_Stop(void)
{
	HAL_ADC_Stop_DMA(&hadc);
	HAL_GPIO_WritePin(ADC_PWR_GPIO_Port, ADC_PWR_Pin, RESET);
}

void ADC_DeInit(void)
{
	ADC_GPIODeInit();
	HAL_ADC_DeInit(&hadc);
}

void ADC_GPIODeInit(void) {
	HAL_GPIO_DeInit(ADC_PWR_GPIO_Port, ADC_PWR_Pin);
}

/* DMA Conversion complete callback */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    BaseType_t xTaskWokenByConvCplt = pdFALSE;
	/* Give semaphore to processing */
	xSemaphoreGiveFromISR(ADC_conSemaphore, &xTaskWokenByConvCplt);
}

double ADC_GetWeightValue(void) {
	return WeightVal;
}

static void
ADC_CreateSemaphore(void)
{
	ADC_conSemaphore = xSemaphoreCreateBinary();
	if (ADC_conSemaphore == NULL)
	{
	    DEBUG_ERROR("ADC_conSemaphore not created!");
		Error_Handler(FreeRTOS_Error);
	}
}

static void
ADC_processData(void)
{
	double BatVolt, ADC_BatVolt;
	double Vdd;
	double ADC_WeightVolt, AmpInWeightVolt;
	double Vrefint_data;
	uint16_t Vrefint_cal = *((uint16_t*)VREFINT_CAL_ADDR);
	DEBUG_INFO(" ");
	debug_printf("Vrefint_cal = %d\r\n", Vrefint_cal);
	debug_printf("rv[0] = %d\r\n", ADC_ReadValue[0]);
	debug_printf("rv[1] = %d\r\n", ADC_ReadValue[1]);
	debug_printf("rv[2] = %d\r\n", ADC_ReadValue[2]);
	/* Are values in the range? */
	if (ADC_ReadValue[0] > ADC_FULL_SCALE
	        || ADC_ReadValue[1] > ADC_FULL_SCALE
	        || ADC_ReadValue[2] > ADC_FULL_SCALE)
	{
		return;
	}
	Vrefint_data = (double) ADC_ReadValue[2];
	debug_printf("Vrefint data = %f\r\n", Vrefint_data);
	Vdd = 3 * (Vrefint_cal / Vrefint_data);
	debug_printf("Vdd = %f\r\n", Vdd);
	ADC_BatVolt = ((double) ADC_ReadValue[1] / ADC_FULL_SCALE) * Vdd;
	BatVolt = ADC_BatVolt * ((R37_OHM + R38_OHM) / (R38_OHM));
	ADC_WeightVolt = ((double) ADC_ReadValue[0] / ADC_FULL_SCALE) * Vdd;
	AmpInWeightVolt = ADC_WeightVolt / (AMP_GAIN);
	debug_printf("BatVolt = %4.2f\r\n", BatVolt);
	debug_printf("ADC_WeightVolt = %4.2f\r\n", ADC_WeightVolt);
	debug_printf("AmpInWeightVolt = %4.2f\r\n", AmpInWeightVolt);

	WeightVal = ADC_calculateWeight(BatVolt, AmpInWeightVolt * 1000);
	debug_printf("WeightVal = %4.2f\r\n", WeightVal);
}

static double
ADC_calculateWeight(double batVolt, double loadCellOutputmV)
{
	float mVolt_kg;

	mVolt_kg = loadCellOutputmV * LOADCELL_MAX_SENSING;
	if (batVolt != 0)
	{
		return mVolt_kg / (LOADCELL_SENSITIVITY * batVolt);
	}
	return 0;
}
