/*
 * sim808.c
 *
 *  Created on: 31. kol 2018.
 *      Author: User
 */

#include "sim808.h"

/* Private define ------------------------------------------------------------*/
#define MAX_NUM_SIZE 30
#define SIM808_ENTER ("\r\n")
#define SEND_SMS (0x1A)		/* CTRL + Z */

/* Private typedef -----------------------------------------------------------*/
typedef ErrorType_t (SIM808_parseResp)(char *);

struct sim808_req {
	const char *cmd;
	const char *resp;
	SIM808_checkResp *cb;
	SIM808_parseResp *parse_cb;
};

enum {
	DISABLE_ECHO = 0,
	SIM808_HANDSHAKE,
	SIM808_CHECK_SIM,
	SIM808_REG_HOME_NETWORK,
	SIM808_CHECK_HOME_NETWORK,
	PING,
	SMS_CENTER_SET,
	SET_NUMBER,
	SMS_TEXT
};

/* Private variables ---------------------------------------------------------*/
xQueueHandle inputQueue;
xTaskHandle  recieveTaskHandle;
xSemaphoreHandle CDCMutex;
char number[MAX_NUM_SIZE];

/* Private function prototypes -----------------------------------------------*/
static void SIM808_PowerOn();
static void SIM808_SetNumber();
static bool SIM808_GetNumber();
static ErrorType_t SIM808_DisableEcho();
static ErrorType_t SIM808_Handshake();
static ErrorType_t SIM808_CheckSIMCard();
static ErrorType_t SIM808_RegHomeNetwork();
static ErrorType_t SIM808_CheckHomeNetwork();
static ErrorType_t SIM808_SetSMSCenter();
static ErrorType_t SIM808_SendAT(char *, uint8_t, uint16_t);
static ErrorType_t SIM808_check_resp(char *, uint8_t);
static ErrorType_t SIM808_CREG_resp(char *);
static ErrorType_t SIM808_CMGF_resp(char *);

static const struct sim808_req sim808_req[] = {
	{"AT&W", "OK", SIM808_check_resp, NULL},
	{"ATE0", "OK", SIM808_check_resp, NULL},     /* ATE0 */
	{"AT", "OK", SIM808_check_resp, NULL},        /* AT */
	{"AT+CCID=?", "OK", SIM808_check_resp, NULL},        /* AT+CCID=? */
	{"AT+CREG=1", "OK", SIM808_check_resp, NULL},     /* AT+CREG=1 */
	{"AT+CREG?", "+CREG: ", SIM808_check_resp, SIM808_CREG_resp},      /* AT+CREG? */
	{"AT+CMGF=1", "OK", SIM808_check_resp, NULL},                /* SMS Text mode */
	{"AT+CMGF?", "+CMGF:", SIM808_check_resp, SIM808_CMGF_resp},
	{"AT+CSCA=\"+385910401\"", "OK", SIM808_check_resp},   /* Set SMS Center */
	{"AT+CMGS=", ">", SIM808_check_resp, NULL},                  /* Set number */
	{NULL, NULL, NULL, NULL, NULL, NULL}                      /* SMS TEXT*/
};


ErrorType_t
SIM808_Init(void)
{
	ErrorType_t status = Ok;
	uint8_t attempts = 5;
	/* Init peripheralas */

	SIM808_PowerOn();
	SIM808_GetNumber();
	do {
		if ((status = SIM808_DisableEcho())) continue;
		if ((status = SIM808_Handshake())) continue;
		if ((status = SIM808_CheckSIMCard())) continue;
		if ((status = SIM808_RegHomeNetwork())) continue;
		if ((status = SIM808_CheckHomeNetwork())) continue;
		if ((status = SIM808_SetSMSCenter())) continue;
		if (status == Ok) break;
	} while (attempts--);
	return status;
}

void
SIM808_DeInit(void)
{
	SIM808_GPIODeInit();
	UART_DeInit();
}


void
SIM808_handleCall(char *input)
{
	/* Get CLIP input while RING */
	if (SIM808_GetNumber()) {
		/* Start Measurement task */
		_setSignal(ADCTask, BIT_2);
	}
}

void
SIM808_GPIOInit(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, SIM_PWR_Pin|SIM_DTR_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pins : LED_STAT_Pin SIM_PWR_Pin SIM_DTR_Pin ADC_PWR_Pin */
	GPIO_InitStruct.Pin = SIM_PWR_Pin|SIM_DTR_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : SIM_RI_Pin */
	GPIO_InitStruct.Pin = SIM_RI_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(SIM_RI_GPIO_Port, &GPIO_InitStruct);

	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void
SIM808_GPIODeInit(void)
{
	HAL_GPIO_DeInit(GPIOB, SIM_PWR_Pin|SIM_DTR_Pin);
	HAL_GPIO_DeInit(SIM_RI_GPIO_Port, SIM_RI_Pin);

	HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
}

static void
SIM808_PowerOn(void)
{
	HAL_GPIO_WritePin(SIM_PWR_GPIO_Port, SIM_PWR_Pin, GPIO_PIN_SET);
	vTaskDelay(1500);
	HAL_GPIO_WritePin(SIM_PWR_GPIO_Port, SIM_PWR_Pin, GPIO_PIN_RESET);
	vTaskDelay(1500);
}

static ErrorType_t
SIM808_SendAT(char *msg, uint8_t req_id, uint16_t timeout)
{
	char data[256];
	char *p = data;
	if (req_id > MAXCOUNT(sim808_req)) {
		return SIM808_Error;
	}
	const struct sim808_req *req = &sim808_req[req_id];
	PUT_DATA(p, req->cmd);
	PUT_DATA(p, msg);
	PUT_DATA(p, SIM808_ENTER);  /* \r\n to send AT command */
	if (req_id == SMS_TEXT) {
		PUT_BYTE(p, SEND_SMS);  /* CTRL + Z to send SMS */
	}
	return UART_Send((uint8_t *)data, timeout, req_id, req->cb);
}

static ErrorType_t
SIM808_DisableEcho(void)
{
	return SIM808_SendAT(NULL, DISABLE_ECHO, 100);
}

static ErrorType_t
SIM808_Handshake(void)
{
	return SIM808_SendAT(NULL, SIM808_HANDSHAKE, 100);
}

static ErrorType_t
SIM808_CheckSIMCard(void)
{
	return SIM808_SendAT(NULL, SIM808_CHECK_SIM, 100);
}

static ErrorType_t
SIM808_RegHomeNetwork(void)
{
	return SIM808_SendAT(NULL, SIM808_REG_HOME_NETWORK, 500);
}

static ErrorType_t
SIM808_CheckHomeNetwork(void)
{
	return SIM808_SendAT(NULL, SIM808_CHECK_HOME_NETWORK, 100);
}

static ErrorType_t
SIM808_SetSMSCenter(void)
{
	return SIM808_SendAT(NULL, SMS_CENTER_SET, 100);
}

static ErrorType_t
SIM808_check_resp(char *resp, uint8_t req_id)
{
	ErrorType_t status = Ok;
	const struct sim808_req *req;
	char *p = resp;

	if (req_id > MAXCOUNT(sim808_req)) {
		return SIM808_Error;
	}
	req = &sim808_req[req_id];
	if (STR_COMPARE(resp, req->resp)) {
		return SIM808_Error;
	}
	if (req->parse_cb) {
		p += strlen(req->resp);
		status = req->parse_cb(p);
	}
	return status;
}

static ErrorType_t
SIM808_CREG_resp(char *resp)
{
	ErrorType_t status = Ok;
	CREG_resp_t s;
	s = TO_DIGIT(*resp);
	switch (s) {
	case NOT_REGISTERED:
		status = SIM808_Error;
		break;
	case REGISTERED:
		status = Ok;
		break;
	case SEARCHING_OPERATOR:
	case REGISTRATION_DENIED:
	case UNKNOWN:
	case ROAMING:
	default:
		status = SIM808_Error;
		break;
	}
	return status;
}

static ErrorType_t
SIM808_CMGF_resp(char *resp)
{
	ErrorType_t status = Ok;
	CMGF_resp_t s;
	s = TO_DIGIT(*resp);
	switch (s) {
	case PDU_MODE:
		status = SIM808_Error;
		break;
	case TEXT_MODE:
		status = Ok;
		break;
	default:
		status = SIM808_Error;
		break;
	}
	return status;
}

/* Ex. +CLIP: "+989108793902",145,"",0,"",0 */
static void
SIM808_parseNumber(char *resp, char *num)
{
	char *start, *end;
	uint8_t len;
	if (!strstr(resp, "+CLIP: ")) {
		return;
	}
	start = strchr(resp, '"');
	start++;
	end = strchr(start, '"');
	len = end - start;
	strncpy(num, start, (size_t)len + 1);
	num[len] = '\0';
}

static bool
SIM808_GetNumber(void)
{
	char CLIP_Data[MAX_COMMAND_INPUT_LENGTH] = {0};
	char CallNumber[MAX_NUM_SIZE] = {0};

	UART_GetData(CLIP_Data);
	SIM808_parseNumber(CLIP_Data, CallNumber);

	/* Compare config_read number and CallNumer */
	if (!strstr(number, CallNumber)) {
		/* Unknown number */
		return false;
	}
	return true;
}

static void
SIM808_SetNumber(void)
{
	storage_write(number, STORAGE_NUMBER_ADDR, strlen(number));
}

ErrorType_t
SIM808_SendSMS(char *message)
{
	ErrorType_t status = Ok;
	if ((status = SIM808_SendAT(number, SET_NUMBER, 200))) {
		return status;
	}
	vTaskDelay(1000);
	if ((status = SIM808_SendAT(message, SMS_TEXT, 1000))) {
		return status;
	}
	return status;
}

/**
  * @brief EXTI line detection callbacks
  * @param GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == SIM_RI_Pin)
  {

  }
}
