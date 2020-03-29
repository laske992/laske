/*
 * at.h
 *
 *  Created on: Mar 29, 2020
 *      Author: ivan
 */

#ifndef DRIVERS_INC_AT_H_
#define DRIVERS_INC_AT_H_

#include "util.h"

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
    SIM808_GET_TIME,
    SIM808_GET_GSM_LOCATION
};

ErrorType_t at_disable_echo();
ErrorType_t at_ping();
ErrorType_t at_check_sim_card();
ErrorType_t at_reg_home_network();
ErrorType_t at_check_home_network();
ErrorType_t at_set_sms_center();
ErrorType_t at_set_sms_text_mode();
ErrorType_t at_sms_prepare_to_send(char *);
ErrorType_t at_sms_send(char *);
ErrorType_t at_sms_read(char *, char *);
ErrorType_t at_sms_delete(char *);
ErrorType_t at_present_call_id();
ErrorType_t at_hang_up();
ErrorType_t at_enter_sleep_mode();
ErrorType_t at_exit_sleep_mode();
ErrorType_t at_set_apn(char *);
ErrorType_t at_get_gprs_status();
ErrorType_t at_bring_up_gprs();
ErrorType_t at_select_tcp_conn_mode(char *);
ErrorType_t at_tcp_enable_ssl();
ErrorType_t at_deactivate_gprs_context();
ErrorType_t at_start_tcp_conn(char *);
ErrorType_t at_close_tcp_conn();
ErrorType_t at_prepare_tcp_packet();
ErrorType_t at_send_tcp_packet(char *);
ErrorType_t at_bearer_configure(char *);
ErrorType_t at_bearer_open_gprs_context();
ErrorType_t at_bearer_close_gprs_context();
ErrorType_t at_http_init();
ErrorType_t at_http_enable_ssl();
ErrorType_t at_http_set_param(char *);
ErrorType_t at_http_get();
ErrorType_t at_http_read_server_data();
ErrorType_t at_http_pepare_post_data(size_t, uint32_t);
ErrorType_t at_http_upload_post_data(char *);
ErrorType_t at_http_post();
ErrorType_t at_http_terminate();
ErrorType_t at_ntp_configure();
ErrorType_t at_ntp_setup_service(char *);
ErrorType_t at_ntp_start_sync();
ErrorType_t at_get_time(void *);
ErrorType_t at_get_local_ip_addr(char *ip_addr);
ErrorType_t at_get_gsm_location(void *);

/******** AT response parsing callbacks ***********/
typedef ErrorType_t (SIM808_parseResp)(char *, void *);

extern ErrorType_t SIM808_parseSMS(char *, void *);
extern ErrorType_t SIM808_CREG_resp(char *, void *);
extern ErrorType_t SIM808_CMGF_resp(char *, void *);
extern ErrorType_t SIM808_GPRS_status(char *, void *);
extern ErrorType_t SIM808_NTP_status(char *, void *);
extern ErrorType_t SIM808_parseTime(char *, void *);
extern ErrorType_t SIM808_ip_addr(char *, void *);
extern ErrorType_t SIM808_location(char *, void *);

#endif /* DRIVERS_INC_AT_H_ */
