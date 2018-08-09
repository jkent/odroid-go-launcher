#pragma once

#include <stdbool.h>

#include "control.h"
#include "graphics.h"


#define MAX_CONTROLS (8)

typedef struct control_t control_t;

typedef struct dialog_t {
    gbuf_t *g;
    rect_t r;
    const char *title;
    rect_t cr;
    bool close;
    bool visible;
    control_t *controls[MAX_CONTROLS];
    int num_controls;
} dialog_t;

dialog_t *dialog_new(rect_t r, const char *title);
void dialog_destroy(dialog_t *d);
void dialog_draw(dialog_t *d);
void dialog_showmodal(dialog_t *d);
void dialog_hide(dialog_t *d);

void dialog_insert_control(dialog_t *d, int index, control_t *control);
void dialog_append_control(dialog_t *d, control_t *control);
control_t *dialog_remove_control(dialog_t *d, int index);
