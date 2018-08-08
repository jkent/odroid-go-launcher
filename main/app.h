#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


struct app_info_t {
    char name[256];
    int slot_num;
    bool installed;
    bool available;
    bool upgradable;
};

struct app_info_t *app_enumerate(size_t *count);
int app_get_slot(const char *name, bool *installed);
void app_info(const char *name, struct app_info_t *info);
bool app_install(const char *name, int slot);
void app_uninstall(const char *name);
void app_run(const char *name, bool upgrade);
