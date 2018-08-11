#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "ui_controls.h"
#include "graphics.h"
#include "tf.h"


typedef struct ui_osk_t {
    ui_edit_t *edit;
    gbuf_t *g;
    rect_t r;
    tf_t *tf;
    short button_width;
    short button_height;
    short row;
    short col;
    size_t keyboard;
    bool hide;
} ui_osk_t;

ui_osk_t *ui_osk_new(ui_edit_t *edit);
void ui_osk_free(ui_osk_t *osk);
bool ui_osk_showmodal(ui_osk_t *osk);
