/*
 * util.c
 *
 *  Created on: Mar 15, 2020
 *      Author: ivan
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "util.h"
#include "communication.h"
#include "usb.h"

static void
vprint(const char *fmt, va_list argp)
{
    char buf[512] = {0};
    int len;
    len = vsnprintf(buf, sizeof(buf), fmt, argp);
    if (len > 0)
    {
        CDC_Transmit_FS(buf, strlen(buf));
    }
}

void
debug_init(void)
{
    USB_Init();
    communicationInit();
}

void
debug_printf(const char *fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    vprint(fmt, argp);
    va_end(argp);
}

/*
 * Copy at most maxlen chars from src to dest.
 * Return number of copied chars.
 */
int
put_data(char **dest, const char *src, int maxlen)
{
    int count = 0;
    if (maxlen > 0)
    {
        while((*src != '\0') && ((maxlen--) != 0))
        {
            PUT_BYTE(*dest, *src++);
            count++;
        }
    }
    return count;
}

uint8_t
gencrc(uint8_t *data, size_t len)
{
    uint8_t crc = 0xff;
    size_t i, j;
    for (i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
        {
            if ((crc & 0x80) != 0)
            {
                crc = (uint8_t)((crc << 1) ^ 0x31);
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void
delay_ms(uint16_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}
