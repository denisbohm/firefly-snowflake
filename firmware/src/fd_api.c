#include "fd_api.h"

#include "fd_hal_interrupt.h"
#include "fd_hal_usb.h"

#include "fd_binary.h"
#include "fd_log.h"

#include <string.h>

#define FD_API_BLE

#ifdef FD_API_BLE
#define fd_api_send_limit 20
#endif

#ifdef FD_API_USB
#define fd_api_send_limit 64
#endif

typedef struct {
    uint32_t ordinal;
    uint32_t length;
    uint32_t offset;
} fd_api_merge_t;

fd_api_merge_t fd_api_merge;

typedef struct {
    uint64_t identifier;
    uint64_t type;
    fd_api_function_t function;
} fd_api_entry_t;

#define fd_api_entry_size 16
fd_api_entry_t fd_api_entrys[fd_api_entry_size];
uint32_t fd_api_entry_count;

uint32_t fd_api_tx_index;
uint32_t fd_api_tx_length;
#define fd_api_tx_size 350
uint8_t fd_api_tx_buffer[fd_api_tx_size];

uint32_t fd_api_rx_index;
uint32_t fd_api_rx_length;
#define fd_api_rx_size 350
uint8_t fd_api_rx_buffer[fd_api_rx_size];

fd_api_can_transmit_handler_t fd_api_can_transmit_handler;
fd_api_transmit_handler_t fd_api_transmit_handler;
fd_api_dispatch_handler_t fd_api_dispatch_handler;

void fd_api_register(uint64_t identifier, uint64_t type, fd_api_function_t function) {
    fd_log_assert(fd_api_entry_count < fd_api_entry_size);
    fd_api_entry_t *entry = &fd_api_entrys[fd_api_entry_count];
    entry->identifier = identifier;
    entry->type = type;
    entry->function = function;
    fd_api_entry_count += 1;
}

fd_api_function_t fd_api_lookup(uint64_t identifier, uint64_t type) {
    for (uint32_t i = 0; i < fd_api_entry_count; ++i) {
        fd_api_entry_t *entry = &fd_api_entrys[i];
        if ((entry->identifier == identifier) && (entry->type == type)) {
            return entry->function;
        }
    }
    return 0;
}

bool fd_hal_ble_can_send(void) {
    return false;
}

void fd_hal_ble_send(uint8_t *data, uint32_t length) {
}

bool fd_api_can_transmit(void) {
#if defined(FD_API_USB)
    return fd_hal_usb_can_send();
#elif defined(FD_API_BLE)
    return fd_hal_ble_can_send();
#else
    return false;
#endif
}

void fd_api_transmit(uint8_t *data, uint32_t length) {
#if defined(FD_API_USB)
    fd_hal_usb_send(data, length);
#elif defined(FD_API_BLE)
    fd_hal_ble_send(data, length);
#endif
}

int fd_api_transmit_count = 0;

// remove data from tx buffer
bool fd_api_process_tx(void) {
    if ((fd_api_tx_length > 0) && fd_api_can_transmit_handler()) {
        uint8_t length = fd_api_tx_buffer[fd_api_tx_index];
        fd_log_assert((fd_api_tx_index + length) <= fd_api_tx_length);
        fd_api_tx_index += 1;
        ++fd_api_transmit_count;
        fd_api_transmit_handler(&fd_api_tx_buffer[fd_api_tx_index], length);
        fd_api_tx_index += length;
        if (fd_api_tx_index == fd_api_tx_length) {
            fd_api_tx_index = 0;
            fd_api_tx_length = 0;
        }
        return true;
    }
    return false;
}

typedef struct {
    uint64_t identifier;
    uint64_t type;
} fd_api_event_t;

fd_api_event_t fd_api_event_history[256];
uint32_t fd_api_event_history_index = 0;

void fd_api_dispatch(uint64_t identifier, uint64_t type, fd_binary_t *binary) {
    fd_api_event_t *event = &fd_api_event_history[fd_api_event_history_index++ & 0xff];
    event->identifier = identifier;
    event->type = type;
    
    fd_api_function_t function = fd_api_lookup(identifier, type);
    if (function) {
        function(identifier, type, binary);
        fd_log_assert(binary->flags == 0);
    } else {
        fd_log_assert_fail("unknown function");
    }
}

// remove data from rx buffer
bool fd_api_process_rx(void) {
    uint32_t primask = fd_hal_interrupt_disable();
    uint32_t index = fd_api_rx_index;
    uint32_t length = fd_api_rx_length;
    fd_hal_interrupt_restore(primask);

    if (index >= length) {
        return false;
    }

    fd_binary_t binary;
    fd_binary_initialize(&binary, &fd_api_rx_buffer[index], length - index);
    uint32_t function_length = (uint32_t)fd_binary_get_uint16(&binary);
    do {
        uint64_t identifier = fd_binary_get_varuint(&binary);
        uint64_t type = fd_binary_get_varuint(&binary);
        uint32_t content_length = (uint32_t)fd_binary_get_varuint(&binary);
        uint32_t content_index = binary.get_index;
        if (binary.flags == 0) {
            fd_api_dispatch_handler(identifier, type, &binary);
        } else {
            fd_log_assert_fail("underflow");
        }
        fd_log_assert(binary.get_index <= (content_index + content_length));
        binary.get_index = content_index + content_length;
    } while (binary.get_index < binary.size);

    primask = fd_hal_interrupt_disable();
    fd_api_rx_index += 2 + function_length;
    // if buffer is empty and there isn't a transfer occurring reset the buffer
    if ((fd_api_rx_index >= fd_api_rx_length) && (fd_api_merge.length == 0)) {
        fd_api_rx_index = 0;
        fd_api_rx_length = 0;
    }
    fd_hal_interrupt_restore(primask);
    return true;
}

void fd_api_process(void) {
    while (fd_api_process_tx());
    while (fd_api_process_rx());
}

// append data to tx buffer
bool fd_api_send(uint64_t identifier, uint64_t type, uint8_t *data, uint32_t length) {
    uint8_t buffer[32];
    fd_binary_t binary;
    fd_binary_initialize(&binary, buffer, sizeof(buffer));
    fd_binary_put_varuint(&binary, identifier);
    fd_binary_put_varuint(&binary, type);
    fd_binary_put_varuint(&binary, length);
    uint32_t header_length = binary.put_index;
    uint32_t content_length = header_length + length;

    uint32_t buffer_index = fd_api_tx_length;
    uint32_t data_index = 0;
    uint32_t data_remaining = length;
    uint64_t ordinal = 0;
    while ((ordinal == 0) || (data_remaining > 0)) {
        uint32_t free = fd_api_tx_size - buffer_index;
        fd_binary_initialize(&binary, &fd_api_tx_buffer[buffer_index], free);
        fd_binary_put_uint8(&binary, 0 /* length placeholder */);
        fd_binary_put_varuint(&binary, ordinal);
        if (ordinal == 0) {
            fd_binary_put_varuint(&binary, content_length);
            fd_binary_put_varuint(&binary, identifier);
            fd_binary_put_varuint(&binary, type);
            fd_binary_put_varuint(&binary, length);
        }
        uint32_t available = 1 + fd_api_send_limit - binary.put_index;
        uint32_t amount = data_remaining;
        if (amount > available) {
            amount = available;
        }
        fd_binary_put_bytes(&binary, &data[data_index], amount);
        if (binary.flags & FD_BINARY_FLAG_OVERFLOW) {
            return false;
        }
        data_index += amount;
        data_remaining -= amount;
        fd_api_tx_buffer[buffer_index] = binary.put_index - 1;
        buffer_index += binary.put_index;
        ordinal += 1;
    }

    fd_api_tx_length = buffer_index;
    return true;
}

void fd_api_merge_clear(void) {
    memset(&fd_api_merge, 0, sizeof(fd_api_merge));
}

// called from interrupt context
void fd_api_tx_callback(void) {
    // wakeup and send next chunk
    // !!! need to implement to add wait to main loop -denis
}

// called from interrupt context
// append data to rx buffer
void fd_api_rx_callback(uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint64_t ordinal = fd_binary_get_varuint(&binary);
    if (ordinal == 0) {
        // start of sequence
        fd_log_assert(fd_api_merge.ordinal == 0);
        fd_api_merge_clear();
        fd_api_merge.length = (uint32_t)fd_binary_get_varuint(&binary);
        fd_log_assert(fd_api_merge.length < fd_api_rx_size);
        fd_binary_t binary_rx;
        fd_binary_initialize(&binary_rx, &fd_api_rx_buffer[fd_api_rx_length], fd_api_rx_size - fd_api_rx_length);
        fd_binary_put_uint16(&binary_rx, fd_api_merge.length);
    } else {
        if (ordinal != (fd_api_merge.ordinal + 1)) {
            fd_log_assert_fail("out of sequence");
            fd_api_merge_clear();
            return;
        }
        fd_api_merge.ordinal += 1;
    }

    uint8_t *content_data = &data[binary.get_index];
    uint32_t content_length = length - binary.get_index;

    uint32_t rx_free = fd_api_rx_size - (fd_api_rx_length + 2 + fd_api_merge.offset + content_length);
    if (content_length > rx_free) {
        fd_log_assert_fail("overflow");
        fd_api_merge_clear();
        return;
    }

    memcpy(&fd_api_rx_buffer[fd_api_rx_length + 2 + fd_api_merge.offset], content_data, content_length);
    fd_api_merge.offset += content_length;

    if (fd_api_merge.offset >= fd_api_merge.length) {
        // merge complete
        fd_api_rx_length += 2 + fd_api_merge.length;
        fd_api_merge_clear();
    }
}

void fd_api_initialize(void) {
    fd_api_entry_count = 0;

    fd_api_tx_index = 0;
    fd_api_tx_length = 0;

    fd_api_rx_index = 0;
    fd_api_rx_length = 0;

    fd_api_can_transmit_handler = fd_api_can_transmit;
    fd_api_transmit_handler = fd_api_transmit;
    fd_api_dispatch_handler = fd_api_dispatch;
}

void fd_api_initialize_ble(void) {
#if 0
    fd_hal_ble_set_data_callback(fd_api_rx_callback);
    fd_hal_ble_set_tx_ready_callback(fd_api_tx_callback);
#endif
}

void fd_api_initialize_usb(void) {
    fd_hal_usb_set_data_callback(fd_api_rx_callback);
    fd_hal_usb_set_tx_ready_callback(fd_api_tx_callback);
}