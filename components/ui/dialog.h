#pragma once

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "control.h"
#include "graphics.h"


#define MAX_CONTROLS (8)

typedef struct dialog_t dialog_t;
typedef struct control_t control_t;

typedef enum direction_t {
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
} direction_t;

typedef struct dialog_t {
    dialog_t *parent;
    rect_t r;
    gbuf_t *g;
    QueueHandle_t keypad;
    const char *title;
    rect_t cr;
    bool hide;
    bool visible;
    control_t *controls[MAX_CONTROLS];
    int num_controls;
    control_t *active;
} dialog_t;

dialog_t *dialog_new(dialog_t *parent, rect_t r, const char *title);
void dialog_destroy(dialog_t *d);
void dialog_draw(dialog_t *d);
void dialog_showmodal(dialog_t *d);
void dialog_hide(dialog_t *d);
void dialog_hide_all(void);
void dialog_insert_control(dialog_t *d, int index, control_t *control);
void dialog_append_control(dialog_t *d, control_t *control);
control_t *dialog_remove_control(dialog_t *d, int index);
control_t *dialog_find_control(dialog_t *d, direction_t dir);
