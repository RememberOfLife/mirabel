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

size_t cj_measure_impl(cj_ovac* ovac, bool packed, uint32_t depth)
{
    size_t acc = 0;
    if (ovac->label_str != NULL) {
        acc += 3 + strlen(ovac->label_str) + (packed ? 0 : 1); // "\"%s\": "
    }
    switch (ovac->type) {
        case CJ_TYPE_NONE: {
            assert(0);
        } break;
        case CJ_TYPE_OBJECT:
        case CJ_TYPE_ARRAY: {
            acc += 2; // "{" and "}" or "[" or "]"
            if (ovac->child_count > 0) {
                if (packed == false) {
                    acc += 1 + 4 * depth; // "\n" and "    " for every child
                }
                for (uint32_t i = 0; i < ovac->child_count; i++) {
                    if (packed == false) {
                        acc += 4 * (depth + 1) + 1; // padding for child and "\n" since unpacked
                    }
                    acc += cj_measure_impl(ovac->children[i], packed, depth + 1);
                    if (packed == false || i < ovac->child_count - 1) {
                        acc += 1; // "," at end of child
                    }
                }
            }
        } break;
        case CJ_TYPE_VNULL: {
            acc += 4;
        } break;
        case CJ_TYPE_U64: {
            acc += snprintf(NULL, 0, "%lu", ovac->v.u64);
        } break;
        case CJ_TYPE_F32: {
            acc += snprintf(NULL, 0, "%f", ovac->v.f32);
        } break;
        case CJ_TYPE_BOOL: {
            acc += ovac->v.b ? 4 : 5;
        } break;
        case CJ_TYPE_STRING: {
            acc += 2 + strlen(ovac->v.s.str); // "\"%s\"" //TODO lots of encoding issues
        } break;
        case CJ_TYPE_COUNT: {
            assert(0);
        } break;
    }
    if (depth == 0) {
        acc += 2; // "\n" and "\0" for root obj
    }
    return acc;
}

size_t cj_measure(cj_ovac* ovac, bool packed)
{
    return cj_measure_impl(ovac, packed, 0);
}

char* cj_serialize_impl(char* buf, cj_ovac* ovac, bool packed, uint32_t depth)
{
    char* wrp = buf;
    if (ovac->label_str != NULL) {
        wrp += sprintf(wrp, "\"%s\":", ovac->label_str);
        if (packed == false) {
            wrp += sprintf(wrp, " ");
        }
    }
    switch (ovac->type) {
        case CJ_TYPE_NONE: {
            assert(0);
        } break;
        case CJ_TYPE_OBJECT:
        case CJ_TYPE_ARRAY: {
            switch (ovac->type) {
                case CJ_TYPE_OBJECT: {
                    wrp += sprintf(wrp, "{");
                } break;
                case CJ_TYPE_ARRAY: {
                    wrp += sprintf(wrp, "[");
                } break;
                default: {
                    assert(0);
                } break;
            }
            if (ovac->child_count > 0) {
                if (packed == false) {
                    wrp += sprintf(wrp, "\n");
                }
                for (uint32_t i = 0; i < ovac->child_count; i++) {
                    if (packed == false) {
                        memset(wrp, ' ', 4 * (depth + 1));
                        wrp += 4 * (depth + 1);
                    }
                    wrp = cj_serialize_impl(wrp, ovac->children[i], packed, depth + 1);
                    if (packed == false || i < ovac->child_count - 1) {
                        wrp += sprintf(wrp, ",");
                    }
                    if (packed == false) {
                        wrp += sprintf(wrp, "\n");
                    }
                }
                if (packed == false) {
                    memset(wrp, ' ', 4 * depth);
                    wrp += 4 * depth;
                }
            }
            switch (ovac->type) {
                case CJ_TYPE_OBJECT: {
                    wrp += sprintf(wrp, "}");
                } break;
                case CJ_TYPE_ARRAY: {
                    wrp += sprintf(wrp, "]");
                } break;
                default: {
                    assert(0);
                } break;
            }
        } break;
        case CJ_TYPE_VNULL: {
            wrp += sprintf(wrp, "null");
        } break;
        case CJ_TYPE_U64: {
            wrp += sprintf(wrp, "%lu", ovac->v.u64);
        } break;
        case CJ_TYPE_F32: {
            wrp += sprintf(wrp, "%f", ovac->v.f32);
        } break;
        case CJ_TYPE_BOOL: {
            wrp += sprintf(wrp, ovac->v.b ? "true" : "false");
        } break;
        case CJ_TYPE_STRING: {
            wrp += sprintf(wrp, "\"%s\"", ovac->v.s.str); //TODO lots of encoding issues
        } break;
        case CJ_TYPE_COUNT: {
            assert(0);
        } break;
    }
    if (depth == 0) {
        wrp += sprintf(wrp, "\n");
    }
    return wrp;
}

void cj_serialize(char* buf, cj_ovac* ovac, bool packed)
{
    cj_serialize_impl(buf, ovac, packed, 0);
}

const char* cj_deserialize_impl_parse_string(const char* buf, cj_sb_string* sbs)
{
    const char* wbuf = buf;
    //TODO //BUG this currently does not allow '\"' escaping, it just stops the string immediately on the first '"'
    while (*wbuf != '\0' && *wbuf != '\"') {
        wbuf++;
    }
    *sbs = (cj_sb_string){
        .cap = (size_t)(wbuf - buf + 1),
        .str = (char*)malloc(wbuf - buf + 1),
    };
    memcpy(sbs->str, buf, sbs->cap);
    sbs->str[sbs->cap - 1] = '\0';
    return wbuf + 1;
}

//TODO need this at all?
//TODO could also just return a string ovac containing the parsing error message
cj_ovac* cj_deserialize_impl_abort(cj_ovac* ovac)
{
    cj_ovac* dp = ovac;
    while (dp->parent) {
        dp = dp->parent;
    }
    cj_ovac_destroy(dp);
    return NULL;
}

//BUG this does basically no error checking at all //TODO
cj_ovac* cj_deserialize(const char* buf)
{
    const char* wbuf = buf;

    cj_ovac* ccon = NULL; // current container (object/array)
    cj_sb_string pstr = (cj_sb_string){.cap = 0, .str = NULL};

    char c = *(wbuf++);
    while (c != '\0') {
        switch (c) {
            case ' ':
            case '\n': {
                // pass
            } break;
            case '{':
            case '[': {
                cj_ovac* new_con;
                if (c == '{') {
                    new_con = cj_create_object(0);
                } else {
                    new_con = cj_create_array(0);
                }
                if (ccon != NULL) {
                    if (ccon->type == CJ_TYPE_OBJECT) {
                        cj_object_append(ccon, pstr.str, new_con);
                        pstr.cap = 0;
                        free(pstr.str);
                        pstr.str = NULL;
                    } else if (ccon->type == CJ_TYPE_ARRAY) {
                        cj_array_append(ccon, new_con);
                    } else {
                        assert(0);
                    }
                }
                new_con->parent = ccon;
                ccon = new_con;
            } break;
            case '}':
            case ']': {
                // close container
                if (ccon->parent == NULL) {
                    //TODO special case if this closed the main object?
                    // check that only whitespace follows, then return parsed obj?
                    return ccon;
                }
                ccon = ccon->parent;
            } break;
            case '"': {
                // parse the string currently at buf
                if (pstr.str == NULL && ccon->type == CJ_TYPE_OBJECT) {
                    // this a key
                    wbuf = cj_deserialize_impl_parse_string(wbuf, &pstr);
                } else {
                    cj_sb_string val_str;
                    wbuf = cj_deserialize_impl_parse_string(wbuf, &val_str);
                    cj_ovac* new_str = cj_create_str(val_str.cap, val_str.str);
                    free(val_str.str);
                    if (pstr.str != NULL) {
                        cj_object_append(ccon, pstr.str, new_str);
                        pstr.cap = 0;
                        free(pstr.str);
                        pstr.str = NULL;
                    } else {
                        cj_array_append(ccon, new_str);
                    }
                }
            } break;
            case ':': {
                // pass
            } break;
            case ',': {
                // pass
            } break;
            default: {
                // value: vnull, u64, f32, bool
                cj_ovac* new_val = NULL;
                uint64_t u64;
                float f32;
                int vlen = 1;
                int scan_len;
                const char* cbuf = wbuf;
                // skip forward to next non value character //TODO //BUG make this proper
                while (*cbuf != '\0' && *cbuf != ',' && *cbuf != ' ' && *cbuf != '\n' && *cbuf != '}' && *cbuf != ']') {
                    cbuf++;
                }
                vlen += cbuf - wbuf;
                if (strncmp(wbuf - 1, "null", 4) == 0) {
                    new_val = cj_create_vnull();
                } else if (strncmp(wbuf - 1, "true", 4) == 0) {
                    new_val = cj_create_bool(true);
                } else if (strncmp(wbuf - 1, "false", 5) == 0) {
                    new_val = cj_create_bool(false);
                } else if (sscanf(wbuf - 1, "%lu%n", &u64, &scan_len) == 1 && scan_len == vlen) {
                    new_val = cj_create_u64(u64);
                } else if (sscanf(wbuf - 1, "%f%n", &f32, &scan_len) == 1 && scan_len == vlen) {
                    new_val = cj_create_f32(f32);
                } else {
                    assert(0);
                }
                wbuf = cbuf;
                // append value to container
                if (pstr.str != NULL) {
                    cj_object_append(ccon, pstr.str, new_val);
                    pstr.cap = 0;
                    free(pstr.str);
                    pstr.str = NULL;
                } else {
                    cj_array_append(ccon, new_val);
                }
            } break;
        }
        c = *(wbuf++);
    }
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
