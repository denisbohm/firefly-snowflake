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

const fd_snowflake_program_t *fd_snowflake_program;

bool fd_snowflake_leds_powered;

uint32_t fd_snowflake_instruction_index;
fd_snowflake_operation_t fd_snowflake_operation;

uint32_t fd_snowflake_illuminate_step;
uint32_t fd_snowflake_illuminate_grbz_index;
uint32_t fd_snowflake_illuminate_step_count;

uint32_t fd_snowflake_pause_steps_remaining;

uint32_t fd_snowflake_sleep_seconds_remaining;

void fd_hal_ble_characteristic_value_change(uint16_t uuid, uint8_t *data, uint16_t length) {
}

void fd_hal_ble_gap_evt_connected(void) {
}

void fd_hal_ble_gap_evt_disconnected(void) {
}

void fd_hal_ble_gap_evt_tx_complete(uint8_t count) {
}

void fd_snowflake_step(void);

fd_hal_ble_time_slot_callback_result_t fd_snowflake_time_slot(void) {
    switch (fd_snowflake_operation) {
        case fd_snowflake_operation_illuminate: { // 16-bit step count, 16-bit starting grbz index
            const uint32_t *grbzs = &fd_snowflake_program->grbzs[fd_snowflake_illuminate_grbz_index];
            fd_WS2812B_data(grbzs, 13);
            fd_snowflake_illuminate_grbz_index += 13;

            if (++fd_snowflake_illuminate_step >= fd_snowflake_illuminate_step_count) {
                fd_snowflake_step();
            }
        } break;
        case fd_snowflake_operation_pause: { // 16-bit step count (holds current LED colors)
            if (fd_snowflake_pause_steps_remaining-- == 0) {
                fd_snowflake_step();
            }
        } break;
        default:
            fd_log_assert_fail("unexpected operation");
    }
    return fd_hal_ble_time_slot_callback_result_continue;
}

void fd_snowflake_power_on_leds(void) {
    // turn on LED power and wait for rail to come up
    fd_snowflake_leds_powered = true;
    fd_hal_gpio_on(FD_SNOWFLAKE_PIN_LED_POWER);
    fd_hal_delay_us(500);
}

void fd_snowflake_power_off_leds(void) {
    fd_hal_gpio_off(FD_SNOWFLAKE_PIN_LED_POWER);
    fd_snowflake_leds_powered = false;
}

void fd_snowflake_start_time_slots(void) {
    if (fd_snowflake_leds_powered) {
        return;
    }

    fd_snowflake_power_on_leds();

#if USE_WITH_SOFTDEVICE == 1
    // start LED animation time slots
    //
    // 50 ms (20 Hz) between LED updates
    // 50 us to send BRG values to all LEDs, plus 50 us to reset the LED communication slot
    // !!! 1000 us seems to be lowest value Nordic time slot SDK will accept (or hello hard fault) -denis
    bool result = fd_hal_ble_time_slot_initialize(50000, 1000, fd_snowflake_time_slot);
    fd_log_assert(result);
#endif
}

void fd_snowflake_stop_time_slots(void) {
    if (!fd_snowflake_leds_powered) {
        return;
    }

#if USE_WITH_SOFTDEVICE == 1
    fd_hal_ble_time_slot_close();
#endif

    fd_snowflake_power_off_leds();
}

void fd_snowflake_illuminate(uint32_t step_count, uint32_t grbz_index) {
    fd_snowflake_illuminate_step = 0;
    fd_snowflake_illuminate_grbz_index = grbz_index;
    fd_snowflake_illuminate_step_count = step_count;

    fd_snowflake_start_time_slots();
}

void fd_snowflake_pause(uint32_t step_count) {
    fd_snowflake_pause_steps_remaining = step_count;
}

void fd_snowflake_sleep_done(void) {
    fd_snowflake_step();
}

void fd_snowflake_sleep(uint32_t seconds) {
    fd_snowflake_stop_time_slots();

    fd_hal_app_timer_start(seconds * 1000, fd_snowflake_sleep_done);

    fd_snowflake_sleep_seconds_remaining = seconds;
}

void fd_snowflake_restart(void) {
    fd_snowflake_instruction_index = 0;
    fd_snowflake_illuminate_step = 0;
    fd_snowflake_illuminate_grbz_index = 0;
    fd_snowflake_illuminate_step_count = 0;
    fd_snowflake_pause_steps_remaining = 0;
    fd_snowflake_sleep_seconds_remaining = 0;

    fd_snowflake_step();
}

uint32_t fd_snowflake_next_operand(void) {
    uint32_t b0 = fd_snowflake_program->instructions[fd_snowflake_instruction_index++];
    uint32_t b1 = fd_snowflake_program->instructions[fd_snowflake_instruction_index++];
    return (b1 << 8) | b0;
}

void fd_snowflake_step(void) {
    fd_snowflake_operation = fd_snowflake_program->instructions[fd_snowflake_instruction_index++];
    switch (fd_snowflake_operation) {
        case fd_snowflake_operation_illuminate: { // 16-bit step count, 16-bit starting grbz index
            uint32_t step_count = fd_snowflake_next_operand();
            uint32_t grbz_index = fd_snowflake_next_operand();
            fd_snowflake_illuminate(step_count, grbz_index);
        } break;
        case fd_snowflake_operation_pause: { // 16-bit step count (holds current LED colors)
            uint32_t step_count = fd_snowflake_next_operand();
            fd_snowflake_pause(step_count);
        } break;
        case fd_snowflake_operation_sleep: { // 16-bit seconds (turns off power to LEDs, etc)
            uint32_t seconds = fd_snowflake_next_operand();
            fd_snowflake_sleep(seconds);
        } break;
        case fd_snowflake_operation_restart: // jump back to first operation
            fd_snowflake_restart();
        default: {
        } break;
    }
}

void fd_snowflake_start(const fd_snowflake_program_t *program) {
    fd_snowflake_program = program;
    fd_snowflake_restart();
}

int main(void) {
    fd_hal_gpio_initialize();
    fd_snowflake_leds_powered = false;
    fd_WS2812B_initialize(FD_SNOWFLAKE_PIN_LED_DIN);

    fd_hal_app_initialize();

#if USE_WITH_SOFTDEVICE == 1
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
    fd_hal_ble_initialize(&configuration);
    fd_api_initialize();
    fd_api_initialize_ble();
    fd_hal_ble_start_advertising();
#endif

#if USE_WITH_SOFTDEVICE == 1
    fd_snowflake_start(&fd_breathe_program);
    while (true) {
        fd_api_process();
        fd_hal_app_dispatch_and_wait();
    }
#else
    fd_snowflake_start(&fd_breathe_program);
    while (true) {
        if (fd_snowflake_leds_powered) {
            fd_snowflake_time_slot();
        }
        fd_hal_delay_ms(50);
        app_sched_execute();
        // !!! need to do something about timers...
    }
#endif
}