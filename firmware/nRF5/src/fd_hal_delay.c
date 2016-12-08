#include "fd_hal_delay.h"

#include "fd_nrf.h"

void fd_hal_delay_ms(uint32_t ms) {
    nrf_delay_ms(ms);
}
