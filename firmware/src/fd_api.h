#ifndef FD_API_H
#define FD_API_H

#include "fd_binary.h"

#include <stdbool.h>
#include <stdint.h>

void fd_api_initialize(void);
void fd_api_initialize_ble(void);
void fd_api_initialize_usb(void);

typedef void (*fd_api_function_t)(uint64_t identifier, uint64_t type, fd_binary_t *binary);
void fd_api_register(uint64_t identifier, uint64_t type, fd_api_function_t function);

void fd_api_process(void);

bool fd_api_send(uint64_t identifier, uint64_t type, uint8_t *data, uint32_t length);

typedef bool (*fd_api_can_transmit_handler_t)(void);
typedef void (*fd_api_transmit_handler_t)(uint8_t *data, uint32_t length);
typedef void (*fd_api_dispatch_handler_t)(uint64_t identifier, uint64_t type, fd_binary_t *binary);

#endif
