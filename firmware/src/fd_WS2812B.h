#ifndef FD_WS2812B_H
#define FD_WS2812B_H

#include <stdint.h>

void fd_WS2812B_initialize(uint32_t pin);

void fd_WS2812B_data(uint32_t *grbs, uint32_t count);

#endif