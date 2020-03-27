/*
 * sim808.c
 *
 *  Created on: 31. kol 2018.
 *      Author: User
 */

#include "sim808.h"
#include "json.h"

/* Private define ------------------------------------------------------------*/
#define SIM808_ENTER ("\r\n")
#define CTRL_Z (0x1A)		/* CTRL + Z */

#define HTTP_URL "\"URL\",\"http://0.tcp.ngrok.io:17908\"" /* /api/v1/site/new */
#define NTP_SERVER  "216.239.35.0"

/* Private typedef -----------------------------------------------------------*/
typedef ErrorType_t (SIM808_parseResp)(char *);

struct sim808_req {
    const char *cmd;
    const char *resp;
    SIM808_checkResp *cb;
    SIM808_parseResp *parse_cb;
};

struct user_phone_number_t {
    num_type_t type;
    char num[MAX_NUM_SIZE];
} __attribute__((__packed__));

struct local_time_t {
    char year[2];
    char month[2];
    char day[2];
    char hour[2];
    char minute[2];
    char second[2];
};

typedef enum {
    NONE = 0,
    HTTP_POST,
    HTTP_GET,
    TCP_REQUEST
} SMS_action_t;

enum {
    SIM808_SAVE_SETTINGS = 0,
    SIM808_DISABLE_ECHO,
    SIM808_PING,
    SIM808_TO_COMMAND_MODE,
    SIM808_TO_DATA_MODE,
    SIM808_CHECK_SIM,
    SIM808_REG_HOME_NETWORK,
    SIM808_CHECK_HOME_NETWORK,
    SIM808_SMS_SET_FORMAT,
    SIM808_SMS_CHECK_FORMAT,
    SIM808_SMS_CENTER_SET,
    SIM808_PRESENT_ID,
    SIM808_HANG_UP,
    SIM808_ENTER_SLEEP_MODE,
    SIM808_EXIT_SLEEP_MODE,
    SIM808_SMS_SET_NUMBER,
    SIM808_SMS_TEXT,
    SIM808_SMS_DELETE,
    SIM808_SMS_READ,
    SIM808_TCP_IP_CONNECTION_MODE,
    SIM808_TCP_IP_APPLICATION_MODE,
    SIM808_GET_GPRS_STATUS,
    SIM808_SET_APN,
    SIM808_BRING_UP_GPRS,
    SIM808_GET_LOCAL_IP,
    SIM808_TCP_ENABLE_SSL,
    SIM808_DEACTIVATE_GRPS_CONTEXT,
    SIM808_START_TCP_IP_CONN,
    SIM808_CLOSE_TCP_IP_CONN,
    SIM808_PREPARE_TCP_PACKET,
    SIM808_SEND_TCP_PACKET,
    SIM808_BEARER_CONFIGURE,
    SIM808_BEARER_OPEN_GPRS,
    SIM808_BEARER_CLOSE_GPRS,
    SIM808_HTTP_INIT,
    SIM808_HTTP_ENABLE_SSL,
    SIM808_HTTP_SET_PARAM,
    SIM808_HTTP_GET,
    SIM808_HTTP_READ_SERVER_DATA,
    SIM808_HTTP_PREPARE_HTTP_POST,
    SIM808_HTTP_UPLOAD_POST_DATA,
    SIM808_HTTP_POST,
    SIM808_HTTP_TERMINATE,
    SIM808_NTP_CONFIGURE_BEARER,
    SIM808_NTP_SETUP_SERVICE,
    SIM808_NTP_START_SERVICE,
    SIM808_GET_TIME
};

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim4;
xQueueHandle inputQueue;
xTaskHandle  recieveTaskHandle;
xSemaphoreHandle CDCMutex;
bool numberMemorized;
struct user_phone_number_t number;

uint32_t down_ticks;
uint32_t up_ticks;
uint32_t tim4_ticks;

/* Private function prototypes -----------------------------------------------*/
static void SIM808_PowerOn();
static SMS_action_t SIM808_get_sms_action();
static void SIM808_parseSMS(char *, char *, size_t);
static void SIM808_readSMS(char *, char *, size_t);
static bool SIM808_GetNumber();
static ErrorType_t SIM808_parseNumber(char *, struct user_phone_number_t *);
static void SIM808_memorizeNumber();
static bool SIM808_save_number(struct user_phone_number_t *, storage_type_t);
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
static ErrorType_t SIM808_SMS_Delete(char *);
static ErrorType_t SIM808_PresentCallID();
static ErrorType_t SIM808_Get_GPRS_Status();
static ErrorType_t SIM808_Set_APN();
static ErrorType_t SIM808_BringUp_GPRS();
static ErrorType_t SIM808_Get_Local_IP();
static ErrorType_t SIM808_TCP_Enable_SSL();
static ErrorType_t SIM808_Deactivate_GRPS_Context();
static ErrorType_t SIM808_Start_TCP_Conn(char *);
static ErrorType_t SIM808_Close_TCP_Conn();
static ErrorType_t SIM808_Configure_Bearer();
static ErrorType_t SIM808_Bearer_Configure(char *);
static ErrorType_t SIM808_Bearer_Open_GPRS_Context();
static ErrorType_t SIM808_Bearer_Close_GPRS_Context();
static ErrorType_t SIM808_HTTP_Init();
static ErrorType_t SIM808_HTTP_Enable_SSL();
static ErrorType_t SIM808_HTTP_Set_Param(char *);
static ErrorType_t SIM808_HTTP_Get();
static ErrorType_t SIM808_HTTP_read_server_data();
static ErrorType_t SIM808_HTTP_Pepare_POST_data(size_t, uint32_t);
static ErrorType_t SIM808_HTTP_Upload_POST_Data(char *);
static ErrorType_t SIM808_HTTP_Post();
static ErrorType_t SIM808_HTTP_Terminate();
static ErrorType_t SIM808_NTP_configure();
static ErrorType_t SIM808_NTP_setup_service(char *);
static ErrorType_t SIM808_NTP_start_sync();
static ErrorType_t SIM808_NTP_setup();
static ErrorType_t SIM808_Get_Time(struct local_time_t *);
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
        {"+++", "OK", SIM808_check_resp, NULL},     /* Switch to the command mode */
        {"ATO0", NULL, NULL, NULL},                 /* Switch to data mode */
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
        {"AT+CMGD=", "OK", SIM808_check_resp, NULL},   /* Delete all SMS messages */
        {"AT+CMGR=", NULL, NULL, NULL},
        {"AT+CIPMUX=", NULL, NULL, NULL},              /* Set TCP/IP Connection Mode */
        {"AT+CIPMODE", NULL, NULL, NULL},              /* Select TCP/IP Application Mode */
        {"AT+CGATT?", "+CGATT:", SIM808_check_resp, NULL},  /* Get GPRS Status */
        {"AT+CSTT=", "OK", SIM808_check_resp, NULL},    /* Set APN */
        {"AT+CIICR", "OK", SIM808_check_resp, NULL},    /* Bring up GPRS */
        {"AT+CIFSR", NULL, NULL, NULL},                 /* Get local IP address */
        {"AT+CIPSSL=1", "OK", SIM808_check_resp, NULL}, /* Enable SSL for TCP */
        {"AT+CIPSHUT", "SHUT OK", SIM808_check_resp, NULL}, /* Deactivate GPRS Context */
        {"AT+CIPSTART=", "OK", SIM808_check_resp, NULL},  /* Start up TCP/IP connection */
        {"AT+CIPCLOSE", "CLOSE OK", SIM808_check_resp, NULL},  /* Close TCP/IP connection */
        {"AT+CIPSEND", ">", SIM808_check_resp, NULL},   /* Start sending data to server */
        {NULL, "SEND OK", SIM808_check_resp, NULL},     /* Send data to server */
        {"AT+SAPBR=3,1,", "OK", SIM808_check_resp, NULL},   /* Configuring Bearer profile 1 */
        {"AT+SAPBR=1,1", "OK", SIM808_check_resp, NULL},    /* Opening GPRS Context */
        {"AT+SAPBR=0,1", "OK", SIM808_check_resp, NULL},    /* Closing GPRS Context */
        {"AT+HTTPINIT", "OK", SIM808_check_resp, NULL}, /* Init HTTP Service */
        {"AT+HTTPSSL=1", "OK", SIM808_check_resp, NULL},      /* Enable SSL for HTTP */
        {"AT+HTTPPARA=", "OK", SIM808_check_resp, NULL}, /* Set HTTP Parameter */
        {"AT+HTTPACTION=0", "OK", SIM808_check_resp, NULL},          /* GET Data */
        {"AT+HTTPREAD", NULL, NULL, NULL},         /* Read the data from HTTP server */
        {"AT+HTTPDATA=", "DOWNLOAD", SIM808_check_resp, NULL},  /* Prepare to upload HTTP data */
        {NULL, "OK", SIM808_check_resp, NULL},          /* Upload HTTP Data */
        {"AT+HTTPACTION=1", "OK", SIM808_check_resp, NULL},          /* POST Data */
        {"AT+HTTPTERM", "OK", SIM808_check_resp, NULL},      /* Terminate HTTP */
        {"AT+CNTPCID=1", "OK", SIM808_check_resp, NULL},    /* Configure NTP bearer */
        {"AT+CNTP=", "OK", SIM808_check_resp, NULL},        /* Setup NTP service */
        {"AT+CNTP", "OK+CNTP: 1", SIM808_check_resp, NULL},         /* Start NTP service */
        {"AT+CCLK?", NULL, NULL, NULL}                      /* Get current time */

};

ErrorType_t
SIM808_Init(void)
{
    ErrorType_t status = Ok;
    int8_t attempts = 3;
    UART_Enable(RX_EN, TX_EN);
    SIM808_PowerOn();
    numberMemorized = SIM808_readEEPROMNumber();
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
        if ((status = SIM808_Set_APN())) continue;
//        if ((status = SIM808_Configure_Bearer())) continue;
//        if ((status = SIM808_NTP_setup())) continue;
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
    SMS_action_t action;
    /* Get SMS action */
    action = SIM808_get_sms_action();
    switch (action) {
    case HTTP_GET:
//        SIM808_send_GET_request();
        break;
    case HTTP_POST:
//        SIM808_send_POST_request();
        break;
    case TCP_REQUEST:
//        SIM808_send_TCP_request();
        break;
    default:
        SIM808_readEEPROMNumber();
        DEBUG_ERROR("Unknown SMS action!");
        break;
    }
}

void
SIM808_handleCall(void)
{
    struct local_time_t timestamp;
    memset(&timestamp, 0, sizeof(struct local_time_t));
    /* Get CLIP input while RING */
    if (SIM808_GetNumber())
    {
        SIM808_Get_Time(&timestamp);
        debug_printf("Current time: %s-%s-%s T%s:%s:%s\r\n", timestamp.day, timestamp.month, timestamp.year, timestamp.hour, timestamp.minute, timestamp.second);
//        SIM808_SendSMS("Hello\r\nWorld\r\n!");
        /* Start Measurement task */
        _setSignal(ADCTask, SIGNAL_START_ADC);
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
    SIM808_SleepMode(SIM808_EXIT_SLEEP_MODE);
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
    if (req_id == SIM808_SMS_TEXT || req_id == SIM808_SEND_TCP_PACKET)
    {
        /* If SMS should be sent assure that there is enough space */
        if (maxlen < 1)
        {
            return SIM808_Error;
        }
        PUT_BYTE(p, CTRL_Z);  /* CTRL + Z to send message */
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
    return SIM808_SendAT(NULL, SIM808_DISABLE_ECHO, 100);
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
    return SIM808_SendAT("\"+385910401\"", SIM808_SMS_CENTER_SET, 100);
}

static ErrorType_t
SIM808_SetTextMode(void)
{
    DEBUG_INFO("Setting SMS mode to txt.");
    return SIM808_SendAT(NULL, SIM808_SMS_SET_FORMAT, 100);
}
static ErrorType_t
SIM808_SMS_Delete(char *idx)
{
    DEBUG_INFO("Deleting SMS at index: %s", idx);
    return SIM808_SendAT(idx, SIM808_SMS_DELETE, 100);
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
SIM808_Get_GPRS_Status(void)
{
    DEBUG_INFO("Checking GPRS Status...");
    return SIM808_SendAT(NULL, SIM808_GET_GPRS_STATUS, 100);
}

static ErrorType_t
SIM808_Set_APN(void)
{
    DEBUG_INFO("Setting APN...");
    return SIM808_SendAT("tomato", SIM808_SET_APN, 100);
}

static ErrorType_t
SIM808_BringUp_GPRS(void)
{
    DEBUG_INFO("Bringing GPRS up...");
    return SIM808_SendAT(NULL, SIM808_BRING_UP_GPRS, 100);
}

static ErrorType_t
SIM808_Get_Local_IP(void)
{
    char ip_addr[16] = {0};
    DEBUG_INFO("Getting Local IP address...");
    SIM808_SendAT(NULL, SIM808_GET_LOCAL_IP, 100);
    UART_GetData(ip_addr, 1000);
    DEBUG_INFO("IP address: %s", ip_addr);
    return Ok;
}

static ErrorType_t
SIM808_TCP_Enable_SSL(void)
{
    DEBUG_INFO("Activating SSL for TCP...");
    return SIM808_SendAT(NULL, SIM808_TCP_ENABLE_SSL, 100);
}

static ErrorType_t
SIM808_Deactivate_GRPS_Context(void)
{
    DEBUG_INFO("Deactivating GRPS context...");
    return SIM808_SendAT(NULL, SIM808_DEACTIVATE_GRPS_CONTEXT, 100);
}

static ErrorType_t
SIM808_Start_TCP_Conn(char *params)
{
    DEBUG_INFO("Starting TCP/IP connection...");
    return SIM808_SendAT(params, SIM808_START_TCP_IP_CONN, 100);
}

static ErrorType_t
SIM808_Close_TCP_Conn(void)
{
    DEBUG_INFO("Closing TCP/IP connection...");
    return SIM808_SendAT(NULL, SIM808_CLOSE_TCP_IP_CONN, 100);
}

ErrorType_t
SIM808_SendTCP_Packet(char *message)
{
    ErrorType_t status = Ok;
    if ((status = SIM808_SendAT(NULL, SIM808_PREPARE_TCP_PACKET, 1000)))
    {
        DEBUG_ERROR("Can not send TCP packet");
        return status;
    }
    vTaskDelay(2000);
    if ((status = SIM808_SendAT(message, SIM808_SEND_TCP_PACKET, 1000)))
    {
        return status;
    }
    return status;
}

static ErrorType_t
SIM808_Bearer_Configure(char *msg)
{
    DEBUG_INFO("Configuring Bearer profile 1...");
    return SIM808_SendAT(msg, SIM808_BEARER_CONFIGURE, 100);
}

static ErrorType_t
SIM808_Bearer_Open_GPRS_Context(void)
{
    DEBUG_INFO("Opening GPRS Context...");
    return SIM808_SendAT(NULL, SIM808_BEARER_OPEN_GPRS, 100);
}

static ErrorType_t
SIM808_Bearer_Close_GPRS_Context(void)
{
    DEBUG_INFO("Closing GPRS Context...");
    return SIM808_SendAT(NULL, SIM808_BEARER_CLOSE_GPRS, 100);
}

static ErrorType_t
SIM808_HTTP_Init(void)
{
    DEBUG_INFO("Initializing HTTP Service...");
    return SIM808_SendAT(NULL, SIM808_HTTP_INIT, 100);
}

static ErrorType_t
SIM808_HTTP_Enable_SSL(void)
{
    DEBUG_INFO("Activating SSL for HTTP Service...");
    return SIM808_SendAT(NULL, SIM808_HTTP_ENABLE_SSL, 100);
}

static ErrorType_t
SIM808_HTTP_Set_Param(char *param)
{
    DEBUG_INFO("Setting HTTP Parameter: %s...", param);
    return SIM808_SendAT(param, SIM808_HTTP_SET_PARAM, 100);
}

static ErrorType_t
SIM808_HTTP_Get(void)
{
    DEBUG_INFO("Sending HTTP GET Request...");
    return SIM808_SendAT(NULL, SIM808_HTTP_GET, 1000);
}

static ErrorType_t
SIM808_HTTP_read_server_data(void)
{
    DEBUG_INFO("Reading HTTP Server response...");
    return SIM808_SendAT(NULL, SIM808_HTTP_READ_SERVER_DATA, 100);
}

static ErrorType_t
SIM808_HTTP_Pepare_POST_data(size_t size, uint32_t max_latency_time)
{
    char msg[20] = {0};
    snprintf(msg, sizeof msg, "%d,%ld", size, max_latency_time);
    return SIM808_SendAT(msg, SIM808_HTTP_PREPARE_HTTP_POST, 100);
}

static ErrorType_t
SIM808_HTTP_Upload_POST_Data(char *msg)
{
    DEBUG_INFO("Upload HTTP data: %s...", msg);
    return SIM808_SendAT(msg, SIM808_HTTP_UPLOAD_POST_DATA, 500);
}

static ErrorType_t
SIM808_HTTP_Post(void)
{
    DEBUG_INFO("HTTP POST...");
    return SIM808_SendAT(NULL, SIM808_HTTP_POST, 100);
}

static ErrorType_t
SIM808_HTTP_Terminate(void)
{
    DEBUG_INFO("Terminating HTTP Service...");
    return SIM808_SendAT(NULL, SIM808_HTTP_TERMINATE, 100);
}

static ErrorType_t
SIM808_NTP_configure(void)
{
    DEBUG_INFO("Configuring NTP bearer...");
    return SIM808_SendAT(NULL, SIM808_NTP_CONFIGURE_BEARER, 100);
}

static ErrorType_t
SIM808_NTP_setup_service(char *params)
{
    DEBUG_INFO("Setuping NTP service...%s", params);
    return SIM808_SendAT(params, SIM808_NTP_SETUP_SERVICE, 100);
}

static ErrorType_t
SIM808_NTP_start_sync(void)
{
    DEBUG_INFO("Starting NTP...");
    return SIM808_SendAT(NULL, SIM808_NTP_START_SERVICE, 100);
}

static ErrorType_t
SIM808_NTP_setup(void)
{
    char ntp_params[20] = {0};
    SIM808_Bearer_Open_GPRS_Context();
    SIM808_NTP_configure();
    snprintf(ntp_params, sizeof(ntp_params), "\"%s\",4", NTP_SERVER);
    SIM808_NTP_setup_service(ntp_params);
    SIM808_NTP_start_sync();
    vTaskDelay(3000);
    SIM808_Bearer_Close_GPRS_Context();
    return Ok;
}

static ErrorType_t
SIM808_Get_Time(struct local_time_t *current_timestamp)
{
    char response[30] = {0};
    DEBUG_INFO("Fetching timestamp...");
    SIM808_SendAT(NULL, SIM808_GET_TIME, 100);
    UART_GetData(response, 1500);
    debug_printf("response: %s\r\n", response);
    return Ok;
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

static SMS_action_t
SIM808_get_sms_action(void)
{
    SMS_action_t ret = NONE;
    char CMTI_resp[MAX_COMMAND_INPUT_LENGTH] = {0};
    char sms[50] = {0};
    /* Get CMTI input */
    UART_GetData(CMTI_resp, 1500);
    debug_printf("%s\r\n", CMTI_resp);
    SIM808_parseSMS(CMTI_resp, sms, sizeof(sms) - 1);
    debug_printf("trebao bih isprintati sms: %s\r\n", sms);
    if (STR_COMPARE(sms, "POST") == 0)
    {
        ret = HTTP_POST;
    }
    else if (STR_COMPARE(sms, "GET") == 0)
    {
        ret = HTTP_GET;
    }
    else if (STR_COMPARE(sms, "TCP") == 0)
    {
        ret = TCP_REQUEST;
    }
    return ret;
}

static void
SIM808_parseSMS(char *resp, char *sms, size_t sms_max_len)
{
    char *pos;
    char *sms_index;
    pos = strstr(resp, "+CMTI: ");
    if (pos == NULL)
    {
        DEBUG_ERROR("SMS not received!");
        return;
    }
    sms_index = strchr(pos, ',');
    sms_index++;
    SIM808_readSMS(sms, sms_index, sms_max_len);
    SIM808_SMS_Delete(sms_index);
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
    SIM808_SendAT(sms_index, SIM808_SMS_READ, 100);
    UART_GetData(buf, 1000);
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
    sms_len -= strlen("OK");    /* Do not count OK at the end (response from SIM808) */
    if (sms_len > sms_max_len)
    {
        DEBUG_ERROR("SMS action too long! Aborting...");
        return;
    }
    token[7][sms_len] = '\0';
    memmove(sms, token[7], sms_len);
}

/* Ex. +CLIP: "+989108793902",145,"",0,"",0 */
static ErrorType_t
SIM808_parseNumber(char *resp, struct user_phone_number_t *num)
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
    struct user_phone_number_t CallNumber = {.type = 0, .num = {0}};
    UART_GetData(CLIP_resp, 1000);
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
SIM808_memorizeNumber(struct user_phone_number_t *num)
{
    /* Only if number is not memorized */
    if (isNumberMemorized())
    {
        return;
    }
    DEBUG_INFO("Memorizing number: %s", num->num);
    if (!SIM808_save_number(num, USER1_PHONE_NUMBER))
    {
        /* Write failed */
        DEBUG_ERROR("Number not memorized!");
        return;
    }
    DEBUG_INFO("Number memorized!");
    memcpy(&number, num, sizeof(struct user_phone_number_t));
    numberMemorized = true;
}

static bool
SIM808_save_number(struct user_phone_number_t *num, storage_type_t type)
{
    size_t i, j;
    uint32_t data[sizeof(struct user_phone_number_t)] = {0};
    data[0] = num->type;
    for (i = 1, j = 0; i < sizeof(struct user_phone_number_t) && num->num[j] != '\0'; i++, j++)
    {
        data[i] = num->num[j];
        debug_printf("%s: data[%d] = %x\r\n", __func__, i, data[i]);
    }
    return storage_write_data(data, sizeof(struct user_phone_number_t), type);
}

static bool
SIM808_readEEPROMNumber(void)
{
    struct user_phone_number_t num = {.type = 0, .num = {0}};
    /* Get data from EEPROM */
    storage_read_data((void *)&num, sizeof(struct user_phone_number_t), USER1_PHONE_NUMBER);

    /* Validate number by number type */
    switch(num.type) {
    case Unknown_type:
    case Net_specific_type:
        return false;
    case National_type:
    case International_type:
        break;
    default:
        return false;
    }
    DEBUG_INFO("Read stored number: %s", num.num);
    memcpy(&number, &num, sizeof(struct user_phone_number_t));
    return true;
}

static void
SIM808_forgetNumber(void)
{
    memset(&number, 0, sizeof(struct user_phone_number_t));
}

ErrorType_t
SIM808_SendSMS(char *message)
{
    ErrorType_t status = Ok;
    if ((status = SIM808_SendAT(number.num, SIM808_SMS_SET_NUMBER, 1000)))
    {
        vTaskDelay(100);
        //return status;
    }
    vTaskDelay(3000);
    if ((status = SIM808_SendAT(message, SIM808_SMS_TEXT, 1000)))
    {
        return status;
    }
    return status;
}

void
SIM808_send_GET_request(char *url, char *response)
{
    SIM808_Configure_Bearer();
    /* Bearer Configure */
    SIM808_Bearer_Open_GPRS_Context();
    /* HTTP GET */
    SIM808_HTTP_Init();
    vTaskDelay(30);
    vTaskDelay(30);
    SIM808_HTTP_Set_Param("\"CID\",1");
    SIM808_HTTP_Set_Param(url);
    SIM808_HTTP_Set_Param("\"CONTENT\",\"application/json\"");
//    SIM808_HTTP_Enable_SSL();
    vTaskDelay(30);
    SIM808_HTTP_Get();
    vTaskDelay(3000);
    SIM808_HTTP_read_server_data();
    UART_GetData(response, 2000);
    SIM808_HTTP_Terminate();
    SIM808_Bearer_Close_GPRS_Context();
    debug_printf("received info: %s\r\n", response);
}

void
SIM808_send_POST_request(char *msg, char *url)
{
    SIM808_Configure_Bearer();
    /* Bearer Configure */
    SIM808_Bearer_Open_GPRS_Context();
    /* HTTP GET */
    SIM808_HTTP_Init();
    vTaskDelay(50);
    SIM808_HTTP_Set_Param("\"CID\",1");
    SIM808_HTTP_Set_Param(url);
    SIM808_HTTP_Set_Param("\"CONTENT\",\"application/json\"");
//    SIM808_HTTP_Enable_SSL();
    vTaskDelay(30);
    SIM808_HTTP_Pepare_POST_data(strlen(msg), 50000);
    SIM808_HTTP_Post();
    vTaskDelay(3000);
    SIM808_HTTP_Terminate();
    SIM808_Bearer_Close_GPRS_Context();
    debug_printf("sent post req: %s\r\n", msg);
}

void
SIM808_send_TCP_request(char *msg, char *tcp_params, char *response)
{
    char connection_status[20] = {0};
    SIM808_Deactivate_GRPS_Context();
//    SIM808_TCP_Enable_SSL();
    vTaskDelay(50);
    SIM808_Start_TCP_Conn(tcp_params);
    UART_GetData(connection_status, 4000);
    debug_printf("connection status = %s\r\n", connection_status);
    if (strstr(connection_status, "CONNECT OK") == NULL)
    {
        DEBUG_ERROR("TCP/IP connection not established!");
        goto out;
    }
    vTaskDelay(50);
    SIM808_SendTCP_Packet(msg);
    UART_GetData(response, 4000);
    debug_printf("###### RECEIVDED DATA: %s\r\n", response);
    out:
    SIM808_Close_TCP_Conn();
    SIM808_Deactivate_GRPS_Context();
}

static ErrorType_t
SIM808_Configure_Bearer(void)
{
    static bool initialized = false;
    ErrorType_t ret = Ok;
    if (initialized == false)
    {
        ret =SIM808_Bearer_Configure("\"Contype\",\"GPRS\"");
        if (ret == Ok)
        {
            ret = SIM808_Bearer_Configure("\"APN\",\"tomato\"");
        }
        if (ret == Ok)
        {
            initialized = true;
        }
    }
    return ret;
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
    uint32_t diff_ticks;
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
            diff_ticks = HAL_GetTick() - ticks;
            if (diff_ticks > 95 && diff_ticks < 120)
            {
                /* If SIM_RI was less than 120ms low, SIM808 received an SMS */
                _setSignal(SIM808Task, SIGNAL_SIM_RI_IRQ_SMS);
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
        _setSignal(SIM808Task, SIGNAL_SIM_RI_IRQ_CALL);
    }
}
