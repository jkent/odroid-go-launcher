#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "sdcard.h"

#include "app.h"


static char *replace_ext(const char *app, const char *ext)
{
    size_t app_len = strlen(app);
    char *ini = malloc(app_len + strlen(ext) + 1);
    if (!ini) abort();
    strcpy(ini, app);
    char *p = strrchr(ini, '.');
    if (!p) p = ini + app_len;
    strcpy(p, ext);
    return ini;
}

static bool ini_get(const char *ini, const char *section, const char *key,
                    char *value, size_t len)
{
    char buf[256];
    char *p;
    bool in_section = false;

    if (section == NULL || strlen(section) == 0) in_section = true;
    
    FILE *f = fopen(ini, "r");
    if (!f) return false;

    while (true) {
        p = fgets(buf, sizeof(buf), f);
        if (p == NULL) return NULL;
        while (*p == ' ') p++;
        if (*p == ';' || *p == '\n' || (*p == '\r' && *p + 1 == '\n')) {
            continue; /* comment or blank line, ignore */
        } else if (*p == '[') {
            char *current_section = p + 1;
            p = strchr(current_section, ']');
            if (p) *p = '\0';
            in_section = (strcasecmp(section, current_section) == 0);
        } else if (in_section) {
            char *current_key = p;
            p = strchr(p, '=');
            if (!p) continue; /* no key/value delimeter */
            char *current_value = p + 1;
            if (p > current_key && *p == '=') *p-- = '\0';
            while (p > current_key && *p == ' ') *p-- = '\0';
            if (strcasecmp(key, current_key) != 0) continue; /* not our key */
            fclose(f);
            while (*current_value == ' ') current_value++;
            p = strchr(current_value, ';'); /* strip comments */
            if (p) *p = '\0';
            p = current_value + strlen(current_value) - 1;
            if (p > current_value && *p == '\n') *p-- = '\0';
            if (p > current_value && *p == '\r') *p-- = '\0';
            while (p > current_value && *p == ' ') *p-- = '\0';
            strncpy(value, current_value, len);
            value[len - 1] = '\0';
            return true;
        }
    }

    fclose(f);
    return false;
} 

static int semvercmp(const char *left, const char *right)
{
    int lmajor = 0, lminor = 0, lpatch = 0;
    int rmajor = 0, rminor = 0, rpatch = 0;
    char *lprerelease = NULL;
    char *rprerelease = NULL;

    char *s_left = strdup(left);
    char *p = strtok(s_left, ".");
    if (p) {
        lmajor = strtol(p, NULL, 10);
        p = strtok(NULL, ".");
        if (p) {
            lminor = strtol(p, NULL, 10);
            p = strtok(NULL, ".");
            if (p) {
                lpatch = strtol(p, NULL, 10);
                p = strtok(NULL, "-");
                if (p) {
                    lprerelease = p;
                }
                strtok(NULL, "+");
            }
        }
    }

    char *s_right = strdup(right);
    p = strtok(s_right, ".");
    if (p) {
        rmajor = strtol(p, NULL, 10);
        p = strtok(NULL, ".");
        if (p) {
            rminor = strtol(p, NULL, 10);
            p = strtok(NULL, ".");
            if (p) {
                rpatch = strtol(p, NULL, 10);
                p = strtok(NULL, "-");
                if (p) {
                    rprerelease = p;
                }
                strtok(NULL, "+");
            }
        }
    }

    if (lmajor < rmajor) {
        free(s_left);
        free(s_right);
        return -1;
    } else if (lmajor > rmajor) {
        free(s_left);
        free(s_right);
        return 1;
    }

    if (lminor < rminor) {
        free(s_left);
        free(s_right);
        return -1;
    } else if (lminor > rminor) {
        free(s_left);
        free(s_right);
        return 1;
    }

    if (lpatch < rpatch) {
        free(s_left);
        free(s_right);
        return -1;
    } else if (lpatch > rpatch) {
        free(s_left);
        free(s_right);
        return 1;
    }

    if (lprerelease && !rprerelease) {
        free(s_left);
        free(s_right);
        return -1;
    } else if (!lprerelease && rprerelease) {
        free(s_left);
        free(s_right);
        return 1;
    } else if (lprerelease && rprerelease) {
        int res = strcmp(lprerelease, rprerelease);
        free(s_left);
        free(s_right);
        return res;
    }

    free(s_left);
    free(s_right);
    return 0;
}

void app_run(const char *app)
{
    char *ini = replace_ext(app, ".ini");
    char sd_version[32];
    bool do_flash = true;
    int i;

    strcpy(sd_version, "0.0.0");
    ini_get(ini, NULL, "version", sd_version, sizeof(sd_version));
    free(ini);

    nvs_handle nvs;
    nvs_open("nvs", NVS_READWRITE, &nvs);

    for (i = 1; i < 7; i++) {
        char key[16];
        char nvs_app[256];
        size_t len = sizeof(nvs_app);
        memset(nvs_app, 0, sizeof(nvs_app));
        sprintf(key, "app%d_file", i);
        nvs_get_str(nvs, key, nvs_app, &len);
        if (strcmp(app, nvs_app) == 0) {
            printf("found in app%d\n", i);
            break;
        }
    }

    char mru[7];
    size_t len = sizeof(mru);
    strcpy(mru, "123456");
    nvs_get_str(nvs, "app_mru", mru, &len);

    printf("mru: %s\n", mru);

    if (i < 7) {
        char key[16];
        char nvs_version[32];
        size_t len = sizeof(nvs_version);
        memset(nvs_version, 0, sizeof(nvs_version));
        sprintf(key, "app%d_version", i);
        nvs_get_str(nvs, key, nvs_version, &len);
        
        printf("sd: %s, nvs: %s\n", sd_version, nvs_version);
        if (!sdcard_present()) {
            do_flash = false;
        } else if (strchr(sd_version, '.') && strchr(nvs_version, '.')) {
            if (semvercmp(sd_version, nvs_version) <= 0) {
                do_flash = false;
            }
        } else if (!strchr(sd_version, '.') && !strchr(nvs_version, '.')) {
            if (strcmp(sd_version, nvs_version) <= 0) {
                do_flash = false;
            }
        }
    } else {
        i = mru[0] - '0';
    }

    const esp_partition_t *part = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_MIN + i, NULL);
    esp_err_t err;

    if (do_flash) {
        printf("writing to app%d\n", i);

        FILE *f = fopen(app, "rb");
        if (!f) abort();

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);

        esp_ota_handle_t ota_handle;
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

        char key[16];
        sprintf(key, "app%d_file", i);
        nvs_set_str(nvs, key, app);
        sprintf(key, "app%d_version", i);
        nvs_set_str(nvs, key, sd_version);
    }

    char *p = strchr(mru, '0' + i);
    memmove(p, p + 1, strlen(mru) - (p - mru));
    mru[strlen(mru)] = '0' + i;
    nvs_set_str(nvs, "app_mru", mru);

    nvs_commit(nvs);
    nvs_close(nvs);

    esp_ota_set_boot_partition(part);

    esp_restart();
}
