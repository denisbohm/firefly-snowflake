#ifndef FD_HAL_GPIO_H
#define FD_HAL_GPIO_H

#include <stdbool.h>
#include <stdint.h>

void fd_hal_gpio_initialize(void);
void fd_hal_gpio_set(uint32_t pin, bool value);
void fd_hal_gpio_on(uint32_t pin);
void fd_hal_gpio_off(uint32_t pin);

#endif