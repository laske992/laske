/*
 * storage.h
 *
 *  Created on: 31. o�u 2019.
 *      Author: User
 */

#ifndef DRIVERS_INC_STORAGE_H_
#define DRIVERS_INC_STORAGE_H_

#include <stdbool.h>

typedef enum {
    USER1_PHONE_NUMBER = 0,
    USER2_PHONE_NUMBER,
    DEVICE_PID
} storage_type_t;

bool storage_write_data(uint32_t *, size_t, storage_type_t);
void storage_read_data(void *, size_t, storage_type_t);

#endif /* DRIVERS_INC_STORAGE_H_ */
