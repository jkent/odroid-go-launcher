#include <string.h>
#include <stdlib.h>

#include "esp_err.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"

#include "display.h"

#include "menu_wifi.h"
#include "wifi.h"


static wifi_ap_record_t *scan_records = NULL;
static size_t scan_records_len = 0;

static int compare_wifi_ap_records(const void *a, const void *b)
{
    const wifi_ap_record_t *wapa = (const wifi_ap_record_t *)a;
    const wifi_ap_record_t *wapb = (const wifi_ap_record_t *)b;

    return strcmp((const char *)wapa->ssid, (const char *)wapb->ssid);
}

static int find_scan_record(wifi_ap_record_t *record)
{
    int i;
    for (i = 0; i < scan_records_len; i++) {
        int n = compare_wifi_ap_records(record, &scan_records[i]);
        if (n == 0) {
            return i;
        }
        if (n < 0) {
            break;
        }
    }
    return -(i + 1);
}

static void wifi_scan_task(struct menu_t *menu)
{
    wifi_ap_record_t record;
    uint16_t record_num = 1;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&record_num, &record));
    if (record_num == 0) {
        return;
    }

    int i = find_scan_record(&record);
    if (i >= 0) {
        return;
    }

    i = -(i + 1);
    scan_records = realloc(scan_records, sizeof(wifi_ap_record_t) * (scan_records_len + 1));
    memmove(&scan_records[i + 1], &scan_records[i], sizeof(wifi_ap_record_t) * (scan_records_len - i));
    memcpy(&scan_records[i], &record, sizeof(wifi_ap_record_t));
    scan_records_len += 1;

    char s[128];
    sprintf(s, "%s [%d]", record.ssid, record.rssi);
    menu_insert_strdup(menu, i + 1, s, NULL, NULL);
}

static void wifi_scan_fn(struct menu_t *menu, int index, void *arg)
{
    wifi_scan_config_t config = {
        .scan_time = {
            .active = {
                .min = 500,
                .max = 1500,
            },
        },
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&config, false));

    struct rect_t r = {
        .x = DISPLAY_WIDTH/2 - 240/2,
        .y = DISPLAY_HEIGHT/2 - 180/2,
        .width = 240,
        .height = 180,
    };

    struct menu_t *m = menu_new(r, wifi_scan_task);
    menu_append_text(m, "Add a network manually", NULL, NULL);
    menu_showmodal(m);
    menu_free(m);

    ESP_ERROR_CHECK(esp_wifi_scan_stop());
    if (scan_records) {
        free(scan_records);
        scan_records = NULL;
        scan_records_len = 0;
    }
}

static void wifi_enable_fn(struct menu_t *menu, int index, void *arg)
{
    if (wifi_enabled) {
        wifi_enable();
        menu_insert_text(menu, 1, "Connect to a network", wifi_scan_fn, NULL);
    } else {
        menu_remove(menu, 1);
        wifi_disable();
    }

    menu_set_value(menu, index, wifi_enabled);
}

void wifi_menu_fn(struct menu_t *menu, int index, void *arg)
{
    struct rect_t r = {
        .x = DISPLAY_WIDTH/2 - 240/2,
        .y = DISPLAY_HEIGHT/2 - 180/2,
        .width = 240,
        .height = 180,
    };

    const char *wifi_enable_list[] = {
        "Enable WiFi",
        "Disable WiFi",
        NULL,
    };

    struct menu_t *m = menu_new(r, NULL);
    menu_append_list(m, wifi_enable_list, wifi_enabled, wifi_enable_fn, NULL);
    if (wifi_enabled) {
        menu_append_text(m, "Scan for networks", wifi_scan_fn, NULL);
    }
    menu_showmodal(m);
    menu_free(m);
}
