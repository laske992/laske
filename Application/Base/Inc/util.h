/*
 * util.h
 *
 *  Created on: Mar 30 2019.
 *      Author: User
 */

#ifndef BASE_INC_UTIL_H_
#define BASE_INC_UTIL_H_

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "stm32l1xx_hal.h"

#define SIM_RI_IRQ_SMS	( 1 << 0 )
#define SIM_RI_IRQ_CALL	( 1 << 1 )
#define BIT_2	( 1 << 2 )
#define MAX_AT_CMD_LEN 558  /* AT + 556 cmd chars */

#define PUT_BYTE(p, byte) \
	*(p) = (byte); (p)++

#define TO_DIGIT(p) \
	(((p) > '0' && (p) < '9') ? ((p) - 48) : (p))

#define STR_COMPARE(s1, s2) \
	strncmp((char *)(s1), (char *)(s2), strlen((char *)(s2)))

#define MAXCOUNT(s) (sizeof(s)/(sizeof(s[0])))

#define MAX_NUM_SIZE 30
#define NUM_TYPE_SIZE 3

typedef enum {
	CALLTask = 1,
	ADCTask,
	SMSTask
} _TaskId;

typedef enum {
    Unknown_type = 129,
    National_type = 161,
    International_type = 145,
    Net_specific_type = 177
} num_type_t;

struct number_t {
    num_type_t type;
    uint8_t crc;
    char num[MAX_NUM_SIZE];
} __attribute__((__packed__));

void debug_init();
void debug_printf(const char *fmt, ...);
#define DEBUG_INFO(s, ...) debug_printf("[INFO][%08d]%s:%d:%s(): " s "\r\n", HAL_GetTick(), __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#define DEBUG_ERROR(s, ...) debug_printf("[ERROR][%08d]%s:%d:%s(): " s "\r\n", HAL_GetTick(), __FILE__, __LINE__, __func__, ##__VA_ARGS__)

void _setSignal(_TaskId, int32_t);
int put_data(char **, const char *, int);
uint8_t gencrc(uint8_t *, size_t);

#endif /* BASE_INC_UTIL_H_ */
