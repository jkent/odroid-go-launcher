#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "ui_dialog.h"
#include "graphics.h"


typedef struct ui_dialog_t ui_dialog_t;
typedef struct ui_control_t ui_control_t;

typedef void (*ui_control_draw_t)(ui_control_t *control);
typedef void (*ui_control_onselect_t)(ui_control_t *control);
typedef void (*ui_control_free_t)(ui_control_t *control);

typedef enum {
    CONTROL_BUTTON,
    CONTROL_EDIT,
    CONTROL_LABEL,
    CONTROL_LIST,
} ui_control_type_t;

typedef struct ui_control_t {
    ui_control_type_t type;
    ui_dialog_t *d;
    rect_t r;
    bool dirty;
    ui_control_draw_t draw;
    ui_control_onselect_t onselect;
    ui_control_free_t free;
} ui_control_t;


/* ui_button */

typedef struct ui_button_t {
    ui_control_type_t type;
    ui_dialog_t *d;
    rect_t r;
    bool dirty;
    ui_control_draw_t draw;
    ui_control_onselect_t onselect;
    ui_control_free_t free;

    char *text;
} ui_button_t;

ui_button_t *ui_button_add(ui_dialog_t *d, rect_t r, const char *text, ui_control_onselect_t onselect);


/* ui_edit */

typedef struct ui_edit_t {
    ui_control_type_t type;
    ui_dialog_t *d;
    rect_t r;
    bool dirty;
    ui_control_draw_t draw;
    ui_control_onselect_t onselect;
    ui_control_free_t free;

    char *text;
    size_t text_len;
    size_t len;
    size_t cursor;
} ui_edit_t;

ui_edit_t *ui_edit_add(ui_dialog_t *d, rect_t r, const char *text, size_t text_len);


/* ui_label */

typedef struct ui_label_t {
    ui_control_type_t type;
    ui_dialog_t *d;
    rect_t r;
    bool dirty;
    ui_control_draw_t draw;
    ui_control_onselect_t onselect;
    ui_control_free_t free;

    char *text;
} ui_label_t;

ui_label_t *ui_label_add(ui_dialog_t *d, rect_t r, const char *text);


/* ui_list */

typedef struct ui_list_t ui_list_t;
typedef void (*ui_list_onselect_t)(ui_list_t *list);

typedef struct ui_list_item_t {
    ui_list_t *list;
    const char *text;
    int value;
    ui_list_onselect_t onselect;
} ui_list_item_t;

typedef struct ui_list_t {
    ui_control_type_t type;
    ui_dialog_t *d;
    rect_t r;
    bool dirty;
    ui_control_draw_t draw;
    ui_control_onselect_t onselect;
    ui_control_free_t free;

    bool selected;
    ui_list_item_t *items;
    size_t item_count;
} ui_list_t;

ui_list_t *ui_list_add(ui_dialog_t *d, rect_t r);
