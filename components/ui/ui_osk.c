#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "display.h"
#include "keypad.h"
#include "OpenSans_Regular_11X12.h"
#include "periodic.h"
#include "ui_dialog.h"
#include "ui_osk.h"
#include "ui_theme.h"


const char keyboards[3][4][11] = {
    {
        {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '@'},
        {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '+'},
        {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '_', ':'},
        {'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '-', '/'},
    },
    {
        {'#', '[', ']', '$', '%', '^', '&', '*', '(', ')', '_'},
        {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@'},
        {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '"'},
        {'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '+', '='},
    },
    {
        {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-'},
        {'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_'},
        {'~', '`', '=', '\\', '+', '{', '}', '|', '[', ']', '\0'},
        {'<', '>', ';', ':', '"', '\'', ',', '.', '?', '/', '\0'},
    },

};

ui_osk_t *ui_osk_new(ui_edit_t *edit)
{
    ui_osk_t *osk = calloc(1, sizeof(ui_osk_t));
    assert(osk != NULL);

    osk->tf = tf_new(&font_OpenSans_Regular_11X12, ui_theme->text_color, fb->width - 4, 0);
    osk->button_width = fb->width / 12;
    osk->button_height = osk->tf->font->height + 5;
    osk->r.x = 0;
    osk->r.y = fb->height - (osk->button_height * 6);
    osk->r.width = fb->width;
    osk->r.height = osk->button_height * 6;

    osk->g = gbuf_new(osk->r.width, osk->r.height, 2, BIG_ENDIAN);

    /* home position */
    osk->row = 1;
    osk->col = 11;

    osk->edit = edit;

    return osk;
}

void ui_osk_free(ui_osk_t *osk)
{
    gbuf_free(osk->g);
    tf_free(osk->tf);
    free(osk);
}

static void osk_draw(ui_osk_t *osk)
{
    fill_rectangle(fb, osk->r, ui_theme->window_color);

    short cx = osk->r.width / 2 - osk->button_width * 12 / 2;
    short cy = osk->r.height / 2 - osk->button_height * 6 / 2;

    point_t start = {
        .x = osk->r.x,
        .y = osk->r.y + cy,
    };
    point_t end = {
        .x = osk->r.x + osk->r.width - 1,
        .y = osk->r.y + cy,
    };
    draw_line(fb, start, end, DRAW_STYLE_SOLID, ui_theme->border3d_light_color);

    if (osk->edit->text) {
        point_t p = {
            .x = osk->r.x + 2,
            .y = osk->r.y + cy + (osk->button_height - 1) / 2 - osk->tf->font->height / 2 + 1,
        };

        if (osk->edit->password) {
            size_t len = strlen(osk->edit->text);
            char s[len + 1];
            memset(s, '*', len);
            s[len] = '\0';
            tf_draw_str(fb, osk->tf, s, p);
        } else {
            tf_draw_str(fb, osk->tf, osk->edit->text, p);
        }
    }

    for (short row = 0; row < 5; row++) {
        for (short col = 0; col < 12; col++) {
            bool draw = false;
            short rows = 1;
            short cols = 1;

            char s[6];
            if (col < 11 && row < 4) {
                s[0] = keyboards[osk->keyboard][row][col];
                s[1] = '\0';
                draw = true;
            } else if (row == 0) {
                strcpy(s, "<--");
                draw = true;
            } else if (row == 1) {
                rows = 4;
                strcpy(s, "OK");
                draw = true;
            } else if (row == 4 && col == 0) {
                cols = 2;
                strcpy(s, "Shift");
                draw = true;
            } else if (row == 4 && col == 2) {
                strcpy(s, "Sym");
                draw = true;
            } else if (row == 4 && col == 3) {
                cols = 8;
                s[0] = '\0';
                draw = true;
            }
            if (draw) {
                rect_t r = {
                    .x = osk->r.x + cx + osk->button_width * col,
                    .y = osk->r.y + cy + osk->button_height * (row + 1) + 1,
                    .width = osk->button_width * cols - 1,
                    .height = osk->button_height * rows - 1,
                };

                fill_rectangle(fb, r, ui_theme->button_color);
                if (row == osk->row && col == osk->col) {
                    draw_rectangle(fb, r, DRAW_STYLE_SOLID, ui_theme->selection_color);
                }
                tf_metrics_t m = tf_get_str_metrics(osk->tf, s);
                point_t bp = {
                    .x = r.x + r.width / 2 - m.width / 2,
                    .y = r.y + r.height / 2 - m.height / 2 + 1,
                };
                tf_draw_str(fb, osk->tf, s, bp);
            }
        }
    }
}

static bool osk_up(ui_osk_t *osk)
{
    bool draw = false;
    if (osk->row > 0) {
        osk->row -= 1;
        draw = true;
    }
    return draw;
}

static bool osk_right(ui_osk_t *osk)
{
    bool draw = false;
    if (osk->col < 11) {
        if (osk->row == 4) {
            if (osk->col == 0) {
                osk->col = 2;
                draw = true;
            } else if (osk->col == 2) {
                osk->col = 3;
                draw = true;
            } else if (osk->col == 3) {
                osk->row = 1;
                osk->col = 11;
                draw = true;
            }
        } else {
            osk->col += 1;
            draw = true;
        }
        if (osk->col == 11 && osk->row > 1 && osk->row <= 3) {
            osk->row = 1;
            draw = true;
        }
    }
    return draw;
}

static bool osk_down(ui_osk_t *osk)
{
    bool draw = false;
    if (osk->row < 4) {
        if (osk->col < 11 || osk->row < 1) {
            osk->row += 1;
            draw = true;
        }
        if (osk->row == 4) {
            if (osk->col == 1) {
                osk->col = 0;
                draw = true;
            } else if (osk->col > 3 && osk->col <= 10) {
                osk->col = 3;
                draw = true;
            }
        }
    }
    return draw;
}

static bool osk_left(ui_osk_t *osk)
{
    bool draw = false;
    if (osk->col > 0) {
        if (osk->row == 4 && osk->col == 2) {
            osk->col = 0;
        } else {
            osk->col -= 1;
        }
        draw = true;
    }
    return draw;
}

static bool osk_a(ui_osk_t *osk)
{
    if (osk->col < 11 && osk->row < 4) {
        size_t len = strlen(osk->edit->text);
        if (len < osk->edit->text_len - 1) {
            osk->edit->text[len] = keyboards[osk->keyboard][osk->row][osk->col];
            osk->edit->text[len + 1] = '\0';
            osk->keyboard = 0;
            return true;
        }
    } else if (osk->col == 11) {
        if (osk->row == 0) { /* Backspace */
            size_t len = strlen(osk->edit->text);
            if (len > 0) {
                osk->edit->text[len - 1] = '\0';
                return true;
            }
        } else if (osk->row == 1) { /* OK */
            osk->hide = true;
            return false;
        }
    } else if (osk->row == 4) {
        if (osk->col == 0) { /* Shift */
            if (osk->keyboard == 1) {
                osk->keyboard = 0;
            } else {
                osk->keyboard = 1;
            }
            return true;
        } else if (osk->col == 2) { /* Symbol */
            if (osk->keyboard == 2) {
                osk->keyboard = 0;
            } else {
                osk->keyboard = 2;
            }
            return true;
        } else if (osk->col == 3) { /* Spacebar */
            size_t len = strlen(osk->edit->text);
            if (len < osk->edit->text_len - 1) {
                osk->edit->text[len] = ' ';
                osk->edit->text[len + 1] = '\0';
                return true;
            }
        }
    }
    return false;
}

bool ui_osk_showmodal(ui_osk_t *osk)
{
    osk->g = gbuf_new(osk->r.width, osk->r.height, 2, BIG_ENDIAN);

    rect_t r = {
        .x = 0,
        .y = 0,
        .width = osk->r.width,
        .height = osk->r.height,
    };

    blit(osk->g, r, fb, osk->r);
    osk_draw(osk);
    display_update_rect(osk->r);

    bool result = false;
    osk->hide = false;
    keypad_info_t keys;
    while (true) {
        if (keypad_queue_receive(osk->edit->d->keypad, &keys, 250 / portTICK_RATE_MS)) {
            bool dirty = false;
            if (keys.pressed & KEYPAD_UP) {
                dirty |= osk_up(osk);
            }

            if (keys.pressed & KEYPAD_RIGHT) {
                dirty |= osk_right(osk);
            }

            if (keys.pressed & KEYPAD_DOWN) {
                dirty |= osk_down(osk);
            }

            if (keys.pressed & KEYPAD_LEFT) {
                dirty |= osk_left(osk);
            }

            if (keys.pressed & KEYPAD_A) {
                dirty |= osk_a(osk);
                if (osk->hide) {
                    result = true;
                    break;
                }
            }

            if (keys.pressed & KEYPAD_B) {
                break;
            }

            if (keys.pressed & KEYPAD_MENU) {
                ui_dialog_unwind();
                break;
            }

            if (dirty) {
                osk_draw(osk);
                display_update_rect(osk->r);
            }
        }
        periodic_tick();
    }

    blit(fb, osk->r, osk->g, r);
    display_update_rect(osk->r);

    osk->edit->dirty = true;

    return result;
}
