#ifndef FD_SNOWFLAKE_H
#define FD_SNOWFLAKE_H

#define FD_SNOWFLAKE_PIN_LED_POWER 8
#define FD_SNOWFLAKE_PIN_LED_DIN 11

#define FD_SNOWFLAKE_PIN_BATTERY_SENSE 24
#define FD_SNOWFLAKE_PIN_BATTERY_DIV_2 1
#define FD_SNOWFLAKE_PIN_USB_POWERED 27
#define FD_SNOWFLAKE_PIN_CHARGE_STATUS 26
#define FD_SNOWFLAKE_PIN_CHARGE_CURRENT 2

#include <stdint.h>

typedef enum {
    fd_snowflake_operation_illuminate, // 16-bit step count, 16-bit starting grbz index
    fd_snowflake_operation_pause, // 16-bit step count (holds current LED colors)
    fd_snowflake_operation_sleep, // 16-bit seconds (turns off power to LEDs, etc)
    fd_snowflake_operation_restart, // jump back to first operation
} fd_snowflake_operation_t;

#define fd_snowflake_operand(n) (n & 0xff), ((n >> 8) & 0xff)

typedef struct {
    const uint8_t *instructions;
    const uint32_t *grbzs;
} fd_snowflake_program_t;

#endif