#include "mirabel/game.h"

#ifndef MIRABEL_GDD_BENAME
#error "mirabel game decldef requires a backend name"
#endif

#ifndef MIRABEL_GDD_GNAME
#error "mirabel game decldef requires a game name"
#endif

#ifndef MIRABEL_GDD_VNAME
#error "mirabel game decldef requires a variant name"
#endif

#ifndef MIRABEL_GDD_INAME
#error "mirabel game decldef requires an implementation name"
#endif

#ifndef MIRABEL_GDD_VERSION
#error "mirabel game decldef requires a game version"
#endif

#ifndef MIRABEL_GDD_INTERNALS
#error "mirabel game decldef requires internals"
#endif

#ifdef MIRABEL_GDD_FFB_ERROR_STRINGS
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_ERROR_STRINGS"
#endif

#ifdef MIRABEL_GDD_FFB_OPTIONS
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_OPTIONS"
#endif

#ifdef MIRABEL_GDD_FFB_SERIALIZABLE
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_SERIALIZABLE"
#endif

#ifdef MIRABEL_GDD_FFB_LEGACY
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_LEGACY"
#endif

#ifdef MIRABEL_GDD_FFB_RANDOM_MOVES
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_RANDOM_MOVES"
#endif

#ifdef MIRABEL_GDD_FFB_HIDDEN_INFORMATION
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_HIDDEN_INFORMATION"
#endif

#ifdef MIRABEL_GDD_FFB_SYNC_DATA
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_SYNC_DATA"
#endif

#ifdef MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES"
#endif

#ifdef MIRABEL_GDD_FFB_SYNC_CTR
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_SYNC_CTR"
#endif

#ifdef MIRABEL_GDD_FFB_BIG_MOVES
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_BIG_MOVES"
#endif

#ifdef MIRABEL_GDD_FFB_MOVE_ORDERING
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_MOVE_ORDERING"
#endif

#ifdef MIRABEL_GDD_FFB_ID
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_ID"
#endif

#ifdef MIRABEL_GDD_FFB_EVAL
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_EVAL"
#endif

#ifdef MIRABEL_GDD_FFB_ACTION_LIST
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_ACTION_LIST"
#endif

#ifdef MIRABEL_GDD_FFB_DISCRETIZE
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_DISCRETIZE"
#endif

#ifdef MIRABEL_GDD_FFB_PLAYOUT
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_PLAYOUT"
#endif

#ifdef MIRABEL_GDD_FFB_PRINT
#error "mirabel gdd internal feature flag bool already defined: MIRABEL_GDD_FFB_PRINT"
#endif

#ifndef MIRABEL_GDD_FF_ERROR_STRINGS
#define MIRABEL_GDD_FFB_ERROR_STRINGS false
#else
#define MIRABEL_GDD_FFB_ERROR_STRINGS true
#endif

#ifndef MIRABEL_GDD_FF_OPTIONS
#define MIRABEL_GDD_FFB_OPTIONS false
#else
#define MIRABEL_GDD_FFB_OPTIONS true
#endif

#ifndef MIRABEL_GDD_FF_SERIALIZABLE
#define MIRABEL_GDD_FFB_SERIALIZABLE false
#else
#define MIRABEL_GDD_FFB_SERIALIZABLE true
#endif

#ifndef MIRABEL_GDD_FF_LEGACY
#define MIRABEL_GDD_FFB_LEGACY false
#else
#define MIRABEL_GDD_FFB_LEGACY true
#endif

#ifndef MIRABEL_GDD_FF_RANDOM_MOVES
#define MIRABEL_GDD_FFB_RANDOM_MOVES false
#else
#define MIRABEL_GDD_FFB_RANDOM_MOVES true
#endif

#ifndef MIRABEL_GDD_FF_HIDDEN_INFORMATION
#define MIRABEL_GDD_FFB_HIDDEN_INFORMATION false
#else
#define MIRABEL_GDD_FFB_HIDDEN_INFORMATION true
#endif

#ifndef MIRABEL_GDD_FF_SYNC_DATA
#define MIRABEL_GDD_FFB_SYNC_DATA false
#else
#define MIRABEL_GDD_FFB_SYNC_DATA true
#endif

#ifndef MIRABEL_GDD_FF_SIMULTANEOUS_MOVES
#define MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES false
#else
#define MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES true
#endif

#ifndef MIRABEL_GDD_FF_SYNC_CTR
#define MIRABEL_GDD_FFB_SYNC_CTR false
#else
#define MIRABEL_GDD_FFB_SYNC_CTR true
#endif

#ifndef MIRABEL_GDD_FF_BIG_MOVES
#define MIRABEL_GDD_FFB_BIG_MOVES false
#else
#define MIRABEL_GDD_FFB_BIG_MOVES true
#endif

#ifndef MIRABEL_GDD_FF_MOVE_ORDERING
#define MIRABEL_GDD_FFB_MOVE_ORDERING false
#else
#define MIRABEL_GDD_FFB_MOVE_ORDERING true
#endif

#ifndef MIRABEL_GDD_FF_ID
#define MIRABEL_GDD_FFB_ID false
#else
#define MIRABEL_GDD_FFB_ID true
#endif

#ifndef MIRABEL_GDD_FF_EVAL
#define MIRABEL_GDD_FFB_EVAL false
#else
#define MIRABEL_GDD_FFB_EVAL true
#endif

#ifndef MIRABEL_GDD_FF_ACTION_LIST
#define MIRABEL_GDD_FFB_ACTION_LIST false
#else
#define MIRABEL_GDD_FFB_ACTION_LIST true
#endif

#ifndef MIRABEL_GDD_FF_DISCRETIZE
#define MIRABEL_GDD_FFB_DISCRETIZE false
#else
#define MIRABEL_GDD_FFB_DISCRETIZE true
#endif

#ifndef MIRABEL_GDD_FF_PLAYOUT
#define MIRABEL_GDD_FFB_PLAYOUT false
#else
#define MIRABEL_GDD_FFB_PLAYOUT true
#endif

#ifndef MIRABEL_GDD_FF_PRINT
#define MIRABEL_GDD_FFB_PRINT false
#else
#define MIRABEL_GDD_FFB_PRINT true
#endif

#if MIRABEL_GDD_FFB_ERROR_STRINGS
static get_last_error_gf_t get_last_error_gf;
#endif
static create_gf_t create_gf;
static destroy_gf_t destroy_gf;
static clone_gf_t clone_gf;
static copy_from_gf_t copy_from_gf;
#if MIRABEL_GDD_FFB_OPTIONS
static export_options_gf_t export_options_gf;
#endif
static player_count_gf_t player_count_gf;
static export_state_gf_t export_state_gf;
static import_state_gf_t import_state_gf;
#if MIRABEL_GDD_FFB_SERIALIZABLE
static serialize_gf_t serialize_gf;
#endif
static players_to_move_gf_t players_to_move_gf;
static get_concrete_moves_gf_t get_concrete_moves_gf;
#if MIRABEL_GDD_FFB_RANDOM_MOVES
static get_concrete_move_probabilities_gf_t get_concrete_move_probabilities_gf;
#endif
#if MIRABEL_GDD_FFB_RANDOM_MOVES
static get_random_move_gf_t get_random_move_gf;
#endif
#if MIRABEL_GDD_FFB_MOVE_ORDERING
static get_concrete_moves_ordered_gf_t get_concrete_moves_ordered_gf;
#endif
#if MIRABEL_GDD_FFB_ACTION_LIST && (MIRABEL_GDD_FFB_HIDDEN_INFORMATION || MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES)
static get_actions_gf_t get_actions_gf;
#endif
static is_legal_move_gf_t is_legal_move_gf;
#if MIRABEL_GDD_FFB_HIDDEN_INFORMATION || MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES
static move_to_action_gf_t move_to_action_gf;
#endif
static make_move_gf_t make_move_gf;
static get_results_gf_t get_results_gf;
#if MIRABEL_GDD_FFB_LEGACY
static export_legacy_gf_t export_legacy_gf;
#endif
#if MIRABEL_GDD_FFB_LEGACY
static get_legacy_results_sgf_t get_legacy_results_sgf;
#endif
#if MIRABEL_GDD_FFB_ID
static id_gf_t id_gf;
#endif
#if MIRABEL_GDD_FFB_EVAL
static eval_gf_t eval_gf;
#endif
#if (MIRABEL_GDD_FFB_RANDOM_MOVES || MIRABEL_GDD_FFB_HIDDEN_INFORMATION || MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES) && MIRABEL_GDD_FFB_DISCRETIZE
static discretize_gf_t discretize_gf;
#endif
#if MIRABEL_GDD_FFB_PLAYOUT
static playout_gf_t playout_gf;
#endif
#if MIRABEL_GDD_FFB_RANDOM_MOVES || MIRABEL_GDD_FFB_HIDDEN_INFORMATION || MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES
static redact_keep_state_gf_t redact_keep_state_gf;
#endif
#if (MIRABEL_GDD_FFB_HIDDEN_INFORMATION || MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES) && MIRABEL_GDD_FFB_SYNC_DATA
static export_sync_data_gf_t export_sync_data_gf;
#endif
#if (MIRABEL_GDD_FFB_HIDDEN_INFORMATION || MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES) && MIRABEL_GDD_FFB_SYNC_DATA
static import_sync_data_gf_t import_sync_data_gf;
#endif
static get_move_data_gf_t get_move_data_gf;
static get_move_str_gf_t get_move_str_gf;
#if MIRABEL_GDD_FFB_PRINT
static print_gf_t print_gf;
#endif

// clang-format off
const game_methods MIRABEL_GDD_BENAME
{
    .game_name = (MIRABEL_GDD_GNAME),
    .variant_name = (MIRABEL_GDD_VNAME),
    .impl_name = (MIRABEL_GDD_INAME),
    .version = (MIRABEL_GDD_VERSION),
    .features = game_feature_flags{
        .error_strings = MIRABEL_GDD_FFB_ERROR_STRINGS,
        .options = MIRABEL_GDD_FFB_OPTIONS,
        .serializable = MIRABEL_GDD_FFB_SERIALIZABLE,
        .legacy = MIRABEL_GDD_FFB_LEGACY,
        .random_moves = MIRABEL_GDD_FFB_RANDOM_MOVES,
        .hidden_information = MIRABEL_GDD_FFB_HIDDEN_INFORMATION,
        .sync_data = MIRABEL_GDD_FFB_SYNC_DATA,
        .simultaneous_moves = MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES,
        .sync_ctr = MIRABEL_GDD_FFB_SYNC_CTR,
        .move_ordering = MIRABEL_GDD_FFB_MOVE_ORDERING,
        .id = MIRABEL_GDD_FFB_ID,
        .eval = MIRABEL_GDD_FFB_EVAL,
        .action_list = MIRABEL_GDD_FFB_ACTION_LIST,
        .discretize = MIRABEL_GDD_FFB_DISCRETIZE,
        .playout = MIRABEL_GDD_FFB_PLAYOUT,
        .print = MIRABEL_GDD_FFB_PRINT,
    },
    .internal_methods = (void*)(MIRABEL_GDD_INTERNALS),
#if MIRABEL_GDD_FFB_ERROR_STRINGS
    .get_last_error = get_last_error_gf,
#else
    .get_last_error = NULL,
#endif
    .create = create_gf,
    .destroy = destroy_gf,
    .clone = clone_gf,
    .copy_from = copy_from_gf,
#if MIRABEL_GDD_FFB_OPTIONS
    .export_options = export_options_gf,
#else
    .export_options = NULL,
#endif
    .player_count = player_count_gf,
    .export_state = export_state_gf,
    .import_state = import_state_gf,
#if MIRABEL_GDD_FFB_SERIALIZABLE
    .serialize = serialize_gf,
#else
    .serialize = NULL,
#endif
    .players_to_move = players_to_move_gf,
    .get_concrete_moves = get_concrete_moves_gf,
#if MIRABEL_GDD_FFB_RANDOM_MOVES
    .get_concrete_move_probabilities = get_concrete_move_probabilities_gf,
#else
    .get_concrete_move_probabilities = NULL,
#endif
#if MIRABEL_GDD_FFB_RANDOM_MOVES
    .get_random_move = get_random_move_gf,
#else
    .get_random_move = NULL,
#endif
#if MIRABEL_GDD_FFB_MOVE_ORDERING
    .get_concrete_moves_ordered = get_concrete_moves_ordered_gf,
#else
    .get_concrete_moves_ordered = NULL,
#endif
#if MIRABEL_GDD_FFB_ACTION_LIST && (MIRABEL_GDD_FFB_HIDDEN_INFORMATION || MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES)
    .get_actions = get_actions_gf,
#else
    .get_actions = NULL,
#endif
    .is_legal_move = is_legal_move_gf,
#if MIRABEL_GDD_FFB_HIDDEN_INFORMATION || MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES
    .move_to_action = move_to_action_gf,
#else
    .move_to_action = NULL,
#endif
    .make_move = make_move_gf,
    .get_results = get_results_gf,
#if MIRABEL_GDD_FFB_LEGACY
    .export_legacy = export_legacy_gf,
#else
    .export_legacy = NULL,
#endif
#if MIRABEL_GDD_FFB_LEGACY
    .s_get_legacy_results = get_legacy_results_sgf,
#else
    .s_get_legacy_results = NULL,
#endif
#if MIRABEL_GDD_FFB_ID
    .id = id_gf,
#else
    .id = NULL,
#endif
#if MIRABEL_GDD_FFB_EVAL
    .eval = eval_gf,
#else
    .eval = NULL,
#endif
#if (MIRABEL_GDD_FFB_RANDOM_MOVES || MIRABEL_GDD_FFB_HIDDEN_INFORMATION || MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES) && MIRABEL_GDD_FFB_DISCRETIZE
    .discretize = discretize_gf,
#else
    .discretize = NULL,
#endif
#if MIRABEL_GDD_FFB_PLAYOUT
    .playout = playout_gf,
#else
    .playout = NULL,
#endif
#if MIRABEL_GDD_FFB_RANDOM_MOVES || MIRABEL_GDD_FFB_HIDDEN_INFORMATION || MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES
    .redact_keep_state = redact_keep_state_gf,
#else
    .redact_keep_state = NULL,
#endif
#if (MIRABEL_GDD_FFB_HIDDEN_INFORMATION || MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES) && MIRABEL_GDD_FFB_SYNC_DATA
    .export_sync_data = export_sync_data_gf,
#else
    .export_sync_data = NULL,
#endif
#if (MIRABEL_GDD_FFB_HIDDEN_INFORMATION || MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES) && MIRABEL_GDD_FFB_SYNC_DATA
    .import_sync_data = import_sync_data_gf,
#else
    .import_sync_data = NULL,
#endif
    .get_move_data = get_move_data_gf,
    .get_move_str = get_move_str_gf,
#if MIRABEL_GDD_FFB_PRINT
    .print = print_gf,
#else
    .print = NULL,
#endif
};
    // clang-format on

#undef MIRABEL_GDD_BENAME
#undef MIRABEL_GDD_GNAME
#undef MIRABEL_GDD_VNAME
#undef MIRABEL_GDD_INAME
#undef MIRABEL_GDD_VERSION
#undef MIRABEL_GDD_INTERNALS

#undef MIRABEL_GDD_FF_ERROR_STRINGS
#undef MIRABEL_GDD_FFB_ERROR_STRINGS

#undef MIRABEL_GDD_FF_OPTIONS
#undef MIRABEL_GDD_FFB_OPTIONS

#undef MIRABEL_GDD_FF_SERIALIZABLE
#undef MIRABEL_GDD_FFB_SERIALIZABLE

#undef MIRABEL_GDD_FF_LEGACY
#undef MIRABEL_GDD_FFB_LEGACY

#undef MIRABEL_GDD_FF_RANDOM_MOVES
#undef MIRABEL_GDD_FFB_RANDOM_MOVES

#undef MIRABEL_GDD_FF_HIDDEN_INFORMATION
#undef MIRABEL_GDD_FFB_HIDDEN_INFORMATION

#undef MIRABEL_GDD_FF_SYNC_DATA
#undef MIRABEL_GDD_FFB_SYNC_DATA

#undef MIRABEL_GDD_FF_SIMULTANEOUS_MOVES
#undef MIRABEL_GDD_FFB_SIMULTANEOUS_MOVES

#undef MIRABEL_GDD_FF_SYNC_CTR
#undef MIRABEL_GDD_FFB_SYNC_CTR

#undef MIRABEL_GDD_FF_BIG_MOVES
#undef MIRABEL_GDD_FFB_BIG_MOVES

#undef MIRABEL_GDD_FF_MOVE_ORDERING
#undef MIRABEL_GDD_FFB_MOVE_ORDERING

#undef MIRABEL_GDD_FF_ID
#undef MIRABEL_GDD_FFB_ID

#undef MIRABEL_GDD_FF_EVAL
#undef MIRABEL_GDD_FFB_EVAL

#undef MIRABEL_GDD_FF_ACTION_LIST
#undef MIRABEL_GDD_FFB_ACTION_LIST

#undef MIRABEL_GDD_FF_DISCRETIZE
#undef MIRABEL_GDD_FFB_DISCRETIZE

#undef MIRABEL_GDD_FF_PLAYOUT
#undef MIRABEL_GDD_FFB_PLAYOUT

#undef MIRABEL_GDD_FF_PRINT
#undef MIRABEL_GDD_FFB_PRINT
