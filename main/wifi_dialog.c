#include <string.h>

#include "esp_err.h"
#include "esp_wifi.h"

#include "display.h"
#include "periodic.h"
#include "ui_dialog.h"
#include "ui_theme.h"
#include "wifi.h"


static periodic_handle_t s_ph_update = NULL;
static wifi_ap_record_t **s_scan_records = NULL;
static size_t s_scan_records_len = 0;


static void wifi_configuration_enable(ui_list_item_t *item, void *arg)
{
    if (wifi_enabled) {
        wifi_disable();
        free(item->text);
        item->text = strdup("Enable Wi-Fi");
    } else {
        wifi_enable();
        free(item->text);
        item->text = strdup("Disable Wi-Fi");
    }
    item->list->dirty = true;
}

static void status_refresh(periodic_handle_t handle, void *arg)
{
    ui_dialog_t *d = (ui_dialog_t *)arg;
    char buf[128];
    wifi_ap_record_t record;

    if (wifi_enabled && wifi_connected) {
        ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&record))
    }

    ui_label_t *label = (ui_label_t *)d->controls[0];
    if (label->text) {
        free(label->text);
    }
    sprintf(buf, "Status: %s", wifi_enabled ?
        (wifi_connected ? "Connected" : "Disconnected") : "Disabled");
    label->text = strdup(buf);
    label->draw((ui_control_t *)label);
    label->dirty = true;

    label = (ui_label_t *)d->controls[1];
    if (label->text) {
        free(label->text);
    }
    if (wifi_enabled && wifi_connected) {
        sprintf(buf, "SSID: %s", record.ssid);
        label->text = strdup(buf);
    }
    label->draw((ui_control_t *)label);
    label->dirty = true;

    label = (ui_label_t *)d->controls[2];
    if (label->text) {
        free(label->text);
    }
    if (wifi_enabled && wifi_connected) {
        sprintf(buf, "BSSID: %02X:%02X:%02X:%02X:%02X:%02X", record.bssid[0],
            record.bssid[1], record.bssid[2], record.bssid[3], record.bssid[4],
            record.bssid[5]);
        label->text = strdup(buf);
    }
    label->draw((ui_control_t *)label);
    label->dirty = true;

    label = (ui_label_t *)d->controls[3];
    if (label->text) {
        free(label->text);
    }
    if (wifi_enabled && wifi_connected) {
        sprintf(buf, "Channel: %d", record.primary);
        label->text = strdup(buf);
    }
    label->draw((ui_control_t *)label);
    label->dirty = true;

    label = (ui_label_t *)d->controls[4];
    if (label->text) {
        free(label->text);
    }
    if (wifi_enabled && wifi_connected) {
        sprintf(buf, "RSSI: %d", record.rssi);
        label->text = strdup(buf);
    }
    label->draw((ui_control_t *)label);
    label->dirty = true;

    label = (ui_label_t *)d->controls[5];
    if (label->text) {
        free(label->text);
    }
    if (wifi_enabled && wifi_connected) {
        char ip[16];
        ip4addr_ntoa_r(&wifi_ip, ip, sizeof(ip));
        sprintf(buf, "IP: %s", ip);
        label->text = strdup(buf);
    }
    label->draw((ui_control_t *)label);
    label->dirty = true;
}

static void wifi_status_dialog(ui_list_item_t *item, void *arg)
{
    rect_t r = {
        .x = fb->width/2 - 160/2,
        .y = fb->height/2 - 120/2,
        .width = 160,
        .height = 120,
    };
    ui_dialog_t *d = ui_dialog_new(item->list->d, r, "Wi-Fi Status");

    rect_t lr = {
        .x = 0,
        .y = 0,
        .width = d->cr.width,
        .height = d->tf->font->height + 4,
    };

    ui_dialog_add_label(d, lr, NULL);
    lr.y += lr.height;
    ui_dialog_add_label(d, lr, NULL);
    lr.y += lr.height;
    ui_dialog_add_label(d, lr, NULL);
    lr.y += lr.height;
    ui_dialog_add_label(d, lr, NULL);
    lr.y += lr.height;
    ui_dialog_add_label(d, lr, NULL);
    lr.y += lr.height;
    ui_dialog_add_label(d, lr, NULL);

    periodic_handle_t ph = periodic_register(100/portTICK_PERIOD_MS, status_refresh, d);
    ui_dialog_showmodal(d);
    ui_dialog_destroy(d);
    periodic_unregister(ph);
}

static void wifi_configuration_add_manual_security(ui_control_t *control, void *arg)
{
    ui_button_t *button = (ui_button_t *)control;
    wifi_network_t *network = (wifi_network_t *)arg;

    if (button->text) {
        free(button->text);
        button->text = NULL;
    }

    switch (network->authmode) {
        case WIFI_AUTH_OPEN:
            network->authmode = WIFI_AUTH_WEP;
            button->text = strdup("WEP");
            break;

        case WIFI_AUTH_WEP:
            network->authmode = WIFI_AUTH_WPA_PSK;
            button->text = strdup("WPA-PSK");
            break;

        case WIFI_AUTH_WPA_PSK:
            network->authmode = WIFI_AUTH_WPA2_PSK;
            button->text = strdup("WPA2-PSK");
            break;

        case WIFI_AUTH_WPA2_PSK:
            network->authmode = WIFI_AUTH_OPEN;
            button->text = strdup("Open");
            break;

        default:
            break;
    }

    button->draw(control);
}

static void wifi_configuration_add_manual_save(ui_control_t *control, void *arg)
{
    ui_button_t *button = (ui_button_t *)control;
    wifi_network_t *network = (wifi_network_t *)arg;

    periodic_unregister(s_ph_update);
    wifi_register_scan_done_callback(NULL, NULL);
    ESP_ERROR_CHECK(esp_wifi_scan_stop());

    wifi_network_add(network);

    button->d->hide = true;
    button->d->parent->controls[0]->hide = true;
}

static void wifi_configuration_add_manual_dialog(ui_list_item_t *item, void *arg)
{
    rect_t r = {
        .x = fb->width/2 - 320/2,
        .y = fb->height/2 - 91/2,
        .width = 320,
        .height = 91,
    };
    ui_dialog_t *d = ui_dialog_new(item->list->d, r, "Manual Wi-Fi Entry");

    rect_t lr = {
        .x = 0,
        .y = 0,
        .width = 75,
        .height = d->tf->font->height + 2*ui_theme->padding,
    };

    wifi_network_t network = { 0 };

    ui_dialog_add_label(d, lr, "SSID");
    lr.y += lr.height;
    ui_dialog_add_label(d, lr, "Password");
    lr.y += lr.height;
    ui_dialog_add_label(d, lr, "Security");

    lr.y = 0;
    lr.x += lr.width;
    lr.width = d->cr.width - lr.width;
    ui_dialog_add_edit(d, lr, network.ssid, sizeof(network.ssid));
    lr.y += lr.height;
    ui_edit_t *edit = ui_dialog_add_edit(d, lr, network.password, sizeof(network.password));
    edit->password = true;
    lr.y += lr.height;
    ui_dialog_add_button(d, lr, "Open", wifi_configuration_add_manual_security, &network);

    lr.y += lr.height * 3 / 2;
    lr.x = 0;
    lr.width = d->cr.width;
    ui_dialog_add_button(d, lr, "Save", wifi_configuration_add_manual_save, &network);

    ui_dialog_showmodal(d);
    ui_dialog_destroy(d);
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
            free(list->items[i + 2]->text);
            list->items[i + 2]->text = strdup(s);
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

static void do_scan(void *arg)
{
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
    ui_list_append_text(list, "Manual entry...", wifi_configuration_add_manual_dialog, NULL);

    if (wifi_enabled) {
        ui_list_append_separator(list);
        do_scan(NULL);
        wifi_register_scan_done_callback(do_scan, NULL);
        s_ph_update = periodic_register(100/portTICK_PERIOD_MS, scan_update_list, list);
    }

    ui_dialog_showmodal(d);
    ui_dialog_destroy(d);

    if (wifi_enabled) {
        wifi_register_scan_done_callback(NULL, NULL);
        periodic_unregister(s_ph_update);
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
    ui_list_append_text(list, "Wi-Fi status...", wifi_status_dialog, NULL);
    ui_list_append_text(list, "Add network...", wifi_configuration_add_dialog, NULL);
    ui_list_append_text(list, "Forget network...", NULL, NULL);
    ui_list_append_text(list, "Backup configuration...", NULL, NULL);
    ui_list_append_text(list, "Restore configuration...", NULL, NULL);

    ui_dialog_showmodal(d);
    ui_dialog_destroy(d);
}
