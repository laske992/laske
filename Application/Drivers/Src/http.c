/*
 * http.c
 *
 *  Created on: Mar 26, 2020
 *      Author: ivan
 */

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "sim808.h"
#include "json.h"
#include "util.h"

#define HTTP_POST_METHOD        "POST"
#define HTTP_SUBSCRIBE_PATH     "/api/v1/sites"
#define HTTP_VERSION            "HTTP/1.0"
#define HTTP_HOST               "0.tcp.ngrok.io"
#define HTTP_HOST_PORT          "13180"
#define HTTP_CONTENT_TYPE_JSON  "application/json"

typedef enum {
    HTTP_RAW_TCP_PACKET = 0,
    HTTP_PACKET
} http_packet_type_t;

typedef enum {
    HTTP_POST = 0,
    HTTP_GET
} http_method_t;

typedef enum {
    HTTP_UNKNOWN = 0,
    HTTP_SUCCESSFUL,
    HTTP_ERROR,
    HTTP_NOT_FOUND,
    HTTP_CONFLICT,
    HTTP_UNPROCESSABLE_ENTITY,
    HTTP_SERVER_ERROR
} http_code_t;

/* Static variables */
char http_url[80];
char http_code_str[7][4] = {
        "UNK",
        "201",
        "401",
        "404",
        "409",
        "422",
        "500"
};

#define HTTP_CHECK_CODE(p, code_type)\
    (strncmp(p, http_code_str[code_type], strlen(http_code_str[code_type])))

/******************************************************/

/* Static Functions */
static void http_read_response(char *, http_packet_type_t);
static http_code_t http_validate(char *);
static void http_send(char *, http_packet_type_t, http_method_t, char *);
static void http_send_raw_tcp(char *, char *);
static void http_send_http(char *, http_method_t, char *);
static bool http_subscribe_prepare(char *, size_t );

/******************************************************/

void
web_server_subscribe(void)
{
    char subscription_msg[300] = {0};
    char server_response[512] = {0};
    DEBUG_INFO("Subscribing to web server...");
    if (http_subscribe_prepare(subscription_msg, sizeof(subscription_msg)))
    {
        http_send(subscription_msg, HTTP_RAW_TCP_PACKET, HTTP_POST, server_response);
        if (server_response[0] != '\0')
        {
            http_read_response(server_response, HTTP_RAW_TCP_PACKET);
        }
        else
        {
            DEBUG_ERROR("Server response not received!");
        }
    }
    else
    {
        DEBUG_ERROR("Can not prepare subscription message!");
    }
}

void
http_set_message(char *message)
{
    if (message != NULL && *message != '\0')
    {
        DEBUG_INFO("http measurement message: %s", message);
        /* TODO */
    }
}

void
http_set_measurement_auth(char *auth)
{
    if (auth != NULL && *auth != '\0')
    {
        DEBUG_INFO("http measurement auth: %s", auth);
        /* TODO */
    }
}

void
http_set_measurement_url(char *url)
{
    if (url != NULL && *url != '\0')
    {
        DEBUG_INFO("http measurement url: %s", url);
        strncpy(http_url, url, sizeof(http_url));
    }
}

/**
 * @brief: parse server http response
 * @param response: response from server
 * @param type: type of request
 */
static void
http_read_response(char *response, http_packet_type_t type)
{
    http_code_t http_code;
    switch(type)
    {
    case HTTP_RAW_TCP_PACKET:
        http_code = http_validate(response);
        if (http_code == HTTP_SUCCESSFUL)
        {
            DEBUG_INFO("Successful send to server. Processing response...");
            json_process(response);
        }
        break;
    case HTTP_PACKET:
        /* TODO */
        break;
    default:
        break;
    }
}

/**
 * @brief: validate that msg has a valid HTTP version at header,
 * and read the http code received from server.
 * @param msg: response from server
 * @return http response code
 */
static http_code_t
http_validate(char *msg)
{
    char *p = NULL;
    http_code_t code = HTTP_UNKNOWN;
    p = strstr(msg, HTTP_VERSION);
    if (p == NULL)
    {
        return code;
    }
    p = strchr(p, ' ');
    if (p == NULL)
    {
        return code;
    }
    p++;
    if (HTTP_CHECK_CODE(p, HTTP_SUCCESSFUL) == 0)
    {
        code = HTTP_SUCCESSFUL;
    }
    else if (HTTP_CHECK_CODE(p, HTTP_ERROR) == 0)
    {
        DEBUG_ERROR("HTTP internal error!");
        code = HTTP_ERROR;
    }
    else if (HTTP_CHECK_CODE(p, HTTP_NOT_FOUND) == 0)
    {
        DEBUG_ERROR("HTTP NOT FOUND!");
        code = HTTP_NOT_FOUND;
    }
    else if (HTTP_CHECK_CODE(p, HTTP_CONFLICT) == 0)
    {
        DEBUG_ERROR("Conflict with the current state of resource!");
        code = HTTP_CONFLICT;
    }
    else if (HTTP_CHECK_CODE(p, HTTP_UNPROCESSABLE_ENTITY) == 0)
    {
        DEBUG_ERROR("HTTP not proccessable request!");
        code = HTTP_UNPROCESSABLE_ENTITY;
    }
    else if (HTTP_CHECK_CODE(p, HTTP_SERVER_ERROR) == 0)
    {
        DEBUG_ERROR("Server internal error!");
        code = HTTP_SERVER_ERROR;
    }
    else
    {
        DEBUG_ERROR("Unknown HTTP code: %s", p);
        code = HTTP_UNKNOWN;
    }
    return code;
}

/**
 * @brief: send HTTP packet
 * @param msg: message to be sent
 * @param type: Raw TCP packet or HTTP encapsulated SIM808 commands
 * @param method: POST or GET
 * @param response: fetch server response
 */
static void
http_send(char *msg, http_packet_type_t type, http_method_t method, char *response)
{
    switch(type)
    {
    case HTTP_RAW_TCP_PACKET:
        http_send_raw_tcp(msg, response);
        break;
    case HTTP_PACKET:
        http_send_http(msg, method, response);
        break;
    default:
        break;
    }
}

/**
 * @brief: send raw tcp packet towards HTTP_HOST:HTTP_HOST_PORT
 * @param msg message to be sent
 * @param response fetch server response
 */
static void
http_send_raw_tcp(char *msg, char *response)
{
    char tcp_params[35] = {0};
    snprintf(tcp_params, sizeof(tcp_params), "\"TCP\",\"%s\",\"%s\"", HTTP_HOST, HTTP_HOST_PORT);
    SIM808_send_TCP_request(msg, tcp_params, response);
}

/**
 * @brief: send POST/GET request using SIM808 HTTP commands.
 * @param msg message to be sent
 * @param method POST or GET
 * @param response fetch server response if method is GET
 */
static void
http_send_http(char *msg, http_method_t method, char *reponse)
{
    switch (method)
    {
    case HTTP_POST:
        SIM808_send_POST_request(msg, http_url);
        break;
    case HTTP_GET:
        SIM808_send_GET_request(http_url, reponse);
        break;
    default:
        break;
    }
}

/**
 * @brief: Prepare HTTP packet to be sent on initial subscribe to
 * web server.
 * @param subscribe_msg: buffer to print subscription message
 * @param maxlen: maximum number of chars to be written to subscribe_msg
 * @return true if successful write, otherwise false
 */
static bool
http_subscribe_prepare(char *subscribe_msg, size_t maxlen)
{
    char json_content[130] = {0};
    size_t json_content_len = 0;
    int count = 0;
    bool status = true;

    json_content_len = json_subscribe_prepare(json_content, sizeof(json_content));
    if (json_content_len == 0)
    {
        status = false;
        goto out;
    }
    count = snprintf(subscribe_msg, maxlen,\
            "%s %s %s\r\n"\
            "HOST: %s\r\n"\
            "Accept: %s\r\n"\
            "Content-Type: %s\r\n"\
            "Content-Length: %d\r\n\r\n"\
            "%s\r\n\n",\
            HTTP_POST_METHOD, HTTP_SUBSCRIBE_PATH, HTTP_VERSION,\
            HTTP_HOST,\
            HTTP_CONTENT_TYPE_JSON,\
            HTTP_CONTENT_TYPE_JSON,
            json_content_len,\
            json_content);
    if (count >= maxlen)
    {
        status = false;
    }
    out:
    return status;
}
