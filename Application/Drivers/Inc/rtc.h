/*
 * rtc.h
 *
 *  Created on: Sep 1, 2018
 *      Author: Mislav
 */

#ifndef DRIVERS_INC_RTC_H_
#define DRIVERS_INC_RTC_H_

#include <stdint.h>

struct rtc_time_t {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

void rtc_init(struct rtc_time_t *timestamp);
void rtc_deinit(void);

#endif /* DRIVERS_INC_RTC_H_ */
