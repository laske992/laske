/*
 * at.c
 *
 *  Created on: Mar 29, 2020
 *      Author: ivan
 */

#include "at.h"
#include "uart.h"

#define SIM808_ENTER            ("\r\n")
#define CTRL_Z                  (0x1A)  /* CTRL + Z */

struct at_req {
    const char *cmd;
    const char *resp;
    uint16_t rx_timeout;
    bool response_wait;
    SIM808_parseResp *parse_cb;
};

static const struct at_req at_req[] = {
    {"AT&W",            "OK",       1000,   true,   NULL},
    {"ATE0",            "OK",       1000,   true,   NULL},              /* ATE0 */
    {"AT",              "OK",       1000,   true,   NULL},              /* AT */
    {"+++",             "OK",       1000,   true,   NULL},              /* Switch to the command mode */
    {"ATO0",            NULL,       1000,   false,  NULL},              /* Switch to data mode */
    {"AT+CCID=?",       "OK",       1000,   true,   NULL},              /* AT+CCID=? */
    {"AT+CREG=1",       "OK",       1000,   true,   NULL},              /* AT+CREG=1 */
    {"AT+CREG?",        "+CREG: ",  1000,   true,   SIM808_CREG_resp},  /* AT+CREG? */
    {"AT+CMGF=1",       "OK",       1000,   true,   NULL},              /* SMS Text mode */
    {"AT+CMGF?",        "+CMGF:",   1000,   true,   SIM808_CMGF_resp},  /* Is SMS in text mode? */
    {"AT+CSCA=",        "OK",       1000,   true,   NULL},              /* Set SMS Center */
    {"AT+CLIP=1",       "OK",       1000,   true,   NULL},              /* Present call number */
    {"ATH",             "OK",       1000,   true,   NULL},              /* Hang up current call */
    {"AT+CSCLK=1",      "OK",       1000,   true,   NULL},              /* Enter Sleep Mode */
    {"AT+CSCLK=0",      "OK",       1000,   true,   NULL},              /* Exit Sleep Mode */
    {"AT+CMGS=",        ">",        1000,   true,   NULL},              /* Set number */
    {NULL,              NULL,       1000,   false,  NULL},              /* SMS TEXT */
    {"AT+CMGD=",        "OK",       1000,   true,   NULL},              /* Delete an SMS messages */
    {"AT+CMGR=",        NULL,       2500,   true,   SIM808_parseSMS},   /* Read SMS message */
    {"AT+CIPMUX=",      NULL,       1000,   false,  NULL},              /* Set TCP/IP Connection Mode */
    {"AT+CIPMODE",      NULL,       1000,   false,  NULL},              /* Select TCP/IP Application Mode */
    {"AT+CGATT?",       "+CGATT:",  1000,   true,   NULL},              /* Get GPRS Status */
    {"AT+CSTT=",        "OK",       1000,   true,   NULL},              /* Set APN */
    {"AT+CIICR",        "OK",       2000,   true,   NULL},              /* Bring up GPRS */
    {"AT+CIFSR",        NULL,       3000,   true,   SIM808_ip_addr},    /* Get local IP address */
    {"AT+CIPSSL=1",     "OK",       1000,   true,   NULL},              /* Enable SSL for TCP */
    {"AT+CIPSHUT",      "SHUT OK",  1000,   true,   NULL},              /* Deactivate GPRS Context */
    {"AT+CIPSTART=",    "OK",       3000,   true,   NULL},              /* Start up TCP/IP connection */
    {"AT+CIPCLOSE",     "CLOSE OK", 3000,   true,   NULL},              /* Close TCP/IP connection */
    {"AT+CIPSEND",      ">",        3000,   true,   NULL},              /* Start sending data to server */
    {NULL,              "SEND OK",  15000,  true,   NULL},              /* Send data to server */
    {"AT+SAPBR=3,1,",   "OK",       1000,   true,   NULL},              /* Configuring Bearer profile 1 */
    {"AT+SAPBR=1,1",    "OK",       3000,   true,   NULL},              /* Opening GPRS Context */
    {"AT+SAPBR=0,1",    "OK",       3000,   true,   NULL},              /* Closing GPRS Context */
    {"AT+HTTPINIT",     "OK",       3000,   true,   NULL},              /* Init HTTP Service */
    {"AT+HTTPSSL=1",    "OK",       1000,   true,   NULL},              /* Enable SSL for HTTP */
    {"AT+HTTPPARA=",    "OK",       1000,   true,   NULL},              /* Set HTTP Parameter */
    {"AT+HTTPACTION=0", "OK",       3000,   true,   NULL},              /* GET Data */
    {"AT+HTTPREAD",     NULL,       15000,  false,  NULL},              /* Read the data from HTTP server */
    {"AT+HTTPDATA=",    "DOWNLOAD", 1000,   true,   NULL},              /* Prepare to upload HTTP data */
    {NULL,              "OK",       15000,  true,   NULL},              /* Upload HTTP Data */
    {"AT+HTTPACTION=1", "OK",       1000,   true,   NULL},              /* POST Data */
    {"AT+HTTPTERM",     "OK",       1000,   true,   NULL},              /* Terminate HTTP */
    {"AT+CNTPCID=1",    "OK",       1000,   true,   NULL},              /* Configure NTP bearer */
    {"AT+CNTP=",        "OK",       1000,   true,   NULL},              /* Setup NTP service */
    {"AT+CNTP",         "OK",       3000,   true,   SIM808_NTP_status}, /* Start NTP service */
    {"AT+CCLK?",        NULL,       1000,   true,   SIM808_parseTime}   /* Get current time */
};

/* Static functions */
static ErrorType_t at_check_resp(char *, uint8_t, void *);

/**
 * @brief: Compile and send AT request. A request is found in
 * sim808_req structure by req_id. AT response will be parsed
 * if requested.
 * @param msg: addition to req->cmd
 * @param req_id: index of AT request
 * @param tx_timeout: timeout for msg transmission
 * @param arg: an arbitrary argument to be used within parse callback
 * @return: Ok if everything is successful
 */
static ErrorType_t
at_send(const char *msg, uint8_t req_id, uint16_t tx_timeout, void *arg)
{
    ErrorType_t status = Ok;
    const struct at_req *req = &at_req[req_id];
    struct at_response at_response = {
            .wait = req->response_wait,
            .rx_timeout = req->rx_timeout,
            .response = {0}
    };
    char at_cmd[MAX_AT_CMD_LEN + 1] = {0};
    char *p = at_cmd;
    int nchars = 0;
    int maxlen = MAX_AT_CMD_LEN;

    if (req_id > MAXCOUNT(at_req))
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
    status = UART_Send((uint8_t *)at_cmd, tx_timeout, &at_response);
    if (status == Ok)
    {
        status = at_check_resp(at_response.response, req_id, arg);
    }
    return status;
}

/**
 * @brief: Validate that AT response is valid (as expected).
 * Parse response additionally if requested.
 * @param resp: SIM808 response
 * @param req_id: AT request index
 * @param arg: arbitrary argument
 * @return: status of validation - Ok if successful
 */
static ErrorType_t
at_check_resp(char *resp, uint8_t req_id, void *arg)
{
    ErrorType_t status = Ok;
    const struct at_req *req;
    char *p = resp;
    size_t offset = 0;

    if (req_id > MAXCOUNT(at_req))
    {
        return SIM808_Error;
    }
    req = &at_req[req_id];
    if (req->resp != NULL)
    {
        /* Compare response and expected response */
        if (STR_COMPARE(resp, req->resp))
        {
            return SIM808_Error;
        }
        offset = strlen(req->resp);
    }
    if (req->parse_cb)
    {
        /* Additional parsing */
        p += offset;
        status = req->parse_cb(p, arg);
    }
    return status;
}

ErrorType_t
at_disable_echo(void)
{
    DEBUG_INFO("Disabling echo...");
    return at_send(NULL, SIM808_DISABLE_ECHO, 100, NULL);
}

ErrorType_t
at_ping(void)
{
    DEBUG_INFO("Checking communication...");
    return at_send(NULL, SIM808_PING, 100, NULL);
}

ErrorType_t
at_check_sim_card(void)
{
    DEBUG_INFO("Checking SIM card...");
    return at_send(NULL, SIM808_CHECK_SIM, 100, NULL);
}

ErrorType_t
at_reg_home_network(void)
{
    DEBUG_INFO("Register Home Network...");
    return at_send(NULL, SIM808_REG_HOME_NETWORK, 500, NULL);
}

ErrorType_t
at_check_home_network(void)
{
    DEBUG_INFO("Checking network...");
    return at_send(NULL, SIM808_CHECK_HOME_NETWORK, 100, NULL);
}

ErrorType_t
at_set_sms_center(void)
{
    DEBUG_INFO("Setting SMS center: \"+385910401\"");
    return at_send("\"+385910401\"", SIM808_SMS_CENTER_SET, 100, NULL);
}

ErrorType_t
at_set_sms_text_mode(void)
{
    DEBUG_INFO("Setting SMS mode to txt.");
    return at_send(NULL, SIM808_SMS_SET_FORMAT, 100, NULL);
}

ErrorType_t
at_sms_prepare_to_send(char *number)
{
    DEBUG_INFO("Preparing to send SMS...");
    return at_send(number, SIM808_SMS_SET_NUMBER, 100, NULL);
}

ErrorType_t
at_sms_send(char *sms)
{
    DEBUG_INFO("Sending SMS...");
    return at_send(sms, SIM808_SMS_TEXT, 500, NULL);
}

ErrorType_t
at_sms_read(char *index, char *sms)
{
    DEBUG_INFO("Reading SMS at index %s...", index);
    return at_send(index, SIM808_SMS_READ, 100, sms);
}

ErrorType_t
at_sms_delete(char *index)
{
    DEBUG_INFO("Deleting SMS at index: %s", index);
    return at_send(index, SIM808_SMS_DELETE, 100, NULL);
}

ErrorType_t
at_present_call_id(void)
{
    DEBUG_INFO("Present call number when receiving call...");
    return at_send(NULL, SIM808_PRESENT_ID, 100, NULL);
}

ErrorType_t
at_hang_up(void)
{
    DEBUG_INFO("Hanging up the call...");
    return at_send(NULL, SIM808_HANG_UP, 100, NULL);
}

ErrorType_t
at_enter_sleep_mode(void)
{
    DEBUG_INFO("Go sleep...");
    return at_send(NULL, SIM808_ENTER_SLEEP_MODE, 100, NULL);
}

ErrorType_t
at_exit_sleep_mode(void)
{
    DEBUG_INFO("Waking up...");
    return at_send(NULL, SIM808_EXIT_SLEEP_MODE, 100, NULL);
}

ErrorType_t
at_set_apn(char *apn)
{
    DEBUG_INFO("Setting APN...");
    return at_send(apn, SIM808_SET_APN, 100, NULL);
}

ErrorType_t
at_get_gprs_status(void)
{
    DEBUG_INFO("Checking GPRS Status...");
    return at_send(NULL, SIM808_GET_GPRS_STATUS, 100, NULL);
}

ErrorType_t
at_bring_up_gprs(void)
{
    DEBUG_INFO("Bringing GPRS up...");
    return at_send(NULL, SIM808_BRING_UP_GPRS, 100, NULL);
}

ErrorType_t
at_select_tcp_conn_mode(char *mode)
{
    DEBUG_INFO("Changing TCP/IP connection mode...");
    return at_send(mode, SIM808_TCP_IP_CONNECTION_MODE, 100, NULL);
}

ErrorType_t
at_tcp_enable_ssl(void)
{
    DEBUG_INFO("Activating SSL for TCP...");
    return at_send(NULL, SIM808_TCP_ENABLE_SSL, 100, NULL);
}

ErrorType_t
at_deactivate_gprs_context(void)
{
    DEBUG_INFO("Deactivating GRPS context...");
    return at_send(NULL, SIM808_DEACTIVATE_GRPS_CONTEXT, 100, NULL);
}

ErrorType_t
at_start_tcp_conn(char *params)
{
    DEBUG_INFO("Starting TCP/IP connection...");
    return at_send(params, SIM808_START_TCP_IP_CONN, 100, NULL);
}

ErrorType_t
at_close_tcp_conn(void)
{
    DEBUG_INFO("Closing TCP/IP connection...");
    return at_send(NULL, SIM808_CLOSE_TCP_IP_CONN, 100, NULL);
}

ErrorType_t
at_prepare_tcp_packet(void)
{
    DEBUG_INFO("Preparing to send TCP/IP packet...");
    return at_send(NULL, SIM808_PREPARE_TCP_PACKET, 1000, NULL);
}

ErrorType_t
at_send_tcp_packet(char *packet)
{
    DEBUG_INFO("Sending TCP/IP packet...");
    return at_send(packet, SIM808_SEND_TCP_PACKET, 1000, NULL);
}

ErrorType_t
at_bearer_configure(char *msg)
{
    DEBUG_INFO("Configuring Bearer profile 1...");
    return at_send(msg, SIM808_BEARER_CONFIGURE, 100, NULL);
}

ErrorType_t
at_bearer_open_gprs_context(void)
{
    DEBUG_INFO("Opening GPRS Context...");
    return at_send(NULL, SIM808_BEARER_OPEN_GPRS, 100, NULL);
}

ErrorType_t
at_bearer_close_gprs_context(void)
{
    DEBUG_INFO("Closing GPRS Context...");
    return at_send(NULL, SIM808_BEARER_CLOSE_GPRS, 100, NULL);
}

ErrorType_t
at_http_init(void)
{
    DEBUG_INFO("Initializing HTTP Service...");
    return at_send(NULL, SIM808_HTTP_INIT, 100, NULL);
}

ErrorType_t
at_http_enable_ssl(void)
{
    DEBUG_INFO("Activating SSL for HTTP Service...");
    return at_send(NULL, SIM808_HTTP_ENABLE_SSL, 100, NULL);
}

ErrorType_t
at_http_set_param(char *param)
{
    DEBUG_INFO("Setting HTTP Parameter: %s...", param);
    return at_send(param, SIM808_HTTP_SET_PARAM, 100, NULL);
}

ErrorType_t
at_http_get(void)
{
    DEBUG_INFO("Sending HTTP GET Request...");
    return at_send(NULL, SIM808_HTTP_GET, 1000, NULL);
}

ErrorType_t
at_http_read_server_data(void)
{
    DEBUG_INFO("Reading HTTP Server response...");
    return at_send(NULL, SIM808_HTTP_READ_SERVER_DATA, 100, NULL);
}

ErrorType_t
at_http_pepare_post_data(size_t size, uint32_t max_latency_time)
{
    char msg[20] = {0};
    snprintf(msg, sizeof msg, "%d,%ld", size, max_latency_time);
    return at_send(msg, SIM808_HTTP_PREPARE_HTTP_POST, 100, NULL);
}

ErrorType_t
at_http_upload_post_data(char *msg)
{
    DEBUG_INFO("Upload HTTP data: %s...", msg);
    return at_send(msg, SIM808_HTTP_UPLOAD_POST_DATA, 500, NULL);
}

ErrorType_t
at_http_post(void)
{
    DEBUG_INFO("HTTP POST...");
    return at_send(NULL, SIM808_HTTP_POST, 100, NULL);
}

ErrorType_t
at_http_terminate(void)
{
    DEBUG_INFO("Terminating HTTP Service...");
    return at_send(NULL, SIM808_HTTP_TERMINATE, 100, NULL);
}

ErrorType_t
at_ntp_configure(void)
{
    DEBUG_INFO("Configuring NTP bearer...");
    return at_send(NULL, SIM808_NTP_CONFIGURE_BEARER, 100, NULL);
}

ErrorType_t
at_ntp_setup_service(char *params)
{
    DEBUG_INFO("Setuping NTP service...%s", params);
    return at_send(params, SIM808_NTP_SETUP_SERVICE, 100, NULL);
}

ErrorType_t
at_ntp_start_sync(void)
{
    DEBUG_INFO("Starting NTP...");
    return at_send(NULL, SIM808_NTP_START_SERVICE, 100, NULL);
}

ErrorType_t
at_get_time(void *current_timestamp)
{
    DEBUG_INFO("Fetching timestamp...");
    return at_send(NULL, SIM808_GET_TIME, 100, current_timestamp);
}

ErrorType_t
at_get_local_ip_addr(char *ip_addr)
{
    DEBUG_INFO("Getting Local IP address...");
    at_send(NULL, SIM808_GET_LOCAL_IP, 100, ip_addr);
}
