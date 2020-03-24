/*
 * storage.c
 *
 *  Created on: 31. o�u 2019.
 *      Author: User
 */

#include <string.h>
#include <stm32l1xx.h>
#include "storage.h"

#define SIZE_OF_NUMBER_T    sizeof(struct number_t)

static void storage_read(uint32_t *, uint32_t, uint32_t);
static bool storage_write(uint32_t *, uint32_t, uint32_t);

void
storage_get_number(struct number_t *num)
{
    size_t i, j;
    uint32_t data[SIZE_OF_NUMBER_T] = {0};
    DEBUG_INFO("Reading stored number...");
    storage_read(data, STORAGE_NUMBER_ADDR, sizeof(uint32_t) * SIZE_OF_NUMBER_T);
    num->type = (uint8_t) data[0];
    num->crc = (uint8_t) data[1];
    for (i = 2, j = 0; i < SIZE_OF_NUMBER_T && data[i] != '\0'; i++, j++)
    {
        num->num[j] = (char) data[i];
    }
}

bool
storage_save_number(struct number_t *num)
{
    size_t i, j;
    uint32_t data[SIZE_OF_NUMBER_T] = {0};
    DEBUG_INFO("Writing number to storage...");
    data[0] = num->type;
    data[1] = num->crc;
    for (i = 2, j = 0; i < SIZE_OF_NUMBER_T && num->num[j] != '\0'; i++, j++)
    {
        data[i] = num->num[j];
    }
    return storage_write(data, STORAGE_NUMBER_ADDR, SIZE_OF_NUMBER_T);

}

static void
storage_read(uint32_t *data, uint32_t addr, uint32_t len)
{
    /* copy data from EEPROM to RAM */
    memmove(data, (uint32_t *)addr, len);
}

static bool
storage_write(uint32_t *data, uint32_t start_addr, uint32_t len)
{
    HAL_FLASHEx_DATAEEPROM_Unlock();

    uint32_t *src = data;
    uint32_t dst = start_addr;
    /* write settings word (uint32_t) at a time */
    for (uint32_t i = 0; i < len; i++)
    {
        if (*src != *((uint32_t *) dst))
        {
            /* Write only if modified */
            if (HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, dst, *src) != HAL_OK)
            {
                return false;
            }
        }
        src++;
        dst += sizeof(uint32_t);
    }
    HAL_FLASHEx_DATAEEPROM_Lock();
    return true;
}
