/*
 * timer_delay.c
 *
 *  Created on: 1. velj 2022.
 *      Author: User
 */

#include "util.h"

static TIM_HandleTypeDef htim11;

void TimerDelay_Init(void)
{
    /**
     * Prescaler = X - 1
     * Period = Y - 1
     * update = 32MHz / (X * Y) = 32MHz / (32 * 10) = 1MHz
     * period is 1us
     **/
    htim11.Instance = TIM11;
    htim11.Init.Prescaler = (uint32_t)(SystemCoreClock / 1000000) - 1;   /* 32MHz / 10e6 - 1 = 32 - 1 */
    htim11.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim11.Init.Period = 10 - 1;
    htim11.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&htim11) != HAL_OK)
    {
        DEBUG_ERROR("Timer 11 error!");
    }
    HAL_TIM_Base_Start(&htim11);
}

void delay_us(volatile uint16_t au16_us)
{
    htim11.Instance->CNT = 0;
    while (htim11.Instance->CNT < au16_us);
}
