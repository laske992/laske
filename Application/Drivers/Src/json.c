/*
 * json.c
 *
 *  Created on: Mar 25, 2020
 *      Author: ivan
 */

#include <string.h>
#include "json.h"
#include "http.h"
#include "util.h"

struct json_data_t {
    char message[80];
    char measurement_auth[50];
    char measurement_url[80];
};

struct json_product_t {
    char serial_num[40];
    char version[7];
};

struct json_subscribe_req_t {
    char name[21];
    struct json_product_t product;
};

/* Static variables */

/***********************************************/

/* Static functions */
static void json_parse(char *, struct json_data_t *);
static bool json_parse_key(char **, char *, size_t);
static bool json_parse_value(char *, char *, size_t);
static void json_set_data(struct json_data_t *);

/***********************************************/

/**
 * @brief: Validate server response to be in json format.
 * If in json format, parse it, otherwise return.
 * @param msg: server response
 */
void
json_process(char *msg)
{
    struct json_data_t jd;
    char json_validate[] = "application/json";
    char *p = NULL;
    char *start = NULL;
    char *end = NULL;
    char json[200] = {0};
    size_t len = 0;
    p = strstr(msg, json_validate);
    if (p == NULL)
    {
        DEBUG_ERROR("Invalid json input!");
        return;
    }
    start = strchr(p, '{');
    if (start == NULL)
    {
        DEBUG_ERROR("Invalid json input!");
        return;
    }
    end = strrchr(start, '}');
    if (end == NULL)
    {
        DEBUG_ERROR("Invalid json input!");
        return;
    }
    len = end - start;
    if (len >= sizeof(json))
    {
        DEBUG_ERROR("Json input too long!");
        return;
    }
    memmove(json, start, len);
    memset(&jd, 0, sizeof(struct json_data_t));
    json_parse(json, &jd);
    json_set_data(&jd);
}

/**
 * @brief: Prepare JSON initial request
 * @param json: variable to write JSON request
 * @param json_maxlen: maximum size of JSON request
 * @return: size of JSON request
 */
size_t
json_subscribe_prepare(char *json, size_t json_maxlen)
{
    struct json_subscribe_req_t js = {
            .name = "Vaga23",
            .product.serial_num = "884f40b5-8e4d-4dec-85be-b9ffe5dd291b",
            .product.version = "v1.1"
    };
    size_t len;
    len = snprintf(json, json_maxlen,\
            "{\"name\": \"%s\", "\
            "\"product\": {"\
            "\"serial_number\": \"%s\", "\
            "\"version_of_a_board\": \"%s\"}"\
            "}",\
            js.name,\
            js.product.serial_num,\
            js.product.version);
    if (len >= json_maxlen)
    {
        len = 0;
        DEBUG_ERROR("Couldn't prepare JSON subscribe request!");
    }
    return len;
}

/**
 * @brief: parse each key-value pair of json input
 * @param json: string of json input
 * @param jd: structure which holds key-value values
 */
static void
json_parse(char *json, struct json_data_t *jd)
{
    char key[50] = {0};
    char value[50] = {0};
    char *token = NULL;
    token = strtok(json, ",");
    while (token != NULL)
    {
        if (json_parse_key(&token, key, sizeof(key)))
        {
            if (json_parse_value(token, value, sizeof(value)))
            {
                if (strncmp(key, "message", strlen("message")))
                {
                    memmove(jd->message, value, sizeof(jd->message));
                }
                else if (strncmp(key, "measurement_auth", strlen("measurement_auth")))
                {
                    memmove(jd->measurement_auth, value, sizeof(jd->measurement_auth));
                }
                else if (strncmp(key, "measurement_url", strlen("measurement_url")))
                {
                    memmove(jd->measurement_url, value, sizeof(jd->measurement_url));
                }
                else
                {
                    /* Unknown key */
                    DEBUG_INFO("Unknown json key received!");
                }
            }
            else
            {
                DEBUG_ERROR("Invalid json input (value)!");
                break;
            }
        }
        else
        {
            DEBUG_ERROR("Invalid json input (key)!");
            break;
        }
        memset(key, 0, sizeof(key));
        memset(value, 0, sizeof(value));
        token = strtok(NULL, ",");
    }
}

/**
 * @brief: Parse key from json fromat
 * @param **buf: pointer to start of json input
 * @param *key: variable to store parsed key
 * @param keylen: maximum key length
 * @return: true if key parsed, otherwise false
 */
static bool
json_parse_key(char **buf, char *key, size_t keylen)
{
    char *start = NULL;
    char *end = NULL;
    size_t len = 0;
    /* Processing key */
    start = strchr(*buf, '"');
    if (start == NULL)
    {
        DEBUG_ERROR("Invalid json input!");
        return false;
    }
    end = strchr(++start, '"');
    if (end == NULL)
    {
        DEBUG_ERROR("Invalid json input!");
        return false;
    }
    len = end - start;
    if (len >= keylen)
    {
        DEBUG_ERROR("Json key too long!");
        return false;
    }
    memcpy(key, start, len);
    *buf = ++end;
    return true;
}

/**
 * @brief: Parse value from json fromat
 * @param *buf: pointer to the ':' of json input
 * @param *value: variable to store parsed value
 * @param valuelen: maximum value length
 * @return: true if value parsed, otherwise false
 */
static bool
json_parse_value(char *buf, char *value, size_t valuelen)
{
    char *start = NULL;
    char *end = NULL;
    size_t len = 0;
    /* Processing value */
    if (*buf != ':')
    {
        DEBUG_ERROR("Invalid json input!");
        return false;
    }
    start = strchr(buf, '"');
    if (start == NULL)
    {
        DEBUG_ERROR("Invalid json input!");
        return false;
    }
    end = strchr(++start, '"');
    if (end == NULL)
    {
        DEBUG_ERROR("Invalid json input!");
        return false;
    }
    len = end - start;
    if (len >= valuelen)
    {
        DEBUG_ERROR("Json value too long!");
        return false;
    }
    memcpy(value, start, len);
    return true;
}

/**
 * @brief: Send parsed values to http.c
 * @param jd: structure which holds json values
 */
static void
json_set_data(struct json_data_t *jd)
{
    if (jd->message[0] != '\0')
    {
        http_set_message(jd->message);
    }
    if (jd->measurement_auth[0] != '\0')
    {
        http_set_measurement_auth(jd->measurement_auth);
    }
    if (jd->measurement_url[0] != '\0')
    {
        http_set_measurement_url(jd->measurement_url);
    }
}
