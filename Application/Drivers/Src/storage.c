/*
 * storage.c
 *
 *  Created on: 31. ožu 2019.
 *      Author: User
 */

#include <stm32l1xx.h>
#include <string.h>


/* CRC handler declaration */
CRC_HandleTypeDef   CrcHandle;


void
CRC_Init(void)
{
	  CrcHandle.Instance = CRC;

	  if (HAL_CRC_Init(&CrcHandle) != HAL_OK)
	  {
	    /* Initialization Error */
	    Error_Handler();
	  }
}

void storage_read(uint32_t *dest, uint32_t addr, uint32_t len)
{
    /* copy data from EEPROM to RAM */
    memcpy(dest, (uint32_t *)addr, len);


   /* __HAL_RCC_CRC_CLK_ENABLE();

    CRC_ResetDR();

    uint32_t computed_crc = CRC_CalcBlockCRC(
            (uint32_t *)GLOBAL_settings_ptr,
            (sizeof(settings_t)-sizeof(uint32_t))/sizeof(uint32_t)size minus the crc32 at the end, IN WORDS
    );

    __HAL_RCC_CRC_CLK_DISABLE();

    if (computed_crc != GLOBAL_settings_ptr->crc32){
        _settings_reset_to_defaults();
    }*/
}

HAL_StatusTypeDef
storage_write(char *data, uint32_t addr, uint32_t len)
{

    __HAL_RCC_CRC_CLK_ENABLE();

    //CRC_ResetDR();

    /*GLOBAL_settings_ptr->crc32 = CRC_CalcBlockCRC( //calculate new CRC
            (uint32_t *)GLOBAL_settings_ptr,
            (sizeof(settings_t)-sizeof(uint32_t))/sizeof(uint32_t)size minus the crc32 at the end, IN WORDS
    );
    __HAL_RCC_CRC_CLK_DISABLE();*/

    HAL_FLASHEx_DATAEEPROM_Unlock();

    uint32_t *src = (uint32_t*)data;
    uint32_t *dst = (uint32_t*)addr;

    /* write settings word (uint32_t) at a time */
    for (uint32_t i = 0; i < len; i++) {
        if (*dst != *src) { /* write only if value has been modified */
			if (HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, addr, *src) != HAL_OK) {
				return HAL_ERROR;
			}
        }
        src++;
        dst++;
    }
    HAL_FLASHEx_DATAEEPROM_Lock();
    //settings_read();
    return 0;
}
