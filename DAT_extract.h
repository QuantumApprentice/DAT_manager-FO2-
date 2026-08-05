#pragma once
#include <stdint.h>
#include "io_Platform.h"

//generic buffer struct?
struct DAT_buffer {
    bool  flipped = false;
    int32_t  size = 0;
    uint8_t* data = nullptr;
};

struct DIR_entry {
    char*   path_ptr;
    int32_t path_size;
    uint8_t packed;
    int32_t unpack_size;
    int32_t packed_size;
    int32_t offset;
    uint8_t* packed_ptr;

    DAT_buffer unpacked_file;
};
struct DIR_entries {
    int32_t count = 0;
    DIR_entry* list = nullptr;
};

struct DAT_file {
    char     file_name[MAX_PATH];
    int32_t  file_size = 0;
    int32_t  data_size = 0;
    uint8_t* data = nullptr;
    DIR_entries dir_entries;
};

struct LST_array {
    int32_t count = 0;
    char** line = nullptr;
};

//TODO: clean this up
DAT_file load_dat_file(const char* file_name, char* game_path);
DIR_entry* extract_from_DAT(const char* file_name, DAT_file* dat);
LST_array lst_convert(char* lst_file, int size);

// bool tt_file_DAT_extract(user_info* usr_nfo, STATE_export* state);
// bool extract_from_DAT(char* file_name, char* dat_name, user_info* usr_nfo, DAT_file* dat_file, Buffer* buff);