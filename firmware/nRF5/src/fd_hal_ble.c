#include "fd_hal_ble.h"

#include "fd_nrf.h"

#include <stdio.h>

typedef void (*aw_service_characteristic_value_handler_t)(uint16_t uuid, uint8_t* data, uint16_t length);

typedef struct {
    uint16_t uuid;
    ble_gatts_char_handles_t handles;
    aw_service_characteristic_value_handler_t value_handler;
    bool is_notification_enabled;
} fd_nrf5_ble_service_characteristic_t;

#define AW_SERVICE_CHARACTERISTICS_LENGTH 16

typedef struct {
    uint8_t uuid_type;
    uint16_t service_handle;
    fd_nrf5_ble_service_characteristic_t characteristics[AW_SERVICE_CHARACTERISTICS_LENGTH];
    int characteristics_count;
    uint16_t conn_handle;
} fd_nrf5_ble_service_t;

#define FD_SERVICE_DETOUR_UUID 0x0002
#define FD_SERVICE_DETOUR_NO_RESPONSE_UUID 0x0003

static fd_hal_ble_configuration_t fd_nrf5_ble_configuration;
static uint16_t fd_nrf5_ble_conn_handle = BLE_CONN_HANDLE_INVALID; /**< Handle of the current connection. */
static ble_gap_evt_auth_status_t fd_nrf5_ble_auth_status;
static ble_gap_sec_keyset_t fd_nrf5_ble_sec_keyset;
static ble_gap_conn_params_t fd_nrf5_ble_conn_params;
static fd_nrf5_ble_service_t fd_nrf5_ble_service;
static char fd_nrf5_ble_hardware_revision[16];
static char fd_nrf5_ble_firmware_revision[16];
static uint32_t fd_hal_ble_timeslot_distance_us;
static uint32_t fd_hal_ble_timeslot_length_us;
static fd_hal_ble_timeslot_callback_t fd_hal_ble_timeslot_callback;
static nrf_radio_request_t fd_hal_ble_timeslot_request;
static nrf_radio_signal_callback_return_param_t fd_hal_ble_signal_callback_return_param;

__attribute__((weak)) void fd_hal_ble_characteristic_value_change(uint16_t uuid, uint8_t *data, uint16_t length) {
}

__attribute__ ((weak)) void fd_hal_ble_gap_evt_connected(void) {
}

__attribute__ ((weak)) void fd_hal_ble_gap_evt_disconnected(void) {
}

__attribute__ ((weak)) void fd_hal_ble_gap_evt_tx_complete(uint8_t count) {
}

static void fd_nrf5_ble_gap_set_device_name(void) {
    ble_gap_conn_sec_mode_t sec_mode;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    uint32_t err_code = sd_ble_gap_device_name_set(&sec_mode, (uint8_t *)fd_nrf5_ble_configuration.device_name, strlen(fd_nrf5_ble_configuration.device_name));
    APP_ERROR_CHECK(err_code);
}

// * iOS (10) comes back with an 18.75 ms connection interval
// the theoretical transfer rate for 20 Byte characteristics and 6 packets per connection event is:
//   20 * 6 * (1 / 0.01875) = 6,400 Bytes Per Second
//
// Mac OS X (10.12.1) comes back with 18.75 ms
//
static ble_gap_conn_params_t fd_nrf5_ble_gap_conn_params(void) {
    ble_gap_conn_params_t gap_conn_params;
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));
    gap_conn_params.min_conn_interval = 8; // 10 ms (1.25 ms units)
    gap_conn_params.max_conn_interval = 16; // 20 ms (1.25 ms units)
    gap_conn_params.slave_latency = 0;
    gap_conn_params.conn_sup_timeout = 400; // 4 s (10 ms units)
    return gap_conn_params;
}

static void fd_nrf5_ble_gap_params_initialize(void) {
    fd_nrf5_ble_gap_set_device_name();

    memset(&fd_nrf5_ble_conn_params, 0, sizeof(fd_nrf5_ble_conn_params));

    ble_gap_conn_params_t gap_conn_params = fd_nrf5_ble_gap_conn_params();
    uint32_t err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

#define MAX_DATA_LENGTH (GATT_MTU_SIZE_DEFAULT - 3)

static void fd_nrf5_ble_service_on_connect(fd_nrf5_ble_service_t *service, ble_evt_t *p_ble_evt) {
    service->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
    uint32_t err_code = sd_ble_gatts_sys_attr_set(service->conn_handle, NULL, 0, 0);
    APP_ERROR_CHECK(err_code);
}

static void fd_nrf5_ble_service_on_disconnect(fd_nrf5_ble_service_t *service, ble_evt_t *p_ble_evt) {
    UNUSED_PARAMETER(p_ble_evt);
    service->conn_handle = BLE_CONN_HANDLE_INVALID;
}

static void fd_nrf5_ble_service_on_write(fd_nrf5_ble_service_t *service, ble_evt_t *p_ble_evt) {
    ble_gatts_evt_write_t *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    for (int i = 0; i < service->characteristics_count; ++i) {
        fd_nrf5_ble_service_characteristic_t *characteristic = &service->characteristics[i];
        if ((p_evt_write->handle == characteristic->handles.cccd_handle) && (p_evt_write->len == 2)) {
            characteristic->is_notification_enabled = ble_srv_is_notification_enabled(p_evt_write->data);
            break;
        } else
        if (p_evt_write->handle == characteristic->handles.value_handle) {
            if (characteristic->value_handler) {
                characteristic->value_handler(characteristic->uuid, p_evt_write->data, p_evt_write->len);
            }
            break;
        }
    }
}

void fd_nrf5_ble_service_ble_evt(fd_nrf5_ble_service_t *service, ble_evt_t *p_ble_evt) {
    if ((service == NULL) || (p_ble_evt == NULL)) {
        return;
    }

    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            fd_nrf5_ble_service_on_connect(service, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            fd_nrf5_ble_service_on_disconnect(service, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            fd_nrf5_ble_service_on_write(service, p_ble_evt);
            break;

        default:
            break;
    }
}

fd_nrf5_ble_service_characteristic_t *fd_nrf5_ble_service_get_characteristic(fd_nrf5_ble_service_t *service, uint32_t uuid) {
    for (int i = 0; i < service->characteristics_count; ++i) {
        fd_nrf5_ble_service_characteristic_t *characteristic = &service->characteristics[i];
        if (characteristic->uuid == uuid) {
            return characteristic;
        }
    }
    return 0;
}

#define WRITE 0x01
#define WRITE_WO_RESP 0x02
#define NOTIFY 0x04

static uint32_t fd_nrf5_ble_service_add_characteristic(fd_nrf5_ble_service_t *service, uint16_t uuid, uint32_t flags) {
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;
    
    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    
    memset(&char_md, 0, sizeof(char_md));
    
    char_md.char_props.write  = flags & WRITE ? 1 : 0;
    char_md.char_props.write_wo_resp = flags & WRITE_WO_RESP ? 1 : 0;
    char_md.char_props.notify = flags & NOTIFY ? 1 : 0;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md;
    char_md.p_sccd_md         = NULL;
    
    ble_uuid.type             = service->uuid_type;
    ble_uuid.uuid             = uuid;
    
    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    
    attr_md.vloc              = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth           = 0;
    attr_md.wr_auth           = 0;
    attr_md.vlen              = 1;
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = sizeof(uint8_t);
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = 20;
    
    fd_nrf5_ble_service_characteristic_t *characteristic = &service->characteristics[service->characteristics_count++];
    characteristic->uuid = uuid;
    uint32_t err_code = sd_ble_gatts_characteristic_add(service->service_handle, &char_md, &attr_char_value, &characteristic->handles);
    APP_ERROR_CHECK(err_code);
    return err_code;
}

static void fd_nrf5_ble_service_init(fd_nrf5_ble_service_t *service, aw_service_characteristic_value_handler_t handler) {
    memset(service, 0, sizeof(fd_nrf5_ble_service_t));
    service->conn_handle = BLE_CONN_HANDLE_INVALID;

    ble_uuid128_t base_uuid;
    memcpy(&base_uuid, fd_nrf5_ble_configuration.service_base, 16);
    uint32_t err_code = sd_ble_uuid_vs_add(&base_uuid, &service->uuid_type);
    APP_ERROR_CHECK(err_code);

    ble_uuid_t ble_uuid;
    ble_uuid.type = service->uuid_type;
    ble_uuid.uuid = fd_nrf5_ble_configuration.service_uuid;
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &service->service_handle);
    APP_ERROR_CHECK(err_code);
    
    fd_nrf5_ble_service_add_characteristic(service, FD_SERVICE_DETOUR_UUID, WRITE | NOTIFY);
    fd_nrf5_ble_service_add_characteristic(service, FD_SERVICE_DETOUR_NO_RESPONSE_UUID, WRITE_WO_RESP);

    for (int i = 0; i < service->characteristics_count; ++i) {
        fd_nrf5_ble_service_characteristic_t *characteristic = &service->characteristics[i];
        characteristic->value_handler = handler;
    }
}

static void fd_nrf5_ble_revision_string(char *buffer, fd_hal_ble_revision_t revision) {
    snprintf(buffer, 16, "%u.%u.%u", revision.major, revision.minor, revision.patch);
}

static void fd_nrf5_ble_services_initialize(void) {
    // Initialize Snowflake Service.
    fd_nrf5_ble_service_init(&fd_nrf5_ble_service, fd_hal_ble_characteristic_value_change);
    
    // Initialize Device Information Service.
    ble_dis_init_t dis_init;
    memset(&dis_init, 0, sizeof(dis_init));
    ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, fd_nrf5_ble_configuration.manufacturer);
    ble_srv_ascii_to_utf8(&dis_init.model_num_str, fd_nrf5_ble_configuration.model);
    ble_srv_ascii_to_utf8(&dis_init.serial_num_str, fd_nrf5_ble_configuration.serial_number);
    fd_nrf5_ble_revision_string(fd_nrf5_ble_hardware_revision, fd_nrf5_ble_configuration.hardware_revision);
    ble_srv_ascii_to_utf8(&dis_init.hw_rev_str, fd_nrf5_ble_hardware_revision);
    fd_nrf5_ble_revision_string(fd_nrf5_ble_firmware_revision, fd_nrf5_ble_configuration.firmware_revision);
    ble_srv_ascii_to_utf8(&dis_init.fw_rev_str, fd_nrf5_ble_firmware_revision);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&dis_init.dis_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init.dis_attr_md.write_perm);
    uint32_t err_code = ble_dis_init(&dis_init);
    APP_ERROR_CHECK(err_code);
}

static void fd_nrf5_ble_advertising_initialize(void) {
    ble_advdata_t advdata;
    memset(&advdata, 0, sizeof(advdata));
    advdata.name_type = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = false;
    advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    ble_uuid_t adv_uuids[] = {{fd_nrf5_ble_configuration.service_uuid, fd_nrf5_ble_service.uuid_type}};
#ifndef FD_HAL_BLE_SUPPORT_SCAN_RESPONSE
    advdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    advdata.uuids_complete.p_uuids = adv_uuids;
    uint32_t err_code = ble_advdata_set(&advdata, 0);
#else
    ble_advdata_t scanrsp;
    memset(&scanrsp, 0, sizeof(scanrsp));
    scanrsp.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    scanrsp.uuids_complete.p_uuids = adv_uuids;
    uint32_t err_code = ble_advdata_set(&advdata, &scanrsp);
#endif
    APP_ERROR_CHECK(err_code);
}

static void fd_nrf5_ble_ble_evt(ble_evt_t *p_ble_evt) {
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_EVT_USER_MEM_REQUEST: {
            sd_ble_user_mem_reply(fd_nrf5_ble_conn_handle, NULL);
        } break;

        case BLE_EVT_USER_MEM_RELEASE: {
        } break;

        case BLE_GAP_EVT_CONNECTED: {
            fd_nrf5_ble_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

            ble_gap_conn_params_t gap_conn_params = fd_nrf5_ble_gap_conn_params();
            uint32_t err_code = sd_ble_gap_conn_param_update(fd_nrf5_ble_conn_handle, &gap_conn_params);
            APP_ERROR_CHECK(err_code);

            fd_hal_ble_gap_evt_connected();
        } break;
            
        case BLE_GAP_EVT_DISCONNECTED: {
            fd_nrf5_ble_conn_handle = BLE_CONN_HANDLE_INVALID;

            fd_hal_ble_gap_evt_disconnected();

            fd_hal_ble_start_advertising();
        } break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE: {
            fd_nrf5_ble_conn_params = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params;
        } break;
            
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST: {
            memset(&fd_nrf5_ble_sec_keyset, 0, sizeof(fd_nrf5_ble_sec_keyset));
            ble_gap_sec_params_t m_sec_params;                               /**< Security requirements for this application. */
            m_sec_params.bond         = 1;
            m_sec_params.mitm         = 0;
            m_sec_params.io_caps      = BLE_GAP_IO_CAPS_NONE;
            m_sec_params.oob          = 0;  
            m_sec_params.min_key_size = 7;
            m_sec_params.max_key_size = 16;
            uint32_t err_code = sd_ble_gap_sec_params_reply(fd_nrf5_ble_conn_handle, BLE_GAP_SEC_STATUS_SUCCESS, &m_sec_params, &fd_nrf5_ble_sec_keyset);
            APP_ERROR_CHECK(err_code);
        } break;
            
        case BLE_GATTS_EVT_SYS_ATTR_MISSING: {
            uint32_t err_code = sd_ble_gatts_sys_attr_set(fd_nrf5_ble_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_AUTH_STATUS: {
            fd_nrf5_ble_auth_status = p_ble_evt->evt.gap_evt.params.auth_status;
        } break;

        case BLE_GAP_EVT_SEC_INFO_REQUEST: {
            uint32_t err_code = sd_ble_gap_sec_info_reply(fd_nrf5_ble_conn_handle, NULL, NULL, NULL);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_EVT_TX_COMPLETE: {
            uint8_t count = p_ble_evt->evt.common_evt.params.tx_complete.count;
            fd_hal_ble_gap_evt_tx_complete(count);
        } break;

        default:
            break;
    }
}

void fd_hal_ble_start_advertising(void) {
    ble_gap_adv_params_t adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.type = BLE_GAP_ADV_TYPE_ADV_IND;
    adv_params.p_peer_addr = NULL;
    adv_params.fp = BLE_GAP_ADV_FP_ANY;
    adv_params.interval = 400; // 250 ms (0.625 ms units)
    adv_params.timeout = 0;
    uint32_t err_code = sd_ble_gap_adv_start(&adv_params);
    APP_ERROR_CHECK(err_code);
}

uint32_t fd_hal_ble_set_characteristic_value(uint16_t service_uuid, uint16_t uuid, uint8_t *value, uint16_t length) {
    fd_nrf5_ble_service_t *service = &fd_nrf5_ble_service;
    if (service == NULL) {
        return NRF_ERROR_NULL;
    }
    if (value == NULL) {
        return NRF_ERROR_NULL;
    }
    if (length > MAX_DATA_LENGTH) {
        return NRF_ERROR_INVALID_PARAM;
    }
    if (service->conn_handle == BLE_CONN_HANDLE_INVALID) {
        return NRF_ERROR_INVALID_STATE;
    }
    fd_nrf5_ble_service_characteristic_t *characteristic = fd_nrf5_ble_service_get_characteristic(service, uuid);
    if (characteristic == 0) {
        return NRF_ERROR_INVALID_PARAM;
    }
    if (!characteristic->is_notification_enabled) {
        return NRF_ERROR_INVALID_STATE;
    }
    
    ble_gatts_hvx_params_t hvx_params;
    memset(&hvx_params, 0, sizeof(hvx_params));
    hvx_params.handle = characteristic->handles.value_handle;
    hvx_params.p_data = value;
    hvx_params.p_len  = &length;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    return sd_ble_gatts_hvx(service->conn_handle, &hvx_params);
}

static
uint32_t fd_hal_ble_request_next_event_earliest(void) {
    fd_hal_ble_timeslot_request.request_type                = NRF_RADIO_REQ_TYPE_EARLIEST;
    fd_hal_ble_timeslot_request.params.earliest.hfclk       = NRF_RADIO_HFCLK_CFG_XTAL_GUARANTEED;
    fd_hal_ble_timeslot_request.params.earliest.priority    = NRF_RADIO_PRIORITY_NORMAL;
    fd_hal_ble_timeslot_request.params.earliest.length_us   = fd_hal_ble_timeslot_length_us;
    fd_hal_ble_timeslot_request.params.earliest.timeout_us  = fd_hal_ble_timeslot_distance_us;
    return sd_radio_request(&fd_hal_ble_timeslot_request);
}

static
void fd_hal_ble_configure_next_event_earliest(void) {
    fd_hal_ble_timeslot_request.request_type                = NRF_RADIO_REQ_TYPE_EARLIEST;
    fd_hal_ble_timeslot_request.params.earliest.hfclk       = NRF_RADIO_HFCLK_CFG_XTAL_GUARANTEED;
    fd_hal_ble_timeslot_request.params.earliest.priority    = NRF_RADIO_PRIORITY_NORMAL;
    fd_hal_ble_timeslot_request.params.earliest.length_us   = fd_hal_ble_timeslot_length_us;
    fd_hal_ble_timeslot_request.params.earliest.timeout_us  = fd_hal_ble_timeslot_distance_us;
}

static
void fd_hal_ble_configure_next_event_normal(void) {
    fd_hal_ble_timeslot_request.request_type               = NRF_RADIO_REQ_TYPE_NORMAL;
    fd_hal_ble_timeslot_request.params.normal.hfclk        = NRF_RADIO_HFCLK_CFG_XTAL_GUARANTEED;
    fd_hal_ble_timeslot_request.params.normal.priority     = NRF_RADIO_PRIORITY_HIGH;
    fd_hal_ble_timeslot_request.params.normal.distance_us  = fd_hal_ble_timeslot_distance_us;
    fd_hal_ble_timeslot_request.params.normal.length_us    = fd_hal_ble_timeslot_length_us;
}

static
void fd_hal_ble_evt_signal_handler(uint32_t evt_id) {
    uint32_t err_code;

    switch (evt_id) {
        case NRF_EVT_RADIO_SIGNAL_CALLBACK_INVALID_RETURN:
            //No implementation needed
            break;
        case NRF_EVT_RADIO_SESSION_IDLE:
            //No implementation needed
            break;
        case NRF_EVT_RADIO_SESSION_CLOSED:
            //No implementation needed, session ended
            break;
        case NRF_EVT_RADIO_BLOCKED:
            //Fall through
        case NRF_EVT_RADIO_CANCELED:
            err_code = fd_hal_ble_request_next_event_earliest();
            APP_ERROR_CHECK(err_code);
            break;
        default:
            break;
    }
}

static
nrf_radio_signal_callback_return_param_t *fd_hal_ble_timeslot_handler(uint8_t signal_type) {
    switch (signal_type) {
        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_START:
            // Start of the timeslot - set up timer interrupt
            fd_hal_ble_signal_callback_return_param.params.request.p_next = NULL;
            fd_hal_ble_signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;

            // Setup timer interrupt for 90% into the time slot to do graceful shutdown (see below)
            NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Msk;
            NRF_TIMER0->CC[0] = fd_hal_ble_timeslot_length_us - fd_hal_ble_timeslot_length_us / 10;
            NVIC_EnableIRQ(TIMER0_IRQn);   

            (*fd_hal_ble_timeslot_callback)();
            break;

        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_RADIO:
            fd_hal_ble_signal_callback_return_param.params.request.p_next = NULL;
            fd_hal_ble_signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
            break;

        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_TIMER0:
            // Timer interrupt - do graceful shutdown - schedule next timeslot
            fd_hal_ble_configure_next_event_normal();
            fd_hal_ble_signal_callback_return_param.params.request.p_next = &fd_hal_ble_timeslot_request;
            fd_hal_ble_signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_REQUEST_AND_END;
            break;
        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_SUCCEEDED:
            // No implementation needed
            break;
        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_EXTEND_FAILED:
            // Try scheduling a new timeslot
            fd_hal_ble_configure_next_event_earliest();
            fd_hal_ble_signal_callback_return_param.params.request.p_next = &fd_hal_ble_timeslot_request;
            fd_hal_ble_signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_REQUEST_AND_END;
            break;
        default:
            // No implementation needed
            break;
    }
    return (&fd_hal_ble_signal_callback_return_param);
}

bool fd_hal_ble_timeslot_initialize(uint32_t distance_us, uint32_t length_us, fd_hal_ble_timeslot_callback_t callback) {
    fd_hal_ble_timeslot_distance_us = distance_us;
    fd_hal_ble_timeslot_length_us = length_us;
    fd_hal_ble_timeslot_callback = callback;

    uint32_t err_code = sd_radio_session_open(fd_hal_ble_timeslot_handler);
    APP_ERROR_CHECK(err_code);

    err_code = fd_hal_ble_request_next_event_earliest();
    if (err_code != NRF_SUCCESS) {
        sd_radio_session_close();
        return false;
    }

    return true;
}

void fd_hal_ble_timeslot_close(void) {
    uint32_t err_code = sd_radio_session_close();
    APP_ERROR_CHECK(err_code);
}

static void fd_nrf5_ble_evt_dispatch(ble_evt_t *p_ble_evt) {
    fd_nrf5_ble_service_ble_evt(&fd_nrf5_ble_service, p_ble_evt);
    fd_nrf5_ble_ble_evt(p_ble_evt);
    fd_hal_ble_evt_signal_handler(p_ble_evt->header.evt_id);
}

static void fd_nrf5_ble_ble_stack_initialize(void) {
#ifdef fd_nrf5_ble_BOOTLOADER_ADDR
    sd_mbr_command_t command = {
        .command = SD_MBR_COMMAND_INIT_SD,
    };
    uint32_t result = sd_mbr_command(&command);
    nrf51_app_error_check(result);

    result = sd_softdevice_vector_table_base_set(LB_BOOTLOADER_ADDR);
    nrf51_app_error_check(result);
#endif

    nrf_clock_lf_cfg_t clock_lf_cfg = {
#ifdef FD_HAL_BLE_LOW_FREQUENCY_XTAL
        .source = NRF_CLOCK_LF_SRC_XTAL,
        .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM,
        .rc_ctiv = 0,
        .rc_temp_ctiv = 0,
#else
        .source        = NRF_CLOCK_LF_SRC_RC,
        .rc_ctiv       = 16,
        .rc_temp_ctiv  = 2,
        .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM
#endif
    };
    SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);
    
    uint32_t err_code = softdevice_ble_evt_handler_set(fd_nrf5_ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params_t));
    softdevice_enable_get_default_config(0, 1, &ble_enable_params);
    uint32_t app_ram_base = 0x20001fe8; // !!! check this by passing in 0 and getting min out -denis
    err_code = sd_ble_enable(&ble_enable_params, &app_ram_base);
    APP_ERROR_CHECK(err_code);
    ble_gap_addr_t addr;
    sd_ble_gap_address_get(&addr);
    sd_ble_gap_address_set(BLE_GAP_ADDR_CYCLE_MODE_NONE, &addr);
}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info) {
    NRF_LOG_ERROR("Fatal\r\n");
    NRF_LOG_FINAL_FLUSH();
    // On assert, the system can only recover with a reset.
#ifndef DEBUG
    NVIC_SystemReset();
#else
    app_error_save_and_stop(id, pc, info);
#endif // DEBUG
}

void fd_hal_ble_initialize(fd_hal_ble_configuration_t *configuration) {
    fd_nrf5_ble_configuration = *configuration;

#if USE_WITH_SOFTDEVICE == 1
    fd_nrf5_ble_ble_stack_initialize();
    fd_nrf5_ble_gap_params_initialize();
    fd_nrf5_ble_services_initialize();
    fd_nrf5_ble_advertising_initialize();
#else
    NRF_CLOCK->LFCLKSRC = (CLOCK_LFCLKSRC_SRC_RC << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->TASKS_LFCLKSTART = 1;

    // Wait for the low frequency clock to start
    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0) {}
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
#endif
}
