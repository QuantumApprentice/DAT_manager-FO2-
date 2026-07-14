//https://fodev.net/files/fo2/dat.html
//https://fallout.fandom.com/wiki/DAT_file
//Thank you to @BakerStaunch!
//This is mostly a rewrite of his source code
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include "DAT_extract.h"

int32_t read_i32(DAT_file* dat, int* offset)
{
    if ((*offset + sizeof(int32_t)) > dat->file_size) {
        return 0;
    }

    uint8_t* data = &dat->data[*offset];

    int32_t out;
    memcpy(&out, data, 4);
    // uint8_t bytes[4];
    // memcpy(bytes, data, 4);
    // out = (bytes[3] <<  0)
    //     | (bytes[2] <<  8)
    //     | (bytes[1] << 16)
    //     | (bytes[0] << 24);

    *offset += sizeof(int32_t);

    return out;
}

DAT_file load_dat_file(const char* file_name, char* game_path)
{
    DAT_file dat = {0};
    if (!file_name || !game_path) {
        return dat;
    }

    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, "%s/%s.dat", game_path, file_name);
    int32_t dat_size = io_file_size(file_path);
    //game probably only supports up to 2gb? wonder if sfall mods this
    if (dat_size < 1 || dat_size > INT32_MAX) {
        return dat;
    }

    FILE* dat_file = fopen(file_path,"rb");
    if (!dat_file) {
        return dat;
    }

    uint8_t* dat_data = (uint8_t*)malloc(dat_size);
    int sz = fread(dat_data,1,dat_size,dat_file);
    if (sz != dat_size) {
        fclose(dat_file);
        return dat;
    }
    fclose(dat_file);

    dat.file_size = dat_size;
    dat.data      = dat_data;
    snprintf(dat.file_name, MAX_PATH, "%s", file_name);

    return dat;
}

void append(char* dst, const char* src, int max_len)
{
    int i = 0;
    char* ptr = NULL;
    while (dst[i] != '\0' && i < max_len) {
        i++;
        if (dst[i] == '\0') {
            i++;
        }
    }
    if (i >= (max_len - strlen(src))) {
        return;
    }
    if (i != 0) {
        ptr = &dst[i];
    } else {
        ptr = dst;
    }
    strncpy(ptr, src, strlen(src));
}

int get_data_size(DAT_file* dat)
{
    int offset    = dat->file_size - 4;
    int data_size = read_i32(dat, &offset);

    return data_size;
}

int get_dir_tree_size(DAT_file* dat)
{
    int offset = dat->file_size - 8;
    int dir_tree_size = read_i32(dat, &offset);

    return dir_tree_size;
}

uint8_t* get_dir_tree_ptr(DAT_file* dat, int dir_tree_size)
{
    int offset = dat->file_size - dir_tree_size - 8;
    uint8_t* dir_tree_ptr = &dat->data[offset];

    return dir_tree_ptr;
}

int32_t get_dir_tree_i32(uint8_t* dir_tree, int* offset)
{
    // int32_t out = *(int32_t*)&dir_tree[*offset];
    int32_t out;
    memcpy(&out, &dir_tree[*offset], 4);
    *offset += sizeof(out);

    return out;
}

char* get_dir_tree_path(uint8_t* dir_tree, int* offset, int path_len)
{
    char* path = (char*)&dir_tree[*offset];
    *offset += path_len;

    return path;
}

uint8_t get_dir_tree_pack(uint8_t* dir_tree, int* offset)
{
    uint8_t packed = dir_tree[(*offset)++];

    return packed;
}

DIR_entries get_dir_entry_list(DAT_file* dat)
{
    DIR_entries handle = {0};
    //TODO: verify this is correct - it doesn't seem like it should match file size
    int size = dat->file_size;

    int dir_tree_size = get_dir_tree_size(dat);
    if (dir_tree_size < 0 || dir_tree_size > size - 8) {
        //TODO: log to file
        // set_popup_warning();
        printf("Error: get_dir_entry_list() %s directory entry size (%d) out of bounds from file size (%d): L%d",
                dat->file_name, dir_tree_size, dat->file_size, __LINE__);
        handle.list = nullptr;
        return handle;
    }

    uint8_t* dir_tree = get_dir_tree_ptr(dat, dir_tree_size);
    int dir_tree_offset = 0;
    int dir_tree_cnt = get_dir_tree_i32(dir_tree, &dir_tree_offset);
    if (dir_tree_cnt < 0) {
        //TODO: log to file
        // set_popup_warning();
        printf("Error: get_dir_entry_list() %s directory count (%d) out of bounds from file size (%d): L%d",
                dat->file_name, dir_tree_cnt, dat->file_size, __LINE__);
        handle.list = nullptr;
        return handle;
    }

    handle.count = dir_tree_cnt;

    DIR_entry* entries = (DIR_entry*)calloc(dir_tree_cnt, sizeof(DIR_entry));
    if (entries == nullptr) {
        printf("ERROR: Unable to allocate memory for tree entries.\n");
        handle.list = nullptr;
        return handle;
    }

    bool extract_success = true;
    uint8_t* eof_ptr = &dat->data[size];
    for (int idx = 0; (idx < dir_tree_cnt) && (&dir_tree[dir_tree_offset+4] < eof_ptr); idx++)
    {

        int path_size = get_dir_tree_i32(dir_tree, &dir_tree_offset);
        if ((path_size < 0)
        || (path_size > MAX_PATH)
        // || (entry_ptr + path_size + 16 >= eof_ptr)) {
        || (&dir_tree[dir_tree_offset + path_size + 16] >= eof_ptr)) {
            //TODO: log to file
            // set_popup_warning();
            printf("Error: get_dir_entry_list() Reached end of file %s after reading only %d of %d entries: L%d",
                    dat->file_name, idx, dir_tree_cnt, __LINE__);
            extract_success = false;
            break;
        }

        DIR_entry* entry   = &entries[idx];
        entry->path_size   = path_size;
        entry->path_ptr    = get_dir_tree_path(dir_tree, &dir_tree_offset, path_size);
        entry->packed      = get_dir_tree_pack(dir_tree, &dir_tree_offset);
        entry->unpack_size = get_dir_tree_i32(dir_tree, &dir_tree_offset);
        entry->packed_size = get_dir_tree_i32(dir_tree, &dir_tree_offset);
        entry->offset      = get_dir_tree_i32(dir_tree, &dir_tree_offset);
        entry->packed_ptr  = &dat->data[entry->offset];
    }

    if (extract_success == false) {
        free(entries);
        handle.list = nullptr;
        return handle;
    }

    handle.list = entries;
    //TODO: possibly also sort entries or make hash table for speed?
    return handle;
}

//TODO: this doesn't seem useful for anything
//      probably delete?
//copies buff.file_data into malloc'd char* txt
//returns pointer to char* txt
// char* DAT_to_txt(DAT_buffer* buff)
// {
//     char* txt = (char*)malloc(buff->size+1);
//     memcpy(txt, buff->data, buff->size);
//     return txt;
// }

void write_to_disk(DIR_entry* entry, char* game_path, DAT_buffer* buff)
{
    //TODO: log to file
    printf("writing file to disk: %s\n", entry->path_ptr);

    char path_buff[MAX_PATH] = {0};
    snprintf(path_buff, MAX_PATH, "%s/data/%s", game_path, entry->path_ptr);
    io_swap_slash(path_buff);

    char* path_case = io_path_check(path_buff);
    if (!io_create_path_from_file(path_case)) {
        //fail state
        printf("Unable to create missing directory: %s\n", path_case);
        // extract_success = false;
        // break;
    }
    if (!io_save_txt_file(path_case, (char*)buff->data)) {
        printf("Unable to write file: %s\n", entry->path_ptr);
        // extract_success = false;
    }
}

DIR_entry* get_entry_from_dat(DIR_entries entries, const char* file_name)
{
    // scan entries for matching names
    //TODO: possibly also sort entries or make hash table for speed?
    DIR_entry* entry = nullptr;
    for (int i = 0; i < entries.count; i++)
    {
        entry = &entries.list[i];

        // printf("%s\n", entry->path_ptr);
        if (entry->path_ptr[0] != file_name[0]) {
            continue;
        }
        int cmp = io_strncasecmp(file_name, entry->path_ptr, entry->path_size);
        if (cmp != 0) {
            continue;
        }

        DAT_buffer buff = {
            buff.size = entry->unpack_size,
            buff.data = (uint8_t*)calloc(1, buff.size),
        };
        if (buff.data == nullptr) {
            printf("ERROR: Unable to allocate memory for file extraction.\n");
            return nullptr;
        }

        // int success = uncompress(buff->file_data, &temp, entry.file_ptr, entry.packed_size);
        // int success = uncompress(buff->file_data, (ulong*)&buff->file_size, entry.file_ptr, entry.packed_size);
        uint64_t extracted_size = buff.size;
        int success = uncompress(buff.data, &extracted_size, entry->packed_ptr, entry->packed_size);
        if (success != Z_OK) {
            printf("ERROR: Extracting failed at uncompress(): %d\n",
                    success);
            // extract_success = false;
            //TODO: probably return false here?
            return nullptr;
            break;
        }
        if (extracted_size != buff.size) {
            printf("ERROR: Uncompress size doesn't match expected size: Expected: %d, Got: %d\n",
                    buff.size, extracted_size);
        }

        entry->unpacked_file = buff;

        // extract_success = true;
        break;
    }

    // return extract_success;
    return entry;
}

bool extract_from_DAT(const char* file_name, char* game_path, DAT_file* dat)//, DAT_Buffer* buff)
{
    if (dat->file_size < 1) {
        //TODO: log to file
        // set_popup_warning();
        printf("ERROR: extract_from_DAT() Unable to load %s DAT file: L%d\n",
                dat->file_name, __LINE__);
        return false;
    }

    int size = get_data_size(dat);
    if (size != dat->file_size) {
        //TODO: log to file
        // set_popup_warning();
        printf("ERROR: extract_from_DAT() %s stored size (%d) doesn't match on-disk size (%d): L%d",
                dat->file_name, size, dat->file_size, __LINE__);
        return false;
    }

    DIR_entries entries = {0};
    if (entries.list == nullptr) {
        entries = get_dir_entry_list(dat);
    }
    if (entries.list == nullptr) {
        printf("ERROR: Failed to extract file entries for %s, check previous errors.\n",
                dat->file_name);
        return false;
    }


    DIR_entry* scenery_lst = get_entry_from_dat(entries, file_name);
    if (scenery_lst == nullptr) {
        printf("ERROR: Failed to extract %s from %s dat file.\n",
                file_name, dat->file_name);
        return false;
    }

    return true;
}