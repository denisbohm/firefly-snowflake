#ifndef SDK_CONFIG_H
#define SDK_CONFIG_H

#ifdef USE_APP_CONFIG
#include "app_config.h"
#endif

// add configuration here

#define GPIOTE_ENABLED 1
#define GPIOTE_CONFIG_IRQ_PRIORITY 3
#define GPIOTE_CH_NUM 4
#define GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS 4

#define APP_GPIOTE_ENABLED 1

#define APP_TIMER_ENABLED 1

#define APP_SCHEDULER_ENABLED 1

#define BLE_DIS_ENABLED 1

#endif
