/*
 * communication.c
 *
 *  Created on: Jan 3, 2018
 *      Author: Mislav
 */

#include "communication.h"
#include "util.h"


/* Queue for input USB data */
xQueueHandle inputQueue;
xTaskHandle  recieveTaskHandle;
xSemaphoreHandle CDCMutex;

void CommunicationRecieveTask(void *argument){
	HAL_StatusTypeDef Status;

  for(;;){
    if(xQueueReceive( inputQueue,&communicationMessageRecieve, 0xFFFFFFFF )==pdTRUE){
			Status=CommunicationParser(communicationMessageRecieve);
			HAL_GPIO_TogglePin(LED_STAT_GPIO_Port, LED_STAT_Pin);
			if (Status==HAL_OK){
				CDC_Transmit_FS(communicationMessageRecieve, strlen((char *)communicationMessageRecieve));
			}else{
				strcpy((char *)communicationMessageRecieve,"[ERROR] Unknown error occured#");
				CDC_Transmit_FS(communicationMessageRecieve, strlen((char *)communicationMessageRecieve));
			}
		}
	}
}

void communicationInit(void) {
	BaseType_t xReturned;
	CDCMutex = xSemaphoreCreateMutex();
	inputQueue = xQueueCreate(2, sizeof(communicationMessageRecieve));

	if(inputQueue == NULL)
		Error_Handler(FreeRTOS_Error);

	xReturned = xTaskCreate(CommunicationRecieveTask, (const char *)"CommRecTask", 100, NULL, 9, &recieveTaskHandle);
	if(xReturned != pdPASS)
		Error_Handler(FreeRTOS_Error);
}

void communicationDeInit(void) {

	if(CDCMutex!=NULL) {
		vSemaphoreDelete(CDCMutex);
		CDCMutex = NULL;
	}
	if(recieveTaskHandle!=NULL) {
		vTaskDelete(recieveTaskHandle);
		recieveTaskHandle = NULL;
	}
	if(inputQueue!=NULL) {
		vQueueDelete(inputQueue);
		inputQueue=NULL;
	}
}

HAL_StatusTypeDef CommunicationParser(char *Message) {
	int i=0;
	unsigned int len = 0;
	char Command[21];
	char *pch;
	char *message = "Hello\r\n";
//	char Argument1[21], Argument2[21], Argument3[21], Argument4[21];
	HAL_StatusTypeDef Return = HAL_ERROR;

	pch = strtok ((char *)Message," ");
	while (pch != NULL)
	{
		len = strlen(pch);
		if(len>20 && i<5)	return Return;
		if (i==0){
			strncpy (Command,pch,20);
			Command[strlen(pch) + 1] = '\0';
		} else {
			return Return;
		}
	}
	if (!strcmp(Command, "SIM_START")
			|| !strcmp(Command, "SIM_STOP")) {
		CDC_Transmit_FS(message, 10);
		HAL_GPIO_WritePin(SIM_PWR_GPIO_Port, SIM_PWR_Pin, GPIO_PIN_SET);
		for (i=0; i<100000; i++);
		HAL_GPIO_WritePin(SIM_PWR_GPIO_Port, SIM_PWR_Pin, GPIO_PIN_RESET);
		Return = HAL_OK;
	}
	return Return;
}
