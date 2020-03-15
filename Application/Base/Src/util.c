/*
 * util.c
 *
 *  Created on: Mar 15, 2020
 *      Author: ivan
 */
#include "util.h"

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
