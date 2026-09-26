#include "extra_data.h"

#include "city/constants.h"
#include "city/festival.h"
#include "core/file.h"
#include "platform/file_manager.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR_SINGLE(path) _mkdir(path)
#else
#define MKDIR_SINGLE(path) mkdir(path, 0755)
#endif

static const char EXTRA_MAGIC[8] = "AUGEXT01";

static void ensure_directory_exists(const char *dir)
{
    if (!dir || !*dir) {
        return;
    }
    char tmp[FILE_NAME_MAX];
    snprintf(tmp, sizeof(tmp), "%s", dir);
    size_t len = strlen(tmp);
    if (len > 0 && (tmp[len - 1] == '/' || tmp[len - 1] == '\\')) {
        tmp[len - 1] = '\0';
    }
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char ch = *p;
            *p = '\0';
            MKDIR_SINGLE(tmp);
            *p = ch;
        }
    }
    MKDIR_SINGLE(tmp);
}

void extra_data_get_path(const char *save_filename, char *out_path, size_t out_size)
{
    if (!save_filename || !out_path || out_size == 0) {
        return;
    }

    const char *last_sep = strrchr(save_filename, '/');
    const char *last_sep_win = strrchr(save_filename, '\\');
    if (last_sep_win && (!last_sep || last_sep_win > last_sep)) {
        last_sep = last_sep_win;
    }

    char dir_part[FILE_NAME_MAX] = "";
    char name_part[FILE_NAME_MAX] = "";

    if (last_sep) {
        size_t dir_len = (size_t)(last_sep - save_filename);
        if (dir_len >= sizeof(dir_part)) {
            dir_len = sizeof(dir_part) - 1;
        }
        strncpy(dir_part, save_filename, dir_len);
        dir_part[dir_len] = '\0';
        snprintf(name_part, sizeof(name_part), "%s", last_sep + 1);
    } else {
        snprintf(name_part, sizeof(name_part), "%s", save_filename);
    }

    // Strip extension from name_part
    char *dot = strrchr(name_part, '.');
    if (dot) {
        *dot = '\0';
    }

    if (dir_part[0] != '\0') {
        snprintf(out_path, out_size, "%s/extra/%s.extra", dir_part, name_part);
    } else {
        snprintf(out_path, out_size, "extra/%s.extra", name_part);
    }
}

void extra_data_save(const char *save_filename)
{
    if (!save_filename) {
        return;
    }

    char extra_path[FILE_NAME_MAX];
    extra_data_get_path(save_filename, extra_path, sizeof(extra_path));

    // Ensure extra directory exists
    const char *last_sep = strrchr(extra_path, '/');
    const char *last_sep_win = strrchr(extra_path, '\\');
    if (last_sep_win && (!last_sep || last_sep_win > last_sep)) {
        last_sep = last_sep_win;
    }

    if (last_sep) {
        char dir_part[FILE_NAME_MAX];
        size_t len = (size_t)(last_sep - extra_path);
        if (len < sizeof(dir_part)) {
            strncpy(dir_part, extra_path, len);
            dir_part[len] = '\0';
            ensure_directory_exists(dir_part);
        }
    }

    FILE *fp = fopen(extra_path, "wb");
    if (!fp) {
        return;
    }

    fwrite(EXTRA_MAGIC, 1, sizeof(EXTRA_MAGIC), fp);

    int32_t enabled = (int32_t)city_festival_auto_enabled();
    int32_t size = (int32_t)city_festival_auto_size();
    int32_t god = (int32_t)city_festival_auto_god();

    fwrite(&enabled, sizeof(int32_t), 1, fp);
    fwrite(&size, sizeof(int32_t), 1, fp);
    fwrite(&god, sizeof(int32_t), 1, fp);

    fclose(fp);
}

void extra_data_load(const char *save_filename)
{
    if (!save_filename) {
        city_festival_set_auto_enabled(0);
        city_festival_set_auto_size(FESTIVAL_SMALL);
        city_festival_set_auto_god(-1);
        return;
    }

    char extra_path[FILE_NAME_MAX];
    extra_data_get_path(save_filename, extra_path, sizeof(extra_path));

    FILE *fp = fopen(extra_path, "rb");
    if (!fp) {
        city_festival_set_auto_enabled(0);
        city_festival_set_auto_size(FESTIVAL_SMALL);
        city_festival_set_auto_god(-1);
        return;
    }

    char magic[8];
    if (fread(magic, 1, sizeof(magic), fp) != sizeof(magic) || memcmp(magic, EXTRA_MAGIC, sizeof(EXTRA_MAGIC)) != 0) {
        fclose(fp);
        city_festival_set_auto_enabled(0);
        city_festival_set_auto_size(FESTIVAL_SMALL);
        city_festival_set_auto_god(-1);
        return;
    }

    int32_t enabled = 0;
    int32_t size = FESTIVAL_SMALL;
    int32_t god = -1;

    if (fread(&enabled, sizeof(int32_t), 1, fp) == 1 &&
        fread(&size, sizeof(int32_t), 1, fp) == 1 &&
        fread(&god, sizeof(int32_t), 1, fp) == 1) {
        city_festival_set_auto_enabled((int)enabled);
        city_festival_set_auto_size((int)size);
        city_festival_set_auto_god((int)god);
    } else {
        city_festival_set_auto_enabled(0);
        city_festival_set_auto_size(FESTIVAL_SMALL);
        city_festival_set_auto_god(-1);
    }

    fclose(fp);
}

void extra_data_delete(const char *save_filename)
{
    if (!save_filename) {
        return;
    }

    char extra_path[FILE_NAME_MAX];
    extra_data_get_path(save_filename, extra_path, sizeof(extra_path));
    file_remove(extra_path);
}
