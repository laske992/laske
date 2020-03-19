/*
 * usb.c
 *
 *  Created on: Sep 1, 2018
 *      Author: Mislav
 */
#include "config.h"
#include "usb_device.h"

void USB_Init(void) {
	USB_GPIOInit();
	MX_USB_DEVICE_Init();
}

void USB_GPIOInit(void) {

	GPIO_InitTypeDef GPIO_InitStruct;

	/*Configure GPIO pin : USB_DETECT_Pin */
  GPIO_InitStruct.Pin = USB_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_DETECT_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

void USB_DeInit(void) {

	USB_GPIODeInit();
	MX_USB_DEVICE_DeInit();
}

void USB_GPIODeInit(void) {
	HAL_GPIO_DeInit(USB_DETECT_GPIO_Port, USB_DETECT_Pin);
	HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
}

