/*
 * storage.h
 *
 *  Created on: 31. o�u 2019.
 *      Author: User
 */

#ifndef DRIVERS_INC_STORAGE_H_
#define DRIVERS_INC_STORAGE_H_

#include "util.h"

#define DATA_EEPROM_START_ADDR      0x08080000
#define DATA_EEPROM_SIZE_BYTES      8192
#define STORAGE_NUMBER_ADDR         DATA_EEPROM_START_ADDR
#define STORAGE_VALUES_ADDR			DATA_EEPROM_START_ADDR + 50

void storage_get_number(struct number_t *);
bool storage_save_number(struct number_t *);

#endif /* DRIVERS_INC_STORAGE_H_ */
