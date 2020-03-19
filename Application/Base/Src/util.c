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
