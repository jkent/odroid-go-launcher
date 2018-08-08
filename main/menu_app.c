#include <string.h>
#include <stdlib.h>

#include "display.h"

#include "app.h"
#include "menu_app.h"


static void app_selection_fn(struct menu_t *menu, int index, void *arg);

static void app_menu_populate(struct menu_t *menu)
{
    size_t count;
    struct app_info_t *apps = app_enumerate(&count);

    menu_clear(menu);

    menu_append_title(menu, "Installed apps");
    for (size_t i = 0; i < count; i++) {
        if (apps[i].installed) {
            menu_append_strdup(menu, apps[i].name, app_selection_fn, NULL);
        }
    }

    menu_append_title(menu, "Available and upgradable apps");
    for (size_t i = 0; i < count; i++) {
        if ((!apps[i].installed && apps[i].available) || apps[i].upgradable) {
            menu_append_strdup(menu, apps[i].name, app_selection_fn, NULL);
        }
    }

    if (apps) {
        free(apps);
    }
}

static void app_launch_fn(struct menu_t *menu, int index, void *arg)
{
    struct menu_t *parent = menu_get_parent(menu);
    const char *name = menu_get_label(parent, menu_get_selected(parent));
    app_run(name, false);
}

static void app_install_fn(struct menu_t *menu, int index, void *arg)
{
    struct menu_t *parent = menu_get_parent(menu);
    const char *name = menu_get_label(parent, menu_get_selected(parent));
    int slot = app_get_slot(name, NULL);
    app_install(name, slot);
    app_menu_populate(parent);
    menu_trigger_redraw(parent);
    menu_trigger_close(menu);
}

static void app_uninstall_fn(struct menu_t *menu, int index, void *arg)
{
    struct menu_t *parent = menu_get_parent(menu);
    const char *name = menu_get_label(parent, menu_get_selected(parent));
    app_uninstall(name);
    app_menu_populate(parent);
    menu_trigger_redraw(parent);
    menu_trigger_close(menu);
}

static void app_selection_fn(struct menu_t *menu, int index, void *arg)
{
    struct app_info_t info;
    const char *name = menu_get_label(menu, index);
    app_info(name, &info);

    struct rect_t r = {
        .x = DISPLAY_WIDTH/2 - 100/2,
        .y = DISPLAY_HEIGHT/2 - 60/2,
        .width = 100,
        .height = 60,
    };

    struct menu_t *m = menu_new(r, NULL);

    menu_append_text(m, "App info", NULL, NULL);

    if (!info.installed && info.available) {
        menu_append_text(m, "Install", app_install_fn, NULL);
    }
    if (info.installed) {
        menu_append_text(m, "Launch", app_launch_fn, NULL);
        menu_append_text(m, "Uninstall", app_uninstall_fn, NULL);
    }
    if (info.installed && info.upgradable) {
        menu_append_text(m, "Upgrade", app_install_fn, NULL);
    }

    menu_showmodal(m);
    menu_free(m);
}

void app_menu_fn(struct menu_t *menu, int index, void *arg)
{
    struct rect_t r = {
        .x = DISPLAY_WIDTH/2 - 240/2,
        .y = DISPLAY_HEIGHT/2 - 180/2,
        .width = 240,
        .height = 180,
    };

    struct menu_t *m = menu_new(r, NULL);

    app_menu_populate(m);

    menu_showmodal(m);
    menu_free(m);
}
