/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2018 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "cmsis_os.h"
#include "led.h"
#include "adc.h"
#include "sim808.h"
#include "util.h"
#include "http.h"

osThreadId SIM808TaskHandle;
osThreadId UnsolicitedTaskHandle;
osThreadId ADCTaskHandle;

static void _HandleSIM808Task(void const *);
static void _HandleUnsolicitedTask(void const *);
static void _ADCTask(void const *);

int main(void)
{
    debug_init();
    config_init();
    LED_Init();
    UART_Init();
    SIM808_GPIOInit();
    ADC_Init();

    /* Create the thread(s) */
    /* Incoming call handle task */
    osThreadDef(HandleSIM808Task, _HandleSIM808Task, osPriorityNormal, 0, 1000);
    SIM808TaskHandle = osThreadCreate(osThread(HandleSIM808Task), NULL);

    /* Task for handling unsolicited inputs from SIM808 */
    osThreadDef(HandleUnsolicitedTask, _HandleUnsolicitedTask, osPriorityNormal, 0, 600);
    UnsolicitedTaskHandle = osThreadCreate(osThread(HandleUnsolicitedTask), NULL);

    /* Measurement task */
    osThreadDef(ADCTask, _ADCTask, osPriorityNormal, 0, 600);
    ADCTaskHandle = osThreadCreate(osThread(ADCTask), NULL);

    /* Start scheduler */
    osKernelStart();

    for(;;);

}

void _setSignal(_TaskId TaskId, int32_t bit)
{
    osThreadId Thread;
    switch(TaskId) {
    case SIM808Task:
        Thread = SIM808TaskHandle;
        break;
    case SIM808UnsolicitedTask:
        Thread = UnsolicitedTaskHandle;
        break;
    case ADCTask:
        Thread = ADCTaskHandle;
        break;
    default:
        return;
    }
    osSignalSet(Thread, bit);
}

/* Handle incoming call function */
static void
_HandleSIM808Task(void const *arg)
{
    osEvent event;
    uint32_t signals = SIGNAL_SIM_RI_IRQ_CALL | SIGNAL_SIM_RI_IRQ_SMS;
    SIM808_Init();
    for (;;)
    {
        /* Wait any of signals */
        event = osSignalWait(signals, osWaitForever);
        if (event.value.signals & SIGNAL_SIM_RI_IRQ_CALL)
        {
            DEBUG_INFO("IRQ Call kicked");
            SIM808_handleCall();
//            web_server_subscribe();
        }
        else if (event.value.signals & SIGNAL_SIM_RI_IRQ_SMS)
        {
            DEBUG_INFO("IRQ SMS kicked");
            SIM808_handleSMS();
        }
    }
    /* Should not reach this point */
    vTaskDelete(SIM808TaskHandle);
}

/* Handle unsolicited inputs from SIM808 */
static void
_HandleUnsolicitedTask(void const *arg)
{
    osEvent event;
    for (;;)
    {
        /* Wait signal from UART RX interrupt */
        event = osSignalWait(SIGNAL_UNSOLICITED, osWaitForever);
        if (event.value.signals & SIGNAL_UNSOLICITED)
        {
            DEBUG_INFO("Unsolicited kicked");
            SIM808_handleUnsolicited();
        }
    }
    /* Should not reach this point */
    vTaskDelete(UnsolicitedTaskHandle);
}

/* Prepare measurement data for SMS */
static void
_ADCTask(void const *arg)
{
    osEvent event;
    for (;;)
    {
        /* Wait signal from SIM808HandleTask */
        event = osSignalWait(SIGNAL_START_ADC, osWaitForever);
        if (event.value.signals & SIGNAL_START_ADC)
        {
            ADC_startMeasurement();
        }
    }
    /* Should not reach this point */
    vTaskDelete(ADCTaskHandle);
}

void
pre_sleep_processing(uint32_t *xModifiableIdleTime)
{
    (void) xModifiableIdleTime;
    UART_Enable(RX_EN, TX_EN);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
}

void
post_sleep_processing(uint32_t *xExpectedIdleTime)
{
    (void) xExpectedIdleTime;
}
