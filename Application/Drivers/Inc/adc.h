/*
 * adc.h
 *
 *  Created on: Sep 1, 2018
 *      Author: Mislav
 */

#ifndef DRIVERS_INC_ADC_H_
#define DRIVERS_INC_ADC_H_

#include "config.h"

void ADC_Init();
void ADC_GPIOInit();
void ADC_startMeasurement();
void ADC_DeInit();
void ADC_GPIODeInit();
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef*);
double ADC_GetWeightValue(void);

#endif /* DRIVERS_INC_ADC_H_ */
