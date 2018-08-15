#include <string.h>

#include "app.h"
#include "display.h"
#include "ui_dialog.h"


static struct app_info_t *s_app_info = NULL;
static size_t s_app_count = 0;

static void app_popup_install(ui_list_item_t *item, void *arg)
{
    struct app_info_t *info = (struct app_info_t *)arg;

    app_install(info->name, info->slot_num);
    item->list->hide = true;
}

static void app_popup_run(ui_list_item_t *item, void *arg)
{
    struct app_info_t *info = (struct app_info_t *)arg;

    app_run(info->name, false);
    item->list->hide = true;
}

static void app_popup_uninstall(ui_list_item_t *item, void *arg)
{
    struct app_info_t *info = (struct app_info_t *)arg;

    app_uninstall(info->name);
    item->list->hide = true;
}

static void fill_app_list(ui_list_t *list);

static void app_list_select(ui_list_item_t *item, void *arg)
{
    struct app_info_t *info = (struct app_info_t *)arg;

    rect_t r = {
        .x = fb->width/2 - 120/2,
        .y = fb->height/2 - 90/2,
        .width = 120,
        .height = 90,
    };
    ui_dialog_t *d = ui_dialog_new(item->list->d, r, NULL);

    rect_t lr = {
        .x = 0,
        .y = 0,
        .width = d->cr.width,
        .height = d->cr.height,
    };
    ui_list_t *list = ui_dialog_add_list(d, lr);
    if (!info->installed && info->available) {
        ui_list_append_text(list, "Install", app_popup_install, info);
    }
    if (info->installed || info->available) {
        ui_list_append_text(list, "Run", app_popup_run, info);
    }
    if (info->installed) {
        ui_list_append_text(list, "Uninstall", app_popup_uninstall, info);
    }
    if (info->installed && info->upgradable) {
        ui_list_append_text(list, "Upgrade", app_popup_install, info);
    }
    ui_dialog_showmodal(d);
    ui_dialog_destroy(d);

    fill_app_list(item->list);
}

static void fill_app_list(ui_list_t *list)
{
    char *active_name = NULL;
    if (s_app_info) {
        struct app_info_t *selected_info = (struct app_info_t *)list->active->arg;
        active_name = strdup(selected_info->name);
        free(s_app_info);
    }

    while (list->item_count > 0) {
        ui_list_remove(list, -1);
    }

    s_app_info = app_enumerate(&s_app_count);

    for (int i = 0; i < s_app_count; i++) {
        char buf[269];
        if (s_app_info[i].installed && s_app_info[i].upgradable) {
            snprintf(buf, sizeof(buf), "%s [Upgradable]", s_app_info[i].name);
        } else if (s_app_info[i].installed) {
            snprintf(buf, sizeof(buf), "%s [Installed]", s_app_info[i].name); 
        } else {
            strncpy(buf, s_app_info[i].name, sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';
        }
        ui_list_item_t *item = ui_list_append_text(list, buf, app_list_select, &s_app_info[i]);
        if (active_name && strcmp(active_name, s_app_info[i].name) == 0) {
            list->active = item;
        }
    }
    if (active_name) {
        free(active_name);
    }

    list->dirty = true;
}

void app_list_dialog(ui_list_item_t *item, void *arg)
{
    rect_t r = {
        .x = fb->width/2 - 240/2,
        .y = fb->height/2 - 180/2,
        .width = 240,
        .height = 180,
    };
    ui_dialog_t *d = ui_dialog_new(item->list->d, r, "App List");

    rect_t lr = {
        .x = 0,
        .y = 0,
        .width = d->cr.width,
        .height = d->cr.height,
    };
    ui_list_t *list = ui_dialog_add_list(d, lr);
    fill_app_list(list);
    ui_dialog_showmodal(d);
    ui_dialog_destroy(d);

    if (s_app_info) {
        free(s_app_info);
        s_app_info = NULL;
    }
}
