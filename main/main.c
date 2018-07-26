#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include "../components/hardware/display.h"
#include "../components/hardware/backlight.h"
#include "../components/hardware/keypad.h"
#include "../components/hardware/sdcard.h"

#include "gbuf.h"
#include "tf.h"
#include "OpenSans_Regular_11X12.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static volatile uint16_t keypad_debounced = 0;

static void keypad_task(void *arg)
{
    uint16_t sample;
    
    while (true) {
        sample = keypad_sample();
        keypad_debounced = keypad_debounce(sample, NULL);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    struct gbuf *fb = display_init();
    memset(fb->pixel_data, 0, fb->width * fb->height * fb->bytes_per_pixel);

    esp_err_t err = sdcard_init("/sd");
    if (err) abort();

    backlight_init();
    keypad_init();

    struct tf *tf = tf_new();
    tf->font = &font_OpenSans_Regular_11X12;

    char s[64];
    strcpy(s, "Press A to boot hello-world.bin app.");
    struct tf_metrics m = tf_get_str_metrics(tf, s);
    tf_draw_str(fb, tf, s, DISPLAY_WIDTH/2 - m.width/2, DISPLAY_HEIGHT/2 - m.height/2);
    display_draw();

    xTaskCreatePinnedToCore(&keypad_task, "keypad_task", 1024, NULL, 5, NULL, APP_CPU_NUM);

    while (!(keypad_debounced & KEYPAD_A)) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    memset(fb->pixel_data, 0, fb->width * fb->height * fb->bytes_per_pixel);
    strcpy(s, "Loading...");
    m = tf_get_str_metrics(tf, s);
    tf_draw_str(fb, tf, s, DISPLAY_WIDTH/2 - m.width/2, DISPLAY_HEIGHT/2 - m.height/2);
    display_draw();

    FILE *f = fopen("/sd/apps/hello-world.bin", "rb");
    if (!f) {
        printf("errno: %d\n", errno);
        abort();
    }

    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);

    esp_ota_handle_t ota_handle;

    const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    err = esp_ota_begin(part, size, &ota_handle);
    if (err) abort();

    char *buf = malloc(4096);
    if (!buf) abort();

    while (true) {
        size_t len = fread(buf, 1, 4096, f);
        if (len == 0) break;

        err = esp_ota_write(ota_handle, buf, len);
        if (err) abort();
    }

    err = esp_ota_end(ota_handle);
    if (err) abort();

    esp_ota_set_boot_partition(part);

    esp_restart();
}
