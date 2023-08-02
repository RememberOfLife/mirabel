#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rosalia/noise.h"
#include "rosalia/serialization.h"

#include "surena/game.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PTRMAX ((void*)UINTPTR_MAX)

const char* general_error_strings[] = {
    [ERR_OK] = "OK",
    [ERR_NOK] = "NOK",
    [ERR_STATE_UNRECOVERABLE] = "state unrecoverable",
    [ERR_STATE_CORRUPTED] = "state corrupted",
    [ERR_OUT_OF_MEMORY] = "out of memory",
    [ERR_FEATURE_UNSUPPORTED] = "feature unsupported",
    [ERR_MISSING_HIDDEN_STATE] = "missing hidden state",
    [ERR_INVALID_PLAYER_COUNT] = "invalid player count",
    [ERR_INVALID_INPUT] = "invalid input",
    [ERR_INVALID_PLAYER] = "invalid player",
    [ERR_INVALID_MOVE] = "invalid move",
    [ERR_INVALID_OPTIONS] = "invalid options",
    [ERR_INVALID_LEGACY] = "invalid legacy",
    [ERR_INVALID_STATE] = "invalid state",
    [ERR_UNENUMERABLE] = "unenumerable",
    [ERR_UNSTABLE_POSITION] = "unstable position",
    [ERR_SYNC_COUNTER_MISMATCH] = "sync counter mismatch",
    [ERR_SYNC_COUNTER_IMPOSSIBLE_REORDER] = "sync counter impossible reorder",
    [ERR_RETRY] = "retry",
    [ERR_CUSTOM_ANY] = "custom any",
    [ERR_ENUM_DEFAULT_OFFSET] = NULL,
};

const char* get_general_error_string(error_code err, const char* fallback)
{
    if (err < ERR_ENUM_DEFAULT_OFFSET) {
        return general_error_strings[err];
    }
    return fallback;
}

error_code rerror(char** pbuf, error_code ec, const char* str, const char* str_end)
{
    if (str_end == NULL) {
        return rerrorf(pbuf, ec, "%s", str);
    } else {
        return rerrorf(pbuf, ec, "%.*s", str_end - str, str);
    }
}

error_code rerrorf(char** pbuf, error_code ec, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    error_code ret = rerrorfv(pbuf, ec, fmt, args);
    va_end(args);
    return ret;
}

error_code rerrorfv(char** pbuf, error_code ec, const char* fmt, va_list args)
{
    if (pbuf == NULL) {
        return ec;
    }
    if (*pbuf != NULL) {
        free(*pbuf);
        *pbuf = NULL;
    }
    if (fmt != NULL) {
        size_t len = vsnprintf(NULL, 0, fmt, args) + 1;
        *pbuf = (char*)malloc(len);
        if (*pbuf == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        vsnprintf(*pbuf, len, fmt, args);
    }
    return ec;
}

//TODO this might be a candidate for a rosalia utility in serialization.h
size_t ls_move_data_serializer(GSIT itype, void* obj_in, void* obj_out, void* buf, void* buf_end)
{
    // flatten the unions, this encodes more data than required, but keeps complexity down

    typedef enum __attribute__((__packed__)) FLAT_MOVE_TYPE_E {
        FLAT_MOVE_TYPE_SMALL = 0,
        FLAT_MOVE_TYPE_BIG,
        FLAT_MOVE_TYPE_COUNT,
        FLAT_MOVE_TYPE_MAX = UINT8_MAX,
    } FLAT_MOVE_TYPE;

    typedef union flat_move_data_union_u {
        uint64_t code;
        blob big;
    } flat_move_data_union;

    typedef struct flat_move_data_s {
        uint8_t tag;
        flat_move_data_union u;
    } flat_move_data;

    const serialization_layout sl_flat_code[] = {
        {SL_TYPE_U64, offsetof(flat_move_data_union, code)},
        {SL_TYPE_STOP},
    };
    const serialization_layout sl_flat_big[] = {
        {SL_TYPE_BLOB, offsetof(flat_move_data_union, big)},
        {SL_TYPE_STOP},
    };

    const serialization_layout* sl_flat_move_data_map[] = {
        [FLAT_MOVE_TYPE_SMALL] = sl_flat_code,
        [FLAT_MOVE_TYPE_BIG] = sl_flat_big,
    };

    const serialization_layout sl_flat_move_data[] = {
        {SL_TYPE_U8, offsetof(flat_move_data, tag)},
        {
            SL_TYPE_UNION_EXTERNALLY_TAGGED,
            offsetof(flat_move_data, u),
            .ext.un = {
                .tag_size = sizeof(FLAT_MOVE_TYPE),
                .tag_offset = offsetof(flat_move_data, tag),
                .tag_max = FLAT_MOVE_TYPE_COUNT,
                .tag_map = sl_flat_move_data_map,
            },
        },
        {SL_TYPE_STOP},
    };

    flat_move_data fo_in;
    flat_move_data fo_out;

    move_data* cin_p = (move_data*)obj_in;
    move_data* cout_p = (move_data*)obj_out;

    // size, copy, serialize, destroy: obj_in -> flat in
    if (itype == GSIT_SIZE || itype == GSIT_COPY || itype == GSIT_SERIALIZE || itype == GSIT_DESTROY) {
        if (game_e_move_is_big(*cin_p)) {
            fo_in.tag = FLAT_MOVE_TYPE_BIG;
            fo_in.u.big.len = cin_p->cl.len;
            fo_in.u.big.data = (cin_p->cl.len == 0 ? NULL : cin_p->data);
        } else {
            fo_in.tag = FLAT_MOVE_TYPE_SMALL;
            fo_in.u.code = cin_p->cl.code;
        }
    }

    //serialize
    size_t rsize = layout_serializer(itype, sl_flat_move_data, &fo_in, &fo_out, buf, buf_end);
    if (rsize == LS_ERR) {
        return LS_ERR;
    }

    // initzero: flat_in -> obj_in
    if (itype == GSIT_INITZERO) {
        fo_out = fo_in;
        obj_out = obj_in;
    }

    // deserialize, copy: flat_out -> obj_out
    if (itype == GSIT_DESERIALIZE || itype == GSIT_COPY) {
        if (fo_out.tag == FLAT_MOVE_TYPE_BIG) {
            cout_p->cl.len = fo_out.u.big.len;
            cout_p->data = (fo_out.u.big.len == 0 ? PTRMAX : fo_out.u.big.data);
        } else {
            cout_p->cl.code = fo_out.u.code;
            cout_p->data = NULL;
        }
    }

    return rsize;
}

const serialization_layout sl_move_data_sync[] = {
    {SL_TYPE_CUSTOM, offsetof(move_data_sync, md), .ext.serializer = ls_move_data_serializer},
    {SL_TYPE_U64, offsetof(move_data_sync, sync_ctr)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_sync_data[] = {
    {SL_TYPE_U8, offsetof(sync_data, player_c)},
    {SL_TYPE_U8 | SL_TYPE_PTRARRAY, offsetof(sync_data, players), .len.offset = offsetof(sync_data, player_c)},
    {SL_TYPE_BLOB, offsetof(sync_data, b)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_game_init_info_standard[] = {
    {SL_TYPE_STRING, offsetof(game_init_standard, opts)},
    {SL_TYPE_U8, offsetof(game_init_standard, player_count)},
    {SL_TYPE_STRING, offsetof(game_init_standard, env_legacy)},
    {SL_TYPE_STRING | SL_TYPE_PTRARRAY, offsetof(game_init_standard, player_legacies), .len.offset = offsetof(game_init_standard, player_count)},
    {SL_TYPE_STRING, offsetof(game_init_standard, state)},
    {SL_TYPE_U64, offsetof(game_init_standard, sync_ctr)},
    {SL_TYPE_STOP},
};

const serialization_layout sl_game_init_info_serialized[] = {
    {SL_TYPE_BLOB, offsetof(game_init_serialized, b)},
    {SL_TYPE_STOP},
};

const serialization_layout* sl_game_init_info_map[GAME_INIT_SOURCE_TYPE_COUNT] = {
    [GAME_INIT_SOURCE_TYPE_DEFAULT] = NULL,
    [GAME_INIT_SOURCE_TYPE_STANDARD] = sl_game_init_info_standard,
    [GAME_INIT_SOURCE_TYPE_SERIALIZED] = sl_game_init_info_serialized,
};

const serialization_layout sl_game_init_info[] = {
    {SL_TYPE_U8, offsetof(game_init, source_type)},
    {
        SL_TYPE_UNION_EXTERNALLY_TAGGED,
        offsetof(game_init, source),
        .ext.un = {
            .tag_size = sizeof(GAME_INIT_SOURCE_TYPE),
            .tag_offset = offsetof(game_init, source_type),
            .tag_max = GAME_INIT_SOURCE_TYPE_COUNT,
            .tag_map = sl_game_init_info_map,
        },
    },
    {SL_TYPE_STOP},
};

void game_init_create_standard(game_init* init_info, const char* opts, uint8_t player_count, const char* env_legacy, const char* const* player_legacies, const char* state, uint64_t sync_ctr)
{
    *init_info = (game_init){
        .source_type = GAME_INIT_SOURCE_TYPE_STANDARD,
        .source = {
            .standard = {
                .opts = (opts == NULL ? NULL : strdup(opts)),
                .player_count = player_count,
                .env_legacy = (env_legacy == NULL ? NULL : strdup(env_legacy)),
                .player_legacies = NULL,
                .state = (state == NULL ? NULL : strdup(state)),
                .sync_ctr = sync_ctr,
            },
        },
    };
    if (player_legacies != NULL) {
        init_info->source.standard.player_legacies = (const char**)malloc(sizeof(const char*) * player_count);
        for (uint8_t pi = 0; pi < player_count; pi++) {
            init_info->source.standard.player_legacies[pi] = (player_legacies[pi] == NULL ? NULL : strdup(player_legacies[pi]));
        }
    }
}

void game_init_create_serialized(game_init* init_info, blob b)
{
    init_info->source_type = GAME_INIT_SOURCE_TYPE_SERIALIZED;
    blob_create(&init_info->source.serialized.b, b.len);
    if (b.len > 0) {
        memcpy(init_info->source.serialized.b.data, b.data, b.len);
    }
}

const char* game_gname(game* self)
{
    assert(self);
    assert(self->methods);
    return self->methods->game_name;
}

const char* game_vname(game* self)
{
    assert(self);
    assert(self->methods);
    return self->methods->variant_name;
}

const char* game_iname(game* self)
{
    assert(self);
    assert(self->methods);
    return self->methods->impl_name;
}

const semver game_version(game* self)
{
    assert(self);
    assert(self->methods);
    return self->methods->version;
}

const game_feature_flags game_ff(game* self)
{
    assert(self);
    return self->methods->features;
}

const char* game_get_last_error(game* self)
{
    assert(self);
    assert(self->methods);
    return self->methods->get_last_error(self);
}

error_code game_create(game* self, game_init* init_info)
{
    assert(self);
    assert(self->methods);
    assert(init_info);
    assert(!(init_info->source_type == GAME_INIT_SOURCE_TYPE_SERIALIZED && game_ff(self).serializable == false));
    self->data1 = NULL;
    self->data2 = NULL;
    error_code ec = self->methods->create(self, init_info);
    if (init_info->source_type == GAME_INIT_SOURCE_TYPE_DEFAULT) {
        self->sync_ctr = SYNC_CTR_DEFAULT;
    } else if (init_info->source_type == GAME_INIT_SOURCE_TYPE_STANDARD) {
        self->sync_ctr = init_info->source.standard.sync_ctr;
    }
    return ec;
}

error_code game_destroy(game* self)
{
    assert(self);
    assert(self->methods);
    error_code ec = self->methods->destroy(self);
    *self = (game){
        .methods = NULL,
        .data1 = NULL,
        .data2 = NULL,
        .sync_ctr = SYNC_CTR_DEFAULT,
    };
    return ec;
}

error_code game_clone(game* self, game* clone_target)
{
    assert(self);
    assert(self->methods);
    assert(clone_target);
    clone_target->methods = self->methods;
    error_code ec = self->methods->clone(self, clone_target);
    clone_target->sync_ctr = self->sync_ctr;
    return ec;
}

error_code game_copy_from(game* self, game* other)
{
    assert(self);
    assert(self->methods);
    assert(other);
    //TODO want to assert that game_methods are equal?
    error_code ec = self->methods->copy_from(self, other);
    self->sync_ctr = other->sync_ctr;
    return ec;
}

error_code game_compare(game* self, game* other, bool* ret_equal)
{
    assert(self);
    assert(self->methods);
    assert(other);
    assert(ret_equal);
    return self->methods->compare(self, other, ret_equal);
}

error_code game_export_options(game* self, size_t* ret_size, const char** ret_str)
{
    assert(self);
    assert(self->methods);
    assert(game_ff(self).options);
    assert(ret_size);
    assert(ret_str);
    return self->methods->export_options(self, ret_size, ret_str);
}

error_code game_player_count(game* self, uint8_t* ret_count)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    return self->methods->player_count(self, ret_count);
}

error_code game_export_state(game* self, size_t* ret_size, const char** ret_str)
{
    assert(self);
    assert(self->methods);
    assert(ret_size);
    assert(ret_str);
    return self->methods->export_state(self, ret_size, ret_str);
}

error_code game_import_state(game* self, const char* str)
{
    assert(self);
    assert(self->methods);
    assert(str);
    return self->methods->import_state(self, str);
}

error_code game_serialize(game* self, const blob** ret_blob)
{
    assert(self);
    assert(self->methods);
    assert(game_ff(self).serializable);
    assert(ret_blob);
    return self->methods->serialize(self, ret_blob);
}

error_code game_players_to_move(game* self, uint8_t* ret_count, const player_id** ret_players)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(ret_players);
    return self->methods->players_to_move(self, ret_count, ret_players);
}

error_code game_get_concrete_moves(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(ret_moves);
    if (game_e_player_to_move(self, player) == false) {
        return ERR_INVALID_INPUT;
    }
    return self->methods->get_concrete_moves(self, player, ret_count, ret_moves);
}

error_code game_get_concrete_move_probabilities(game* self, uint32_t* ret_count, const float** ret_move_probabilities)
{
    assert(self);
    assert(self->methods);
    assert(game_ff(self).random_moves);
    assert(ret_count);
    assert(ret_move_probabilities);
    if (game_e_player_to_move(self, PLAYER_ENV) == false) {
        return ERR_INVALID_INPUT;
    }
    return self->methods->get_concrete_move_probabilities(self, ret_count, ret_move_probabilities);
}

error_code game_get_random_move(game* self, seed128 seed, move_data_sync** ret_move)
{
    assert(self);
    assert(self->methods);
    assert(game_ff(self).random_moves);
    assert(ret_move);
    if (game_e_player_to_move(self, PLAYER_ENV) == false) {
        return ERR_INVALID_INPUT;
    }
    return self->methods->get_random_move(self, seed, ret_move);
}

error_code game_get_concrete_moves_ordered(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves)
{
    assert(self);
    assert(self->methods);
    assert(game_ff(self).move_ordering);
    assert(ret_count);
    assert(ret_moves);
    if (game_e_player_to_move(self, player) == false) {
        return ERR_INVALID_INPUT;
    }
    return self->methods->get_concrete_moves_ordered(self, player, ret_count, ret_moves);
}

error_code game_get_actions(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves)
{
    assert(self);
    assert(self->methods);
    assert(game_ff(self).action_list && (game_ff(self).hidden_information || game_ff(self).simultaneous_moves));
    assert(ret_count);
    assert(ret_moves);
    if (game_e_player_to_move(self, player) == false) {
        return ERR_INVALID_INPUT;
    }
    return self->methods->get_actions(self, player, ret_count, ret_moves);
}

error_code game_is_legal_move(game* self, player_id player, move_data_sync move)
{
    assert(self);
    assert(self->methods);
    if (game_e_player_to_move(self, player) == false) {
        return ERR_INVALID_INPUT;
    }
    if (self->sync_ctr != move.sync_ctr && game_ff(self).sync_ctr == false) {
        return ERR_SYNC_COUNTER_MISMATCH;
    }
    if (game_ff(self).big_moves == false && game_e_move_is_big(move.md) == true) {
        return ERR_INVALID_INPUT;
    }
    return self->methods->is_legal_move(self, player, move);
}

error_code game_move_to_action(game* self, player_id player, move_data_sync move, uint8_t target_count, const player_id* target_players, move_data_sync** ret_action)
{
    assert(self);
    assert(self->methods);
    assert(game_ff(self).hidden_information || game_ff(self).simultaneous_moves);
    assert(ret_action);
    assert(target_players);
    assert(player != PLAYER_NONE);
    assert(target_count > 0);
    if (self->sync_ctr != move.sync_ctr && game_ff(self).sync_ctr == false) {
        return ERR_SYNC_COUNTER_MISMATCH;
    }
    return self->methods->move_to_action(self, player, move, target_count, target_players, ret_action);
}

error_code game_make_move(game* self, player_id player, move_data_sync move)
{
    assert(self);
    assert(self->methods);
    assert(player != PLAYER_NONE);
    error_code ec = game_is_legal_move(self, player, move);
    if (ec != ERR_OK) {
        return ec;
    }
    ec = self->methods->make_move(self, player, move);
    if (ec == ERR_OK) {
        self->sync_ctr++;
    }
    return ec;
}

error_code game_get_results(game* self, uint8_t* ret_count, const player_id** ret_players)
{
    assert(self);
    assert(self->methods);
    assert(ret_count);
    assert(ret_players);
    return self->methods->get_results(self, ret_count, ret_players);
}

error_code game_export_legacy(game* self, player_id player, size_t* ret_size, const char** ret_str)
{
    assert(self);
    assert(self->methods);
    assert(game_ff(self).legacy);
    assert(ret_size);
    assert(ret_str);
    assert(player != PLAYER_NONE);
    return self->methods->export_legacy(self, player, ret_size, ret_str);
}

error_code game_s_get_legacy_results(game_methods* methods, const char* opts_str, const char* env_legacy, uint16_t player_legacy_count, const char* const* player_legacies, uint16_t* ret_count, const uint16_t** ret_legacy_idxs)
{
    assert(methods);
    assert(opts_str);
    assert(env_legacy);
    assert(player_legacies);
    assert(ret_count);
    assert(ret_legacy_idxs);
    assert(player_legacy_count > 0);
    return methods->s_get_legacy_results(methods, opts_str, env_legacy, player_legacy_count, player_legacies, ret_count, ret_legacy_idxs);
}

error_code game_id(game* self, uint64_t* ret_id)
{
    assert(self);
    assert(self->methods);
    assert(game_ff(self).id);
    assert(ret_id);
    return self->methods->id(self, ret_id);
}

error_code game_eval(game* self, player_id player, float* ret_eval)
{
    assert(self);
    assert(self->methods);
    assert(game_ff(self).eval);
    assert(ret_eval);
    assert(player != PLAYER_NONE && player != PLAYER_ENV);
    return self->methods->eval(self, player, ret_eval);
}

error_code game_discretize(game* self, seed128 seed)
{
    assert(self);
    assert(self->methods);
    assert((game_ff(self).random_moves || game_ff(self).hidden_information || game_ff(self).simultaneous_moves) && game_ff(self).discretize);
    assert(memcmp(&seed, &SEED128_NONE, sizeof(seed128)) != 0);
    return self->methods->discretize(self, seed);
}

error_code game_playout(game* self, seed128 seed)
{
    assert(self);
    assert(self->methods);
    assert(game_ff(self).playout);
    assert(memcmp(&seed, &SEED128_NONE, sizeof(seed128)) != 0);
    return self->methods->playout(self, seed);
}

error_code game_redact_keep_state(game* self, uint8_t count, const player_id* players)
{
    assert(self);
    assert(self->methods);
    assert(game_ff(self).random_moves || game_ff(self).hidden_information || game_ff(self).simultaneous_moves);
    assert(players);
    return self->methods->redact_keep_state(self, count, players);
}

error_code game_export_sync_data(game* self, uint32_t* ret_count, const sync_data** ret_sync_data)
{
    assert(self);
    assert(self->methods);
    assert((game_ff(self).hidden_information || game_ff(self).simultaneous_moves) && game_ff(self).sync_data);
    assert(ret_count);
    assert(ret_sync_data);
    return self->methods->export_sync_data(self, ret_count, ret_sync_data);
}

error_code game_import_sync_data(game* self, blob b)
{
    assert(self);
    assert(self->methods);
    assert((game_ff(self).hidden_information || game_ff(self).simultaneous_moves) && game_ff(self).sync_data);
    assert(!blob_is_null(&b));
    return self->methods->import_sync_data(self, b);
}

error_code game_get_move_data(game* self, player_id player, const char* str, move_data_sync** ret_move)
{
    assert(self);
    assert(self->methods);
    assert(str);
    assert(ret_move);
    assert(player != PLAYER_NONE);
    error_code ec = self->methods->get_move_data(self, player, str, ret_move);
    if (ec == ERR_OK) {
        (*ret_move)->sync_ctr = self->sync_ctr;
    }
    return ec;
}

error_code game_get_move_str(game* self, player_id player, move_data_sync move, size_t* ret_size, const char** ret_str)
{
    assert(self);
    assert(self->methods);
    assert(ret_size);
    assert(ret_str);
    assert(player != PLAYER_NONE);
    if (self->sync_ctr != move.sync_ctr && game_ff(self).simultaneous_moves == false) {
        return ERR_SYNC_COUNTER_MISMATCH;
    }
    return self->methods->get_move_str(self, player, move, ret_size, ret_str);
}

error_code game_print(game* self, size_t* ret_size, const char** ret_str)
{
    assert(self);
    assert(self->methods);
    assert(game_ff(self).print);
    assert(ret_size);
    assert(ret_str);
    return self->methods->print(self, ret_size, ret_str);
}

move_data game_e_create_move_small(move_code move)
{
    return (move_data){.cl.code = move, .data = NULL};
}

move_data game_e_create_move_big(size_t len, uint8_t* buf)
{
    uint8_t* new_data = NULL;
    if (len > 0) {
        new_data = (uint8_t*)malloc(len);
        memcpy(new_data, buf, len);
    } else {
        new_data = PTRMAX;
    }
    return (move_data){.cl.len = len, .data = new_data};
}

move_data_sync game_e_create_move_sync_small(game* self, move_code move)
{
    assert(self);
    assert(self->methods);
    return (move_data_sync){.md = {.cl.code = move, .data = NULL}, .sync_ctr = self->sync_ctr};
}

move_data_sync game_e_create_move_sync_big(game* self, size_t len, uint8_t* buf)
{
    assert(self);
    assert(self->methods);
    uint8_t* new_data = NULL;
    if (len > 0) {
        new_data = (uint8_t*)malloc(len);
        memcpy(new_data, buf, len);
    } else {
        new_data = PTRMAX;
    }
    return (move_data_sync){.md = {.cl.len = len, .data = new_data}, .sync_ctr = self->sync_ctr};
}

move_data_sync game_e_move_make_sync(game* self, move_data move)
{
    assert(self);
    assert(self->methods);
    return (move_data_sync){.md = move, .sync_ctr = self->sync_ctr};
}

bool game_e_move_compare(move_data left, move_data right)
{
    assert(game_e_move_is_big(left) && game_e_move_is_big(right));
    if (game_e_move_is_big(left) == true) {
        return left.cl.len == right.cl.len && memcmp(left.data, right.data, left.cl.len) == 0;
    } else {
        return left.cl.code == right.cl.code;
    }
}

bool game_e_move_sync_compare(move_data_sync left, move_data_sync right)
{
    return game_e_move_compare(left.md, right.md) && left.sync_ctr == right.sync_ctr;
}

bool game_e_move_copy(move_data* target_move, const move_data* source_move)
{
    size_t rs = ls_move_data_serializer(GSIT_COPY, (void*)source_move, target_move, NULL, NULL); // void* cast is fine because copy only reads from obj_in
    return rs != LS_ERR;
}

bool game_e_move_sync_copy(move_data_sync* target_move, const move_data_sync* source_move)
{
    target_move->sync_ctr = source_move->sync_ctr;
    return game_e_move_copy(&target_move->md, &source_move->md);
}

void game_e_move_destroy(move_data move)
{
    if (game_e_move_is_big(move) == true) {
        ls_move_data_serializer(GSIT_DESTROY, &move, NULL, NULL, NULL);
    }
}

void game_e_move_sync_destroy(move_data_sync move)
{
    if (game_e_move_is_big(move.md) == true) {
        layout_serializer(GSIT_DESTROY, sl_move_data_sync, &move, NULL, NULL, NULL);
    }
}

bool game_e_move_is_big(move_data move)
{
    return move.data != NULL;
}

move_data_sync game_e_get_random_move_sync(game* self, seed128 seed)
{
    // get percentage change of random moves
    uint32_t moves_c;
    const float* moves_prob;
    game_get_concrete_move_probabilities(self, &moves_c, &moves_prob);
    assert(moves_c > 0);
    // choose 1 random move via roulette wheel
    float selected_move = get_1d_zto(game_e_seed_rand_intn(seed, UINT32_MAX), 0);
    float move_prob_sum = 0;
    uint32_t move_idx = 0;
    for (; move_idx < moves_c; move_idx++) {
        move_prob_sum += moves_prob[move_idx];
        if (selected_move < move_prob_sum) {
            break;
        }
    }
    if (move_idx == moves_c) {
        move_idx = moves_c - 1; // maybe can possibly get here with some floating point inaccuracies accumulating over time
    }
    // get random moves available
    const move_data* moves;
    game_get_concrete_moves(self, PLAYER_ENV, &moves_c, &moves);
    // make sync, copy, return
    move_data_sync the_move = game_e_move_make_sync(self, moves[move_idx]); // underlying data still owned by the game, do not destroy
    move_data_sync ret_move;
    game_e_move_sync_copy(&ret_move, &the_move);
    return ret_move;
}

bool game_e_player_to_move(game* self, player_id player)
{
    uint8_t ptm_c;
    const player_id* ptm;
    game_players_to_move(self, &ptm_c, &ptm);
    for (uint8_t pidx = 0; pidx < ptm_c; pidx++) {
        if (ptm[pidx] == player) {
            return true;
        }
    }
    return false;
}

uint32_t game_e_seed_rand_intn(seed128 seed, uint32_t n)
{
    // fold the seed into a managable u32 for uintn
    //TODO replace by proper, safer, random
    uint32_t p = 0;
    uint32_t s = 0;
    for (int i = 0; i < 4; i++) {
        uint32_t round_32 = ((uint32_t*)seed.bytes)[i];
        s ^= round_32;
        p = squirrelnoise5(round_32, s);
    }
    return noise_get_uintn(p, s, n);
}

error_code grerror(game* self, error_code ec, const char* str, const char* str_end)
{
    return rerror((char**)&self->data2, ec, str, str_end);
}

error_code grerrorf(game* self, error_code ec, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    error_code ret = rerrorfv((char**)&self->data2, ec, fmt, args);
    va_end(args);
    return ret;
}

error_code grerrorfv(game* self, error_code ec, const char* fmt, va_list args)
{
    return rerrorfv((char**)&self->data2, ec, fmt, args);
}

#ifdef __cplusplus
}
#endif
