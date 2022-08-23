#include <cassert>
#include <cstdbool>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <unordered_map>

#include "surena/util/noise.h"

#include "mirabel/config.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint32_t CJ_BASE_CAP = 4;

cj_ovac* cj_create_object(uint32_t cap)
{
    cj_ovac* obj = (cj_ovac*)malloc(sizeof(cj_ovac));
    *obj = (cj_ovac){
        .parent = NULL,
        .child_cap = cap,
        .child_count = 0,
        .children = (cap > 0 ? (cj_ovac**)malloc(sizeof(cj_ovac*) * cap) : NULL),
        .label_str = NULL,
        .label_hash = 0,
        .type = CJ_TYPE_OBJECT,
    };
    return obj;
}

void cj_object_append(cj_ovac* obj, const char* key, cj_ovac* ovac)
{
    if (obj->child_cap == 0) {
        obj->child_cap = CJ_BASE_CAP;
        obj->children = (cj_ovac**)malloc(sizeof(cj_ovac*) * obj->child_cap);
    }
    if (obj->child_count + 1 > obj->child_cap) {
        obj->child_cap *= 2;
        cj_ovac** children_new = (cj_ovac**)malloc(sizeof(cj_ovac*) * obj->child_cap);
        memcpy(children_new, obj->children, sizeof(cj_ovac*) * obj->child_count);
        obj->children = children_new;
    }
    obj->children[obj->child_count++] = ovac;
    ovac->parent = obj;
    ovac->label_hash = strhash(key, NULL);
    ovac->label_str = strdup(key);
}

cj_ovac* cj_object_get(cj_ovac* obj, const char* key)
{
    uint32_t key_hash = strhash(key, NULL);
    for (uint32_t idx = 0; idx < obj->child_count; idx++) {
        cj_ovac* cp = obj->children[idx];
        if (cp->label_hash == key_hash && strcmp(cp->label_str, key) == 0) {
            return cp;
        }
    }
    return NULL;
}

void cj_object_replace(cj_ovac* obj, const char* key, cj_ovac* new_ovac)
{
    cj_ovac_replace(cj_object_get(obj, key), new_ovac);
}

cj_ovac* cj_object_detach(cj_ovac* obj, const char* key)
{
    uint32_t key_hash = strhash(key, NULL);
    for (uint32_t idx = 0; idx < obj->child_count; idx++) {
        cj_ovac* cp = obj->children[idx];
        if (cp->label_hash == key_hash && strcmp(cp->label_str, key) == 0) {
            cp->parent = NULL;
            memmove(obj->children + idx, obj->children + idx + 1, sizeof(cj_ovac*) * obj->child_count - idx - 1);
            if (obj->child_count < obj->child_cap / 4) {
                // resize obj cap if it falls too low
                obj->child_cap /= 2;
                cj_ovac** children_new = (cj_ovac**)malloc(sizeof(cj_ovac*) * obj->child_cap);
                memcpy(children_new, obj->children, sizeof(cj_ovac*) * obj->child_count);
                obj->children = children_new;
            }
            return cp;
        }
    }
    return NULL;
}

void cj_object_remove(cj_ovac* obj, const char* key)
{
    uint32_t key_hash = strhash(key, NULL);
    for (uint32_t idx = 0; idx < obj->child_count; idx++) {
        cj_ovac* cp = obj->children[idx];
        if (cp->label_hash == key_hash && strcmp(cp->label_str, key) == 0) {
            cp->parent = NULL;
            cj_ovac_destroy(cp);
            memmove(obj->children + idx, obj->children + idx + 1, sizeof(cj_ovac*) * obj->child_count - idx - 1);
            if (obj->child_count < obj->child_cap / 4) {
                // resize obj cap if it falls too low
                obj->child_cap /= 2;
                cj_ovac** children_new = (cj_ovac**)malloc(sizeof(cj_ovac*) * obj->child_cap);
                memcpy(children_new, obj->children, sizeof(cj_ovac*) * obj->child_count);
                obj->children = children_new;
            }
            return;
        }
    }
}

cj_ovac* cj_create_vnull()
{
    cj_ovac* val = (cj_ovac*)malloc(sizeof(cj_ovac));
    *val = (cj_ovac){
        .parent = NULL,
        .child_cap = 0,
        .child_count = 0,
        .children = 0,
        .label_str = NULL,
        .label_hash = 0,
        .type = CJ_TYPE_VNULL,
    };
    return val;
}

cj_ovac* cj_create_u64(uint64_t value)
{
    cj_ovac* val = (cj_ovac*)malloc(sizeof(cj_ovac));
    *val = (cj_ovac){
        .parent = NULL,
        .child_cap = 0,
        .child_count = 0,
        .children = 0,
        .label_str = NULL,
        .label_hash = 0,
        .type = CJ_TYPE_U64,
        .v = (cj_value){
            .u64 = value,
        },
    };
    return val;
}

cj_ovac* cj_create_f32(float value)
{
    cj_ovac* val = (cj_ovac*)malloc(sizeof(cj_ovac));
    *val = (cj_ovac){
        .parent = NULL,
        .child_cap = 0,
        .child_count = 0,
        .children = 0,
        .label_str = NULL,
        .label_hash = 0,
        .type = CJ_TYPE_F32,
        .v = (cj_value){
            .f32 = value,
        },
    };
    return val;
}

cj_ovac* cj_create_bool(bool value)
{
    cj_ovac* val = (cj_ovac*)malloc(sizeof(cj_ovac));
    *val = (cj_ovac){
        .parent = NULL,
        .child_cap = 0,
        .child_count = 0,
        .children = 0,
        .label_str = NULL,
        .label_hash = 0,
        .type = CJ_TYPE_BOOL,
        .v = (cj_value){
            .b = value,
        },
    };
    return val;
}

cj_ovac* cj_create_str(size_t cap, const char* str)
{
    cj_ovac* val = (cj_ovac*)malloc(sizeof(cj_ovac));
    *val = (cj_ovac){
        .parent = NULL,
        .child_cap = 0,
        .child_count = 0,
        .children = 0,
        .label_str = NULL,
        .label_hash = 0,
        .type = CJ_TYPE_STRING,
        .v = (cj_value){
            .s = (cj_sb_string){
                .cap = cap,
                .str = (char*)malloc(cap),
            },
        },
    };
    strcpy(val->v.s.str, str);
    return val;
}

cj_ovac* cj_create_array(uint32_t cap)
{
    cj_ovac* arr = (cj_ovac*)malloc(sizeof(cj_ovac));
    *arr = (cj_ovac){
        .parent = NULL,
        .child_cap = cap,
        .child_count = 0,
        .children = (cap > 0 ? (cj_ovac**)malloc(sizeof(cj_ovac*) * cap) : NULL),
        .label_str = NULL,
        .label_hash = 0,
        .type = CJ_TYPE_ARRAY,
    };
    return arr;
}

void cj_array_append(cj_ovac* arr, cj_ovac* ovac)
{
    if (arr->child_cap == 0) {
        arr->child_cap = CJ_BASE_CAP;
        arr->children = (cj_ovac**)malloc(sizeof(cj_ovac*) * arr->child_cap);
    }
    if (arr->child_count + 1 > arr->child_cap) {
        arr->child_cap *= 2;
        cj_ovac** children_new = (cj_ovac**)malloc(sizeof(cj_ovac*) * arr->child_cap);
        memcpy(children_new, arr->children, sizeof(cj_ovac*) * arr->child_count);
        arr->children = children_new;
    }
    arr->children[arr->child_count++] = ovac;
    ovac->parent = arr;
}

void cj_array_insert(cj_ovac* arr, uint32_t idx, cj_ovac* ovac)
{
    //TODO out of bounds behaviour: for now assumes child_count <= idx, could also fill with vnull on too high?
    if (arr->child_cap == 0) {
        arr->child_cap = CJ_BASE_CAP;
        arr->children = (cj_ovac**)malloc(sizeof(cj_ovac*) * arr->child_cap);
    }
    if (arr->child_count + 1 > arr->child_cap) {
        arr->child_cap *= 2;
        cj_ovac** children_new = (cj_ovac**)malloc(sizeof(cj_ovac*) * arr->child_cap);
        memcpy(children_new, arr->children, sizeof(cj_ovac*) * arr->child_count);
        arr->children = children_new;
    }
    //TODO move this memmove up into the if, so its only 2 of half size instead of one large and 1 half
    memmove(arr->children + idx + 1, arr-> children + idx, sizeof(cj_ovac*) * (arr->child_count - idx));
    arr->children[idx] = ovac;
    arr->child_count++;
    ovac->parent = arr;
}

void cj_array_replace(cj_ovac* arr, uint32_t idx, cj_ovac* new_ovac)
{
    cj_ovac_replace(arr->children[idx], new_ovac);
}

cj_ovac* cj_array_detach(cj_ovac* arr, uint32_t idx)
{
    if (idx >= arr->child_count) {
        return NULL;
    }
    cj_ovac* cp = arr->children[idx];
    cp->parent = NULL;
    memmove(arr->children + idx, arr->children + idx + 1, sizeof(cj_ovac*) * arr->child_count - idx - 1);
    if (arr->child_count < arr->child_cap / 4) {
        // resize arr cap if it falls too low
        arr->child_cap /= 2;
        cj_ovac** children_new = (cj_ovac**)malloc(sizeof(cj_ovac*) * arr->child_cap);
        memcpy(children_new, arr->children, sizeof(cj_ovac*) * arr->child_count);
        arr->children = children_new;
    }
    return cp;
}

void cj_array_remove(cj_ovac* arr, uint32_t idx)
{
    arr->children[idx]->parent = NULL;
    cj_ovac_destroy(arr->children[idx]);
    memmove(arr->children + idx, arr->children + idx + 1, sizeof(cj_ovac*) * arr->child_count - idx - 1);
    if (arr->child_count < arr->child_cap / 4) {
        // resize arr cap if it falls too low
        arr->child_cap /= 2;
        cj_ovac** children_new = (cj_ovac**)malloc(sizeof(cj_ovac*) * arr->child_cap);
        memcpy(children_new, arr->children, sizeof(cj_ovac*) * arr->child_count);
        arr->children = children_new;
    }
}

cj_ovac* cj_ovac_duplicate(cj_ovac* ovac)
{
    cj_ovac* clone = (cj_ovac*)malloc(sizeof(cj_ovac));
    switch (ovac->type) {
        case CJ_TYPE_NONE: {
            assert(0);
        } break;
        case CJ_TYPE_OBJECT:
        case CJ_TYPE_ARRAY: {
            *clone = (cj_ovac){
                .parent = NULL,
                .child_cap = ovac->child_cap,
                .child_count = ovac->child_count,
                .children = (cj_ovac**)malloc(sizeof(cj_ovac*) * ovac->child_cap),
                .label_str = (ovac->label_str ? strdup(ovac->label_str) : NULL),
                .label_hash = ovac->label_hash,
                .type = ovac->type,
            };
            for (uint32_t idx = 0; idx < ovac->child_count; idx++) {
                clone->children[idx] = cj_ovac_duplicate(ovac->children[idx]); //TODO make this iterative instead of recursive
            }
        } break;
        case CJ_TYPE_VNULL:
        case CJ_TYPE_U64:
        case CJ_TYPE_F32:
        case CJ_TYPE_BOOL: {
            *clone = (cj_ovac){
                .parent = NULL,
                .child_cap = 0,
                .child_count = 0,
                .children = 0,
                .label_str = (ovac->label_str ? strdup(ovac->label_str) : NULL),
                .label_hash = ovac->label_hash,
                .type = ovac->type,
                .v = ovac->v,
            };
        } break;
        case CJ_TYPE_STRING: {
            *clone = (cj_ovac){
                .parent = NULL,
                .child_cap = 0,
                .child_count = 0,
                .children = 0,
                .label_str = (ovac->label_str ? strdup(ovac->label_str) : NULL),
                .label_hash = ovac->label_hash,
                .type = CJ_TYPE_STRING,
                .v = (cj_value){
                    .s = (cj_sb_string){
                        .cap = ovac->v.s.cap,
                        .str = (char*)malloc(ovac->v.s.cap),
                    },
                },
            };
            strcpy(clone->v.s.str, ovac->v.s.str);
        } break;
        case CJ_TYPE_COL4U:
        case CJ_TYPE_COL4F:
        case CJ_TYPE_COUNT: {
            assert(0);
        } break;
    }
    return clone;
}

void cj_ovac_replace(cj_ovac* entry_ovac, cj_ovac* data_ovac)
{
    if (entry_ovac->children != NULL) {
        // destroy all children of entry ovac, better way?
        for (uint32_t idx = 0; idx < entry_ovac->child_count; idx++) {
            entry_ovac->children[idx]->parent = NULL;
            cj_ovac_destroy(entry_ovac->children[idx]);
        }
        free(entry_ovac->children);
    }
    entry_ovac->child_cap = data_ovac->child_cap;
    entry_ovac->child_count = data_ovac->child_count;
    entry_ovac->children = data_ovac->children;
    entry_ovac->type = data_ovac->type;
    entry_ovac->v = data_ovac->v;
    if (data_ovac->label_str != NULL) {
        free(data_ovac->label_str);
    }
    free(data_ovac);
}

void cj_ovac_destroy(cj_ovac* ovac)
{
    if (ovac->parent != NULL) {
        // remove from parent, automitcally deletes the subtree
        switch (ovac->parent->type) {
            case CJ_TYPE_OBJECT: {
                cj_object_detach(ovac->parent, ovac->label_str);
            } break;
            case CJ_TYPE_ARRAY: {
                //TODO if idx in parent gets added accelerate finding the child for removal here
                for (uint32_t idx = 0; idx < ovac->parent->child_count; idx++) {
                    if (ovac->parent->children[idx] == ovac) {
                        cj_array_detach(ovac->parent, idx);
                        break;
                    }
                }
            } break;
            default: {
                assert(0);
            } break;
        }
    }
    if (ovac->children != NULL) {
        for (uint32_t idx = 0; idx < ovac->child_count; idx++) {
            ovac->children[idx]->parent = NULL;
            cj_ovac_destroy(ovac->children[idx]); //TODO make this iterative instead of recursive
        }
        free(ovac->children);
    }
    if (ovac->label_str != NULL) {
        free(ovac->label_str);
    }
    if (ovac->type == CJ_TYPE_STRING) {
        free(ovac->v.s.str); //BUG can str be null here?
    }
    free(ovac);
}

size_t cj_measure(cj_ovac* ovac, bool packed)
{
    //TODO
    return 0;
}

void cj_serialize(char* buf, cj_ovac* ovac, bool packed)
{
    //TODO
}

cj_ovac* cj_deserialize(const char* buf, size_t len)
{
    //TODO
    return NULL;
}

//////////////
// cfg locking

struct cfg_lock {
    std::mutex m; //TODO use a proper rwlock, but shared mutex needs cpp17
};

void* cfg_lock_create()
{
    cfg_lock* locki = new cfg_lock;
    return locki;
}

void cfg_lock_destroy(void* lock)
{
    delete (cfg_lock*)lock;
}

void cfg_rlock(void* lock)
{
    cfg_lock* locki = (cfg_lock*)lock;
    locki->m.lock();
}

void cfg_runlock(void* lock)
{
    cfg_lock* locki = (cfg_lock*)lock;
    locki->m.unlock();
}

void cfg_wlock(void* lock)
{
    cfg_lock* locki = (cfg_lock*)lock;
    locki->m.lock();
}

void cfg_wunlock(void* lock)
{
    cfg_lock* locki = (cfg_lock*)lock;
    locki->m.unlock();
}

#ifdef __cplusplus
}
#endif
