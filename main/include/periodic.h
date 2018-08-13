#pragma once

#include "freertos/FreeRTOS.h"

typedef void* periodic_handle_t;
typedef void (*periodic_callback_t)(periodic_handle_t handle, void *arg);

periodic_handle_t periodic_register(TickType_t interval, periodic_callback_t callback, void *arg);
void periodic_unregister(periodic_handle_t handle);
void periodic_tick(void);
