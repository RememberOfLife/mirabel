#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* two very nice c json implementations:
https://github.com/DaveGamble/cJSON
https://github.com/json-parser/json-parser + builder
*/

typedef enum CJ_TYPE_E : uint8_t {
    CJ_TYPE_NONE = 0,
    CJ_TYPE_OBJECT,
    CJ_TYPE_ARRAY,
    CJ_TYPE_VNULL, // this is json null, read value-null
    CJ_TYPE_U64,
    CJ_TYPE_F32,
    CJ_TYPE_BOOL,
    CJ_TYPE_STRING,
    CJ_TYPE_ERROR,
    // CJ_TYPE_COL4U,
    // CJ_TYPE_COL4F,
    CJ_TYPE_COUNT,
} CJ_TYPE;

// static buffer string with capacity
typedef struct cj_sb_string_s {
    size_t cap;
    char* str;
} cj_sb_string;

typedef struct cj_color4u_s {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} cj_color4u;

typedef struct cj_color4f_s {
    float r;
    float g;
    float b;
    float a;
} cj_color4f;

typedef union cj_value_u {
    uint64_t u64;
    float f32;
    bool b;
    cj_sb_string s;
} cj_value;

// config object/value/array-container
typedef struct cj_ovac_s cj_ovac;

struct cj_ovac_s {
    cj_ovac* parent;

    // only valid if object/array
    uint32_t child_cap;
    uint32_t child_count;
    cj_ovac** children; //TODO maybe want to store the actual children instead of pointers here?
    //TODO use proper hashmap for object access! (additionaly, obj children are ordered so the plain array will always exist too!)

    //TODO could use ref counting and pointer to newer version here

    // only valid if parent is an object
    // if this is a child of an object, its key is o_label_str and o_label_hash=strhash(o_label_str)
    char* label_str;
    uint32_t label_hash; //TODO could malloc this side of the children to accelerate searching times in wide objects

    CJ_TYPE type;
    cj_value v;
};

//TODO file api for loading whole file to membuffer and writing whole file from membuffer

// operation on inplace sb_strings
void cj_sb_string_create(cj_sb_string* sbs, size_t cap, const char* str); // str is optional, and can be left NULL, use cap 0 to either infer the strlen or init as NULL is no str is given, if cap is non zero the initial str will be truncated if necessary
void cj_sb_string_resize(cj_sb_string* sbs, size_t cap, bool can_shrink);
void cj_sb_string_destroy(cj_sb_string* sbs);

// if str is an ovac of type vnull, retype it to sb_string, if cap > 0 then automaticall creates the internal string as well otherwise init to NULL
void cj_str_assume(cj_ovac* str, size_t cap);

cj_color4u cj_color4u_read(const char* str);
cj_color4f cj_color4f_read(const char* str);
// str must have capacity of at least 7 characters (9 if write_alpha is true)
void cj_color4u_write(char* str, cj_color4u c4u, bool write_alpha);
void cj_color4f_write(char* str, cj_color4f c4f, bool write_alpha);

cj_ovac* cj_create_object(uint32_t cap);
void cj_object_append(cj_ovac* obj, const char* key, cj_ovac* ovac);
//TODO cj_object_insert by idx?
cj_ovac* cj_object_get(cj_ovac* obj, const char* key); // returns the ovac found for this key under this obj, if it exists
void cj_object_replace(cj_ovac* obj, const char* key, cj_ovac* new_ovac);
cj_ovac* cj_object_detach(cj_ovac* obj, const char* key);
void cj_object_remove(cj_ovac* obj, const char* key);

cj_ovac* cj_create_vnull();
cj_ovac* cj_create_u64(uint64_t value);
cj_ovac* cj_create_f32(float value);
cj_ovac* cj_create_bool(bool value);
cj_ovac* cj_create_str(size_t cap, const char* str); // copies str into self, cap must be >= strlen(str)+1 //TODO make this more accessible: version with implicit cap and version without an actual string, i.e. null

cj_ovac* cj_create_array(uint32_t cap);
void cj_array_append(cj_ovac* arr, cj_ovac* ovac);
void cj_array_insert(cj_ovac* arr, uint32_t idx, cj_ovac* ovac); // insert such that child is reachable at idx in the array //TODO out of bounds behaviour?
void cj_array_replace(cj_ovac* arr, uint32_t idx, cj_ovac* new_ovac);
cj_ovac* cj_array_detach(cj_ovac* arr, uint32_t idx);
void cj_array_remove(cj_ovac* arr, uint32_t idx);

cj_ovac* cj_ovac_duplicate(cj_ovac* ovac);

// replace the type and value (i.e. children for obj/arr) of entry_ovac with that of data_ovac, but keeps entry_ovac stable as viewed from its parent, this consumes data ovac, effectively destroying it
void cj_ovac_replace(cj_ovac* entry_ovac, cj_ovac* data_ovac);

void cj_ovac_destroy(cj_ovac* ovac); // if ovac is in a parent container, it will automatically be removed from there first

// if str_hint is true then "\!XXX" and "\0" are enabled for use in strings
size_t cj_measure(cj_ovac* ovac, bool packed, bool str_hint); // includes a final zero terminator, returns 0 if a measure error occured, or ovac is NULL
char* cj_serialize(char* buf, cj_ovac* ovac, bool packed, bool str_hint); // returns a pointer just beyond the last written character, or NULL if a serialization error occured (e.g. invalid types)
cj_ovac* cj_deserialize(const char* buf, bool str_hint); // must be zero terminated, returns an obj ovac if successful, otherwise an ovac of type CJ_TYPE_ERROR, which has a string value containing the error string, or NULL if no error specified

// cj_find uses a data path api to find the specified entry in the ovac (json) tree
// examples:
// "client.global.palette.wood_dark" returns a color4u ovac
// "client.plugins.loadlist[0]" returns the sb_str ovac which is the first element of the loadlist array
// for arrays one can also use [-1] to get the last element
//WARNING array indices for set not currently supported, future feature only
// when using set, array indices also have more utilizations:
// [0] replaces element at idx 0, [+] appends to the list, [+0] makes available at idx 0 but preserves all other elements
// for delete there is also [*] to delete all elements, equivalent to setting parent to an empty list
//TODO '[' that are part of a label string (key in an obj) need to be escaped accessed via thing["subkey\[text"], use normal escaping for '"' and '\'
cj_ovac* cj_find(cj_ovac* root, const char* data_path);
// cj_ovac* cj_set(cj_ovac* root, const char* data_path, cj_ovac* ovac, bool overwrite); // behaves like cj_find + cj_ovac_replace
// cj_ovac* cj_remove(cj_ovac* root, const char* data_path); // behaves like cj_find + cj_ovac_destroy
//TODO general detached, duplicate, replace?

// same as find but only return a shallow value copy, return true if successfully written to rv
// check vnull to see if it was value null, optional (if not existant then function returns false on vnull b/c no value was written to rv)
bool cj_get_u64(cj_ovac* root, const char* data_path, uint64_t* rv, bool* vnull);
bool cj_get_f32(cj_ovac* root, const char* data_path, float* rv, bool* vnull);
bool cj_get_bool(cj_ovac* root, const char* data_path, bool* rv, bool* vnull);
// bool cj_get_str(cj_ovac* root, const char* data_path, cj_sb_string* rv); //TODO does this allocate or copy?
// shallow color get assumes data_path leads to a string ovac, this is then parsed as a color
bool cj_get_c4u(cj_ovac* root, const char* data_path, cj_color4u* rv, bool* vnull);
bool cj_get_c4f(cj_ovac* root, const char* data_path, cj_color4f* rv, bool* vnull);

// shallow value set, return true if the data path existed and was of the correct type, false otherwise
// bool cj_set_u64(cj_ovac* root, const char* data_path, uint64_t v); //TODO want shallow value set?
//...

//TODO some easy way to set defaults on startup and still be able to overwrite fixed values

// for concurrent access to the tree, later on might make the tree more capable and lock free
void* cfg_lock_create();
void cfg_lock_destroy(void* lock);
void cfg_rlock(void* lock);
void cfg_runlock(void* lock);
void cfg_wlock(void* lock);
void cfg_wunlock(void* lock);

#ifdef __cplusplus
}
#endif
