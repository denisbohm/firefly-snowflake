#ifndef FD_HAL_BLE_H
#define FD_HAL_BLE_H

#include <stdint.h>

typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} fd_hal_ble_revision_t;
    
typedef struct {
    fd_hal_ble_revision_t hardware_revision;
    fd_hal_ble_revision_t firmware_revision;
    char *manufacturer;
    char *model;
    char *serial_number;
    char *device_name;
    uint8_t *service_base;
    uint16_t service_uuid;
} fd_hal_ble_configuration_t;

void fd_hal_ble_initialize(fd_hal_ble_configuration_t *configuration);

void fd_hal_ble_start_advertising(void);

uint32_t fd_hal_ble_set_characteristic_value(uint16_t service_uuid, uint16_t characteristic_uuid, uint8_t *value, uint16_t length);

void fd_hal_ble_gap_evt_connected(void);
void fd_hal_ble_gap_evt_disconnected(void);
void fd_hal_ble_gap_evt_tx_complete(uint8_t count);
void fd_hal_ble_characteristic_value_change(uint16_t uuid, uint8_t *data, uint16_t length);

#endif