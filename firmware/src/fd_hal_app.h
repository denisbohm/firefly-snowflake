#ifndef FD_HAL_APP_H
#define FD_HAL_APP_H

#include <stdint.h>

void fd_hal_app_initialize(void);

void fd_hal_app_dispatch_and_wait(void);

typedef void (*fd_hal_app_timer_callback_t)(void);

void fd_hal_app_timer_start(uint32_t ms, fd_hal_app_timer_callback_t callback);
void fd_hal_app_timer_stop(void);

#endif