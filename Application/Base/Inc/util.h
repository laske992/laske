/*
 * util.h
 *
 *  Created on: 30. ožu 2019.
 *      Author: User
 */

#ifndef BASE_INC_UTIL_H_
#define BASE_INC_UTIL_H_

#include <string.h>
#include <stdbool.h>

#define BIT_0	( 1 << 0 )
#define BIT_1	( 1 << 1 )
#define BIT_2	( 1 << 2 )

#define PUT_DATA(p, d) \
	strncpy((char *)(p), (const char *)(d), strlen((char *)(d))); \
	(p) += strlen((char *)(d))

#define PUT_BYTE(p, byte) \
	*(p) = (byte); (p)++

#define TO_DIGIT(p) \
	(((p) > '0' && (p) < '9') ? ((p) - 48) : (p))

#define STR_COMPARE(s1, s2) \
	strncmp((char *)(s1), (char *)(s2), strlen((char *)(s2)))

#define MAXCOUNT(s) (sizeof(s)/(sizeof(s[0])))

typedef enum {
	CALLTask = 1,
	ADCTask,
	SMSTask
} _TaskId;

void _setSignal(_TaskId, int32_t);

#endif /* BASE_INC_UTIL_H_ */
