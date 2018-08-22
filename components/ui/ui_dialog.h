#pragma once

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "ui_controls.h"
#include "graphics.h"
#include "tf.h"


typedef struct ui_dialog_t ui_dialog_t;
typedef struct ui_control_t ui_control_t;

typedef enum direction_t {
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
} direction_t;

typedef struct ui_dialog_t {
    ui_dialog_t *parent;
    rect_t r;
    gbuf_t *g;
    tf_t *tf;
    QueueHandle_t keypad;
    const char *title;
    rect_t cr;
    bool hide;
    bool visible;
    ui_control_t **controls;
    size_t controls_size;
    ui_control_t *active;
} ui_dialog_t;

ui_dialog_t *ui_dialog_new(ui_dialog_t *parent, rect_t r, const char *title);
void ui_dialog_layout(ui_dialog_t *d);
void ui_dialog_destroy(ui_dialog_t *d);
void ui_dialog_draw(ui_dialog_t *d);
void ui_dialog_showmodal(ui_dialog_t *d);
void ui_dialog_hide(ui_dialog_t *d);
void ui_dialog_unwind(void);
void ui_dialog_add_control(ui_dialog_t *d, ui_control_t *control);
ui_control_t *ui_dialog_find_control(ui_dialog_t *d, direction_t dir);
ui_dialog_t *ui_dialog_get_top(void);
