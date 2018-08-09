#pragma once

#include <stddef.h>

#include "dialog.h"


typedef struct dialog_t dialog_t;
typedef struct control_t control_t;
typedef void (*control_draw_t)(control_t *control);
typedef void (*control_onselect_t)(control_t *control);

typedef enum {
    CONTROL_LABEL,
    CONTROL_BUTTON,
    CONTROL_EDIT,
    CONTROL_LISTBOX,
} control_type_t;

typedef struct control_t {
    control_type_t type;
    dialog_t *d;
    rect_t r;
    bool dirty;
    control_draw_t draw;
    control_onselect_t onselect;
} control_t;

typedef struct {
    control_type_t type;
    dialog_t *d;
    rect_t r;
    bool dirty;
    control_draw_t draw;
    control_onselect_t onselect;
    char *text;
} control_label_t;

typedef struct {
    control_type_t type;
    dialog_t *d;
    rect_t r;
    bool dirty;
    control_draw_t draw;
    control_onselect_t onselect;
    char *text;
} control_button_t;

typedef struct {
    control_type_t type;
    dialog_t *d;
    rect_t r;
    bool dirty;
    control_draw_t draw;
    control_onselect_t onselect;
    char *value;
    size_t max_len;
    size_t len;
    size_t cursor;
} control_edit_t;

typedef struct {
    control_type_t type;
    dialog_t *d;
    rect_t r;
    bool dirty;
    control_draw_t draw;
    control_onselect_t onselect;
    /* TODO */
} control_listbox_t;

control_label_t *control_label_new(dialog_t *d, rect_t r, const char *text);
void control_label_delete(control_label_t *label);
control_button_t *control_button_new(dialog_t *d, rect_t r, const char *text, control_onselect_t onselect);
void control_button_delete(control_button_t *button);
