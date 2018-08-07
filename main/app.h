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

size_t app_enumerate(struct app_info_t **apps);
bool app_info(const char *name, struct app_info_t *info);
bool app_install(const char *name, int slot);
void app_remove(const char *name);
void app_run(const char *name, bool upgrade);
