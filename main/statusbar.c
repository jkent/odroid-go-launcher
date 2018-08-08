#include <string.h>

#include "esp_wifi.h"

#include "display.h"
#include "sdcard.h"

#include "OpenSans_Regular_11X12.h"
#include "icons_16X16.h"
#include "menu_wifi.h"
#include "statusbar.h"
#include "tf.h"
#include "wifi.h"


#define STATUSBAR_HEIGHT (16)

struct stateinfo_t {
    bool sdcard_present;
    bool wifi_enabled;
    int wifi_bars;
};

static struct tf_t *s_icons;
static struct rect_t s_rect;
static struct stateinfo_t last_state;


static int rssi_to_bars(int rssi, int levels)
{
    if (rssi < -100) {
        return 0;
    } else if (rssi > -55) {
        return levels - 1;
    } else {
       return (rssi + 55) * 45 / (levels - 1);
    }
}

void statusbar_init(void)
{
    s_icons = tf_new(&font_icons_16X16, 0, 0);
    s_rect.x = 0;
    s_rect.y = 0;
    s_rect.width = DISPLAY_WIDTH;
    s_rect.height = STATUSBAR_HEIGHT;

    statusbar_update();
}

void statusbar_update(void)
{
    int wifi_bars = 0;
    if (wifi_enabled && wifi_connected) {
        wifi_ap_record_t record;
        ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&record));
        wifi_bars = rssi_to_bars(record.rssi, 5);
    }

    struct stateinfo_t state = {
        .sdcard_present = sdcard_present(),
        .wifi_enabled = wifi_enabled,
        .wifi_bars = wifi_bars,
    };

    if (memcmp(&last_state, &state, sizeof(struct stateinfo_t)) == 0) {
        return;
    }

    memcpy(&last_state, &state, sizeof(struct stateinfo_t));
    memset(fb->data, 0, DISPLAY_WIDTH * STATUSBAR_HEIGHT * fb->bytes_per_pixel);

    struct point_t p = {
        .x = fb->width - 16,
        .y = 0,
    };
    tf_draw_glyph(fb, s_icons, FONT_ICON_BATTERY5, p);
    p.x -= 16;
    if (sdcard_present()) {
        tf_draw_glyph(fb, s_icons, FONT_ICON_SDCARD, p);
        p.x -= 16;
    }
    if (state.wifi_enabled) {
        tf_draw_glyph(fb, s_icons, FONT_ICON_WIFI0 + state.wifi_bars, p);
        p.x -= 16;
    }
    tf_draw_glyph(fb, s_icons, FONT_ICON_SPEAKER3, p);
    p.x -= 16;

    display_update_rect(s_rect.x, s_rect.y, s_rect.width, s_rect.height);
}
