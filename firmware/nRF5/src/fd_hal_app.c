#include "fd_hal_app.h"

#include "fd_nrf.h"

#define APP_TIMER_PRESCALER 256 // Value of the RTC1 PRESCALER register.

APP_TIMER_DEF(fd_hal_app_timer_1);

fd_hal_app_timer_callback_t fd_hal_app_timer_callback_1;

static void fd_hal_app_timer_1_handler(void *context) {
    if (fd_hal_app_timer_callback_1) {
        (*fd_hal_app_timer_callback_1)();
    }
}

void fd_hal_app_initialize(void) {
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

    NRF_CLOCK->TASKS_LFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0);

    APP_GPIOTE_INIT(3);
    APP_SCHED_INIT(8, 32);
    APP_TIMER_INIT(APP_TIMER_PRESCALER, 8, app_timer_evt_schedule);

    uint32_t err_code = app_timer_create(&fd_hal_app_timer_1, APP_TIMER_MODE_SINGLE_SHOT, fd_hal_app_timer_1_handler);
    APP_ERROR_CHECK(err_code);
}

void fd_hal_app_dispatch_and_wait(void) {
    app_sched_execute();

    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

void fd_hal_app_timer_start(uint32_t ms, fd_hal_app_timer_callback_t callback) {
    fd_hal_app_timer_callback_1 = callback;

    uint32_t err_code = app_timer_start(fd_hal_app_timer_1, APP_TIMER_TICKS(ms, APP_TIMER_PRESCALER), 0);
    APP_ERROR_CHECK(err_code);
}

void fd_hal_app_timer_stop(void) {
    uint32_t err_code = app_timer_stop(fd_hal_app_timer_1);
    APP_ERROR_CHECK(err_code);
}