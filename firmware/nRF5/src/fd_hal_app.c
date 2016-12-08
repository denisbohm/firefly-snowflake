#include "fd_hal_app.h"

#include "fd_nrf.h"

void fd_hal_app_initialize(void) {
    APP_GPIOTE_INIT(3);
    APP_SCHED_INIT(1, 32);
    APP_TIMER_INIT(0, 8, app_timer_evt_schedule);
}

void fd_hal_app_dispatch_and_wait(void) {
    app_sched_execute();

    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}