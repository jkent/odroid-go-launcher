#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "ui_dialog.h"
#include "graphics.h"
#include "tf.h"

typedef struct ui_dialog_t ui_dialog_t;
typedef struct ui_control_t ui_control_t;

typedef void (*ui_control_draw_t)(ui_control_t *control);
typedef void (*ui_control_onselect_t)(ui_control_t *control, void *arg);
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
    tf_t *tf;
    bool dirty;
    bool hide;
    ui_control_draw_t draw;
    ui_control_onselect_t onselect;
    ui_control_free_t free;
    void *arg;
} ui_control_t;


/* ui_button */

typedef struct ui_button_t {
    ui_control_type_t type;
    ui_dialog_t *d;
    rect_t r;
    tf_t *tf;
    bool dirty;
    bool hide;
    ui_control_draw_t draw;
    ui_control_onselect_t onselect;
    ui_control_free_t free;
    void *arg;

    char *text;
} ui_button_t;

ui_button_t *ui_dialog_add_button(ui_dialog_t *d, rect_t r, const char *text, ui_control_onselect_t onselect, void *arg);


/* ui_edit */

typedef struct ui_edit_t {
    ui_control_type_t type;
    ui_dialog_t *d;
    rect_t r;
    tf_t *tf;
    bool dirty;
    bool hide;
    ui_control_draw_t draw;
    ui_control_onselect_t onselect;
    ui_control_free_t free;
    void *arg;

    char *text;
    size_t text_len;
    size_t len;
    size_t cursor;
    bool password;
} ui_edit_t;

ui_edit_t *ui_dialog_add_edit(ui_dialog_t *d, rect_t r, char *text, size_t text_len);


/* ui_label */

typedef struct ui_label_t {
    ui_control_type_t type;
    ui_dialog_t *d;
    rect_t r;
    tf_t *tf;
    bool dirty;
    bool hide;
    ui_control_draw_t draw;
    ui_control_onselect_t onselect;
    ui_control_free_t free;
    void *arg;

    char *text;
} ui_label_t;

ui_label_t *ui_dialog_add_label(ui_dialog_t *d, rect_t r, const char *text);
void ui_label_set_text(ui_label_t *label, const char *text);


/* ui_list */

typedef struct ui_list_t ui_list_t;
typedef struct ui_list_item_t ui_list_item_t;
typedef void (*ui_list_item_onselect_t)(ui_list_item_t *item, void *arg);

typedef struct ui_list_t {
    ui_control_type_t type;
    ui_dialog_t *d;
    rect_t r;
    tf_t *tf;
    bool dirty;
    bool hide;
    ui_control_draw_t draw;
    ui_control_onselect_t onselect;
    ui_control_free_t free;
    void *arg;

    bool selected;
    ui_list_item_t **items;
    ui_list_item_t *active;
    size_t item_count;
    int first_index;
    int shift;
} ui_list_t;

typedef enum {
    LIST_ITEM_TEXT,
    LIST_ITEM_SEPARATOR,
} ui_list_item_type_t;

typedef struct ui_list_item_t {
    ui_list_item_type_t type;
    ui_list_t *list;
    char *text;
    void *arg;
    ui_list_item_onselect_t onselect;
} ui_list_item_t;

ui_list_t *ui_dialog_add_list(ui_dialog_t *d, rect_t r);
ui_list_item_t *ui_list_insert_text(ui_list_t *list, int index, char *text, ui_list_item_onselect_t onselect, void *arg);
ui_list_item_t *ui_list_append_text(ui_list_t *list, char *text, ui_list_item_onselect_t onselect, void *arg);
ui_list_item_t *ui_list_insert_separator(ui_list_t *list, int index);
ui_list_item_t *ui_list_append_separator(ui_list_t *list);
void ui_list_remove(ui_list_t *list, int index);
