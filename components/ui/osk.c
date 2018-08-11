#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "dialog.h"
#include "display.h"
#include "keypad.h"
#include "OpenSans_Regular_11X12.h"
#include "osk.h"
#include "statusbar.h"

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

osk_t *osk_new(control_edit_t *edit)
{
    osk_t *osk = calloc(1, sizeof(osk_t));
    assert(osk != NULL);

    osk->tf = tf_new(&font_OpenSans_Regular_11X12, fb->width - 4, 0);
    osk->button_width = fb->width / 12;
    osk->button_height = osk->tf->font->height + 5;
    osk->r.x = 0;
    osk->r.y = fb->height - (osk->button_height * 6);
    osk->r.width = fb->width;
    osk->r.height = osk->button_height * 6;

    osk->row = 1;
    osk->col = 11;

    osk->edit = edit;

    return osk;
}

void osk_free(osk_t *osk)
{
    tf_free(osk->tf);
    free(osk);
}

static void osk_draw(osk_t *osk)
{
    fill_rectangle(fb, osk->r, 0x0000);

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
    draw_line(fb, start, end, DRAW_STYLE_SOLID, 0xFFFF);

    point_t p = {
        .x = osk->r.x + 2,
        .y = osk->r.y + cy + (osk->button_height - 1) / 2 - osk->tf->font->height / 2 + 1,
    };
    tf_draw_str(fb, osk->tf, osk->edit->text, p);

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
                    .y = osk->r.y + cy + osk->button_height * (row + 1),
                    .width = osk->button_width * cols - 1,
                    .height = osk->button_height * rows - 1,
                };

                fill_rectangle(fb, r, 0x3186);
                if (row == osk->row && col == osk->col) {
                    draw_rectangle(fb, r, DRAW_STYLE_SOLID, 0xFFFF);
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

void osk_showmodal(osk_t *osk)
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

    bool done = false;

    uint16_t keys = 0, changes = 0, pressed;
    do {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        keys = keypad_debounce(keypad_sample(), &changes);
        pressed = keys & changes;
        statusbar_update();

        bool dirty = false;
        if (pressed & KEYPAD_UP && osk->row > 0) {
            osk->row -= 1;
            dirty = true;
        }
        if (pressed & KEYPAD_RIGHT && osk->col < 11) {
            if (osk->row == 4) {
                if (osk->col == 0) {
                    osk->col = 2;
                } else if (osk->col == 2) {
                    osk->col = 3;
                } else if (osk->col == 3) {
                    osk->row = 1;
                    osk->col = 11;
                }
            } else {
                osk->col += 1;
            }
            if (osk->col == 11 && osk->row > 1 && osk->row <= 3) {
                osk->row = 1;
            }
            dirty = true;
        }
        if (pressed & KEYPAD_DOWN && osk->row < 4) {
            if (osk->col < 11 || osk->row < 1) {
                osk->row += 1;
            }
            if (osk->row == 4) {
                if (osk->col == 1) {
                    osk->col = 0;
                } else if (osk->col > 3 && osk->col <= 10) {
                    osk->col = 3;
                }
            }
            dirty = true;
        }
        if (pressed & KEYPAD_LEFT && osk->col > 0) {
            if (osk->row == 4 && osk->col == 2) {
                osk->col = 0;
            } else {
                osk->col -= 1;
            }
            dirty = true;
        }
        if (pressed & KEYPAD_A) {
            if (osk->col < 11 && osk->row < 4) {
                size_t len = strlen(osk->edit->text);
                if (len < osk->edit->text_len - 1) {
                    osk->edit->text[len] = keyboards[osk->keyboard][osk->row][osk->col];
                    osk->edit->text[len + 1] = '\0';
                    osk->keyboard = 0;
                }
            } else if (osk->col == 11) {
                if (osk->row == 0) { /* Backspace */
                    size_t len = strlen(osk->edit->text);
                    if (len > 0) {
                        osk->edit->text[len - 1] = '\0';
                    }
                } else if (osk->row == 1) { /* OK */
                    done = true;
                }
            } else if (osk->row == 4) {
                if (osk->col == 0) { /* Shift */
                    if (osk->keyboard == 1) {
                        osk->keyboard = 0;
                    } else {
                        osk->keyboard = 1;
                    }
                } else if (osk->col == 2) { /* Symbol */
                    if (osk->keyboard == 2) {
                        osk->keyboard = 0;
                    } else {
                        osk->keyboard = 2;
                    }
                } else if (osk->col == 3) { /* Spacebar */
                    size_t len = strlen(osk->edit->text);
                    if (len < osk->edit->text_len - 1) {
                        osk->edit->text[len] = ' ';
                        osk->edit->text[len + 1] = '\0';
                    }
                }
            }
            dirty = true;
        }
        if (pressed & KEYPAD_B) {
            size_t len = strlen(osk->edit->text);
            if (len > 0) {
                osk->edit->text[len - 1] = '\0';
                dirty = true;
            }
        }

        if (dirty) {
            osk_draw(osk);
            display_update_rect(osk->r);
        }
    } while (!done);

    blit(fb, osk->r, osk->g, r);
    display_update_rect(osk->r);

    osk->edit->dirty = true;
}
