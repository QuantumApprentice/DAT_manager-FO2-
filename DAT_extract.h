#pragma once
#include <stdint.h>
#include "io_Platform.h"
// #include "Load_Settings.h"
// #include "town_map_tiles.h"

struct DAT_file {
    char     file_name[MAX_PATH];
    int32_t  file_size;
    int32_t  data_size;
    uint8_t* data;
};

//generic buffer struct?
struct DAT_buffer {
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
    int32_t count;
    DIR_entry* list;
};


//TODO: clean this up
DAT_file load_dat_file(const char* file_name, char* game_path);
bool extract_from_DAT(const char* file_name, char* game_path, DAT_file* dat);//, DAT_Buffer* buff);

// bool tt_file_DAT_extract(user_info* usr_nfo, STATE_export* state);
// bool extract_from_DAT(char* file_name, char* dat_name, user_info* usr_nfo, DAT_file* dat_file, Buffer* buff);