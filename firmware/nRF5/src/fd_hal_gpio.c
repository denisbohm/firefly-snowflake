#include "fd_hal_gpio.h"

#include "fd_snowflake.h"

#include "fd_nrf.h"

void fd_hal_gpio_initialize(void) {
    nrf_gpio_pin_clear(FD_SNOWFLAKE_PIN_LED_POWER);
    nrf_gpio_cfg_output(FD_SNOWFLAKE_PIN_LED_POWER);

    nrf_gpio_pin_clear(FD_SNOWFLAKE_PIN_LED_DIN);
    nrf_gpio_cfg_output(FD_SNOWFLAKE_PIN_LED_DIN);

    nrf_gpio_cfg_input(FD_SNOWFLAKE_PIN_USB_POWERED, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(FD_SNOWFLAKE_PIN_CHARGE_STATUS, NRF_GPIO_PIN_NOPULL);

/*
#define FD_SNOWFLAKE_PIN_BATTERY_SENSE 24
#define FD_SNOWFLAKE_PIN_BATTERY_DIV_2 1
#define FD_SNOWFLAKE_PIN_CHARGE_CURRENT 2
*/
}

void fd_hal_gpio_set(uint32_t pin, bool value) {
    nrf_gpio_pin_write(pin, value ? 1 : 0);
}

void fd_hal_gpio_on(uint32_t pin) {
    nrf_gpio_pin_set(pin);
}

void fd_hal_gpio_off(uint32_t pin) {
    nrf_gpio_pin_clear(pin);
}
