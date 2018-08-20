#include <string.h>

#include "esp_wifi.h"

#include "display.h"
#include "sdcard.h"

#include "OpenSans_Regular_11X12.h"
#include "icons_16X16.h"
#include "periodic.h"
#include "statusbar.h"
#include "tf.h"
#include "wifi.h"


#define STATUSBAR_HEIGHT (16)

typedef struct {
    bool sdcard_present;
    wifi_state_t wifi_state;
    int wifi_bars;
} stateinfo_t;

static tf_t *s_icons;
static rect_t s_rect;
static stateinfo_t last_state;


static int rssi_to_bars(int rssi, int levels)
{
    if (rssi <= -100) {
        return 0;
    } else if (rssi >= -55) {
        return levels - 1;
    } else {
        return (rssi + 100) * (levels - 1) / 45;
    }
}

static void statusbar_update(periodic_handle_t handle, void *arg)
{
    int wifi_bars = 0;
    if (wifi_state == WIFI_STATE_CONNECTED) {
        wifi_ap_record_t record;
        ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&record));
        wifi_bars = rssi_to_bars(record.rssi, 5);
    }

    stateinfo_t state = {
        .sdcard_present = sdcard_present(),
        .wifi_state = wifi_state,
        .wifi_bars = wifi_bars,
    };

    if (memcmp(&last_state, &state, sizeof(stateinfo_t)) == 0) {
        return;
    }

    memcpy(&last_state, &state, sizeof(stateinfo_t));
    memset(fb->data, 0, DISPLAY_WIDTH * STATUSBAR_HEIGHT * fb->bytes_per_pixel);

    point_t p = {
        .x = fb->width - 16,
        .y = 0,
    };
    tf_draw_glyph(fb, s_icons, FONT_ICON_BATTERY5, p);
    p.x -= 16;
    if (sdcard_present()) {
        tf_draw_glyph(fb, s_icons, FONT_ICON_SDCARD, p);
        p.x -= 16;
    }
    if (wifi_state != WIFI_STATE_DISABLED) {
        tf_draw_glyph(fb, s_icons, FONT_ICON_WIFI0 + state.wifi_bars, p);
        p.x -= 16;
    }
    tf_draw_glyph(fb, s_icons, FONT_ICON_SPEAKER3, p);
    p.x -= 16;

    display_update_rect(s_rect);
}

void statusbar_init(void)
{
    s_icons = tf_new(&font_icons_16X16, 0xFFFF, 0, 0);
    s_rect.x = 0;
    s_rect.y = 0;
    s_rect.width = DISPLAY_WIDTH;
    s_rect.height = STATUSBAR_HEIGHT;

    statusbar_update(NULL, NULL);
    periodic_register(250/portTICK_PERIOD_MS, statusbar_update, NULL);
}
