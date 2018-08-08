#pragma once

#include <stdbool.h>

#include "graphics.h"


typedef struct {
    gbuf_t *g;
    rect_t r;
    const char *title;
    bool close;
    bool visible;
} ui_dialog_t;

ui_dialog_t *ui_dialog_new(rect_t r, const char *title);
void ui_dialog_destroy(ui_dialog_t *d);
void ui_dialog_showmodal(ui_dialog_t *d);
void ui_dialog_hide(ui_dialog_t *d);
