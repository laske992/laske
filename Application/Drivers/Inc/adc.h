/*
 * adc.h
 *
 *  Created on: Sep 1, 2018
 *      Author: Mislav
 */

#ifndef DRIVERS_INC_ADC_H_
#define DRIVERS_INC_ADC_H_

#include "config.h"

void ADC_Init(void);
void ADC_GPIOInit(void);
void ADC_Start(void);
void ADC_Stop(void);
void ADC_DeInit(void);
void ADC_GPIODeInit(void);
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef*);
double ADC_GetWeightValue(void);

#endif /* DRIVERS_INC_ADC_H_ */
