#include <string.h>
#include <stdlib.h>

#include "esp_err.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"

#include "wifi.h"


bool wifi_enabled = false;
bool wifi_connected = false;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            wifi_connected = true;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            wifi_connected = false;
            break;
        default:
            break;
    }
    return ESP_OK;
}

void wifi_init(void)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
}

void wifi_enable(void)
{
    if (wifi_enabled) {
        return;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    wifi_enabled = true;
}

void wifi_disable(void)
{
    if (!wifi_enabled) {
        return;
    }

    ESP_ERROR_CHECK(esp_wifi_stop());
    wifi_enabled = false;
}
