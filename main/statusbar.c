#include "statusbar.h"

#include "display.h"
#include "sdcard.h"

#include "OpenSans_Regular_11X12.h"
#include "icons_16X16.h"
#include "tf.h"

#include <string.h>


#define STATUSBAR_HEIGHT (16)

struct gbuf_t *s_fb;
struct tf_t *s_icons;
struct rect_t s_rect;

void statusbar_init(struct gbuf_t *fb)
{
    s_fb = fb;
    s_icons = tf_new(&font_icons_16X16, 0, 0);
    s_rect.x = 0;
    s_rect.y = 0;
    s_rect.width = s_fb->width;
    s_rect.height = STATUSBAR_HEIGHT;
}

void statusbar_update(void)
{
    memset(s_fb->pixel_data, 0, s_fb->width *
            STATUSBAR_HEIGHT * s_fb->bytes_per_pixel);

    struct point_t p = {s_fb->width - 16, 0};
    tf_draw_glyph(s_fb, s_icons, FONT_ICON_BATTERY5, p);
    p.x -= 16;
    if (sdcard_present()) {
        tf_draw_glyph(s_fb, s_icons, FONT_ICON_SDCARD, p);
        p.x -= 16;
    }
    if (/*wifi_enabled()*/ 1) {
        tf_draw_glyph(s_fb, s_icons, FONT_ICON_WIFI4, p);
        p.x -= 16;
    }
    tf_draw_glyph(s_fb, s_icons, FONT_ICON_SPEAKER3, p);
    p.x -= 16;

    display_update_rect(s_rect);
}
