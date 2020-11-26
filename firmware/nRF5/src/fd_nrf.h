#ifndef FD_NRF_H
#define FD_NRF_H

// unfortunately Nordic includes generate lots of warnings... -denis
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "nrf.h"

#include "app_gpiote.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "app_timer_appsh.h"
#include "app_util_platform.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_dis.h"
#include "nrf_delay.h"
#include "nrf_drv_common.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "softdevice_handler.h"

#endif