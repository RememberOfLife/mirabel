#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//WARNING this api has no usable backend yet, DO NOT USE



//TODO what happens if refs to an object exists and it gets overwritten as another type?

//TODO how does this all even work with multi threading

//TODO get list of child objs?

//TODO get and get ref just check if type tag matches for the configuration path obj

typedef enum CFG_TYPE_E : uint8_t {
    CFG_TYPE_NULL = 0,
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

// config datatypes

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

// config object/value/array-container
typedef struct cfg_ovac_s cfg_ovac;
struct cfg_ovac_s {
    // using parent, left_child, right_sibling binary tree
    cfg_ovac* p;
    cfg_ovac* lc;
    cfg_ovac* rs;

    uint8_t type;

    uint8_t _reserved;

    //TODO want this?
    // if this is an array type then cache the number of children here
    uint16_t a_child_count;

    // if this is a child of an object, its key is o_label_str and o_label_hash=strhash(o_label_str)
    uint32_t o_label_hash;
    char* o_label_str;

    cfg_value v;
};

typedef struct config_registry_s config_registry;

void config_registry_create(config_registry* cr);
void config_registry_destroy(config_registry* cr);

void config_registry_load(config_registry* cr, const char* file_path);
void config_registry_save(config_registry* cr, const char* file_path);

//TODO some way to merge change refs into the main tree? or immeditately do it?

//TODO add/get/get_ref/set need functionality for objects and lists, or just use ovac?

//TODO how to do defaults?, do we even support a reset to defaults button?
// add the ovac under this path, only if no ovac exists under this path yet
void config_registry_add(config_registry* cr, const char* data_path, cfg_ovac ovac);
void config_registry_add_u64(config_registry* cr, const char* data_path, uint64_t u64);
void config_registry_delete(config_registry* cr, const char* data_path);

// if a ref is newly created on an object or a list, it is deep copied, the non ref versions do NOT provide a deep copy

// use the type specified functions instead of the ovac generalization
cfg_ovac config_registry_get(config_registry* cr, const char* data_path);
uint64_t config_registry_get_u64(config_registry* cr, const char* data_path);

// use the type specified functions instead of the ovac generalization
cfg_ovac* config_registry_get_ref(config_registry* cr, const char* data_path);
uint64_t* config_registry_get_ref_u64(config_registry* cr, const char* data_path);

// if a more recent version of the data exists fetch will release the current ref and effectively get the newer one
// use this if you want this behaviour but faster/cheaper than doing it separately
void config_registry_fetch_ref(config_registry* cr, void* ovac_ref);

// decrement the ref counter of the ref cache entry, if zero it will be freed
void config_registry_release_ref(config_registry* cr, void* ovac_ref);

// set the 
void config_registry_set(config_registry* cr, const char* data_path, cfg_ovac ovac);
void config_registry_set_u64(config_registry* cr, const char* data_path, uint64_t ovac);

//TODO reflection functions required?

#ifdef __cplusplus
}
#endif
