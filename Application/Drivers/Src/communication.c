/*
 * communication.c
 *
 *  Created on: Jan 3, 2018
 *      Author: Mislav
 */

#include "communication.h"


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
//		if (i==1){
//			strncpy (Argument1,pch,20);
//			Argument1[20]='\0';
//		}
//		if (i==2){
//			strncpy (Argument2,pch,20);
//			Argument2[20]='\0';
//		}
//		if (i==3){
//			strncpy (Argument3,pch,230);
//			Argument3[20]='\0';
//		}
//		if (i==4){
//			strncpy (Argument4,pch,20);
//			Argument4[20]='\0';
//		}
//		if (i==5){
//			break;
//		}
//		pch = strtok (NULL, " ");
//		if(i==6)	return Return;
//		i++;
//	}
//
	if (!strcmp(Command, "SIM_START")
			|| !strcmp(Command, "SIM_STOP")) {
		CDC_Transmit_FS(message, 10);
		HAL_GPIO_WritePin(SIM_PWR_GPIO_Port, SIM_PWR_Pin, GPIO_PIN_SET);
		for (i=0; i<100000; i++);
		HAL_GPIO_WritePin(SIM_PWR_GPIO_Port, SIM_PWR_Pin, GPIO_PIN_RESET);
		Return = HAL_OK;
	}
//		else if(!strcmp(Argument1, "FAULT")) {
//			if(BMS_Mode == BMS_MODE_AUTO) {
//				uint8_t data;
//				registerRead_bq76920(SYS_STAT, &data);
//				sprintf((char *)Message, "UV: %s, OV: %s, SCD: %s, OCD: %s#\r\n", (data & 0x08) ? "HIGH" : "LOW",
//						(data & 0x04) ? "HIGH" : "LOW", (data & 0x02) ? "HIGH" : "LOW", (data & 0x01) ? "HIGH" : "LOW");
//			}
//		}
		/*
		 * Read all cells voltages
		 */
//		else if(!strcmp(Argument1, "ALL_CELLS")) {
//			uint16_t all_cells[4];
//			getCellVoltages(all_cells);
//			sprintf((char *)Message, "Cell 1: %dmV, Cell 2: %dmV, Cell 3: %dmV, Cell 4: %dmV#\r\n"
//					, all_cells[0], all_cells[1], all_cells[2], all_cells[3]);
//			Return = HAL_OK;
//		}
//		/*
//		 * Read battery pack voltage
//		 */
//		else if(!strcmp(Argument1, "PACK_VOLTAGE")) {
//			uint16_t pack_voltage = getPackVoltage();
//			sprintf((char *)Message, "Pack Voltage: %dmV#\r\n", pack_voltage);
//			Return = HAL_OK;
//		}
//		/*
//		 * Read battery pack current
//		 */
//		else if(!strcmp(Argument1, "PACK_CURRENT")) {
//			int32_t pack_current = getPackCurrent();
//			sprintf((char *)Message, "Pack Current: %dmA#\r\n", (int)pack_current);
//			Return = HAL_OK;
//		}
//		/*
//		 * Read temperature value from first thermistor of bq76920 device
//		 */
//		else if(!strcmp(Argument1, "TEMP1")) {
//			int16_t temp1 = tempRead_bq76920(1);
//			sprintf((char *)Message, "Thermistor 1 value in Celsius: %d#\r\n", temp1);
//			Return = HAL_OK;
//		}
//		/*
//		 * ADC Gain reading
//		 */
//		/*
//		else if(!strcmp(Argument1, "ADC_GAIN")) {
//			sprintf((char *)Message, "ADC gain: %d#\r\n", adcGain);
//			Return = HAL_OK;
//		}
//		*/
//		/*
//		 * ADC offset reading
//		 */
//		/*
//		else if(!strcmp(Argument1, "ADC_OFFSET")) {
//			sprintf((char *)Message, "ADC offset: %d#\r\n", adcOffset);
//			Return = HAL_OK;
//		}
//		*/
//		/*
//		 * Check the balancing status
//		 */
//		else if(!strcmp(Argument1, "BAL_STATUS")) {
//			uint8_t balReg[4];
//			readBalancReg_bq76920(balReg);
//			sprintf((char *)Message, "Cell 1: %d, Cell 2: %d, Cell 3: %d, Cell 4: %d#\r\n", balReg[0],
//								balReg[1], balReg[2], balReg[3]);
//			Return = HAL_OK;
//		}
//		/*
//		 * Check battery level status
//		 */
//		else if(!strcmp(Argument1, "BAT_LEVEL")) {
//			uint8_t bat_level = getSOCValue();
//			sprintf((char *)Message, "Battery level: %d#\r\n", bat_level);
//			Return = HAL_OK;
//		}
//		/*
//		 * Check the Alert pin
//		 */
//		/*
//		else if(!strcmp(Argument1, "ALERT_PIN")) {
//			GPIO_PinState status;
//			status = HAL_GPIO_ReadPin(BQ_ALERT_GPIO_Port, BQ_ALERT_Pin);
//			if(status == GPIO_PIN_SET)
//				strcpy((char *)Message, "Alert pin HIGH#\r\n");
//			else
//				strcpy((char *)Message, "Alert pin LOW#\r\n");
//			Return = HAL_OK;
//		}
//		*/
//		/*
//		 * Check the SYSCTRL1 register value
//		 */
//		else if(!strcmp(Argument1, "SYSCTRL1")) {
//			uint8_t data;
//			registerRead_bq76920(SYS_CTRL1, &data);
//			sprintf((char *)Message, "SYSCTRL1 register value: %#04x #\r\n", data);
//			Return = HAL_OK;
//		}
//		/*
//		 * Check the SYSCTRL2 register value
//		 */
//		else if(!strcmp(Argument1, "SYSCTRL2")) {
//			uint8_t data;
//			registerRead_bq76920(SYS_CTRL2, &data);
//			sprintf((char *)Message, "SYSCTRL2 register value: %#04x #\r\n", data);
//			Return = HAL_OK;
//		}
//		else{
//			strcpy((char *)Message,"[ERROR] Unknown command#\r\n");
//			Return = HAL_OK;
//		}
//	}
//	else if(strcmp(Command, "SET")==0 && i==3) {
//		/*
//		 * Change the mode of BMS controller
//		 */
//		if(!strcmp(Argument1, "BMS_MODE")) {
//			if(!strcmp(Argument2, "AUTO")) {
//				if(BMS_Mode == BMS_MODE_MANUAL) {
//					BMS_Mode = BMS_MODE_AUTO;
//					strcpy((char *)Message, "Mode successfully changed!#\r\n");
//				}
//				else
//					strcpy((char *)Message, "AUTO mode is in use already#\r\n");
//			}
//			else if(!strcmp(Argument2, "MANUAL")) {
//				if(BMS_Mode == BMS_MODE_AUTO) {
//					BMS_Mode = BMS_MODE_MANUAL;
//					strcpy((char *)Message, "Mode successfully changed!#\r\n");
//				}
//				else
//					strcpy((char *)Message, "MANUAL mode is in use already#\r\n");
//			}
//			else
//				strcpy((char *)Message, "[ERROR]Invalid argument2#\r\n");
//			Return = HAL_OK;
//		}
//		else if(BMS_Mode == BMS_MODE_MANUAL) {
//			/*
//			 * Turn on/off ADC conversion of bq76920 controller
//			 */
//			/*
//			if(!strcmp(Argument1, "ADC")) {
//				if(!strcmp(Argument2, "ENABLE")) {
//					uint8_t ctrl1;
//					registerRead_bq76920(SYS_CTRL1, &ctrl1);
//					ctrl1 = ctrl1 | ADC_EN;
//					registerWrite_bq76920(SYS_CTRL1, ctrl1);
//					strcpy((char *)Message, "ADC successfully enabled#\r\n");
//				}
//				else if(!strcmp(Argument2, "DISABLE")) {
//					uint8_t ctrl1;
//					registerRead_bq76920(SYS_CTRL1, &ctrl1);
//					ctrl1 = ctrl1 & (~ADC_EN);
//					registerWrite_bq76920(SYS_CTRL1, ctrl1);
//					strcpy((char *)Message, "ADC successfully disabled#\r\n");
//				}
//				else
//					strcpy((char *)Message, "[ERROR]Invalid argument2#\r\n");
//				Return = HAL_OK;
//			}
//			*/
//			/*
//			 * Turn on/off DSG FETs
//			 */
//			if(!strcmp(Argument1, "DSG_FET")) {
//				if(!strcmp(Argument2, "ON")) {
//					uint8_t ctrl2;
//					registerRead_bq76920(SYS_CTRL2, &ctrl2);
//					ctrl2 = ctrl2 | DSG_ON;
//					registerWrite_bq76920(SYS_CTRL2, ctrl2);
//					strcpy((char *)Message, "DSG FET successfully turned ON#\r\n");
//				}
//				else if(!strcmp(Argument2, "OFF")) {
//					uint8_t ctrl2;
//					registerRead_bq76920(SYS_CTRL2, &ctrl2);
//					ctrl2 = ctrl2 & (~DSG_ON);
//					registerWrite_bq76920(SYS_CTRL2, ctrl2);
//					strcpy((char *)Message, "DSG FET successfully turned OFF#\r\n");
//				}
//				else
//					strcpy((char *)Message, "[ERROR]Invalid argument2#\r\n");
//				Return = HAL_OK;
//			}
//			/*
//			 * Turn on/off CHG FETs
//			 */
//			else if(!strcmp(Argument1, "CHG_FET")) {
//				if(!strcmp(Argument2, "ON")) {
//					uint8_t ctrl2;
//					registerRead_bq76920(SYS_CTRL2, &ctrl2);
//					ctrl2 = ctrl2 | CHG_ON;
//					registerWrite_bq76920(SYS_CTRL2, ctrl2);
//					strcpy((char *)Message, "CHG FET successfully turned ON#\r\n");
//				}
//				else if(!strcmp(Argument2, "OFF")) {
//					uint8_t ctrl2;
//					registerRead_bq76920(SYS_CTRL2, &ctrl2);
//					ctrl2 = ctrl2 & (~CHG_ON);
//					registerWrite_bq76920(SYS_CTRL2, ctrl2);
//					strcpy((char *)Message, "CHG FET successfully turned OFF#\r\n");
//				}
//				else
//					strcpy((char *)Message, "[ERROR]Invalid argument2#\r\n");
//				Return = HAL_OK;
//			}
//			/*
//			 * Change Coloumb Counter mode
//			 */
//			/*
//			else if(!strcmp(Argument1, "CC")) {
//				if(!strcmp(Argument2, "CONTINUOUS")) {
//					uint8_t ctrl2;
//					registerRead_bq76920(SYS_CTRL2, &ctrl2);
//					ctrl2 = ctrl2 | CC_EN;
//					registerWrite_bq76920(SYS_CTRL2, ctrl2);
//					strcpy((char *)Message, "CC continuous readings successfully enabled#\r\n");
//				}
//				else if(!strcmp(Argument2, "ONESHOT")) {
//					uint8_t ctrl2;
//					registerRead_bq76920(SYS_CTRL2, &ctrl2);
//					ctrl2 = ctrl2 & (~CC_EN);
//					registerWrite_bq76920(SYS_CTRL2, ctrl2);
//					strcpy((char *)Message, "CC continuous readings successfully disabled#\r\n");
//				}
//				else
//					strcpy((char *)Message, "[ERROR]Invalid argument2#\r\n");
//				Return = HAL_OK;
//			}
//			*/
//			/*
//			 * Enable single CC reading
//			 */
//			/*
//			else if(!strcmp(Argument1, "CC_ONESHOT")) {
//				if(!strcmp(Argument2, "START")) {
//					uint8_t ctrl2;
//					registerRead_bq76920(SYS_CTRL2, &ctrl2);
//					ctrl2 = ctrl2 | CC_ONESHOT;
//					registerWrite_bq76920(SYS_CTRL2, ctrl2);
//					strcpy((char *)Message, "Single CC reading successfully enabled#\r\n");
//				}
//				else
//					strcpy((char *)Message, "[ERROR]Invalid argument2#\r\n");
//				Return = HAL_OK;
//			}
//			*/
//			/*
//			 * Write to PROTECT1 register
//			 */
//			/*
//			else if(!strcmp(Argument1, "PROTECT1")) {
//				uint8_t length = strlen(Argument2);
//				uint8_t i;
//				uint8_t isDigit = 1;
//				for(i=0; i<length;i++) {
//					if(!isdigit(Argument2[i]))
//						isDigit = 0;
//				}
//				if(isDigit){
//					registerWrite_bq76920(PROTECT1, atoi(Argument2));
//					strcpy((char *)Message, "PROTECT1 register successfully set#\r\n");
//				}
//				else
//					strcpy((char *)Message, "[ERROR]Invalid argument2#\r\n");
//				Return = HAL_OK;
//			}
//			*/
//			/*
//			 * Write to PROTECT2 register
//			 */
//			/*
//			else if(!strcmp(Argument1, "PROTECT2")) {
//				uint8_t length = strlen(Argument2);
//				uint8_t i;
//				uint8_t isDigit = 1;
//				for(i=0; i<length;i++) {
//					if(!isdigit(Argument2[i]))
//						isDigit = 0;
//				}
//				if(isDigit){
//					registerWrite_bq76920(PROTECT2, atoi(Argument2));
//					strcpy((char *)Message, "PROTECT2 register successfully set#\r\n");
//				}
//				else
//					strcpy((char *)Message, "[ERROR]Invalid argument2#\r\n");
//				Return = HAL_OK;
//			}
//			*/
//			/*
//			 * Write to PROTECT3 register
//			 */
//			/*
//			else if(!strcmp(Argument1, "PROTECT3")) {
//				uint8_t length = strlen(Argument2);
//				uint8_t i;
//				uint8_t isDigit = 1;
//				for(i=0; i<length;i++) {
//					if(!isdigit(Argument2[i]))
//						isDigit = 0;
//				}
//				if(isDigit){
//					registerWrite_bq76920(PROTECT3, atoi(Argument2));
//					strcpy((char *)Message, "PROTECT3 register successfully set#\r\n");
//				}
//				else
//					strcpy((char *)Message, "[ERROR]Invalid argument2#\r\n");
//				Return = HAL_OK;
//			}
//			*/
//			/*
//			 * Write to OV_TRIP register
//			 */
//			/*
//			else if(!strcmp(Argument1, "OV_TRIP")) {
//				uint8_t length = strlen(Argument2);
//				uint8_t i;
//				uint8_t isDigit = 1;
//				for(i=0; i<length;i++) {
//					if(!isdigit(Argument2[i]))
//						isDigit = 0;
//				}
//				if(isDigit){
//					registerWrite_bq76920(OV_TRIP, atoi(Argument2));
//					strcpy((char *)Message, "OV_TRIP register successfully set#\r\n");
//				}
//				else
//					strcpy((char *)Message, "[ERROR]Invalid argument2#\r\n");
//				Return = HAL_OK;
//			}
//			*/
//			/*
//			 * Write to UV_TRIP register
//			 */
//			/*
//			else if(!strcmp(Argument1, "UV_TRIP")) {
//				uint8_t length = strlen(Argument2);
//				uint8_t i;
//				uint8_t isDigit = 1;
//				for(i=0; i<length;i++) {
//					if(!isdigit(Argument2[i]))
//						isDigit = 0;
//				}
//				if(isDigit){
//					registerWrite_bq76920(UV_TRIP, atoi(Argument2));
//					strcpy((char *)Message, "UV_TRIP register successfully set#\r\n");
//				}
//				else
//					strcpy((char *)Message, "[ERROR]Invalid argument2#\r\n");
//				Return = HAL_OK;
//			}
//			*/
//			/*
//			 * Write to CELLBAL1 register
//			 */
//			/*
//			else if(!strcmp(Argument1, "CELLBAL1")) {
//				uint8_t length = strlen(Argument2);
//				uint8_t i;
//				uint8_t isDigit = 1;
//				for(i=0; i<length;i++) {
//					if(!isdigit(Argument2[i]))
//						isDigit = 0;
//				}
//				if(isDigit){
//					registerWrite_bq76920(CELLBAL1, atoi(Argument2));
//					strcpy((char *)Message, "CELLBAL1 register successfully set#\r\n");
//				}
//				else
//					strcpy((char *)Message, "[ERROR]Invalid argument2#\r\n");
//				Return = HAL_OK;
//			}
//			*/
//		}
//		else
//			strcpy((char *)Message, "[ERROR]Command not available in AUTO mode#\r\n");
//	}
//	else if(strcmp(Command, "RESET")==0 && i==0) {
//
//	}
//	else {
//		strcpy((char *)Message,"[Error] Unknown command#\r\n");
//		Return = HAL_OK;
//	}

	return Return;
}
