/*
 * sim808.c
 *
 *  Created on: 31. kol 2018.
 *      Author: User
 */

#include "sim808.h"

/* Private define ------------------------------------------------------------*/
#define MAX_NUM_SIZE 30
#define NUM_TYPE_SIZE 3
#define SIM808_ENTER ("\r\n")
#define SEND_SMS (0x1A)		/* CTRL + Z */

/* Private typedef -----------------------------------------------------------*/
typedef ErrorType_t (SIM808_parseResp)(char *);
typedef enum {
    Unknown_type = 129,
    National_type = 161,
    International_type = 145,
    Net_specific_type = 177
} num_type_t;

struct sim808_req {
    const char *cmd;
    const char *resp;
    SIM808_checkResp *cb;
    SIM808_parseResp *parse_cb;
};

struct number_t {
    num_type_t type;
    char num[MAX_NUM_SIZE];
} __attribute__((__packed__));

enum {
    SIM808_SAVE_SETTINGS = 0,
    DISABLE_ECHO,
    SIM808_PING,
    SIM808_CHECK_SIM,
    SIM808_REG_HOME_NETWORK,
    SIM808_CHECK_HOME_NETWORK,
    SET_SMS_FORMAT,
    CHECK_SMS_FORMAT,
    SMS_CENTER_SET,
    SIM808_PRESENT_ID,
    SIM808_HANG_UP,
    ENTER_SLEEP_MODE,
    EXIT_SLEEP_MODE,
    SET_NUMBER,
    SMS_TEXT,
    SMS_READ
};

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim4;
xQueueHandle inputQueue;
xTaskHandle  recieveTaskHandle;
xSemaphoreHandle CDCMutex;
bool numberMemorized;
struct number_t number;

uint32_t down_ticks;
uint32_t up_ticks;
uint32_t tim4_ticks;

/* Private function prototypes -----------------------------------------------*/
static void SIM808_PowerOn();
static bool SIM808_GetSMS();
static ErrorType_t SIM808_parseSMS(char *, char *, size_t);
static void SIM808_readSMS(char *, char *, size_t);
static bool SIM808_GetNumber();
static ErrorType_t SIM808_parseNumber(char *, struct number_t *);
static void SIM808_memorizeNumber();
static bool SIM808_readEEPROMNumber();
static void SIM808_forgetNumber();
static bool isNumberMemorized();
static ErrorType_t SIM808_Flush_out();
static ErrorType_t SIM808_DisableEcho();
static ErrorType_t SIM808_Ping();
static ErrorType_t SIM808_CheckSIMCard();
static ErrorType_t SIM808_RegHomeNetwork();
static ErrorType_t SIM808_CheckHomeNetwork();
static ErrorType_t SIM808_SetSMSCenter();
static ErrorType_t SIM808_SetTextMode();
static ErrorType_t SIM808_PresentCallID();
static ErrorType_t SIM808_HangUp();
static ErrorType_t SIM808_SleepMode(uint32_t);
static ErrorType_t SIM808_SendAT(const char *, uint8_t, uint16_t);
static ErrorType_t SIM808_check_resp(char *, uint8_t);
static ErrorType_t SIM808_CREG_resp(char *);
static ErrorType_t SIM808_CMGF_resp(char *);
static void SIM808_Timer4Start();
static void SIM808_Timer4Stop();
static void SIM808_EnableIRQs();

static const struct sim808_req sim808_req[] = {
        {"AT&W", "OK", SIM808_check_resp, NULL},
        {"ATE0", "OK", SIM808_check_resp, NULL},     /* ATE0 */
        {"AT", "OK", SIM808_check_resp, NULL},        /* AT */
        {"AT+CCID=?", "OK", SIM808_check_resp, NULL},        /* AT+CCID=? */
        {"AT+CREG=1", "OK", SIM808_check_resp, NULL},     /* AT+CREG=1 */
        {"AT+CREG?", "+CREG: ", SIM808_check_resp, SIM808_CREG_resp},      /* AT+CREG? */
        {"AT+CMGF=1", "OK", SIM808_check_resp, NULL},                /* SMS Text mode */
        {"AT+CMGF?", "+CMGF:", SIM808_check_resp, SIM808_CMGF_resp},
        {"AT+CSCA=", "OK", SIM808_check_resp, NULL},   /* Set SMS Center */
        {"AT+CLIP=1", "OK", SIM808_check_resp, NULL},    /* Present call number */
        {"ATH", "OK", SIM808_check_resp, NULL},          /* Hang up current call */
        {"AT+CSCLK=1", "OK", SIM808_check_resp, NULL},   /* Enter Sleep Mode */
        {"AT+CSCLK=0", "OK", SIM808_check_resp, NULL},   /* Exit Sleep Mode */
        {"AT+CMGS=", ">", SIM808_check_resp, NULL},                  /* Set number */
        {NULL, NULL, NULL, NULL},                      /* SMS TEXT*/
        {"AT+CMGR=", NULL, NULL, NULL}
};

ErrorType_t
SIM808_Init(void)
{
    ErrorType_t status = Ok;
    int8_t attempts = 3;
    UART_Enable(RX_EN, TX_EN);
    SIM808_PowerOn();
    //	if (!(numberMemorized = SIM808_readEEPROMNumber())) {
    //		SIM808_forgetNumber();
    //	}
    do {
        vTaskDelay(40);
        if ((status = SIM808_DisableEcho())) continue;
        if ((status = SIM808_Flush_out())) continue;
        if ((status = SIM808_CheckSIMCard())) continue;
        if ((status = SIM808_RegHomeNetwork())) continue;
        if ((status = SIM808_CheckHomeNetwork())) continue;
        if ((status = SIM808_SetSMSCenter())) continue;
        if ((status = SIM808_SetTextMode())) continue;
        if ((status = SIM808_PresentCallID())) continue;
        if (status == Ok) break;
    } while (attempts--);
    if (attempts < 0)
    {
        DEBUG_ERROR("SIM808 not initiated!");
        assert_param(0);
    } else
    {
        DEBUG_INFO("SIM808 initiated!");
        SIM808_EnableIRQs();
    }
    return status;
}

void
SIM808_DeInit(void)
{
    SIM808_GPIODeInit();
    UART_DeInit();
}

void
SIM808_handleSMS(void)
{
    /* Get CMTI input */
    if (SIM808_GetSMS())
    {
        //SIM808_SendSMS("Hello World!");
        /* Start Measurement task */
        _setSignal(ADCTask, BIT_2);
    }
}

void
SIM808_handleCall(void)
{
    /* Get CLIP input while RING */
    if (SIM808_GetNumber())
    {
        //SIM808_SendSMS("Hello World!");
        /* Start Measurement task */
        _setSignal(ADCTask, BIT_2);
    }
}

void
SIM808_GPIOInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, SIM_PWR_Pin | SIM_DTR_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pins : LED_STAT_Pin SIM_PWR_Pin SIM_DTR_Pin ADC_PWR_Pin */
    GPIO_InitStruct.Pin = SIM_PWR_Pin | SIM_DTR_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Prepare Init Struct for next pin */
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitTypeDef));

    /*Configure GPIO pin : SIM_RI_Pin */
    GPIO_InitStruct.Pin = SIM_RI_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SIM_RI_GPIO_Port, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);

    // Init timer
    __HAL_RCC_TIM4_CLK_ENABLE();

    /* Period is 1s */
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = (uint32_t)(SystemCoreClock / 1000) - 1;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 1200 - 1;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim4);

    HAL_NVIC_SetPriority(TIM4_IRQn, 8, 0);
}

void
SIM808_GPIODeInit(void)
{
    HAL_GPIO_DeInit(GPIOB, SIM_PWR_Pin|SIM_DTR_Pin);
    HAL_GPIO_DeInit(SIM_RI_GPIO_Port, SIM_RI_Pin);

    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
}
/*
 * @brief: return true if pin is RESET.
 */
bool
SIM808_RI_active(void)
{
    return !HAL_GPIO_ReadPin(GPIOB, SIM_DTR_Pin);
}

void
SIM808_wakeUp(void)
{
    /* Pull down DTR pin */
    HAL_GPIO_WritePin(GPIOB, SIM_DTR_Pin, GPIO_PIN_RESET);

    vTaskDelay(40);
    SIM808_Ping();
    SIM808_SleepMode(EXIT_SLEEP_MODE);
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
SIM808_SendAT(const char *msg, uint8_t req_id, uint16_t timeout)
{
    const struct sim808_req *req = &sim808_req[req_id];
    char at_cmd[MAX_AT_CMD_LEN + 1] = {0};
    char *p = at_cmd;
    int nchars = 0;
    int maxlen = MAX_AT_CMD_LEN;

    if (req_id > MAXCOUNT(sim808_req))
    {
        return SIM808_Error;
    }
    if (req->cmd != NULL)
    {
        /* Prepare AT command */
        nchars = put_data(&p, req->cmd, maxlen);
        maxlen -= nchars;
    }
    if (msg != NULL)
    {
        /* Prepare additional AT command data */
        nchars = put_data(&p, msg, maxlen);
        maxlen -= nchars;
    }
    if (req_id == SMS_TEXT)
    {
        /* If SMS should be sent assure that there is enough space */
        if (maxlen < 1)
        {
            return SIM808_Error;
        }
        PUT_BYTE(p, SEND_SMS);  /* CTRL + Z to send SMS */
    }
    else
    {
        /* If any other command should be sent assure that there is enough space */
        if (maxlen < 2)
        {
            return SIM808_Error;
        }
        nchars = put_data(&p, SIM808_ENTER, 2);
    }
    return UART_Send((uint8_t *)at_cmd, timeout, req_id, req->cb);
}

static ErrorType_t
SIM808_Flush_out(void)
{
    DEBUG_INFO("Flushing SIM808 out...");
    vTaskDelay(500);
    SIM808_Ping(NULL, SIM808_PING, 100);
    vTaskDelay(500);
    SIM808_Ping(NULL, SIM808_PING, 100);
    vTaskDelay(3100);
    SIM808_Ping(NULL, SIM808_PING, 100);
    vTaskDelay(1000);
    SIM808_Ping(NULL, SIM808_PING, 100);
    return Ok;
}

static ErrorType_t
SIM808_DisableEcho(void)
{
    DEBUG_INFO("Disabling echo...");
    return SIM808_SendAT(NULL, DISABLE_ECHO, 100);
}

static ErrorType_t
SIM808_Ping(void)
{
    DEBUG_INFO("Checking communication...");
    return SIM808_SendAT(NULL, SIM808_PING, 100);
}

static ErrorType_t
SIM808_CheckSIMCard(void)
{
    DEBUG_INFO("Checking SIM card...");
    return SIM808_SendAT(NULL, SIM808_CHECK_SIM, 100);
}

static ErrorType_t
SIM808_RegHomeNetwork(void)
{
    DEBUG_INFO("Register Home Network...");
    return SIM808_SendAT(NULL, SIM808_REG_HOME_NETWORK, 500);
}

static ErrorType_t
SIM808_CheckHomeNetwork(void)
{
    DEBUG_INFO("Checking network...");
    return SIM808_SendAT(NULL, SIM808_CHECK_HOME_NETWORK, 100);
}

static ErrorType_t
SIM808_SetSMSCenter(void)
{
    DEBUG_INFO("Setting SMS center: \"+385910401\"");
    return SIM808_SendAT("\"+385910401\"", SMS_CENTER_SET, 100);
}

static ErrorType_t
SIM808_SetTextMode(void)
{
    DEBUG_INFO("Setting SMS mode to txt.");
    return SIM808_SendAT(NULL, SET_SMS_FORMAT, 100);
}

static
ErrorType_t SIM808_PresentCallID(void)
{
    DEBUG_INFO("Present call number when receiving call...");
    return SIM808_SendAT(NULL, SIM808_PRESENT_ID, 100);
}

static ErrorType_t
SIM808_HangUp(void)
{
    DEBUG_INFO("Hanging up the call...");
    return SIM808_SendAT(NULL, SIM808_HANG_UP, 1000);
}

static ErrorType_t
SIM808_SleepMode(uint32_t cmd)
{
    return SIM808_SendAT(NULL, cmd, 100);
}

static ErrorType_t
SIM808_check_resp(char *resp, uint8_t req_id)
{
    ErrorType_t status = Ok;
    const struct sim808_req *req;
    char *p = resp;

    if (req_id > MAXCOUNT(sim808_req))
    {
        return SIM808_Error;
    }
    req = &sim808_req[req_id];
    if (STR_COMPARE(resp, req->resp))
    {
        return SIM808_Error;
    }
    if (req->parse_cb)
    {
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

static bool
SIM808_GetSMS(void)
{
    ErrorType_t status = SIM808_Error;
    char CMTI_resp[MAX_COMMAND_INPUT_LENGTH] = {0};
    char sms[100 + 1] = {0};
    UART_GetData(CMTI_resp);
    debug_printf("%s\r\n", CMTI_resp);
    status = SIM808_parseSMS(CMTI_resp, sms, 100);
    debug_printf("trebao bih isprintati sms: %s\r\n", sms);
    return false;
    //status = SIM808_parseNumber(CLIP_resp, &CallNumber);
}

static ErrorType_t
SIM808_parseSMS(char *resp, char *sms, size_t sms_max_len)
{
    char *pos;
    char *sms_index;
    pos = strstr(resp, "+CMTI: ");
    if (pos == NULL)
    {
        DEBUG_ERROR("SMS not received!");
        return false;
    }
    sms_index = strchr(pos, ',');
    sms_index++;
    SIM808_readSMS(sms, sms_index, sms_max_len);
    return true;
}

static void
SIM808_readSMS(char *sms, char *sms_index, size_t sms_max_len)
{
    char buf[256] = {0};
    char *pos;
    char *token[8 + 1] = {NULL};
    char delim[] = "\"";
    int i = 0;
    size_t sms_len = 0;
    SIM808_SendAT(sms_index, SMS_READ, 100);
    UART_GetData(buf);
    debug_printf("%s\r\n", buf);
    pos = strstr(buf, "+CMGR: ");
    if (pos == NULL)
    {
        debug_printf("going out..\r\n");
        return;
    }
    token[i] = strtok(pos, delim);
    while (token[i] != NULL)
    {
        token[++i] = strtok(NULL, delim);
    }
    if (strcmp(token[0], "+CMGR: ") != 0)
    {
        debug_printf("NOK\r\n");
        return;
    }
    if (strstr(number.num, token[3]) == NULL)
    {
        /* Unknown number */
        DEBUG_INFO("SMS from unauthorized number!");
        return;
    }
    sms_len = strlen(token[7]);
    token[7][sms_len - 2] = '\0';
    memmove(sms, token[7], sms_max_len);
}

/* Ex. +CLIP: "+989108793902",145,"",0,"",0 */
static ErrorType_t
SIM808_parseNumber(char *resp, struct number_t *num)
{
    char *pos, *start, *end;
    char type[NUM_TYPE_SIZE + 1] = {0};
    uint8_t len;
    pos = strstr(resp, "+CLIP: ");
    if (pos == NULL)
    {
        return SIM808_Error;
    }
    start = strchr(pos, '"');
    end = strchr(start, ',');
    len = end - start;
    if (len >= MAX_NUM_SIZE)
    {
        return SIM808_Error;
    }
    /* num is inside "" */
    strncpy(num->num, start, (size_t)len);
    /* Get number type */
    end++;
    strncpy(type, end, NUM_TYPE_SIZE);
    num->type = atoi(type);
    return Ok;
}

static bool
SIM808_GetNumber(void)
{
    ErrorType_t status = SIM808_Error;
    char CLIP_resp[MAX_COMMAND_INPUT_LENGTH] = {0};
    struct number_t CallNumber = {.type = 0, .num = {0}};
    UART_GetData(CLIP_resp);
    debug_printf("%s\r\n", CLIP_resp);
    status = SIM808_parseNumber(CLIP_resp, &CallNumber);
    if (status != Ok)
    {
        DEBUG_ERROR("Can not obtain number!");
        return false;
    }
    while (SIM808_HangUp())
    {
        vTaskDelay(40); /* Until success */
    }
    if (!isNumberMemorized())
    {
        /* Memorize the number on first call */
        SIM808_memorizeNumber(&CallNumber);
    }
    /* Compare config_read number and CallNumber */
    if (STR_COMPARE(number.num, CallNumber.num))
    {
        /* Unknown number */
        DEBUG_INFO("Call from unauthorized number!");
        return false;
    }
    debug_printf("Call from %s:%d\r\n", CallNumber.num, CallNumber.type);
    return true;
}

static void
SIM808_memorizeNumber(struct number_t *num)
{
    /* Only if number is not memorized */
    if (isNumberMemorized())
    {
        return;
    }
    //	if (!storage_write((void *)num, STORAGE_NUMBER_ADDR, sizeof(struct number_t))) {
    //		/* Write failed */
    //		return;
    //	}
    memcpy(&number, num, sizeof(struct number_t));
    numberMemorized = true;
}

static bool
SIM808_readEEPROMNumber(void)
{
    /* Get data from EEPROM */
    storage_read(&number, STORAGE_NUMBER_ADDR, sizeof(struct number_t));

    /* Validate number by number type */
    switch(number.type) {
    case Unknown_type:
    case Net_specific_type:
        return false;
    case National_type:
    case International_type:
        break;
    default:
        return false;
    }
    return true;
}

static void
SIM808_forgetNumber(void)
{
    memset(&number, 0, sizeof(struct number_t));
}

ErrorType_t
SIM808_SendSMS(char *message)
{
    ErrorType_t status = Ok;
    if ((status = SIM808_SendAT(number.num, SET_NUMBER, 1000)))
    {
        vTaskDelay(100);
        //return status;
    }
    vTaskDelay(3000);
    if ((status = SIM808_SendAT(message, SMS_TEXT, 1000)))
    {
        return status;
    }
    return status;
}

static bool
isNumberMemorized(void)
{
    return numberMemorized == true;
}

static void
SIM808_Timer4Start(void)
{
    /* Clear interrupt flags */
    __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);
    __HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);
    /* Start the timer */
    htim4.Instance->CNT = 0;
    HAL_TIM_Base_Start_IT(&htim4);
}

static void
SIM808_Timer4Stop(void)
{
    HAL_TIM_Base_Stop_IT(&htim4);
}

static void
SIM808_EnableIRQs(void)
{
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
}

/**
 * @brief EXTI line detection callbacks
 * @param GPIO_Pin: Specifies the pins connected EXTI line
 * @retval None
 */
void
HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t ticks = 0;
    if (GPIO_Pin == SIM_RI_Pin)
    {
        if (HAL_GPIO_ReadPin(GPIOB, SIM_RI_Pin) == GPIO_PIN_RESET)
        {
            /* Remember timestamp */
            ticks = HAL_GetTick();
            /* Start the timer when SIM_RI goes low */
            SIM808_Timer4Start();
        }
        else
        {
            /* Stop the timer when SIM_RI goes back (high) */
            SIM808_Timer4Stop();
            if ((HAL_GetTick() - ticks) < 120)
            {
                /* If SIM_RI was less than 120ms low, SIM808 received an SMS */
                _setSignal(CALLTask, SIM_RI_IRQ_SMS);
            }
            /* Restart the timestamp */
            ticks = 0;
        }
    }
}

/*
 * @brief: If timer kicks the timeout, SIM808 is receiving the call
 *         Timeout is 1200ms
 */
void
TIM4_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&htim4, TIM_FLAG_UPDATE) != RESET)
    {
        __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);
        __HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);
        HAL_NVIC_ClearPendingIRQ(TIM4_IRQn);

        SIM808_Timer4Stop();
        /* Give signal to Call task */
        _setSignal(CALLTask, SIM_RI_IRQ_CALL);
    }
}
