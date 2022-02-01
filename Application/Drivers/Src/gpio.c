/*
 * gpio.c
 *
 *  Created on: 31. sij 2022.
 *      Author: User
 */

#include "gpio.h"

void gpio_set_value(struct gpio_t *gpio, bool state)
{
    GPIO_PinState pin_state;

    pin_state = (state) ? GPIO_PIN_SET : GPIO_PIN_RESET;

    HAL_GPIO_WritePin(gpio->gpio_d, gpio->pin, pin_state);
}

int gpio_get_value(struct gpio_t *gpio)
{
    return (int)HAL_GPIO_ReadPin(gpio->gpio_d, gpio->pin);
}
