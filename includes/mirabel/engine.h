#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rosalia/semver.h"

#include "surena/game.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint64_t SURENA_ENGINE_API_VERSION = 11;

typedef uint32_t eevent_type;

// engine events
enum EE_TYPE {
    // special
    EE_TYPE_NULL = 0,

    EE_TYPE_EXIT,
    EE_TYPE_LOG, // engine outbound, also serves errors
    EE_TYPE_HEARTBEAT, // ==SYNC in/out keepalive check, can ALWAYS be issued and should be answered asap (isready/readyok)
    // game: wrap the supported games
    EE_TYPE_GAME_LOAD, //TODO does the game already have to be loaded into at least the default state?
    EE_TYPE_GAME_UNLOAD, //TODO maybe remove this in favor of load with a null game
    EE_TYPE_GAME_STATE,
    EE_TYPE_GAME_MOVE,
    EE_TYPE_GAME_SYNC,
    EE_TYPE_GAME_DRAW,
    EE_TYPE_GAME_RESIGN, //TODO how often can the engine send draw/resign?
    // engine:
    EE_TYPE_ENGINE_ID,
    EE_TYPE_ENGINE_OPTION,
    EE_TYPE_ENGINE_START, // mirrored by engine without any data
    EE_TYPE_ENGINE_SEARCHINFO,
    EE_TYPE_ENGINE_SCOREINFO,
    EE_TYPE_ENGINE_LINEINFO,
    EE_TYPE_ENGINE_STOP, // engine answers [stop (without data), if stopped] + [searchinfo] + <scoreinfo> + [lineinfo] + <bestmove> + [movescore]
    EE_TYPE_ENGINE_BESTMOVE, // gui->engine (without data): engine sends info like it would on stop, but keeps on searching
    EE_TYPE_ENGINE_MOVESCORE,

    // EE_TYPE_INTERNAL,

    EE_TYPE_COUNT,
};

typedef struct ee_log_s {
    error_code ec;
    char* text;
} ee_log;

typedef struct ee_heartbeat_s {
    uint32_t id; // use this to sync, e.g. after loading a different board
} ee_heartbeat;

typedef struct ee_game_load_s {
    game* the_game;
} ee_game_load;

typedef struct ee_game_state_s {
    char* str;
} ee_game_state;

typedef struct ee_game_move_s {
    player_id player;
    move_code move;
} ee_game_move;

typedef struct ee_game_sync_s {
    void* data_start;
    void* data_end;
} ee_game_sync;

typedef struct ee_game_draw_s {
    bool accept;
} ee_game_draw;

typedef struct ee_engine_id_s {
    char* name;
    char* author;
} ee_engine_id;

typedef uint8_t eevent_option_type;

enum EE_OPTION_TYPE {
    EE_OPTION_TYPE_NONE = 0, // sending an existing name with NONE, removes that option from the interface

    EE_OPTION_TYPE_CHECK,
    EE_OPTION_TYPE_SPIN,
    EE_OPTION_TYPE_COMBO,
    EE_OPTION_TYPE_BUTTON,
    EE_OPTION_TYPE_STRING,
    EE_OPTION_TYPE_SPIND, // float64 spinner
    EE_OPTION_TYPE_U64,

    //TODO add tree group meta options

    EE_OPTION_TYPE_COUNT,
};

typedef struct ee_engine_option_s {
    char* name;
    eevent_option_type type;

    union {
        bool check;
        uint64_t spin;
        char* combo;
        char* str;
        double spind;
        uint64_t u64;
    } value; // this is the default value when sent by the engine, and the new value to use when sent by the gui

    union {
        struct {
            uint64_t min;
            uint64_t max; // iff SPIN, must fullfill assert(max <= UINT64_MAX / 2), U64 does NOT have this restriction
        } mm;

        struct {
            double min;
            double max;
        } mmd;

        struct {
            char* var; // separated by '\0', normal terminator at the end still applies, i.e. ends on \0\0
        } v;
    } l; // limits: min,max / var, only engine outbound
} ee_engine_option;

//TODO replace by game timecontrol

typedef uint8_t time_control_type;

enum TIME_CONTROL_TYPE {
    TIME_CONTROL_TYPE_NONE = 0,

    TIME_CONTROL_TYPE_TIME,
    TIME_CONTROL_TYPE_BONUS, // add a persistent bonus AFTER every move
    TIME_CONTROL_TYPE_DELAY, // transient delay at the start of a move BEFORE the clock starts counting down, unused delay time is lost
    TIME_CONTROL_TYPE_BYO, // available time resets every move, timing out transfers to the next time control, can also be used for correspondence
    TIME_CONTROL_TYPE_UPCOUNT, // time counts upwards

    TIME_CONTROL_TYPE_COUNT,
};

typedef struct time_control_s {
    time_control_type type;
    bool discard_time; // if true: sets the remaining time to 0 before applying this time control
    bool chain; // for byo, if true: this and the previous time controls CHAIN up until a BYO will share a move counter for advancing to the end of the BYO chain
    uint32_t time; // ms
    uint32_t mod; // bonus/delay
    uint32_t moves; // if > 0: moves to next time control
} time_control;

typedef struct time_control_player_s {
    player_id player;
    uint32_t time_ctl_idx;
    uint32_t time; // ms
    uint32_t moves; // already played moves with this time control
} time_control_player;

typedef struct ee_engine_start_s {
    player_id player;
    uint32_t timeout; // in ms
    bool ponder;
    // if time control is used, all time control info has to be sent to the engine, for all players
    uint32_t time_ctl_count;
    time_control* time_ctl;
    uint8_t time_ctl_player_count;
    time_control_player* time_ctl_player;
} ee_engine_start;

typedef uint8_t ee_searchinfo_flags;

enum EE_SEARCHINFO_FLAG_TYPE {
    EE_SEARCHINFO_FLAG_TYPE_TIME = 1 << 0,
    EE_SEARCHINFO_FLAG_TYPE_DEPTH = 1 << 1,
    EE_SEARCHINFO_FLAG_TYPE_SELDEPTH = 1 << 2,
    EE_SEARCHINFO_FLAG_TYPE_NODES = 1 << 3,
    EE_SEARCHINFO_FLAG_TYPE_NPS = 1 << 4,
    EE_SEARCHINFO_FLAG_TYPE_HASHFULL = 1 << 5, // uses just a float [0,1] instead of uci permill
};

typedef struct ee_engine_searchinfo_s {
    ee_searchinfo_flags flags;
    uint32_t time;
    uint32_t depth;
    uint32_t seldepth;
    uint64_t nodes;
    uint64_t nps;
    float hashfull;
} ee_engine_searchinfo;

static const uint32_t EE_SCOREINFO_FORCED_UNKNOWN = UINT32_MAX;

// on search stop / scoreinfo, this must contain one score for every player_to_move (if triggered by bestmove, may only be for one player)
typedef struct ee_engine_scoreinfo_s {
    uint32_t count;
    player_id* score_player;
    float* score_eval;
    uint32_t* forced_end; // EE_SCOREINFO_FORCED_UNKNOWN for unknown, if score_eval is (+/-)INFINITE this means an unknown number of moves instead
} ee_engine_scoreinfo;

typedef struct ee_engine_lineinfo_s {
    // principal variation
    uint32_t idx;
    uint32_t count;
    player_id* player;
    move_code* move;
} ee_engine_lineinfo;

typedef struct ee_engine_stop_s {
    bool all_score_infos; // send scoreinfos for ALL players
    bool all_move_scores;
} ee_engine_stop;

// engine sends count >= 1 if it was instructed to search for multiple players
// must send one for every player_to_move
typedef struct ee_engine_bestmove_s {
    uint32_t count;
    player_id* player;
    move_code* move;
    float* confidence;
} ee_engine_bestmove;

typedef struct ee_engine_movescore_s {
    uint32_t count;
    move_code* move;
    float* eval;
} ee_engine_movescore;

typedef struct engine_event_s {
    eevent_type type;
    uint32_t engine_id;

    union {
        ee_log log;
        ee_heartbeat heartbeat;
        ee_game_load load;
        ee_game_state state;
        ee_game_move move;
        ee_game_sync sync;
        ee_game_draw draw;
        ee_engine_id id;
        ee_engine_option option;
        ee_engine_start start;
        ee_engine_searchinfo searchinfo;
        ee_engine_scoreinfo scoreinfo;
        ee_engine_lineinfo lineinfo;
        ee_engine_stop stop;
        ee_engine_bestmove bestmove;
        ee_engine_movescore movescore;
    };
} engine_event;

//TODO internal event struct?

// eevent_create_* and eevent_set_* methods COPY/CLONE strings/binary/games into the event

//TODO creation bool for if we want bestmove confidence and scoreinfo forced_end or not

void eevent_create(engine_event* e, uint32_t engine_id, eevent_type type);

void eevent_create_log(engine_event* e, uint32_t engine_id, error_code ec, const char* text);

void eevent_create_heartbeat(engine_event* e, uint32_t engine_id, uint32_t heartbeat_id);

void eevent_create_load(engine_event* e, uint32_t engine_id, game* the_game);

void eevent_create_state(engine_event* e, uint32_t engine_id, const char* state);

void eevent_create_move(engine_event* e, uint32_t engine_id, player_id player, move_code move);

void eevent_create_sync(engine_event* e, uint32_t engine_id, void* data_start, void* data_end);

void eevent_create_draw(engine_event* e, uint32_t engine_id, bool accept);

void eevent_create_id(engine_event* e, uint32_t engine_id, const char* name, const char* author);

void eevent_create_option_none(engine_event* e, uint32_t engine_id, const char* name);

void eevent_create_option_check(engine_event* e, uint32_t engine_id, const char* name, bool check);

void eevent_create_option_spin(engine_event* e, uint32_t engine_id, const char* name, uint64_t spin);

void eevent_create_option_spin_mm(engine_event* e, uint32_t engine_id, const char* name, uint64_t spin, uint64_t min, uint64_t max);

void eevent_create_option_combo(engine_event* e, uint32_t engine_id, const char* name, const char* combo);

void eevent_create_option_combo_var(engine_event* e, uint32_t engine_id, const char* name, const char* combo, const char* var);

void eevent_create_option_button(engine_event* e, uint32_t engine_id, const char* name);

void eevent_create_option_string(engine_event* e, uint32_t engine_id, const char* name, const char* str);

void eevent_create_option_spind(engine_event* e, uint32_t engine_id, const char* name, double spin);

void eevent_create_option_spind_mmd(engine_event* e, uint32_t engine_id, const char* name, double spin, double min, double max);

void eevent_create_option_u64(engine_event* e, uint32_t engine_id, const char* name, uint64_t u64);

void eevent_create_option_u64_mm(engine_event* e, uint32_t engine_id, const char* name, uint64_t u64, uint64_t min, uint64_t max);

void eevent_create_start_empty(engine_event* e, uint32_t engine_id);

void eevent_create_start(engine_event* e, uint32_t engine_id, player_id player, uint32_t timeout, bool ponder, uint32_t time_ctl_count, uint8_t time_ctl_player_count);

void eevent_create_searchinfo(engine_event* e, uint32_t engine_id);

void eevent_set_searchinfo_time(engine_event* e, uint32_t time);

void eevent_set_searchinfo_depth(engine_event* e, uint32_t depth);

void eevent_set_searchinfo_seldepth(engine_event* e, uint32_t seldepth);

void eevent_set_searchinfo_nodes(engine_event* e, uint64_t nodes);

void eevent_set_searchinfo_nps(engine_event* e, uint64_t nps);

void eevent_set_searchinfo_hashfull(engine_event* e, float hashfull);

void eevent_create_scoreinfo(engine_event* e, uint32_t engine_id, uint32_t count);

void eevent_create_lineinfo(engine_event* e, uint32_t engine_id, uint32_t pv_idx, uint32_t count);

void eevent_create_stop_empty(engine_event* e, uint32_t engine_id);

void eevent_create_stop(engine_event* e, uint32_t engine_id, bool all_score_infos, bool all_move_scores);

void eevent_create_bestmove_empty(engine_event* e, uint32_t engine_id);

void eevent_create_bestmove(engine_event* e, uint32_t engine_id, uint32_t count);

void eevent_create_movescore(engine_event* e, uint32_t engine_id, uint32_t count);

void eevent_destroy(engine_event* e);

typedef struct eevent_queue_s {
    char _padding[168];
} eevent_queue;

// the queue takes ownership of everything in the event and resets it to type NULL

void eevent_queue_create(eevent_queue* eq);

void eevent_queue_destroy(eevent_queue* eq);

void eevent_queue_push(eevent_queue* eq, engine_event* e);

void eevent_queue_pop(eevent_queue* eq, engine_event* e, uint32_t t);

typedef struct engine_feature_flags_s {
    bool error_strings : 1;
    bool options : 1;
    bool score_all_moves : 1; // if not supported and requested in stop event, ignore
    bool running_bestmove : 1; // if not supported, log error if engine receives bestmove
    bool draw_and_resign : 1; // if not supported the handler must auto decline draw offers on behalf of the engine
} engine_feature_flags;

typedef struct engine_s engine;

typedef struct engine_methods_s {

    // minimum length of 1 character, with allowed character set:
    // "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" additionally '-' and '_' but not at the start or end
    const char* engine_name;
    const semver version;
    const engine_feature_flags features; // these will never change depending on options

    // the frontend method specific internal method struct, NULL if not available
    // use the engine_name to make sure you know what this will be
    const void* internal_methods;

    // FEATURE: error_strings
    // returns the error string complementing the most recent occured error
    // returns NULL if there is error string available for this error
    // the string is still owned by the frontend method backend, do not free it
    const char* (*get_last_error)(engine* self);

    // sets outbox and inbox to the corresponding queues owned by the engine
    // use inbox to send things to the engine, check outbox for eevents from the engine
    error_code (*create)(engine* self, uint32_t engine_id, eevent_queue* outbox, eevent_queue** inbox, const char* opts);

    error_code (*destroy)(engine* self);

    error_code (*is_game_compatible)(engine* self, game* compat_game); //TODO should this even take the engine at all?

} engine_methods;

struct engine_s {
    const engine_methods* methods;
    uint32_t engine_id;
    void* data1;
    void* data2;
};

#ifdef __cplusplus
}
#endif
