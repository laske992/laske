/*
 * config.h
 *
 *  Created on: Sep 1, 2018
 *      Author: Mislav
 */

#ifndef BASE_INC_CONFIG_H_
#define BASE_INC_CONFIG_H_

/* basic includes */
#include "stm32l1xx_hal.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "util.h"

#define ADC_WEIGHT_Pin              GPIO_PIN_1
#define ADC_WEIGHT_GPIO_Port        GPIOA
#define ADC_BAT_Pin                 GPIO_PIN_2
#define ADC_BAT_GPIO_Port           GPIOA
#define LED_STAT_Pin                GPIO_PIN_12
#define LED_STAT_GPIO_Port          GPIOB
#define SIM_PWR_Pin                 GPIO_PIN_13
#define SIM_PWR_GPIO_Port           GPIOB
#define SIM_RI_Pin                  GPIO_PIN_14
#define SIM_RI_GPIO_Port            GPIOB
#define SIM_RI_EXTI_IRQn            EXTI15_10_IRQn
#define SIM_DTR_Pin                 GPIO_PIN_15
#define SIM_DTR_GPIO_Port           GPIOB
#define USB_DETECT_Pin              GPIO_PIN_8
#define USB_DETECT_GPIO_Port        GPIOA
#define USB_DETECT_EXTI_IRQn        EXTI9_5_IRQn
#define ADC_PWR_Pin                 GPIO_PIN_0
#define ADC_PWR_GPIO_Port           GPIOA

void config_init();

void Error_Handler(ErrorType_t);

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *);

void _Error_Handler(char *, int);

#endif /* BASE_INC_CONFIG_H_ */
