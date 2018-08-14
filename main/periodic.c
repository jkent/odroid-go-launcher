#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "periodic.h"


typedef struct periodic_data_t {
    TickType_t interval;
    periodic_callback_t callback;
    void *arg;
    TickType_t last_ticks;
} periodic_data_t;

static size_t periodic_count = 0;
static periodic_data_t **periodic_data = NULL;


periodic_handle_t periodic_register(TickType_t interval, periodic_callback_t callback, void *arg)
{
    periodic_data = realloc(periodic_data, sizeof(periodic_data_t *) * (periodic_count + 1));
    assert(periodic_data != NULL);
    periodic_data_t *data = malloc(sizeof(periodic_data_t));
    assert(data != NULL);

    data->interval = interval;
    data->callback = callback;
    data->arg = arg;
    data->last_ticks = xTaskGetTickCount();

    periodic_data[periodic_count] = data;
    periodic_count += 1;

    return (periodic_handle_t)data;
}

void periodic_unregister(periodic_handle_t handle)
{
    int i;
    for (i = 0; i < periodic_count; i++) {
        if (handle == (periodic_handle_t)periodic_data[i]) {
            break;
        }
    }
    if (i >= periodic_count) {
        return;
    }

    free(periodic_data[i]);

    if (periodic_count == 1) {
        free(periodic_data);
        periodic_data = NULL;
        periodic_count = 0;
        return;
    }

    if (i < periodic_count - 1) {
        memmove(&periodic_data[i], &periodic_data[i + 1], sizeof(periodic_data_t *) * (periodic_count - i - 1));
    }
    periodic_data = realloc(periodic_data, sizeof(periodic_data_t *) * (periodic_count - 1));
    assert(periodic_data != NULL);
    periodic_count -= 1;
}

void periodic_tick(void)
{
    TickType_t ticks = xTaskGetTickCount();

    for (int i = 0; i < periodic_count; i++) {
        periodic_data_t *data = periodic_data[i];

        TickType_t elapsed = ticks - data->last_ticks;
        if (elapsed >= data->interval) {
            data->last_ticks += elapsed;
            data->callback((periodic_handle_t)data, data->arg);
        }
    }
}
