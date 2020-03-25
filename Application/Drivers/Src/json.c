/*
 * json.c
 *
 *  Created on: Mar 25, 2020
 *      Author: ivan
 */

#include <string.h>
#include "json.h"
#include "util.h"

static void json_parse(char *, struct json_values_t *);

void
json_process(char *msg, struct json_values_t *json_values)
{
    char json_validate[] = "application/jsonContent";
    char *p = NULL;
    char *start = NULL;
    char *end = NULL;
    char json[150] = {0};
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
    json_parse(json, json_values);
}

static void
json_parse(char *json, struct json_values_t *json_values)
{
    char *key = json_values->key;
    char *value = json_values->value;
    int key_len = sizeof(json_values->key);
    int value_len = sizeof(json_values->value);
    char *start = NULL;
    char *end = NULL;
    size_t len = 0;
    /* Processing key */
    start = strchr(json, '"');
    if (start == NULL)
    {
        DEBUG_ERROR("Invalid json input!");
        return;
    }
    end = strchr(++start, '"');
    if (end == NULL)
    {
        DEBUG_ERROR("Invalid json input!");
        return;
    }
    len = end - start;
    if (len >= key_len)
    {
        DEBUG_ERROR("Json key too long!");
        return;
    }
    memcpy(key, start, len);
    /* Processing value */
    end++;
    if (*end != ':')
    {
        DEBUG_ERROR("Invalid json input!");
        return;
    }
    start = strchr(end, '"');
    if (start == NULL)
    {
        DEBUG_ERROR("Invalid json input!");
        return;
    }
    end = strchr(++start, '"');
    if (end == NULL)
    {
        DEBUG_ERROR("Invalid json input!");
        return;
    }
    len = end - start;
    if (len >= value_len)
    {
        DEBUG_ERROR("Json value too long!");
        return;
    }
    memcpy(value, start, len);
}
