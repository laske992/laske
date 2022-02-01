/*
 * gpio.h
 *
 *  Created on: 31. sij 2022.
 *      Author: User
 */

#ifndef DRIVERS_INC_GPIO_H_
#define DRIVERS_INC_GPIO_H_

#include "stm32l1xx.h"
#include "stm32l1xx_hal.h"
#include "stm32l1xx_hal_def.h"

#include "util.h"
#include "config.h"

struct gpio_t {
    GPIO_TypeDef *gpio_d;
    uint16_t pin;
};

extern void gpio_set_value(struct gpio_t *, bool);
extern int gpio_get_value(struct gpio_t *);

#endif /* DRIVERS_INC_GPIO_H_ */
