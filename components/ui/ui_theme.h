#pragma once

#include <stdint.h>

#include "tf.h"


typedef struct ui_theme_t {
    uint16_t bg_color;
    uint16_t active_highlight_color;
    uint16_t inactive_highlight_color;
    uint16_t selection_color;
    uint16_t border3d_light_color;
    uint16_t border3d_dark_color;
    uint16_t window_color;
    uint16_t text_color;
    uint16_t button_color;
    uint16_t control_color;
    short padding;
    const tf_font_t *font;
} ui_theme_t;

ui_theme_t *ui_theme;
