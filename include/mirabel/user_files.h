#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct user_file_data_s {
    const char* file_name;
    uint8_t* data;
    uint32_t size;
} user_file_data;

// use this to prompt or offer the user to load or save a file
// for persistent data using software defined paths, use //TODO// instead!
typedef struct user_files_manager_s {
    // interface or web browser single-file path prompt -> load that data
    user_file_data (*load_data_file)(); // file_name will be NULL if no file was loaded
    // interface or web browser single-file path prompt -> save data there
    void (*save_data_file)(user_file_data info);

    //TODO in the future we might wrap an (open -> read/write -> close) or memmap based workflow via managed files, which can be created and released here
} user_files_manager;

#ifdef __cplusplus
}
#endif
