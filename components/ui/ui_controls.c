#include <string.h>

#include "display.h"
#include "keypad.h"
#include "OpenSans_Regular_11X12.h"
#include "periodic.h"
#include "tf.h"
#include "ui_controls.h"
#include "ui_dialog.h"
#include "ui_osk.h"
#include "ui_theme.h"

#define BORDER (1)


/* ui_button */

static void button_draw(ui_control_t *control)
{
    ui_button_t *button = (ui_button_t *)control;

    button->tf->clip = button->r;
    button->tf->clip.x += button->d->cr.x + BORDER;
    button->tf->clip.y += button->d->cr.y + BORDER;
    button->tf->clip.width -= 2*BORDER;
    button->tf->clip.height -= 2*BORDER;

    rect_t rb = button->r;
    rb.x += button->d->cr.x;
    rb.y += button->d->cr.y;

    fill_rectangle(fb, button->tf->clip, ui_theme->button_color);
    draw_rectangle3d(fb, rb, ui_theme->border3d_light_color, ui_theme->border3d_dark_color);
    if (control == control->d->active) {
        draw_rectangle(fb, button->tf->clip, DRAW_STYLE_DOTTED, ui_theme->selection_color);
    }

    if (button->text) {
        tf_metrics_t m = tf_get_str_metrics(button->tf, button->text);
        point_t p = {
            .x = button->d->cr.x + button->r.x + ui_theme->padding,
            .y = button->d->cr.y + button->r.y + button->r.height/2 - m.height/2 + 1,
        };
        tf_draw_str(fb, button->tf, button->text, p);
    }

    button->dirty = true;
}

static void button_free(ui_control_t *control)
{
    ui_button_t *button = (ui_button_t *)control;

    if (button->text) {
        free(button->text);
    }
    tf_free(button->tf);
    free(button);
}

ui_button_t *ui_dialog_add_button(ui_dialog_t *d, rect_t r, const char *text, ui_control_onselect_t onselect, void *arg)
{
    ui_button_t *button = calloc(1, sizeof(ui_button_t));

    button->type = CONTROL_BUTTON;
    button->d = d;
    button->r = r;
    button->draw = button_draw;
    button->onselect = onselect;
    button->arg = arg;
    button->free = button_free;
    button->text = strdup(text);
    button->tf = tf_new(&font_OpenSans_Regular_11X12, ui_theme->text_color, button->r.width - 2*ui_theme->padding, TF_ALIGN_CENTER | TF_ELIDE);

    ui_dialog_add_control(d, (ui_control_t *)button);

    return button;
}


/* ui_edit */

static void edit_draw(ui_control_t *control)
{
    ui_edit_t *edit = (ui_edit_t *)control;

    edit->tf->clip = edit->r;
    edit->tf->clip.x += edit->d->cr.x + BORDER;
    edit->tf->clip.y += edit->d->cr.y + BORDER;
    edit->tf->clip.width -= 2*BORDER;
    edit->tf->clip.height -= 2*BORDER;

    rect_t rb = edit->r;
    rb.x += edit->d->cr.x;
    rb.y += edit->d->cr.y;

    fill_rectangle(fb, edit->tf->clip, ui_theme->control_color);
    draw_rectangle3d(fb, rb, ui_theme->border3d_dark_color, ui_theme->border3d_light_color);
    if (control == control->d->active) {
        draw_rectangle(fb, edit->tf->clip, DRAW_STYLE_DOTTED, ui_theme->selection_color);
    }

    if (edit->text) {
        point_t p = {
            .x = edit->d->cr.x + edit->r.x + ui_theme->padding,
            .y = edit->d->cr.y + edit->r.y + edit->r.height/2 - edit->tf->font->height/2 + 1,
        };
        if (edit->password) {
            size_t len = strlen(edit->text);
            char s[len + 1];
            memset(s, '*', len);
            s[len] = '\0';
            tf_draw_str(fb, edit->tf, s, p);
        } else {
            tf_draw_str(fb, edit->tf, edit->text, p);
        }
    }

    edit->dirty = true;
}

static void edit_onselect(ui_control_t *control, void *arg)
{
    ui_edit_t *edit = (ui_edit_t *)control;

    ui_osk_t *osk = ui_osk_new(edit);
    ui_osk_showmodal(osk);
    ui_osk_free(osk);

    control->draw(control);
    display_update_rect(control->d->cr);
}

static void edit_free(ui_control_t *control)
{
    ui_edit_t *edit = (ui_edit_t *)control;

    tf_free(edit->tf);
    free(edit);
}

ui_edit_t *ui_dialog_add_edit(ui_dialog_t *d, rect_t r, char *text, size_t text_len)
{
    ui_edit_t *edit = calloc(1, sizeof(ui_edit_t));

    edit->type = CONTROL_EDIT;
    edit->d = d;
    edit->r = r;
    edit->draw = edit_draw;
    edit->onselect = edit_onselect;
    edit->free = edit_free;
    edit->text = text;
    edit->text_len = text_len;
    edit->tf = tf_new(&font_OpenSans_Regular_11X12, ui_theme->text_color, edit->r.width - 2*ui_theme->padding, TF_ELIDE);

    ui_dialog_add_control(d, (ui_control_t *)edit);

    return edit;
}


/* ui_label */

static void label_draw(ui_control_t *control)
{
    ui_label_t *label = (ui_label_t *)control;

    label->tf->clip = label->r;
    label->tf->clip.x += label->d->cr.x;
    label->tf->clip.y += label->d->cr.y;

    fill_rectangle(fb, label->tf->clip, ui_theme->window_color);
    if (label->text) {
        tf_metrics_t m = tf_get_str_metrics(label->tf, label->text);
        point_t p = {
            .x = label->d->cr.x + label->r.x + ui_theme->padding,
            .y = label->d->cr.y + label->r.y + label->r.height/2 - m.height/2,
        };
        tf_draw_str(fb, label->tf, label->text, p);
    }

    label->dirty = true;
}

static void label_free(ui_control_t *control)
{
    ui_label_t *label = (ui_label_t *)control;

    if (label->text) {
        free(label->text);
    }
    tf_free(label->tf);
    free(label);
}

ui_label_t *ui_dialog_add_label(ui_dialog_t *d, rect_t r, const char *text)
{
    ui_label_t *label = calloc(1, sizeof(ui_label_t));

    label->type = CONTROL_LABEL;
    label->d = d;
    label->r = r;
    label->draw = label_draw;
    label->free = label_free;
    if (text) {
        label->text = strdup(text);
    }
    label->tf = tf_new(&font_OpenSans_Regular_11X12, ui_theme->text_color, label->r.width - 2*ui_theme->padding, TF_ELIDE);

    ui_dialog_add_control(d, (ui_control_t *)label);

    return label;
}

void ui_label_set_text(ui_label_t *label, const char *text)
{
    if (label->text) {
        free(label->text);
        label->text = NULL;
    }
    if (text) {
        label->text = strdup(text);
    }
    if (label->d == ui_dialog_get_top()) {
        label->draw((ui_control_t *)label);
        label->dirty = true;
    }
}

/* ui_list */

static int list_find_index(ui_list_t *list, ui_list_item_t *item)
{
    for (int i = 0; i < list->item_count; i++) {
        if (list->items[i] == item) {
            return i;
        }
    }
    return -1;
}

static void list_draw(ui_control_t *control)
{
    ui_list_t *list = (ui_list_t *)control;

    int item_height = list->tf->font->height + 2*ui_theme->padding;
    int rows = (list->r.height - 2*BORDER + item_height - 1) / item_height;

    list->tf->clip = list->r;
    list->tf->clip.x += list->d->cr.x + BORDER;
    list->tf->clip.y += list->d->cr.y + BORDER;
    list->tf->clip.width -= 2*BORDER;
    list->tf->clip.height -= 2*BORDER;

    rect_t rb = list->r;
    rb.x += list->d->cr.x;
    rb.y += list->d->cr.y;

    fill_rectangle(fb, list->tf->clip, ui_theme->control_color);
    draw_rectangle3d(fb, rb, ui_theme->border3d_dark_color, ui_theme->border3d_light_color);
    if (control == control->d->active && !list->selected) {
        draw_rectangle(fb, list->tf->clip, DRAW_STYLE_DOTTED, ui_theme->selection_color);
    }

    int index = list_find_index(list, list->active);
    if (index < 0 && list->item_count > 0) {
        list->active = list->items[0];
        index = 0;
    }

    if (index > list->first_index + rows - 1) {
        list->first_index = index - rows + 1;
    }
    if (index < list->first_index) {
        list->first_index = index;
    }
    if (index < list->first_index + 1) {
        list->shift = 0;
    }
    if (index >= list->first_index + rows - 1) {
        list->shift = item_height - (list->r.height - 2*BORDER) % item_height;
        if (list->shift >= item_height) {
            list->shift = 0;
        }
    }

    for (unsigned int row = 0; row < rows; row++) {
        if (row >= list->item_count) {
            break;
        }
        ui_list_item_t *item = list->items[list->first_index + row];

        rect_t r = list->r;
        r.x += list->d->cr.x + BORDER + 1;
        r.y += list->d->cr.y - list->shift + row * item_height + BORDER + 1;
        r.width = list->r.width - 2*BORDER - 2;
        r.height = item_height - 2;

        point_t p = {
            .x = r.x + ui_theme->padding,
            .y = r.y + r.height/2 - list->tf->font->height/2 + 1,
        };

        if (list->first_index + row == index) {
            fill_rectangle(fb, r, list->selected ? ui_theme->active_highlight_color : ui_theme->inactive_highlight_color);
        }
        switch (item->type) {
            case LIST_ITEM_TEXT:
                tf_draw_str(fb, list->tf, item->text, p);
                break;

            case LIST_ITEM_SEPARATOR: {
                point_t start = {
                    .x = r.x + ui_theme->padding,
                    .y = r.y + item_height/2,
                };
                point_t end = {
                    .x = r.x + r.width - 1 - ui_theme->padding,
                    .y = r.y + item_height/2,
                };
                if (start.y < list->d->cr.y + list->r.y + list->r.height - 2*BORDER - 1) {
                    draw_line(fb, start, end, DRAW_STYLE_SOLID, ui_theme->text_color);
                }
                break;
            }
        }
    }

    control->dirty = true;
}

static void list_free(ui_control_t *control)
{
    ui_list_t *list = (ui_list_t *)control;

    while (list->item_count > 0) {
        ui_list_remove(list, -1);
    }
    tf_free(list->tf);
    free(list);
}

static void list_onselect(ui_control_t *control, void *arg)
{
    ui_list_t *list = (ui_list_t *)control;

    rect_t r = {
        .x = list->d->cr.x + list->r.x,
        .y = list->d->cr.y + list->r.y,
        .width = list->r.width,
        .height = list->r.height,
    };

    list->selected = true;
    list->draw(control);
    display_update_rect(r);

    keypad_info_t keys;
    while (!list->hide) {
        if (keypad_queue_receive(list->d->keypad, &keys, 50/portTICK_RATE_MS)) {
            int index = list_find_index(list, list->active);

            if (keys.pressed & KEYPAD_UP) {
                int i;
                for (i = index - 1; i >= 0; i--) {
                    if (list->items[i]->type == LIST_ITEM_TEXT) {
                        break;
                    }
                }
                if (i >= 0) {
                    list->active = list->items[i];
                    list->dirty = true;
                }
            }

            if (keys.pressed & KEYPAD_DOWN) {
                int i;
                for (i = index + 1; i < list->item_count; i++) {
                    if (list->items[i]->type == LIST_ITEM_TEXT) {
                        break;
                    }
                }
                if (i < list->item_count) {
                    list->active = list->items[i];
                    list->dirty = true;
                }
            }

            if (keys.pressed & KEYPAD_A) {
                if (index >= 0 && list->active->onselect) {
                    list->active->onselect(list->active, list->active->arg);
                }
            }

            if (keys.pressed & KEYPAD_B) {
                break;
            }

            if (keys.pressed & KEYPAD_MENU) {
                ui_dialog_unwind();
                break;
            }
        }
        if (list->dirty) {
            list->draw(control);
            display_update_rect(r);
            list->dirty = false;
        }
        periodic_tick();
    }

    list->selected = false;
    list->draw(control);
    list->dirty = true;
}

ui_list_t *ui_dialog_add_list(ui_dialog_t *d, rect_t r)
{
    ui_list_t *list = calloc(1, sizeof(ui_list_t));

    list->type = CONTROL_LIST;
    list->d = d;
    list->r = r;
    list->draw = list_draw;
    list->onselect = list_onselect;
    list->free = list_free;
    list->tf = tf_new(&font_OpenSans_Regular_11X12, ui_theme->text_color, list->r.width - 2*ui_theme->padding, TF_ELIDE);

    ui_dialog_add_control(d, (ui_control_t *)list);

    return list;
}

static ui_list_item_t *list_insert_new(ui_list_t *list, int index)
{
    if (index < 0) {
        index = list->item_count + index;
    }
    assert(index >= 0 && index <= list->item_count);

    list->items = realloc(list->items, sizeof(ui_list_item_t *) * (list->item_count + 1));
    assert(list->items != NULL);

    if (index < list->item_count) {
        memmove(&list->items[index + 1], &list->items[index], sizeof(ui_list_item_t *) * (list->item_count - index));
    }

    ui_list_item_t *item = calloc(1, sizeof(ui_list_item_t));

    list->items[index] = item;
    list->item_count += 1;
    return item;
}

ui_list_item_t *ui_list_insert_text(ui_list_t *list, int index, char *text, ui_list_item_onselect_t onselect, void *arg)
{
    ui_list_item_t *item = list_insert_new(list, index);
    item->type = LIST_ITEM_TEXT;
    item->list = list;
    item->text = strdup(text);
    item->onselect = onselect;
    item->arg = arg;
    list->dirty = true;

    return item;
}

ui_list_item_t *ui_list_append_text(ui_list_t *list, char *text, ui_list_item_onselect_t onselect, void *arg)
{
    return ui_list_insert_text(list, list->item_count, text, onselect, arg);
}

ui_list_item_t *ui_list_insert_separator(ui_list_t *list, int index)
{
    ui_list_item_t *item = list_insert_new(list, index);
    item->type = LIST_ITEM_SEPARATOR;
    item->list = list;
    list->dirty = true;

    return item;
}

ui_list_item_t *ui_list_append_separator(ui_list_t *list)
{
    return ui_list_insert_separator(list, list->item_count);
}

void ui_list_remove(ui_list_t *list, int index)
{
    if (index < 0) {
        index = list->item_count + index;
    }
    assert(index >= 0 && index < list->item_count);

    struct ui_list_item_t *item = list->items[index];
    switch (item->type) {
        case LIST_ITEM_TEXT:
            if (item->text) {
                free(item->text);
            }
            break;

        default:
            break;
    }

    free(list->items[index]);
    if (list->item_count == 1) {
        free(list->items);
        list->items = NULL;
    } else {
        if (index < list->item_count - 1) {
            memmove(&list->items[index], &list->items[index + 1], sizeof(ui_list_item_t *) * (list->item_count - index - 1));
        }
        list->items = realloc(list->items, sizeof(ui_list_item_t *) * list->item_count - 1);
        assert(list->items != NULL);
    }

    list->item_count -= 1;
}
