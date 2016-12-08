#ifndef FD_HAL_INTERRUPT_H
#define FD_HAL_INTERRUPT_H

#include <stdint.h>

uint32_t fd_hal_interrupt_disable(void);
void fd_hal_interrupt_restore(uint32_t context);

#endif
