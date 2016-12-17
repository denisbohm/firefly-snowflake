#include "fd_api.h"
#include "fd_hal_app.h"
#include "fd_hal_ble.h"
#include "fd_hal_delay.h"
#include "fd_hal_gpio.h"
#include "fd_log.h"

#include "fd_snowflake.h"
#include "fd_breathe_animation.h"
#include "fd_WS2812B.h"

#include <stdbool.h>
#include <stdint.h>

uint32_t fd_snowflake_pause;
uint32_t fd_snowflake_animation_step;
const uint32_t *fd_snowflake_animation_grbzs;
bool fd_snowflake_step;

void fd_hal_ble_characteristic_value_change(uint16_t uuid, uint8_t *data, uint16_t length) {
}

void fd_hal_ble_gap_evt_connected(void) {
}

void fd_hal_ble_gap_evt_disconnected(void) {
}

void fd_hal_ble_gap_evt_tx_complete(uint8_t count) {
}

void fd_snowflake_next_animation_step(void) {
    if (fd_snowflake_pause > 0) {
        static const uint32_t all_off[] = {
                                    0x00000000,
                        0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                        0x00000000, 0x00000000, 0x00000000,
                                    0x00000000,
        };
        fd_WS2812B_data(all_off, 13);
        --fd_snowflake_pause;
        return;
    }

    // display animation pattern for this step
    fd_WS2812B_data(fd_snowflake_animation_grbzs, 13);

    // get ready for next step
    if (++fd_snowflake_animation_step >= fd_breathe_steps) {
        fd_snowflake_animation_step = 0;
        fd_snowflake_animation_grbzs = fd_breathe_grbzs;
    } else {
        fd_snowflake_animation_grbzs += 13;
    }

    if ((fd_snowflake_animation_step % 78) == 0) {
        fd_snowflake_pause = 80;
    }
}

#if 0
void main_test_pattern(void) {
    static const uint32_t grbs[] = {
                                0x11000000,
                    0x00110000, 0x00001100, 0x11000000,
        0x00110000, 0x00001100, 0x11000000, 0x00110000, 0x00001100,
                    0x00110000, 0x00001100, 0x11000000,
                                0x11000000,
    };
    fd_WS2812B_data(fd_snowflake_animation_grbzs, 13);
}
#endif

int main(void) {
    fd_hal_gpio_initialize();

    uint8_t service_base[16] = {0xB3, 0x49, 0x1D, 0x48, 0x47, 0x86, 0x79, 0x97, 0x07, 0x48, 0x3E, 0x55, 0x00, 0x00, 0x7F, 0x57};
    fd_hal_ble_configuration_t configuration = {
        .hardware_revision = {
            .major = 1,
            .minor = 0,
            .patch = 0,
        },
        .firmware_revision = {
            .major = 1,
            .minor = 0,
            .patch = 0,
        },
        .manufacturer = "Firefly Design LLC",
        .model = "Snowflake",
        .serial_number = "0000",
        .device_name = "flake-01",
        .service_base = service_base,
        .service_uuid = 0xB8B4,
    };
    fd_hal_app_initialize();
    fd_hal_ble_initialize(&configuration);
    fd_api_initialize();
    fd_api_initialize_ble();

    fd_hal_gpio_on(FD_SNOWFLAKE_PIN_LED_POWER);
    fd_hal_delay_ms(10);
    fd_WS2812B_initialize(FD_SNOWFLAKE_PIN_LED_DIN);
    fd_snowflake_pause = 0;
    fd_snowflake_animation_step = 0;
    fd_snowflake_animation_grbzs = fd_breathe_grbzs;
    fd_snowflake_step = false;

#if USE_WITH_SOFTDEVICE == 1
    fd_hal_ble_start_advertising();
    // 50 us to send BRG values to all LEDs
    // 50 us to reset the LED communication slot
    // 50 ms (20 Hz) between LED updates
    bool result = fd_hal_ble_timeslot_initialize(50000, 1000, fd_snowflake_next_animation_step);
    fd_log_assert(result);
    while (true) {
        fd_api_process();
        fd_hal_app_dispatch_and_wait();
    }
#else
    while (true) {
        fd_snowflake_next_animation_step();
        fd_hal_delay_ms(50);
    }
#endif
}