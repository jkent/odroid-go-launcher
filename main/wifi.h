#pragma once

#include <stdbool.h>


bool wifi_enabled;
bool wifi_connected;

void wifi_init(void);
void wifi_enable(void);
void wifi_disable(void);
