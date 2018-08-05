#pragma once

#include <stdbool.h>

#include "menu.h"


bool wifi_connected;
bool wifi_enabled;

void wifi_init(void);
void wifi_menu_fn(struct menu_t *menu, int index, void *arg);
