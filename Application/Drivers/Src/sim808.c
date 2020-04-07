/*
 * sim808.c
 *
 *  Created on: 31. kol 2018.
 *      Author: User
 */

#include "sim808.h"
#include "json.h"
#include "at.h"

/* Private define ------------------------------------------------------------*/

#define SMS_ACTION_MAX_LEN      (50)
#define HTTP_URL                "\"URL\",\"http://0.tcp.ngrok.io:17908\""
#define NTP_SERVER              "216.239.35.0"

struct user_phone_number_t {
    num_type_t type;
    char num[MAX_NUM_SIZE];
} __attribute__((__packed__));

struct local_time_t {
    char year[3];
    char month[3];
    char day[3];
    char hour[3];
    char minute[3];
    char second[3];
};

struct gsm_location_t {
    float longitude;
    float latitude;
};

struct unsolicited_message_t {
    char messageID;
    char message[100];
};

typedef enum {
    NONE = 0,
    HTTP_POST,
    HTTP_GET,
    TCP_REQUEST
} SMS_action_t;

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim4;
xQueueHandle unsolicitedInputQueue;
bool numberMemorized;
struct user_phone_number_t number;

static volatile bool sim808_rdy;
static volatile bool sim808_pin_ready;
static volatile bool sim808_call_ready;
static volatile bool sim808_sms_ready;
static volatile bool sim808_registered_to_network;
static volatile bool sim808_tcp_connected;

/******************************************************************************/

/* Private function prototypes -----------------------------------------------*/
static void SIM808_PowerOn();
static SMS_action_t SIM808_get_sms_action();
static ErrorType_t SIM808_readSMS();
static bool SIM808_GetNumber();
static ErrorType_t SIM808_parseNumber(char *, struct user_phone_number_t *);
static void SIM808_memorizeNumber();
static bool SIM808_save_number(struct user_phone_number_t *, storage_type_t);
static bool SIM808_readEEPROMNumber();
static void SIM808_forgetNumber();
static bool isNumberMemorized();
static ErrorType_t SIM808_Get_Local_IP();
static ErrorType_t SIM808_select_connection_mode(uint8_t);
static ErrorType_t SIM808_enable_gprs();
static ErrorType_t SIM808_Configure_Bearer();
static ErrorType_t SIM808_NTP_setup();
static ErrorType_t SIM808_Set_APN();
static ErrorType_t SIM808_wait(volatile bool *);
static void SIM808_set_ready(bool);
static ErrorType_t SIM808_wait_to_become_ready();
static void SIM808_set_pin_ok(bool);
static ErrorType_t SIM808_wait_pin_ok();
static void SIM808_set_call_ready(bool);
static ErrorType_t SIM808_wait_call_ready();
static void SIM808_set_sms_ready(bool);
static ErrorType_t SIM808_wait_sms_ready();
static void SIM808_set_registered_to_network(bool);
static ErrorType_t SIM808_wait_reg_to_network();
static void SIM808_set_tcp_connected(bool);
static ErrorType_t SIM808_wait_tcp_to_connect();
static void SIM808_get_unsolicited_input(struct unsolicited_message_t *, uint32_t);
static void SIM808_Timer4Start();
static void SIM808_Timer4Stop();
static void SIM808_EnableIRQs();


ErrorType_t
SIM808_Init(void)
{
    ErrorType_t status = Ok;
    int8_t attempts = 3;
    UART_Enable(RX_EN, TX_EN);
    SIM808_PowerOn();
    numberMemorized = SIM808_readEEPROMNumber();
    unsolicitedInputQueue = xQueueCreate(2, sizeof(struct unsolicited_message_t));
    if (unsolicitedInputQueue == NULL)
    {
        DEBUG_ERROR("Can not initialize queue!");
        return SIM808_Error;
    }
    do {
//        at_ping();
        if ((status = at_disable_echo()) != Ok) continue;
        if ((status = SIM808_wait_to_become_ready()) != Ok) continue;
        if ((status = SIM808_wait_pin_ok()) != Ok) continue;
        if ((status = SIM808_wait_call_ready()) != Ok) continue;
        if ((status = SIM808_wait_sms_ready()) != Ok) continue;
        if ((status = at_check_sim_card())) continue;
        if ((status = at_reg_home_network())) continue;
        if ((status = SIM808_wait_reg_to_network()) != Ok) continue;
        if ((status = at_set_sms_center())) continue;
        if ((status = at_set_sms_text_mode())) continue;
        if ((status = at_present_call_id())) continue;
        delay_ms(3000);
        if ((status = SIM808_enable_gprs())) continue;
        if ((status = SIM808_Set_APN())) continue;
        if ((status = SIM808_Configure_Bearer())) continue;
        if ((status = SIM808_NTP_setup())) continue;
        break;
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

/**
 * @brief: Function called from main task once SMS is received.
 * This function will parse SMS to get SMS requested action and dispatch
 * it to perform requested action.
 */
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

/**
 * @brief: Function called from main task once voice call is received.
 * This function will check calling number and start ADC measurements
 * if authorized user called in.
 */
void
SIM808_handleCall(void)
{
    struct local_time_t timestamp;
    struct gsm_location_t location;
    memset(&timestamp, 0, sizeof(struct local_time_t));
    memset(&location, 0, sizeof(struct gsm_location_t));
    /* Get CLIP input while RING */
    if (SIM808_GetNumber())
    {
        at_get_time(&timestamp);
        at_bearer_open_gprs_context();
//        at_set_lsb_server_address("\"lbs-simcom.com:3002\"");
        at_get_gsm_location(&location);
        at_bearer_close_gprs_context();
        debug_printf("Current time: %s-%s-%s T%s:%s:%s\r\n", timestamp.day, timestamp.month, timestamp.year, timestamp.hour, timestamp.minute, timestamp.second);
        debug_printf("Coordintes: %fdeg - %fdeg\r\n", location.latitude, location.longitude);
//        SIM808_SendSMS("Hello\r\nWorld\r\n!");
        /* Start Measurement task */
        _setSignal(ADCTask, SIGNAL_START_ADC);
    }
}

/**
 * @brief: Function called from main task once unsolicited input is received.
 * This function will parse unsolicited input and dispatch action
 * accordingly.
 */
void
SIM808_handleUnsolicited(void)
{
    struct unsolicited_message_t unsolicited;
    memset(&unsolicited, 0, sizeof(struct unsolicited_message_t));
    UART_Get_rxData(unsolicited.message, 5);
    DEBUG_INFO("#######Unsolicited input: %s ######\r\n", unsolicited.message);
    if (STR_COMPARE(unsolicited.message, "RDY+CFUN: 1") == 0)
    {
        DEBUG_INFO("SIM808 ready");
        SIM808_set_ready(true);
    }
    else if (STR_COMPARE(unsolicited.message, "+CPIN: READY") == 0)
    {
        DEBUG_INFO("No pin requested");
        SIM808_set_pin_ok(true);
    }
    else if (STR_COMPARE(unsolicited.message, "Call Ready") == 0)
    {
        DEBUG_INFO("call ready");
        SIM808_set_call_ready(true);
    }
    else if (STR_COMPARE(unsolicited.message, "SMS Ready") == 0)
    {
        DEBUG_INFO("SMS ready");
        SIM808_set_sms_ready(true);
    }
    else if (STR_COMPARE(unsolicited.message, "+CREG: 1") == 0)
    {
        DEBUG_INFO("Network ready");
        SIM808_set_registered_to_network(true);
    }
    else if (STR_COMPARE(unsolicited.message, "CONNECT OK") == 0)
    {
        DEBUG_INFO("TCP connection successful!");
        SIM808_set_tcp_connected(true);
    }
    else if (STR_COMPARE(unsolicited.message, "RING+CLIP: ") == 0)
    {
        xQueueSend(unsolicitedInputQueue, (void *) &unsolicited, 0);
    }
    else if (STR_COMPARE(unsolicited.message, "+CMTI: ") == 0)
    {
        xQueueSend(unsolicitedInputQueue, (void *) &unsolicited, 0);
    }
    else if (STR_COMPARE(unsolicited.message, "+CNTP: ") == 0)
    {
        unsolicited.messageID = SIM808_NTP_START_SERVICE;
        DEBUG_INFO("NTP unsolicited result code! Sending data to NTP parser!");
        xQueueSend(unsolicitedInputQueue, (void *) &unsolicited, 0);
    }
    else if (STR_COMPARE(unsolicited.message, "+CLBS: ") == 0)
    {
        xQueueSend(unsolicitedInputQueue, (void *) &unsolicited, 0);
    }
    else
    {
        DEBUG_INFO("Unknown unsolicited: %s", unsolicited.message);
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

    delay_ms(40);
    at_ping();
    at_exit_sleep_mode();
}

static void
SIM808_PowerOn(void)
{
    HAL_GPIO_WritePin(SIM_PWR_GPIO_Port, SIM_PWR_Pin, GPIO_PIN_SET);
    delay_ms(1500);
    HAL_GPIO_WritePin(SIM_PWR_GPIO_Port, SIM_PWR_Pin, GPIO_PIN_RESET);
    delay_ms(1800);
}

static ErrorType_t
SIM808_Set_APN(void)
{
    ErrorType_t status = Ok;
    static bool apn_initialized = false;
    if (!apn_initialized)
    {
        status = at_set_apn("tomato");
        if (status == Ok)
        {
            apn_initialized = true;
        }
    }
    return status;
}

static ErrorType_t
SIM808_Get_Local_IP(void)
{
    ErrorType_t status;
    char ip_addr[16] = {0};
    status = at_get_local_ip_addr(ip_addr);
    DEBUG_INFO("IP address: %s", ip_addr);
    return status;
}

static ErrorType_t
SIM808_select_connection_mode(uint8_t mode)
{
    char mode_str[2] = {0};
    snprintf(mode_str, 2, "%d", mode);
    DEBUG_INFO("Selecting %s TCP/IP connection mode...", mode ? "multi" : "single");
    return at_select_tcp_conn_mode(mode_str);
}

ErrorType_t
SIM808_SendTCP_Packet(char *message)
{
    ErrorType_t status = Ok;
    if ((status = at_prepare_tcp_packet()))
    {
        DEBUG_ERROR("Can not send TCP packet");
        return status;
    }
    delay_ms(2000);
    if ((status = at_send_tcp_packet(message)))
    {
        return status;
    }
    return status;
}

static ErrorType_t
SIM808_enable_gprs(void)
{
    ErrorType_t status;
    status = at_get_gprs_status();
    switch (status)
    {
    case GPRS_down:
        status = at_bring_up_gprs();
        break;
    default:
        break;
    }
    return status;
}

static ErrorType_t
SIM808_NTP_setup(void)
{
    char ntp_params[20] = {0};
    at_bearer_open_gprs_context();
    at_ntp_configure();
    snprintf(ntp_params, sizeof(ntp_params), "\"%s\",4", NTP_SERVER);
    at_ntp_setup_service(ntp_params);
    at_ntp_start_sync();
    at_bearer_close_gprs_context();
    return Ok;
}

/**
 * @brief: Read the user action from received SMS
 * @return: return the action (see SMS_action_t)
 */
static SMS_action_t
SIM808_get_sms_action(void)
{
    char sms_action[SMS_ACTION_MAX_LEN + 1] = {0};
    SMS_action_t ret = NONE;
    ErrorType_t status;

    status = SIM808_readSMS(sms_action);
    if (status == Ok)
    {
        if (STR_COMPARE(sms_action, "POST") == 0)
        {
            ret = HTTP_POST;
        }
        else if (STR_COMPARE(sms_action, "GET") == 0)
        {
            ret = HTTP_GET;
        }
        else if (STR_COMPARE(sms_action, "TCP") == 0)
        {
            ret = TCP_REQUEST;
        }
    }
    return ret;
}

/**
 * @brief: Parse unsolicited CMTI input to get the index
 * of received SMS. Then read the SMS at certain index.
 * Once read is done, delete SMS to not overload SIM808 memory.
 * @return: Ok if all operations are successfully done
 */
static ErrorType_t
SIM808_readSMS(char *sms_action)
{
    struct unsolicited_message_t cmti = {.messageID = '\0', .message = {0}};
    char *pos = NULL;
    char *sms_index;
    ErrorType_t status = Ok;

    /* Get unsolicited CMTI input */
    SIM808_get_unsolicited_input(&cmti, 5);
    pos = strstr(cmti.message, "+CMTI: ");
    if (pos == NULL)
    {
        DEBUG_ERROR("SMS not received!");
        status = SIM808_Error;
    }
    else
    {
        /* Get index of received SMS */
        sms_index = strchr(pos, ',');
        sms_index++;
        status = at_sms_read(sms_index, sms_action);
        /* Delete SMS to not overload SIM808 memory */
        at_sms_delete(sms_index);
    }
    return status;
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

/**
 * @brief: Function to parse caller phone number from
 * unsolicited +CLIP input. If calling first time, function
 * will memorize phone number as USER1. If number already
 * memorized, function will discard call if number is not authorized.
 * @return: true if everything is ok, false otherwise
 */
static bool
SIM808_GetNumber(void)
{
    ErrorType_t status = SIM808_Error;
    struct unsolicited_message_t clip = {.messageID = '\0', .message = {0}};
    struct user_phone_number_t CallNumber = {.type = 0, .num = {0}};
    SIM808_get_unsolicited_input(&clip, 5);
    status = SIM808_parseNumber(clip.message, &CallNumber);
    if (status != Ok)
    {
        DEBUG_ERROR("Can not obtain number!");
        return false;
    }
    while (at_hang_up())
    {
        delay_ms(40); /* Until success */
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

/**
 * @brief: Function to memorize USER number. Number will be written
 * to EEPROM and to global struct number_t.
 * @param num: Number to memorize
 */
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

/**
 * @brief: Function to prepare struct number_t to be written to
 * EEPROM.
 * @param num: number to store to EEPROM
 * @param type: type of data to be stored to EEPROM (specifies address offset)
 * @return: true if ok, false otherwise
 */
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

/**
 * @brief: Function to retrieve USER number from EEPROM.
 * @return: true if ok, false otherwise
 */
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
    if ((status = at_sms_prepare_to_send(number.num)))
    {
        delay_ms(100);
        //return status;
    }
    delay_ms(3000);
    if ((status = at_sms_send(message)))
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
    at_bearer_open_gprs_context();
    /* HTTP GET */
    at_http_init();
    delay_ms(30);
    delay_ms(30);
    at_http_set_param("\"CID\",1");
    at_http_set_param(url);
    at_http_set_param("\"CONTENT\",\"application/json\"");
//    SIM808_HTTP_Enable_SSL();
    delay_ms(30);
    at_http_get();
    delay_ms(3000);
    at_http_read_server_data();
    UART_Get_rxData(response, 2000);
    at_http_terminate();
    at_bearer_close_gprs_context();
    debug_printf("received info: %s\r\n", response);
}

void
SIM808_send_POST_request(char *msg, char *url)
{
    SIM808_Configure_Bearer();
    /* Bearer Configure */
    at_bearer_open_gprs_context();
    /* HTTP GET */
    at_http_init();
    delay_ms(50);
    at_http_set_param("\"CID\",1");
    at_http_set_param(url);
    at_http_set_param("\"CONTENT\",\"application/json\"");
//    SIM808_HTTP_Enable_SSL();
    delay_ms(30);
    at_http_pepare_post_data(strlen(msg), 50000);
    at_http_post();
    delay_ms(3000);
    at_http_terminate();
    at_bearer_close_gprs_context();
    debug_printf("sent post req: %s\r\n", msg);
}

void
SIM808_send_TCP_request(char *msg, char *tcp_params, char *response)
{
    at_deactivate_gprs_context();
//    SIM808_TCP_Enable_SSL();
    delay_ms(50);
    at_start_tcp_conn(tcp_params);
    if (!SIM808_wait_tcp_to_connect())
    {
        DEBUG_ERROR("TCP/IP connection not established!");
        goto out;
    }
    delay_ms(50);
    SIM808_SendTCP_Packet(msg);
    UART_Get_rxData(response, 4000);
    debug_printf("###### RECEIVDED DATA: %s\r\n", response);
    out:
    at_close_tcp_conn();
    at_deactivate_gprs_context();
}

static ErrorType_t
SIM808_Configure_Bearer(void)
{
    static bool initialized = false;
    ErrorType_t ret = Ok;
    if (initialized == false)
    {
        ret = at_bearer_configure("\"Contype\",\"GPRS\"");
        if (ret == Ok)
        {
            ret = at_bearer_configure("\"APN\",\"tomato\"");
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

/**
 * @brief: Handles all volatile sim808 flags.
 * Function is waiting 20 seconds requested flag to become ready.
 * Volatile flags are being set from task which handles
 * unsolicited inputs.
 * @param flag: requested flag to check
 * @return: Ok if flag becomes ready in desired timeout (20 seconds),
 * otherwise SIM808_Error
 */
static ErrorType_t
SIM808_wait(volatile bool *flag)
{
    ErrorType_t status = SIM808_Error;
    uint32_t ticks = HAL_GetTick();
    uint32_t time_elapsed = 0;
    while (time_elapsed <= (20 * 1000))
    {
        if (*flag == true)
        {
            status = Ok;
            break;
        }
        time_elapsed = HAL_GetTick() - ticks;
    }
    return status;
}

/**
 * @brief: Set sim808_rdy flag if corresponding unsolicited
 * code is received (RDY+CFUN: 1).
 * @param status: true/false
 */
static void
SIM808_set_ready(bool status)
{
    sim808_rdy = status;
}

/**
 * @brief: Wait until SIM808 becomes ready or timeout expires.
 * @return: Ok if ready, otherwise SIM808_Error
 */
static ErrorType_t
SIM808_wait_to_become_ready(void)
{
    return SIM808_wait(&sim808_rdy);
}

/**
 * @brief: Set sim808_pin_ready flag if corresponding unsolicited
 * code is received (+CPIN: READY).
 * @param status: true/false
 */
static void
SIM808_set_pin_ok(bool status)
{
    sim808_pin_ready = status;
}

/**
 * @brief: Wait until SIM808 PIN becomes ready or timeout expires.
 * @return: Ok if ready, otherwise SIM808_Error
 */
static ErrorType_t
SIM808_wait_pin_ok(void)
{
    return SIM808_wait(&sim808_pin_ready);
}

/**
 * @brief: Set sim808_call_ready flag if corresponding unsolicited
 * code is received (Call Ready).
 * @param status: true/false
 */
static void
SIM808_set_call_ready(bool status)
{
    sim808_call_ready = status;
}

/**
 * @brief: Wait until SIM808 Call becomes ready or timeout expires.
 * @return: Ok if ready, otherwise SIM808_Error
 */
static ErrorType_t
SIM808_wait_call_ready(void)
{
    return SIM808_wait(&sim808_call_ready);
}

/**
 * @brief: Set sim808_sms_ready flag if corresponding unsolicited
 * code is received (SMS Ready).
 * @param status: true/false
 */
static void
SIM808_set_sms_ready(bool status)
{
    sim808_sms_ready = status;
}

/**
 * @brief: Wait until SIM808 SMS becomes ready or timeout expires.
 * @return: Ok if ready, otherwise SIM808_Error
 */
static ErrorType_t
SIM808_wait_sms_ready(void)
{
    return SIM808_wait(&sim808_sms_ready);
}

/**
 * @brief: Set sim808_registered_to_network flag if corresponding
 * unsolicited code is received (+CREG: 1).
 * @param status: true/false
 */
static void
SIM808_set_registered_to_network(bool status)
{
    sim808_registered_to_network = status;
}

/**
 * @brief: Wait until SIM808 registers to network or timeout expires.
 * @return: Ok if ready, otherwise SIM808_Error
 */
static ErrorType_t
SIM808_wait_reg_to_network(void)
{
    return SIM808_wait(&sim808_registered_to_network);
}

/**
 * @brief: Set sim808_tcp_connected flag if corresponding
 * unsolicited code is received (CONNECT OK).
 * @param status: true/false
 */
static void
SIM808_set_tcp_connected(bool status)
{
    sim808_tcp_connected = status;
}

/**
 * @brief: Wait until SIM808 establishes TCP connection.
 * @return: Ok if ready, otherwise SIM808_Error
 */
static ErrorType_t
SIM808_wait_tcp_to_connect(void)
{
    return SIM808_wait(&sim808_tcp_connected);
}

/**
 * @brief: Receive an unsolicited input from task which handles
 * all unsolicited result codes. Block for timeout ticks if a
 * message is not immediately available.
 */
static void
SIM808_get_unsolicited_input(struct unsolicited_message_t *um, uint32_t timeout)
{
    if (unsolicitedInputQueue != NULL)
    {
        xQueueReceive(unsolicitedInputQueue, um, (TickType_t) timeout);
    }
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

/**
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

/**** AT response parsing callbacks ***********/

/**
 * @brief: Callback for parsing SMS when received. Write parsed sms
 * action to global variable sms_action.
 * @param sms: response from SIM808
 * @param arg: pointer to store parsed sms_action
 * @return: Ok if SMS is parsed successfully
 */
ErrorType_t
SIM808_parseSMS(char *sms, void *arg)
{
    char *pos;
    char *token[8 + 1] = {NULL};
    char delim[] = "\"";
    int i = 0;
    char *sms_action = arg;
    size_t sms_action_len = 0;
    ErrorType_t status = Ok;

    debug_printf("%s\r\n", sms);
    pos = strstr(sms, "+CMGR: ");
    if (pos == NULL)
    {
        debug_printf("going out..\r\n");
        return SIM808_Error;
    }
    token[i] = strtok(pos, delim);
    while (token[i] != NULL)
    {
        token[++i] = strtok(NULL, delim);
    }
    if (strcmp(token[0], "+CMGR: ") != 0)
    {
        debug_printf("NOK\r\n");
        return SIM808_Error;
    }
    if (strstr(number.num, token[3]) == NULL)
    {
        /* Unknown number */
        DEBUG_INFO("SMS from unauthorized number!");
        return SIM808_Error;
    }
    sms_action_len = strlen(token[7]);
    sms_action_len -= strlen("OK");    /* Do not count OK at the end (response from SIM808) */
    if (sms_action_len > SMS_ACTION_MAX_LEN)
    {
        DEBUG_ERROR("SMS action too long! Aborting...");
        return SIM808_Error;
    }
    token[7][sms_action_len] = '\0';
    memmove(sms_action, token[7], sms_action_len);
    return status;
}

/**
 * @brief: parse CREG response (Check SIM808 status)
 * @param resp: string to be parsed
 * @param arg: arbitrary argument
 * @return: Ok if SIM808 is registerd to the network
 */
ErrorType_t
SIM808_CREG_resp(char *resp, void *arg)
{
    ErrorType_t status = Ok;
    CREG_resp_t s;
    s = TO_DIGIT(*resp);
    switch (s) {
    case NOT_REGISTERED:
        status = SIM808_Error;
        break;
    case REGISTERED:
        DEBUG_INFO("SIM808 Registered to the network!");
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

/**
 * @brief: validate that SMS mode is set to TEXT
 * @param: response from SIM808
 * @param arg: arbitrary argument
 * @return: Ok if TEXT mode is selected
 */
ErrorType_t
SIM808_CMGF_resp(char *resp, void *arg)
{
    ErrorType_t status = Ok;
    CMGF_resp_t s;
    s = TO_DIGIT(*resp);
    switch (s) {
    case PDU_MODE:
        status = SIM808_Error;
        break;
    case TEXT_MODE:
        DEBUG_INFO("SMS Text mode selected!");
        status = Ok;
        break;
    default:
        status = SIM808_Error;
        break;
    }
    return status;
}

/**
 * @brief: Check GPRS status
 * @param: response from SIM808
 * @param unused
 * @return: Ok if GPRS enabled, GPRS_down if disabled
 */
ErrorType_t
SIM808_GPRS_status(char *resp, void *unused)
{
    ErrorType_t status = Ok;
    uint8_t s;
    s = TO_DIGIT(*resp);
    switch (s) {
    case 0:
        status = GPRS_down;
        break;
    case 1:
        DEBUG_INFO("GPRS enabled!");
        status = Ok;
        break;
    default:
        status = SIM808_Error;
        break;
    }
    return status;
}

/**
 * @brief: Callback for checking NTP status after NTP started.
 * Function should receive NTP status as an unsolicited input
 * task which handles unsolicited inputs.
 * @param resp: response from SIM808 (should be empty)
 * @param unused
 * @return: Ok if response is 1 (Success)
 */
ErrorType_t
SIM808_NTP_status(char *resp, void *unused)
{
    ErrorType_t status;
    NTP_status_t s;
    struct unsolicited_message_t cntp = {.messageID = '\0', .message = {0}};
    char *p = NULL;

    SIM808_get_unsolicited_input(&cntp, 1500);
    p = strstr(cntp.message, "+CNTP: ");
    if (p == NULL)
    {
        DEBUG_ERROR("NTP not started properly!");
        return SIM808_Error;
    }
    p += strlen("+CNTP: ");
    s = atoi(p);
    switch (s) {
    case NTP_SUCCESS:
        DEBUG_INFO("NTP successfully started");
        status = Ok;
        break;
    case NTP_NET_ERROR:
    case NTP_DNS_ERROR:
    case NTP_CONNECT_ERR:
    case NTP_TIMEOUT:
    case NTP_SERVER_ERROR:
    case NTP_OPERATION_NOT_ALLOWED:
    default:
        DEBUG_ERROR("NTP not started properly switch case!");
        status = SIM808_Error;
        break;
    }
    return status;
}

/**
 * @brief: Callback for parsing SIM808 time.
 * Time will be stored to local_time_t struct.
 * @param resp: response from SIM808
 * @param arg: pointer to local_time_t struct
 * @return: Ok if successful
 */
ErrorType_t
SIM808_parseTime(char *resp, void *arg)
{
    struct local_time_t *lt = arg;
    char *p = NULL;
    char *year = (char *)lt;
    char *month =  year   + sizeof(lt->year);
    char *day =    month  + sizeof(lt->month);
    char *hour =   day    + sizeof(lt->day);
    char *minute = hour   + sizeof(lt->hour);
    char *second = minute + sizeof(lt->minute);
    ErrorType_t status = Ok;
    debug_printf("%s: %s\r\n", __func__, resp);
    p = strstr(resp, "+CCLK: ");
    if (p == NULL)
    {
        status = SIM808_Error;
    }
    else
    {
        p += strlen("+CCLK: ");
        if (*p != '"')
        {
            status = SIM808_Error;
        }
        else
        {
            p += 1;
            put_data(&year, p, 2);
            p += 3;
            put_data(&month, p, 2);
            p += 3;
            put_data(&day, p, 2);
            p += 3;
            put_data(&hour, p, 2);
            p += 3;
            put_data(&minute, p, 2);
            p += 3;
            put_data(&second, p, 2);
        }
    }
    return status;
}

/**
 * @brief: Callback for parsing SIM808 IP address.
 * IP address will be stored to arg.
 * @param resp: response from SIM808
 * @param arg: pointer to ip address buf
 * @return: Ok if successful
 */
ErrorType_t
SIM808_ip_addr(char *resp, void *arg)
{
    char *ip_addr = arg;
    if (resp != NULL)
    {
        memmove(ip_addr, resp, 15);
    }
    return Ok;
}

/**
 * @brief: Callback for parsing SIM808 GSM location.
 * location will be stored to struct gsm_location_t.
 * Example: +CIPGSMLOC: 0,6.141064,46.219772,2016/08/24,00:55:40
 * @param resp: response from SIM808
 * @param arg: pointer to struct gsm_location_t
 * @return: Ok if successful
 */
ErrorType_t
SIM808_location(char *resp, void *arg)
{
    gsm_loc_status_t state;
    struct gsm_location_t *gl = arg;
    struct unsolicited_message_t clbs = {.messageID = '\0', .message = {0}};
    char *p = NULL;
    ErrorType_t status = Ok;

    /* Get CLBS unsolicited input */
    SIM808_get_unsolicited_input(&clbs, 10000);
    debug_printf("%s: clbs input: %s\r\n", __func__, clbs.message);
    p = strstr(clbs.message, "+CLBS: ");
    if (p == NULL)
    {
        DEBUG_ERROR("Bad input");
        return SIM808_Error;
    }
    p += strlen("+CLBS: ");
    state = atoi(p);
    if (state == GSM_LOC_SUCCESS)
    {
        p = strchr(p, ',');
        if (p == NULL)
        {
            DEBUG_ERROR("Can not obtain location!");
            status = SIM808_Error;
        }
        else
        {
            /* Read the longitude */
            gl->longitude = (float)atof(++p);
            p = strchr(p, ',');
            if (p == NULL)
            {
                DEBUG_ERROR("Can not obtain location!");
                status = SIM808_Error;
            }
            else
            {
                /* Read the latitude */
                gl->latitude = (float)atof(++p);
            }
        }
    }
    else
    {
        DEBUG_ERROR("Can not obtain location! Network problem!");
        status = SIM808_Error;
    }
    if (status != Ok)
    {
        /* Forget all changes */
        memset(gl, 0, sizeof(struct gsm_location_t));
    }
    return status;
}
