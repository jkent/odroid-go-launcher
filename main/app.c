#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "app.h"
#include "frozen.h"
#include "sdcard.h"


#define NUM_OTA_PARTITIONS (6)
#define APP_DIR "/sdcard/apps"
#define APPDATA_DIR "/spiffs/appdata"
#define APP_HEADER_MAGIC (0x21505041)
#define APP_HEADER_VERSION (1)
#define APP_ICON_LEN (48 * 48 * 2)

struct app_header_t {
    uint32_t magic;
    uint32_t version;
    uint32_t header_len;
    uint32_t json_len;
    uint32_t icon_len;
    uint32_t binary_len;
};


static bool endswith(const char *str, const char *suffix)
{
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix > lenstr)
        return false;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

static void remove_end(char *str, size_t count)
{
    size_t lenstr = strlen(str);
    *(str + lenstr - count) = '\0';
}

static int cmp_app_info(const void *a, const void *b)
{
    struct app_info_t *aa = (struct app_info_t *)a;
    struct app_info_t *bb = (struct app_info_t *)b;
    return strcasecmp(aa->name, bb->name);
}

static char *app_json_fread(char *filename)
{
    char *json;
    FILE *f = fopen(filename, "rb");
    struct app_header_t header;
    if (fread(&header, sizeof(struct app_header_t), 1, f) != 1 ||
            header.magic != APP_HEADER_MAGIC ||
            header.version != APP_HEADER_VERSION) {
        fclose(f);
        return NULL;
    }

    json = malloc(header.json_len + 1);
    if (!json) return NULL;
    if (fread(json, header.json_len, 1, f) != 1) {
        fclose(f);
        free(json);
        return NULL;
    }
    json[header.json_len] = '\0';
    fclose(f);
    return json;
}

/* when comparison fails, right is considered newer */
static int versioncmp(const char *left, const char *right)
{
    int lmajor = 0, lminor = 0, lpatch = 0;
    int rmajor = 0, rminor = 0, rpatch = 0;
    char *lpre = NULL, *rpre = NULL;

    bool l_semver = !!strchr(left, '.');
    bool r_semver = !!strchr(right, '.');

    if (!l_semver && !r_semver) {
        return strcmp(left, right);
    } else if (l_semver && r_semver) {
        char *ls = strdup(left);
        char *rs = strdup(right);
        if (ls == NULL || rs == NULL) abort();

        char *rest = ls;
        char *p = strtok_r(rest, ".", &rest);
        if (p) {
            lmajor = strtol(p, NULL, 10);
            p = strtok_r(rest, ".", &rest);
            if (p) {
                lminor = strtol(p, NULL, 10);
                p = strtok_r(rest, "-+", &rest);
                if (p) {
                    lpatch = strtol(p, NULL, 10);
                    lpre = strtok_r(rest, "+", &rest);
                }
            }
        }

        rest = rs;
        p = strtok_r(rest, ".", &rest);
        if (p) {
            rmajor = strtol(p, NULL, 10);
            p = strtok_r(rest, ".", &rest);
            if (p) {
                rminor = strtol(p, NULL, 10);
                p = strtok_r(rest, "-+", &rest);
                if (p) {
                    rpatch = strtol(p, NULL, 10);
                    rpre = strtok_r(rest, "+", &rest);
                }
            }
        }

        int cmp = (lmajor > rmajor) - (lmajor < rmajor);
        if (cmp) {
            goto semver_done;
        }

        cmp = (lminor > rminor) - (lminor < rminor);
        if (cmp) {
            goto semver_done;
        }

        cmp = (lpatch > rpatch) - (lpatch < rpatch);
        if (cmp) {
            goto semver_done;
        }

        if (lpre && !rpre) {
            cmp = -1;
            goto semver_done;
        } else if (!lpre && rpre) {
            cmp = 1;
            goto semver_done;
        } else if (lpre && rpre) {
            cmp = strcmp(lpre, rpre);
            goto semver_done;
        }

semver_done:
        free(ls);
        free(rs);
        return cmp;
    } else {
        return -1;
    }
}

static void update_mru(nvs_handle nvs, int slot)
{
    char mru[NUM_OTA_PARTITIONS + 1];
    for (int i = 0; i < NUM_OTA_PARTITIONS; i++) {
        mru[i] = '1' + i;
    }
    size_t len = sizeof(mru);
    nvs_get_str(nvs, "mru", mru, &len);
    char *p = strchr(mru, '0' + slot);
    len = strlen(mru);
    if (p) {
        memmove(p, p + 1, len - (p - mru));
        mru[len - 1] = '0' + slot;
    } else {
        mru[len] = '0' + slot;
    }
    ESP_ERROR_CHECK(nvs_set_str(nvs, "mru", mru));
}

int app_get_slot(const char *name, bool *installed)
{
    int slot;
    if (installed) {
        *installed = false;
    }

    nvs_handle nvs;
    ESP_ERROR_CHECK(nvs_open("nvs", NVS_READWRITE, &nvs));

    for (slot = 1; slot <= NUM_OTA_PARTITIONS; slot++) {
        char key[5], value[256];
        size_t len = sizeof(value);
        snprintf(key, sizeof(key), "app%d", slot);
        if (nvs_get_str(nvs, key, value, &len) != ESP_OK) {
            continue;
        }
        if (strcmp(name, value) == 0) {
            if (installed) {
                *installed = true;
            }
            goto end;
        }
    }

    char mru[NUM_OTA_PARTITIONS + 1];
    for (int i = 0; i < NUM_OTA_PARTITIONS; i++) {
        mru[i] = '1' + i;
    }
    mru[NUM_OTA_PARTITIONS] = '\0';
    size_t len = sizeof(mru);
    nvs_get_str(nvs, "mru", mru, &len);

    slot = mru[0] - '0';

end:
    nvs_close(nvs);
    return slot;
}

void app_info(const char *name, struct app_info_t *info)
{
    char filename[PATH_MAX];

    memset(info, 0, sizeof(struct app_info_t));

    strncpy(info->name, name, sizeof(info->name));
    info->slot_num = app_get_slot(name, &info->installed);

    struct stat st;
    snprintf(filename, sizeof(filename), "%s/%s.app", APP_DIR, name);
    info->available = stat(filename, &st) == 0;

    if (info->installed && info->available) {
        /* first get sdcard version */
        char *json = app_json_fread(filename);
        if (json == NULL) {
            info->available = false;
            return;
        }
        char *sdcard_version;
        json_scanf(json, strlen(json), "{version: %Q}", &sdcard_version);
        free(json);
        if (sdcard_version == NULL) {
            info->available = false;
            return;
        }

        /* then get the installed version */
        snprintf(filename, sizeof(filename), "%s/app%d.json", APPDATA_DIR, info->slot_num);
        json = json_fread(filename);
        if (json == NULL) {
            free(sdcard_version);
            info->installed = false;
            return;
        }
        char *installed_version;
        json_scanf(json, strlen(json), "{version: %Q}", &installed_version);
        free(json);
        if (installed_version == NULL) {
            free(sdcard_version);
            info->installed = false;
            return;
        }

        /* and compare them */
        if (versioncmp(installed_version, sdcard_version) < 0) {
            free(installed_version);
            free(sdcard_version);
            info->upgradable = true;
            return;
        }
        free(installed_version);
        free(sdcard_version);
    }

    return;
}

struct app_info_t *app_enumerate(size_t *count)
{
    struct app_info_t *info = NULL;
    *count = 0;

    /* first find all apps in flash */
    nvs_handle nvs;
    ESP_ERROR_CHECK(nvs_open("nvs", NVS_READWRITE, &nvs));

    for (int i = 0; i < NUM_OTA_PARTITIONS; i++) {
        char key[5], value[256];
        size_t len = sizeof(value);

        snprintf(key, sizeof(key), "app%d", i + 1);
        if (nvs_get_str(nvs, key, value, &len) != ESP_OK) {
            continue;
        }

        info = realloc(info, sizeof(struct app_info_t) * (*count + 1));
        app_info(value, &info[*count]);
        *count += 1;
    }

    nvs_close(nvs);

    /* next list all files in the apps directory */
    DIR *dir;
    struct dirent entry;
    struct dirent *result;

    if ((dir = opendir(APP_DIR)) == NULL) {
        goto sort;
    }

    while (readdir_r(dir, &entry, &result) == 0 && result != NULL) {
        if (entry.d_type != DT_REG || !endswith(entry.d_name, ".app")) {
            continue;
        }
        remove_end(entry.d_name, 4);

        int i;
        for (i = 0; i < *count; i++) {
            if (strcmp(info[i].name, entry.d_name) == 0) {
                break;
            }
        }

        if (i < *count) {
            continue;
        }

        info = realloc(info, sizeof(struct app_info_t) * (*count + 1));
        app_info(entry.d_name, &info[*count]);
        *count += 1;
    }

    closedir(dir);

sort:
    qsort(info, *count, sizeof(struct app_info_t), cmp_app_info);
    return info;
}

bool app_install(const char *name, int slot)
{
    nvs_handle nvs = 0;
    char key[5];
    const esp_partition_t *part;
    char filename[PATH_MAX];
    FILE *app = NULL, *out = NULL;
    char *buf = NULL;
    struct app_header_t header;
    esp_ota_handle_t ota_handle;

    assert(slot > 0 && slot <= NUM_OTA_PARTITIONS);

    part = esp_partition_find_first(ESP_PARTITION_TYPE_APP,
        ESP_PARTITION_SUBTYPE_APP_OTA_MIN + slot, NULL);
    if (!part) {
        goto error;
    }

    snprintf(filename, sizeof(filename), "%s/%s.app", APP_DIR, name);
    if (!(app = fopen(filename, "rb"))) {
        goto error;
    }

    if (fread(&header, sizeof(struct app_header_t), 1, app) != 1 ||
            header.magic != APP_HEADER_MAGIC ||
            header.version != APP_HEADER_VERSION) {
        goto error;
    }

    /* mark slot as free */
    ESP_ERROR_CHECK(nvs_open("nvs", NVS_READWRITE, &nvs));
    snprintf(key, sizeof(key), "app%d", slot);
    nvs_erase_key(nvs, key);
    ESP_ERROR_CHECK(nvs_commit(nvs));

    /* copy json to appdata */
    snprintf(filename, sizeof(filename), "%s/app%d.json", APPDATA_DIR, slot);
    if (!(buf = malloc(header.json_len))) {
        goto error;
    }

    if (fread(buf, header.json_len, 1, app) != 1) {
        goto error;
    }

    if (!(out = fopen(filename, "w"))) {
        goto error;
    }
    if (fwrite(buf, header.json_len, 1, out) != 1) {
        goto error;
    }
    fclose(out);
    out = NULL;

    /* copy icons to appdata */
    snprintf(filename, sizeof(filename), "%s/app%d.icons", APPDATA_DIR, slot);

    if (header.icon_len == 0) {
        unlink(filename);
    } else {
        if (header.icon_len % APP_ICON_LEN != 0) {
            goto error;
        }

        size_t count = header.icon_len / APP_ICON_LEN;

        if (!(buf = realloc(buf, APP_ICON_LEN))) {
            goto error;
        }

        if (!(out = fopen(filename, "wb"))) {
            goto error;
        }

        for (int i = 0; i < count; i++) {
            if (fread(buf, APP_ICON_LEN, 1, app) != 1) {
                goto error;
            }
            if (fwrite(buf, APP_ICON_LEN, 1, out) != 1) {
                goto error;
            }
        }

        fclose(out);
        out = NULL;
    }

    /* copy binary to flash */
    if (!(buf = realloc(buf, 4096))) {
        goto error;
    }

    ESP_ERROR_CHECK(esp_ota_begin(part, header.binary_len, &ota_handle));
    while (true) {
        size_t len = fread(buf, 1, 4096, app);
        if (len == 0) {
            break;
        }
        ESP_ERROR_CHECK(esp_ota_write(ota_handle, buf, len));
    }
    ESP_ERROR_CHECK(esp_ota_end(ota_handle));

    fclose(app);

    /* mark slot with app name */
    snprintf(key, sizeof(key), "app%d", slot);
    ESP_ERROR_CHECK(nvs_set_str(nvs, key, name));
    update_mru(nvs, slot);
    ESP_ERROR_CHECK(nvs_commit(nvs));
    nvs_close(nvs);
    return false;

error:
    if (nvs) {
        nvs_close(nvs);
    }
    if (app) {
        fclose(app);
    }
    if (out) {
        fclose(out);
    }
    if (buf) {
        free(buf);
    }
    return true;
}

void app_uninstall(const char *name)
{
    struct app_info_t info;

    app_info(name, &info);
    if (!info.installed) {
        return;
    }

    DIR *dir;
    struct dirent entry;
    struct dirent *result;
    char filename[PATH_MAX];
    char prefix[64];

    snprintf(prefix, sizeof(prefix), "%s/app%d.", APPDATA_DIR, info.slot_num);
    if ((dir = opendir(APPDATA_DIR)) != NULL) {
        while (readdir_r(dir, &entry, &result) == 0 && result != NULL) {
            if (strncmp(entry.d_name, filename, strlen(filename)) == 0) {
                snprintf(filename, sizeof(filename), "%s/%s", APPDATA_DIR, entry.d_name);
                unlink(filename);
            }
        }
    }
    closedir(dir);

    nvs_handle nvs;
    ESP_ERROR_CHECK(nvs_open("nvs", NVS_READWRITE, &nvs));
    char key[5];
    snprintf(key, sizeof(key), "app%d", info.slot_num);
    nvs_erase_key(nvs, key);
    ESP_ERROR_CHECK(nvs_commit(nvs));
    nvs_close(nvs);
}

void app_run(const char *name, bool upgrade)
{
    struct app_info_t info;

    app_info(name, &info);
    if (!info.installed || (upgrade && info.available && info.upgradable)) {
        if (app_install(name, info.slot_num)) {
            return;
        }
    }

    nvs_handle nvs;
    ESP_ERROR_CHECK(nvs_open("nvs", NVS_READWRITE, &nvs));
    update_mru(nvs, info.slot_num);
    ESP_ERROR_CHECK(nvs_commit(nvs));
    nvs_close(nvs);

    const esp_partition_t *part = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_MIN + info.slot_num, NULL);

    esp_ota_set_boot_partition(part);
    esp_restart();
}
