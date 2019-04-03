/*
 * storage.c
 *
 *  Created on: 31. ožu 2019.
 *      Author: User
 */

#include <string.h>
#include <stm32l1xx.h>
#include "util.h"

void
storage_read(void *dest, uint32_t addr, uint32_t len)
{
    /* copy data from EEPROM to RAM */
    memmove(dest, (uint32_t *)addr, len);
}

bool
storage_write(void *data, uint32_t addr, uint32_t len)
{
    HAL_FLASHEx_DATAEEPROM_Unlock();

    uint32_t *src = (uint32_t*)data;
    uint32_t *dst = (uint32_t*)addr;

    /* write settings word (uint32_t) at a time */
    for (uint32_t i = 0; i < len; i++) {
        if (*dst != *src) { /* write only if value has been modified */
			if (HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE, addr, *src) != HAL_OK) {
				return false;
			}
        }
        src++;
        dst++;
    }
    HAL_FLASHEx_DATAEEPROM_Lock();
    return true;
}
