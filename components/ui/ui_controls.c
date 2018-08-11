#include <string.h>

#include "display.h"
#include "OpenSans_Regular_11X12.h"
#include "tf.h"
#include "ui_controls.h"
#include "ui_dialog.h"
#include "ui_osk.h"


/* ui_button */

static void button_draw(ui_control_t *control)
{
    ui_button_t *button = (ui_button_t *)control;

    tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, button->r.width - 4, TF_ALIGN_CENTER | TF_ELIDE);
    tf->clip = button->r;
    tf->clip.x += button->d->cr.x;
    tf->clip.y += button->d->cr.y;

    fill_rectangle(fb, tf->clip, 0x632C);
    if (control == control->d->active) {
        draw_rectangle(fb, tf->clip, DRAW_STYLE_SOLID, 0xFFFF);
    }

    if (button->text) {
        tf_metrics_t m = tf_get_str_metrics(tf, button->text);
        point_t p = {
            .x = button->d->cr.x + button->r.x + 2,
            .y = button->d->cr.y + button->r.y + button->r.height/2 - m.height/2 + 1,
        };
        tf_draw_str(fb, tf, button->text, p);
    }

    tf_free(tf);
    button->dirty = true;
}

static void button_free(ui_control_t *control)
{
    ui_button_t *button = (ui_button_t *)control;

    if (button->text) {
        free(button->text);
    }
    free(button);
}

ui_button_t *ui_button_add(ui_dialog_t *d, rect_t r, const char *text, ui_control_onselect_t onselect)
{
    ui_button_t *button = calloc(1, sizeof(ui_button_t));

    button->type = CONTROL_BUTTON;
    button->d = d;
    button->r = r;
    button->draw = button_draw;
    button->onselect = onselect;
    button->free = button_free;
    button->text = strdup(text);

    ui_dialog_add(d, (ui_control_t *)button);

    return button;
}


/* ui_edit */

static void edit_draw(ui_control_t *control)
{
    ui_edit_t *edit = (ui_edit_t *)control;

    tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, edit->r.width - 4, 0);
    tf->clip = edit->r;
    tf->clip.x += edit->d->cr.x;
    tf->clip.y += edit->d->cr.y;

    fill_rectangle(fb, tf->clip, 0x3186);
    if (control == control->d->active) {
        draw_rectangle(fb, tf->clip, DRAW_STYLE_SOLID, 0xFFFF);
    }

    if (edit->text) {
        tf_metrics_t m = tf_get_str_metrics(tf, edit->text);
        point_t p = {
            .x = edit->d->cr.x + edit->r.x + 2,
            .y = edit->d->cr.y + edit->r.y + edit->r.height/2 - m.height/2 + 1,
        };
        tf_draw_str(fb, tf, edit->text, p);
    }

    tf_free(tf);
    edit->dirty = true;
}

static void edit_onselect(ui_control_t *control)
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

    if (edit->text) {
        free(edit->text);
    }
    free(edit);
}

ui_edit_t *ui_edit_add(ui_dialog_t *d, rect_t r, const char *text, size_t text_len)
{
    ui_edit_t *edit = calloc(1, sizeof(ui_edit_t));

    edit->type = CONTROL_EDIT;
    edit->d = d;
    edit->r = r;
    edit->draw = edit_draw;
    edit->onselect = edit_onselect;
    edit->free = edit_free;
    edit->text = malloc(text_len);
    assert(edit->text != NULL);
    strncpy(edit->text, text, text_len);
    edit->text[text_len - 1] = '\0';
    edit->text_len = text_len;

    ui_dialog_add(d, (ui_control_t *)edit);

    return edit;
}


/* ui_label */

static void label_draw(ui_control_t *control)
{
    ui_label_t *label = (ui_label_t *)control;

    tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, label->r.width, TF_ELIDE);
    tf->clip = label->r;
    tf->clip.x += label->d->cr.x;
    tf->clip.y += label->d->cr.y;

    if (label->text) {
        tf_metrics_t m = tf_get_str_metrics(tf, label->text);
        point_t p = {
            .x = label->d->cr.x + label->r.x,
            .y = label->d->cr.y + label->r.y + label->r.height/2 - m.height/2,
        };
        tf_draw_str(fb, tf, label->text, p);
    }

    tf_free(tf);
    label->dirty = true;
}

static void label_free(ui_control_t *control)
{
    ui_label_t *label = (ui_label_t *)control;

    if (label->text) {
        free(label->text);
    }
    free(label);
}

ui_label_t *ui_label_add(ui_dialog_t *d, rect_t r, const char *text)
{
    ui_label_t *label = calloc(1, sizeof(ui_label_t));

    label->type = CONTROL_LABEL;
    label->d = d;
    label->r = r;
    label->draw = label_draw;
    label->free = label_free;
    label->text = strdup(text);

    ui_dialog_add(d, (ui_control_t *)label);

    return label;
}


/* ui_list */
