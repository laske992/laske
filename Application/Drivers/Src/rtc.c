/*
 * rtc.c
 *
 *  Created on: Sep 1, 2018
 *      Author: Mislav
 */
#include "config.h"
#include "rtc.h"
#include "led.h"

RTC_HandleTypeDef hrtc;

void
rtc_init(struct rtc_time_t *timestamp)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    RCC_PeriphCLKInitTypeDef PeriphClkInit;

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }

    /**Initialize RTC Only
     * RTCCLK = LSE = 32.768 kHz
     * PREDIV_A = 0x7F = 127
     * PREDIV_S = 0xFF = 255
     *
     * ck_spre = RTLCLK / (PREDIV_A + 1 ) × ( PREDIV_S + 1 )) = 1Hz
     *
     */
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 0x7F;
    hrtc.Init.SynchPrediv = 0xFF;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    if (HAL_RTC_Init(&hrtc) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);

    /**Initialize RTC and set the Time and Date
     */
    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != 0x32F2)
    {
        sTime.Hours = timestamp->hour;
        sTime.Minutes = timestamp->minute;
        sTime.Seconds = timestamp->second;
        sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
        sTime.StoreOperation = RTC_STOREOPERATION_RESET;
        if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
        {
            _Error_Handler(__FILE__, __LINE__);
        }

        sDate.WeekDay = RTC_WEEKDAY_MONDAY;
        sDate.Month = timestamp->month;
        sDate.Date = timestamp->day;
        sDate.Year = timestamp->year;

        if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
        {
            _Error_Handler(__FILE__, __LINE__);
        }

        HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR0,0x32F2);
    }

    /**Enable the WakeUp
     */
    if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 60, RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }
}

void
rtc_deinit(void)
{
    HAL_RTC_DeInit(&hrtc);
}

void
HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
}

/**
 * @brief This function handles RTC Wakeup interrupt request.
 */
void
RTC_WKUP_IRQHandler(void)
{
    HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
}
