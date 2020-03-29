/*
 * storage.c
 *
 *  Created on: 31. o�u 2019.
 *      Author: User
 */

#include <string.h>
#include <stm32l1xx.h>
#include "storage.h"
#include "util.h"

/* Private defines */
#define EEPROM_START_ADDR           0x08080000
#define EEPROM_SIZE_BYTES           8192
#define USER1_NUMBER_ADDR_OFFSET    0  * sizeof(uint32_t)
#define USER2_NUMBER_ADDR_OFFSET    18 * sizeof(uint32_t)
#define DEVICE_PID_OFFSET           36 * sizeof(uint32_t)
/********************************************************************/
#define SIZE_OF_NUMBER_T    sizeof(struct number_t)

char crypt_key[81] = "UJ2bhMnTU5lesiImRKNtcoNFjt5dCsTutrG2omxzcoNFjt5dCsTutrG2omxzJc0Uhu2lizblj0B9OQSn";

static void storage_read(void *, uint32_t, size_t);
static bool storage_write(uint32_t *, uint32_t, size_t);
static void storage_crypt_data(uint32_t *, size_t);
static void storage_decrypt_data(uint32_t *, size_t);

/**
 * @brief: Write data to EEPROM. CRC will be calculated and stored
 * at to the EEPROM the head of data.
 * @param data: data to be written
 * @param data_len: number of instances to be written
 * @param type: type of data (see storage_type_t)
 * @return: true if write was successful
 */
bool
storage_write_data(uint32_t *data, size_t data_len, storage_type_t type)
{
    uint32_t offset;
    uint32_t crc;
    bool status;
    switch (type)
    {
    case USER1_PHONE_NUMBER:
        offset = USER1_NUMBER_ADDR_OFFSET;
        break;
    case USER2_PHONE_NUMBER:
        offset = USER2_NUMBER_ADDR_OFFSET;
        break;
    case DEVICE_PID:
        offset = DEVICE_PID_OFFSET;
        break;
    default:
        DEBUG_ERROR("Unknown data type! Can not write!");
        return false;
    }
    /* Generate and store crc */
    crc = (uint32_t)gencrc((uint8_t *) data, data_len);
    status = storage_write(&crc, offset, 1);

    if (status == true)
    {
        /* Crypt and store the data */
        storage_crypt_data(data, data_len);
        status = storage_write(data, offset + sizeof(crc), data_len);
    }
    return status;
}

/**
 * @brief: Read data from EEPROM and deliver it to caller.
 * @param data: data to be delivered
 * @param data_len: number of data instances to be read
 * @param type: type of stored value
 */
void
storage_read_data(void *data, size_t data_len, storage_type_t type)
{
    uint32_t offset;
    switch (type)
    {
    case USER1_PHONE_NUMBER:
        offset = USER1_NUMBER_ADDR_OFFSET;
        break;
    case USER2_PHONE_NUMBER:
        offset = USER2_NUMBER_ADDR_OFFSET;
        break;
    case DEVICE_PID:
        offset = DEVICE_PID_OFFSET;
        break;
    default:
        DEBUG_ERROR("Unknown data type! Can not read!");
        return;
    }
    storage_read(data, offset, data_len);
}

/**
 * @brief: Copy data from EEPROm and check its CRC.
 * @param offset: Offset regarding the base EEPROM address
 * @param count: number of instances to be copied.
 */
static void
storage_read(void *dest, uint32_t offset, size_t count)
{
    uint32_t raw_data[50] = {0};
    uint8_t data[50] = {0};
    uint32_t src = EEPROM_START_ADDR + offset;
    uint8_t read_crc;
    uint8_t calc_crc;

    /* Get crc */
    memmove(&read_crc, (uint32_t *)src, sizeof(read_crc));
    /* copy data from EEPROM to RAM */
    memmove(raw_data, (uint32_t *)(src + sizeof(uint32_t)), count * sizeof(uint32_t));

    /* Decrypt while in uint32_t */
    storage_decrypt_data(raw_data, count);

    /* Convert to uint8_t */
    for (size_t i = 0; i < count && raw_data[i] != 0U; i++)
    {
        data[i] = (uint8_t) raw_data[i];
    }

    /* Validate crc */
    calc_crc = gencrc((uint8_t *) raw_data, count);
    if (read_crc != calc_crc)
    {
        DEBUG_ERROR("CRC mismatch!");
        return;
    }
    /* Copy to destination */
    memmove(dest, data, count);
}

/**
 * @brief: Write WORDS to the EEPROM.
 * @param data: uint32_t data to be written.
 * @param offset: Offset regarding the base EEPROM address
 * @param count: number of uint32_t words to be written
 * @return: true if successful
 */
static bool
storage_write(uint32_t *data, uint32_t offset, size_t count)
{
    HAL_FLASHEx_DATAEEPROM_Unlock();

    uint32_t *src = data;
    uint32_t dst = EEPROM_START_ADDR + offset;
    /* write settings word (uint32_t) at a time */
    for (size_t i = 0; i < count; i++)
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

/**
 * @brief: simple encrypt function.
 * @param data: data to be encrypted
 * @param len: length of data to be encrypted
 */
static void
storage_crypt_data(uint32_t *data, size_t len)
{
    int i;
    for (i = 0; i < len && *data != (uint32_t)0; i++, data++)
    {
        *data = ~(*data);
        *data = *data^((uint32_t)crypt_key[i]);
    }
}

/**
 * @brief: decrypt encrypted data
 * @param data: data to be decrypted
 * @param len: length of data to be decrypted
 */
static void
storage_decrypt_data(uint32_t *data, size_t len)
{
    int i;
    for (i = 0; i < len && *data != 0U; i++, data++)
    {
        *data = *data^((uint32_t)crypt_key[i]);
        *data = ~(*data);
    }
}
