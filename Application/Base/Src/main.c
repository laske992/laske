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

#include "stm32l1xx_hal.h"
#include "cmsis_os.h"

#include "communication.h"
#include "sim808.h"
#include "usb_device.h"

#include "led.h"
#include "power.h"
#include "adc.h"
#include "usb.h"
#include "util.h"

osThreadId SIM808_SetupTaskHandle;
osThreadId CallTaskHandle;
osThreadId SMSTaskHandle;
osThreadId ADCTaskHandle;

static void _SIM808_SetupTask(void const *argument);
static void _HandleCallTask(void const *argument);
static void _SMSTask(void const *argument);
static void _ADCTask(void const *argument);

int main(void)
{

	CONFIG_Init();
	LED_Init();
	SIM808_GPIOInit();
	MX_USB_DEVICE_Init();

	/* Create the thread(s) */
	/* SIM808 initial setup task */
	osThreadDef(SIM808_SetupTask, _SIM808_SetupTask, osPriorityLow, 0, 500);
	SIM808_SetupTaskHandle = osThreadCreate(osThread(SIM808_SetupTask), NULL);

	/* Incoming call handle task */
	osThreadDef(HandleCallTask, _HandleCallTask, osPriorityHigh, 0, 500);
	CallTaskHandle = osThreadCreate(osThread(HandleCallTask), NULL);

	/* Prepare and send SMS task */
	osThreadDef(SMSTask, _SMSTask, osPriorityNormal, 0, 500);
	SMSTaskHandle = osThreadCreate(osThread(SMSTask), NULL);

	/* Measurement task */
	osThreadDef(ADCTask, _ADCTask, osPriorityNormal, 0, 500);
	ADCTaskHandle = osThreadCreate(osThread(ADCTask), NULL);

	/* Start scheduler */
	osKernelStart();

	for(;;);
}

void _setSignal(_TaskId TaskId, int32_t bit)
{
	osThreadId Thread;
	switch(TaskId) {
	case CALLTask:
		Thread = CallTaskHandle;
		break;
	case ADCTask:
		Thread = ADCTaskHandle;
		break;
	case SMSTask:
		Thread = SMSTaskHandle;
		break;
	default:
		return;
	}
	osSignalSet(Thread, bit);
}

/* _SIM808_SetupTask function */
static void _SIM808_SetupTask(void const * argument)
{
	SIM808_Init();
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
  }
  vTaskDelete(SIM808_SetupTaskHandle);
  /* USER CODE END 5 */ 
}

/* Handle incoming call function */
static void _HandleCallTask(void const * argument)
{
	osEvent event;

  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
	  /* Wait an interrupt on SIM_RI pin */
	  event = osSignalWait(BIT_1, osWaitForever);
	  if (event.value.signals == BIT_1) {
		  SIM808_handleCall();
	  }
  }
  vTaskDelete(CallTaskHandle);
  /* USER CODE END 5 */
}

/* Prepare and send SMS function */
static void _SMSTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
  }
  vTaskDelete(SMSTaskHandle);
  /* USER CODE END 5 */
}

/* Prepare measurement data for SMS */
static void _ADCTask(void const *argument)
{
	/* USER CODE BEGIN 5 */
	/* Infinite loop */
	for(;;)
	{
	}
	vTaskDelete(ADCTaskHandle);
	/* USER CODE END 5 */
}
