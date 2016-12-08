#ifndef FD_HAL_USB_H
#define FD_HAL_USB_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef void (*fd_hal_usb_data_callback_t)(uint8_t *buffer, uint32_t length);
typedef void (*fd_hal_usb_tx_ready_callback_t)(void);

void fd_hal_usb_initialize(void);
void fd_hal_usb_set_tx_ready_callback(fd_hal_usb_tx_ready_callback_t callback);
void fd_hal_usb_set_data_callback(fd_hal_usb_data_callback_t callback);
void fd_hal_usb_power_up(void);
bool fd_hal_usb_can_send(void);
void fd_hal_usb_send(uint8_t *buffer, size_t length);

#endif