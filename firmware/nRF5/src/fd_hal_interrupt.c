#include "fd_hal_interrupt.h"

#include "fd_nrf.h"

static uint32_t fd_hal_interrupt_lock_count = 0;

uint32_t fd_hal_interrupt_disable(void)
{
    uint8_t is_nested_critical_region = 0;
    if (fd_hal_interrupt_lock_count == 0) {
#ifdef SOFTDEVICE_PRESENT
        app_util_critical_region_enter(&is_nested_critical_region);
#else
        app_util_critical_region_enter(0);
#endif
    }
    if (fd_hal_interrupt_lock_count < UINT32_MAX) {
        fd_hal_interrupt_lock_count++;
    }
    return is_nested_critical_region;
}

void fd_hal_interrupt_restore(uint32_t is_nested_critical_region) {
    if (fd_hal_interrupt_lock_count > 0) {
        if (--fd_hal_interrupt_lock_count == 0) {
#ifdef SOFTDEVICE_PRESENT
            app_util_critical_region_exit(is_nested_critical_region);
#else
            app_util_critical_region_exit(0);
#endif
        }
    }
}