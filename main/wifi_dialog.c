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


static void enable_toggle(ui_list_item_t *item, void *arg)
{
    wifi_state_t wifi_state = wifi_get_state();

    if (wifi_state != WIFI_STATE_DISABLED) {
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
    wifi_state_t wifi_state = wifi_get_state();

    if (wifi_state == WIFI_STATE_CONNECTED) {
        ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&record))
    }

    ui_label_t *label = (ui_label_t *)d->controls[0];
    if (label->text) {
        free(label->text);
    }
    char *status = "";
    switch (wifi_state) {
        case WIFI_STATE_DISABLED:
            status = "Disabled";
            break;
        case WIFI_STATE_DISCONNECTED:
            status = "Disconnected";
            break;
        case WIFI_STATE_CONNECTING:
            status = "Connecting";
            break;
        case WIFI_STATE_CONNECTED:
            status = "Connected";
            break;
        default:
            ; /* do nothing */
    }

    sprintf(buf, "Status: %s", status);
    label->text = strdup(buf);
    label->draw((ui_control_t *)label);
    label->dirty = true;

    label = (ui_label_t *)d->controls[1];
    if (label->text) {
        free(label->text);
        label->text = NULL;
    }
    if (wifi_state == WIFI_STATE_CONNECTED) {
        sprintf(buf, "SSID: %s", record.ssid);
        label->text = strdup(buf);
    }
    label->draw((ui_control_t *)label);
    label->dirty = true;

    label = (ui_label_t *)d->controls[2];
    if (label->text) {
        free(label->text);
        label->text = NULL;
    }
    if (wifi_state == WIFI_STATE_CONNECTED) {
        sprintf(buf, "BSSID: " MACSTR, MAC2STR(record.bssid));
        label->text = strdup(buf);
    }
    label->draw((ui_control_t *)label);
    label->dirty = true;

    label = (ui_label_t *)d->controls[3];
    if (label->text) {
        free(label->text);
        label->text = NULL;
    }
    if (wifi_state == WIFI_STATE_CONNECTED) {
        sprintf(buf, "Channel: %d", record.primary);
        label->text = strdup(buf);
    }
    label->draw((ui_control_t *)label);
    label->dirty = true;

    label = (ui_label_t *)d->controls[4];
    if (label->text) {
        free(label->text);
        label->text = NULL;
    }
    if (wifi_state == WIFI_STATE_CONNECTED) {
        sprintf(buf, "RSSI: %d", record.rssi);
        label->text = strdup(buf);
    }
    label->draw((ui_control_t *)label);
    label->dirty = true;

    label = (ui_label_t *)d->controls[5];
    if (label->text) {
        free(label->text);
        label->text = NULL;
    }
    if (wifi_state == WIFI_STATE_CONNECTED) {
        ip4_addr_t ip = wifi_get_ip();
        sprintf(buf, "IP: " IPSTR, IP2STR(&ip));
        label->text = strdup(buf);
    }
    label->draw((ui_control_t *)label);
    label->dirty = true;
}

static void status_dialog(ui_list_item_t *item, void *arg)
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

    periodic_handle_t ph = periodic_register(250/portTICK_PERIOD_MS, status_refresh, d);
    ui_dialog_showmodal(d);
    ui_dialog_destroy(d);
    periodic_unregister(ph);
}

static void add_manual_security(ui_control_t *control, void *arg)
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
            network->authmode = WIFI_AUTH_WPA_WPA2_PSK;
            button->text = strdup("WPA/WPA2-PSK");
            break;

        case WIFI_AUTH_WPA_WPA2_PSK:
        default:
            network->authmode = WIFI_AUTH_OPEN;
            button->text = strdup("Open");
            break;
   }

    button->draw(control);
}

static void networks_popup(ui_list_item_t *item, void *arg);

static void add_entry_save(ui_control_t *control, void *arg)
{
    ui_button_t *button = (ui_button_t *)control;
    wifi_network_t *network = (wifi_network_t *)arg;

    if (strlen(network->ssid) == 0) {
        return;
    }

    periodic_unregister(s_ph_update);
    wifi_register_scan_done_callback(NULL, NULL);

    size_t i = wifi_network_add(network);

    ui_list_t *list = (ui_list_t *)button->d->parent->parent->controls[0];
    ui_list_insert_text(list, i + 2, wifi_networks[i]->ssid, networks_popup, wifi_networks[i]);

    button->d->hide = true;
    button->d->parent->controls[0]->hide = true;
}

static void add_entry_dialog(ui_list_item_t *item, void *arg)
{
    wifi_ap_record_t *record = (wifi_ap_record_t *)arg;

    rect_t r = {
        .x = fb->width/2 - 320/2,
        .y = fb->height/2 - 91/2,
        .width = 320,
        .height = 91,
    };
    ui_dialog_t *d = ui_dialog_new(item->list->d, r, "Add Wi-Fi Entry");

    rect_t lr = {
        .x = 0,
        .y = 0,
        .width = 75,
        .height = d->tf->font->height + 2*ui_theme->padding,
    };

    wifi_network_t network = { 0 };

    if (record) {
        strncpy(network.ssid, (char *)record->ssid, sizeof(network.ssid));
        network.authmode = record->authmode;
    }

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
    char *label = "Open";
    switch (network.authmode) {
        case WIFI_AUTH_WEP:
            label = "WEP";
            break;

        case WIFI_AUTH_WPA_PSK:
            label = "WPA-PSK";
            break;

        case WIFI_AUTH_WPA2_PSK:
            label = "WPA2-PSK";
            break;

        case WIFI_AUTH_WPA_WPA2_PSK:
            label = "WPA/WPA2-PSK";
            break;

        default:
            network.authmode = 0;
            break;
    }
    ui_dialog_add_button(d, lr, label, add_manual_security, &network);

    lr.y += lr.height * 3 / 2;
    lr.x = 0;
    lr.width = d->cr.width;
    ui_dialog_add_button(d, lr, "Save", add_entry_save, &network);

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
        ui_list_insert_text(list, i + 2, s, add_entry_dialog, s_scan_records[i]);
    }
}

static void do_scan(void *arg)
{
    wifi_scan_config_t config = { 0 };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&config, false));
}

static void add_network_dialog(ui_list_item_t *item, void *arg)
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
    ui_list_append_text(list, "Manual entry...", add_entry_dialog, NULL);

    wifi_state_t wifi_state = wifi_get_state();

    if (wifi_state != WIFI_STATE_DISABLED) {
        ui_list_append_separator(list);
        do_scan(NULL);
        wifi_register_scan_done_callback(do_scan, NULL);
        s_ph_update = periodic_register(100/portTICK_PERIOD_MS, scan_update_list, list);
    }

    ui_dialog_showmodal(d);
    ui_dialog_destroy(d);

    if (wifi_state != WIFI_STATE_DISABLED) {
        wifi_register_scan_done_callback(NULL, NULL);
        periodic_unregister(s_ph_update);
        /* don't stop the scan because we could be attempting to connect! */
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

static void connect_network(ui_list_item_t *item, void *arg) 
{
    struct wifi_network_t *network = (struct wifi_network_t *)arg;
    wifi_connect_network(network);

    item->list->hide = true;
}

static void forget_network(ui_list_item_t *item, void *arg)
{
    struct wifi_network_t *network = (struct wifi_network_t *)arg;

    int i = wifi_network_delete(network);
    ui_list_t *list = (ui_list_t *)item->list->d->parent->active;
    ui_list_remove(list, i + 2);

    list->dirty = true;
    item->list->hide = true;
}

static void networks_popup(ui_list_item_t *item, void *arg)
{
    struct wifi_network_t *network = (struct wifi_network_t *)arg;

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
    ui_list_append_text(list, "Connect", connect_network, network);
    ui_list_append_text(list, "Forget", forget_network, network);

    ui_dialog_showmodal(d);
    ui_dialog_destroy(d);
}

static void networks_dialog(ui_list_item_t *item, void *arg)
{
    rect_t r = {
        .x = fb->width/2 - 240/2,
        .y = fb->height/2 - 180/2,
        .width = 240,
        .height = 180,
    };
    ui_dialog_t *d = ui_dialog_new(item->list->d, r, "Wi-Fi Networks");

    rect_t lr = {
        .x = 0,
        .y = 0,
        .width = d->cr.width,
        .height = d->cr.height,
    };
    ui_list_t *list = ui_dialog_add_list(d, lr);

    ui_list_append_text(list, "Add network...", add_network_dialog, NULL);
    ui_list_append_separator(list);

    wifi_network_t *network = NULL;
    while ((network = wifi_network_iterate(network))) {
        ui_list_append_text(list, network->ssid, networks_popup, network);
    }

    ui_dialog_showmodal(d);
    ui_dialog_destroy(d);
}

static void backup_config(ui_list_item_t *item, void *arg)
{
    wifi_backup_config();
}

static void restore_config(ui_list_item_t *item, void *arg)
{
    wifi_restore_config();
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

    wifi_state_t wifi_state = wifi_get_state();

    ui_list_append_text(list, wifi_state != WIFI_STATE_DISABLED ? "Disable Wi-Fi" : "Enable Wi-Fi", enable_toggle, NULL);
    ui_list_append_text(list, "Status...", status_dialog, NULL);
    ui_list_append_text(list, "Networks...", networks_dialog, NULL);
    ui_list_append_text(list, "Backup configuration", backup_config, NULL);
    ui_list_append_text(list, "Restore configuration", restore_config, NULL);

    ui_dialog_showmodal(d);
    ui_dialog_destroy(d);
}
