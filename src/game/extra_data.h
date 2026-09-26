#ifndef GAME_EXTRA_DATA_H
#define GAME_EXTRA_DATA_H

#include <stddef.h>

void extra_data_get_path(const char *save_filename, char *out_path, size_t out_size);
void extra_data_save(const char *save_filename);
void extra_data_load(const char *save_filename);
void extra_data_delete(const char *save_filename);

#endif // GAME_EXTRA_DATA_H
