#pragma once

#include <stddef.h>

#include "control.h"
#include "graphics.h"
#include "tf.h"


typedef struct osk_t {
    control_edit_t *edit;
    gbuf_t *g;
    rect_t r;
    tf_t *tf;
    short button_width;
    short button_height;
    short row;
    short col;
    size_t keyboard;
    bool hide;
} osk_t;

osk_t *osk_new(control_edit_t *edit);
void osk_free(osk_t *osk);
void osk_showmodal(osk_t *osk);
