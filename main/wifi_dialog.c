#include <string.h>

#include "esp_err.h"
#include "esp_wifi.h"

#include "display.h"
#include "periodic.h"
#include "ui_dialog.h"
#include "wifi.h"


static periodic_handle_t s_periodic_scan = NULL;
static periodic_handle_t s_periodic_update = NULL;
static wifi_ap_record_t **s_scan_records = NULL;
static size_t s_scan_records_len = 0;

static void wifi_configuration_enable(ui_list_item_t *item, void *arg)
{
    if (wifi_enabled) {
        wifi_disable();
        free(item->text);
        item->text = strdup("Enable Wi-Fi");
        item->list->dirty = true;
    } else {
        wifi_enable();
        free(item->text);
        item->text = strdup("Disable Wi-Fi");
        item->list->dirty = true;
    }
}

static int compare_wifi_ap_records(const void *a, const void *b)
{
    const wifi_ap_record_t *wapa = (const wifi_ap_record_t *)a;
    const wifi_ap_record_t *wapb = (const wifi_ap_record_t *)b;

    return strcasecmp((const char *)wapa->ssid, (const char *)wapb->ssid);
}

static int find_scan_record(wifi_ap_record_t *record)
{
    int i;
    for (i = 0; i < s_scan_records_len; i++) {
        int n = compare_wifi_ap_records(record, s_scan_records[i]);
        if (n == 0) {
            return i;
        }
        if (n < 0) {
            break;
        }
    }
    return -(i + 1);
}

static void scan_update_list(periodic_handle_t handle, void *arg)
{
    wifi_ap_record_t record;
    uint16_t record_num = 1;
    ui_list_t *list = (ui_list_t *)arg;
    char s[72];

    while (true) {
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&record_num, &record));
        if (record_num == 0) {
            return;
        }

        int i = find_scan_record(&record);
        if (i >= 0) {
            s_scan_records[i]->rssi = record.rssi;
            sprintf(s, "%s [%d]", record.ssid, record.rssi);
            free(list->items[i + 2].text);
            list->items[i + 2].text = strdup(s);
            list->dirty = true;
            return;
        }

        i = -(i + 1);
        s_scan_records = realloc(s_scan_records, sizeof(wifi_ap_record_t *) * (s_scan_records_len + 1));
        assert(s_scan_records != NULL);
        memmove(&s_scan_records[i + 1], &s_scan_records[i], sizeof(wifi_ap_record_t *) * (s_scan_records_len - i));
        s_scan_records[i] = malloc(sizeof(wifi_ap_record_t));
        assert(s_scan_records[i] != NULL);
        memcpy(s_scan_records[i], &record, sizeof(wifi_ap_record_t));
        s_scan_records_len += 1;

        sprintf(s, "%s [%d]", record.ssid, record.rssi);
        ui_list_insert_text(list, i + 2, s, NULL, s_scan_records[i]);
    }
}

static void scan_restart(periodic_handle_t handle, void *arg)
{
    ESP_ERROR_CHECK(esp_wifi_scan_stop());
    wifi_scan_config_t config = { 0 };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&config, false));
}

static void wifi_configuration_add_dialog(ui_list_item_t *item, void *arg)
{
    rect_t r = {
        .x = fb->width/2 - 240/2,
        .y = fb->height/2 - 180/2,
        .width = 240,
        .height = 180,
    };
    ui_dialog_t *d = ui_dialog_new(item->list->d, r, "Add Wi-Fi Network");

    rect_t lr = {
        .x = 0,
        .y = 0,
        .width = d->cr.width,
        .height = d->cr.height,
    };
    ui_list_t *list = ui_dialog_add_list(d, lr);
    ui_list_append_text(list, "Manual entry...", NULL, NULL);

    if (wifi_enabled) {
        ui_list_append_separator(list);
        scan_restart(NULL, NULL);
        s_periodic_update = periodic_register(100/portTICK_PERIOD_MS, scan_update_list, list);
        s_periodic_scan = periodic_register(2000/portTICK_PERIOD_MS, scan_restart, NULL);
    }

    ui_dialog_showmodal(d);
    ui_dialog_destroy(d);

    if (wifi_enabled) {
        periodic_unregister(s_periodic_update);
        periodic_unregister(s_periodic_scan);
        ESP_ERROR_CHECK(esp_wifi_scan_stop());
        for (int i = 0; i < s_scan_records_len; i++) {
            free(s_scan_records[i]);
        }
        if (s_scan_records) {
            free(s_scan_records);
        }
        s_scan_records = NULL;
        s_scan_records_len = 0;
    }
}

void wifi_configuration_dialog(ui_list_item_t *item, void *arg)
{
    rect_t r = {
        .x = fb->width/2 - 240/2,
        .y = fb->height/2 - 180/2,
        .width = 240,
        .height = 180,
    };
    ui_dialog_t *d = ui_dialog_new(item->list->d, r, "Wi-Fi Configuration");

    rect_t lr = {
        .x = 0,
        .y = 0,
        .width = d->cr.width,
        .height = d->cr.height,
    };
    ui_list_t *list = ui_dialog_add_list(d, lr);

    ui_list_append_text(list, wifi_enabled ? "Disable Wi-Fi" : "Enable Wi-Fi", wifi_configuration_enable, NULL);
    ui_list_append_text(list, "Add network...", wifi_configuration_add_dialog, NULL);
    ui_list_append_text(list, "Forget network...", NULL, NULL);
    ui_list_append_text(list, "Backup configuration...", NULL, NULL);
    ui_list_append_text(list, "Restore configuration...", NULL, NULL);

    ui_dialog_showmodal(d);
    ui_dialog_destroy(d);
}
