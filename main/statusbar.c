#include "statusbar.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../components/hardware/display.h"
#include "../components/hardware/sdcard.h"

#include "OpenSans_Regular_11X12.h"
#include "icons_16X16.h"
#include "tf.h"

#include <string.h>


#define STATUSBAR_HEIGHT (16)

static bool task_running = false;
static struct {
    struct gbuf_t *fb;
    struct tf_t *tf_icons;
} task_data;

static void statusbar_task(void *arg)
{
    while (task_running) {
        xSemaphoreTake(task_data.fb->mutex, portMAX_DELAY);
        memset(task_data.fb->pixel_data, 0, task_data.fb->width *
                STATUSBAR_HEIGHT * task_data.fb->bytes_per_pixel);

        struct point_t p = {task_data.fb->width - 16, 0};
        tf_draw_glyph(task_data.fb, task_data.tf_icons, FONT_ICON_BATTERY5, p);
        p.x -= 16;
        if (sdcard_present()) {
            tf_draw_glyph(task_data.fb, task_data.tf_icons, FONT_ICON_SDCARD, p);
            p.x -= 16;
        }
        if (/*wifi_enabled()*/ 1) {
            tf_draw_glyph(task_data.fb, task_data.tf_icons, FONT_ICON_WIFI4, p);
            p.x -= 16;            
        }
        tf_draw_glyph(task_data.fb, task_data.tf_icons, FONT_ICON_SPEAKER3, p);
        p.x -= 16;

        struct rect_t r = {0, 0, task_data.fb->width, STATUSBAR_HEIGHT};
        display_update_rect(r);
        xSemaphoreGive(task_data.fb->mutex);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    tf_free(task_data.tf_icons);
}

void statusbar_init(struct gbuf_t *fb)
{
    task_running = true;

    task_data.fb = fb;
    task_data.tf_icons = tf_new(&font_icons_16X16, 0, 0);
  
    xTaskCreate(statusbar_task, "statusbar", 1536, NULL, 5, NULL);
}

void statusbar_deinit(void)
{
    task_running = false;
}
