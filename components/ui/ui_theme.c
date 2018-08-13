#include "ui_theme.h"
#include "OpenSans_Regular_11X12.h"


static ui_theme_t theme_default = {
    .bg_color = 0x0000,
    .active_highlight_color = 0x001f,
    .inactive_highlight_color = 0x632c,
    .selection_color = 0xffff,
    .border3d_light_color = 0x9cd3,
    .border3d_dark_color = 0x52aa,
    .window_color = 0x0000,
    .text_color = 0xffff,
    .control_color = 0x0000,
    .button_color = 0x632c,
    .padding = 2,
    .font = &font_OpenSans_Regular_11X12,
};

/*static ui_theme_t theme_alternate = {
    .bg_color = 0xFFFF,
    .active_highlight_color = 0x001f,
    .inactive_highlight_color = 0x632c,
    .selection_color = 0x0000,
    .border3d_light_color = 0x9cd3,
    .border3d_dark_color = 0x52aa,
    .window_color = 0x632c,
    .text_color = 0x0000,
    .control_color = 0xffff,
    .button_color = 0x632c,
    .padding = 2,
    .font = &font_OpenSans_Regular_11X12,
};*/

ui_theme_t *ui_theme = &theme_default;
