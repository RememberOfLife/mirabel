#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//WARNING this api has no usable backend yet, DO NOT USE

// this is essentially a json api, but provides data paths for getting / setting info, and ref counted cached subtree clones for multithreading support
/* two very nice c json implementations:
https://github.com/DaveGamble/cJSON
https://github.com/json-parser/json-parser + builder
*/

typedef enum CFG_TYPE_E : uint8_t {
    CFG_TYPE_NONE = 0,
    CFG_TYPE_OBJECT,
    CFG_TYPE_ARRAY,
    CFG_TYPE_VNULL, // this is json null, read value-null
    CFG_TYPE_U64,
    CFG_TYPE_F32,
    CFG_TYPE_BOOL,
    CFG_TYPE_STRING,
    CFG_TYPE_COL4U,
    CFG_TYPE_COL4F,
    CFG_TYPE_COUNT,
} CFG_TYPE;

// static buffer string with capacity
typedef struct cgf_sb_string_s {
    size_t cap;
    char* str;
} cgf_sb_string;

typedef struct cfg_color4u_s {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} cfg_color4u;

typedef struct cfg_color4f_s {
    float r;
    float g;
    float b;
    float a;
} cfg_color4f;

typedef union cfg_value_u {
    uint64_t u64;
    float f32;
    bool b;
    cgf_sb_string s;
    cfg_color4u col4u;
    cfg_color4f col4f;
} cfg_value;

typedef struct cfg_reference_s cfg_reference;

// config object/value/array-container
typedef struct cfg_ovac_s cfg_ovac;
struct cfg_ovac_s {
    // using parent, left_child, right_sibling binary tree
    cfg_ovac* p;
    cfg_ovac* lc;
    cfg_ovac* rs;

    // cfg_reference* ref_cache_tail; // the oldest still held reference to this ovac //TODO this fine?

    uint8_t type;

    uint8_t _reserved;

    //TODO want this?
    // if this is an object/array type then cache the number of children here
    uint16_t oa_child_count;

    // if this is a child of an object, its key is o_label_str and o_label_hash=strhash(o_label_str)
    uint32_t o_label_hash;
    char* o_label_str;

    cfg_value v;
};

////////////////////////////////
////////////////////////////////
////////////////////////////////

//TODO actually want color types?

/////
// general json wrap util

cfg_ovac* cj_create_object();
void cj_object_add(cfg_ovac* obj, const char* key, cfg_ovac* ovac);
void cj_object_replace(cfg_ovac* obj, const char* key, cfg_ovac* new_ovac);
cfg_ovac* cj_object_detach(cfg_ovac* obj, const char* key);
void cj_object_remove(cfg_ovac* obj, const char* key);

cfg_ovac* cj_create_vnull();
cfg_ovac* cj_create_u64(uint64_t value);
cfg_ovac* cj_create_f32(float value);
cfg_ovac* cj_create_bool(bool value);
cfg_ovac* cj_create_str(size_t cap, const char* str);
// cfg_ovac* cj_create_col4u(cfg_color4u value);
// cfg_ovac* cj_create_col4f(cfg_color4f value);

cfg_ovac* cj_create_array();
void cj_array_add(cfg_ovac* arr, cfg_ovac* ovac);
void cj_array_insert(cfg_ovac* arr, uint16_t idx, cfg_ovac* ovac); // insert such that child is reachable at idx in the array
void cj_array_replace(cfg_ovac* arr, uint16_t idx, cfg_ovac* new_ovac);
cfg_ovac* cj_array_detach(cfg_ovac* arr, uint16_t idx);
void cj_array_remove(cfg_ovac* arr, uint16_t idx);

void cj_ovac_duplicate(cfg_ovac* ovac);

// replace the type and value of entry_ovac with that of data_ovac, but keeps entry_ovac stable as viewed from its parent
void cj_ovac_replace(cfg_ovac* entry_ovac, cfg_ovac* data_ovac);

void cj_ovac_destroy();

size_t cj_measure(cfg_ovac* ovac, bool packed);
void cj_serialize(char* buf, cfg_ovac* ovac, bool packed);
cfg_ovac* cj_deserialize(const char* buf, size_t len);

// cj_find uses a data path api to find the specified entry in the ovac (json) tree
// examples:
// "client.global.palette.wood_dark" returns a color4u ovac
//WARNING array indices not currently supported, future feature only
// "client.plugins.loadlist[0]" returns the sb_str ovac which is the first element of the loadlist array
// for arrays one can also use [-1] to get the last element
// when using set, array indices also have more utilizations:
// [0] replaces element at idx 0, [+] appends to the list, [+0] makes available at idx 0 but preserves all other elements
// for delete there is also [*] to delete all elements, equivalent to setting parent to an empty list
cfg_ovac* cj_find(cfg_ovac* root, const char* data_path);
cfg_ovac* cj_get(cfg_ovac* root, const char* data_path); // behaves like cj_find + cj_ovac_duplicate
cfg_ovac* cj_set(cfg_ovac* root, const char* data_path, cfg_ovac* ovac, bool overwrite); // behaves like cj_find + cj_ovac_replace
cfg_ovac* cj_remove(cfg_ovac* root, const char* data_path);
//TODO general detach, dupe ?

// same as find but only return a shallow value copy, return true if successful
//TODO use argument(bool* vnull), check vnull to see if it was value null, optional (if not existant then function returns false on vnull?)
bool cj_get_u64(cfg_ovac* root, const char* data_path, uint64_t* rv);
bool cj_get_f32(cfg_ovac* root, const char* data_path, float* rv);
bool cj_get_bool(cfg_ovac* root, const char* data_path, bool* rv);
bool cj_get_str(cfg_ovac* root, const char* data_path, cgf_sb_string* rv); //TODO does this allocate or copy?
// bool cj_get_col4u(cfg_ovac* ovac, const char* data_path, cfg_color4u* rv);
// bool cj_get_col4f(cfg_ovac* ovac, const char* data_path, cfg_color4f* rv);

// shallow value set, return true if the data path existed and was of the correct type, false otherwise
bool cj_set_u64(cfg_ovac* root, const char* data_path, uint64_t v);
//...

// no helpers for get/set on lists/objects, you have to use the proper ovac / ref api

/////
// thread-safe ref working functions
//TODO everything breaks when using refs with multi threading, how to fix?

//TODO just ref count the tree, updating is cheap and reading is flat or fails, use a global tree mutex to lock during updating?

// all refs are of cfg_ovac type use .v to access the primitive values therein
cfg_ovac* cj_get_ref(cfg_ovac* root, const char* data_path);
// does not require set because cj_set already makes sure all old refs get the newest

// updates the ref pointer with the newest data, returns false if the type changed / value does not exist anymore
// same as cj_release_ref + cj_get_ref, but only the updater pays for the searching overhead
bool cj_fetch_ref(cfg_ovac** ref_ovac);

// decrement the ref counter of the ref cache entry, if zero and no older refs still held it will be freed
void cj_release_ref(cfg_ovac* ref_ovac);

////////////////////////////////
////////////////////////////////
////////////////////////////////

//TODO some way to merge change refs into the main tree? need to immeditately do it so other thread gets will get the newest data, also if someone gets a ref to an ovac that already has a valid cache entry then need to provide that instead of a new one, keep a pointer to the cache head in the main tree?

#ifdef __cplusplus
}
#endif
