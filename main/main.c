#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"

#include "../components/hardware/display.h"
#include "../components/hardware/backlight.h"
#include "../components/hardware/keypad.h"
#include "../components/hardware/sdcard.h"

#include "app.h"
#include "graphics.h"
#include "menu.h"
#include "tf.h"
#include "OpenSans_Regular_11X12.h"
#include "statusbar.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void dismiss_fn(struct menu_t *menu, int offset, void *arg)
{
    menu_dismiss(menu);
}

void app_main(void)
{
    struct gbuf_t *fb = display_init();
    backlight_init();

    struct tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, 240, TF_ALIGN_CENTER | TF_WORDWRAP);
    const char *s;
    struct tf_metrics_t m;
    struct point_t p;

    s = "Initializing...";
    m = tf_get_str_metrics(tf, s);
    p.x = DISPLAY_WIDTH/2 - tf->width/2;
    p.y = DISPLAY_HEIGHT/2 - m.height/2;
    tf_draw_str(fb, tf, s, p);
    display_update();

    keypad_init();
    nvs_flash_init();
    sdcard_init("/sdcard");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    assert(ret == ESP_OK);

    s = "Press Menu button for menu or A to boot hello-world.bin app.";
    m = tf_get_str_metrics(tf, s);
    p.x = DISPLAY_WIDTH/2 - tf->width/2;
    p.y = DISPLAY_HEIGHT/2 - m.height/2;
    memset(fb->pixel_data + fb->width * 16 * fb->bytes_per_pixel, 0, fb->width * (fb->height - 32) * fb->bytes_per_pixel);
    tf_draw_str(fb, tf, s, p);
    display_update();

    while (true) {
        uint16_t keys = 0, changes = 0, pressed = 0;
        do {
            vTaskDelay(10 / portTICK_PERIOD_MS);
            keys = keypad_debounce(keypad_sample(), &changes);
            pressed = keys & changes;
        } while (!(pressed & KEYPAD_MENU) && !(pressed & KEYPAD_A));

        if (pressed & KEYPAD_A) {
            break;
        }

        struct menu_t *menu = menu_new(fb, 240, 180);
        const char *wifi_state_list[] = {
            "Wifi Disabled",
            "Wifi Enabled",
            NULL,
        };
        menu_append_list(menu, wifi_state_list, 0, menu_list_cycle, NULL);

        menu_append_text(menu, "The quick brown fox jumps over the lazy dog.", NULL, NULL);
        menu_append_text(menu, "Hello World!", NULL, NULL);
        menu_append_text(menu, "3nd line", NULL, NULL);
        menu_append_text(menu, "4th line", NULL, NULL);
        menu_append_text(menu, "5th line", NULL, NULL);
        menu_append_text(menu, "Dismiss", dismiss_fn, NULL);
        menu_showmodal(menu);
        menu_free(menu);
    }

    s = "Loading...";
    m = tf_get_str_metrics(tf, s);
    p.x = DISPLAY_WIDTH/2 - tf->width/2;
    p.y = DISPLAY_HEIGHT/2 - m.height/2;
    memset(fb->pixel_data + fb->width * 16 * fb->bytes_per_pixel , 0, fb->width * (fb->height - 32) * fb->bytes_per_pixel);
    tf_draw_str(fb, tf, s, p);
    display_update();

    app_run("/sdcard/apps/hello-world.bin");

    s = "App not found.";
    m = tf_get_str_metrics(tf, s);
    p.x = DISPLAY_WIDTH/2 - tf->width/2;
    p.y = DISPLAY_HEIGHT/2 - m.height/2;
    memset(fb->pixel_data + fb->width * 16 * fb->bytes_per_pixel , 0, fb->width * (fb->height - 32) * fb->bytes_per_pixel);
    tf_draw_str(fb, tf, s, p);
    display_update();
}
