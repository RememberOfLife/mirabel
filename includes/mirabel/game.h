#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rosalia/semver.h"
#include "rosalia/serialization.h"
#include "rosalia/timestamp.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint64_t MIRABEL_GAME_API_VERSION = 36;

typedef uint32_t error_code;

// general purpose error codes
enum ERR {
    ERR_OK = 0,
    ERR_NOK,
    ERR_STATE_UNRECOVERABLE,
    ERR_STATE_CORRUPTED,
    ERR_OUT_OF_MEMORY,
    ERR_FEATURE_UNSUPPORTED,
    ERR_MISSING_HIDDEN_STATE,
    ERR_INVALID_PLAYER_COUNT,
    ERR_INVALID_INPUT,
    ERR_INVALID_PLAYER,
    ERR_INVALID_MOVE,
    ERR_INVALID_OPTIONS,
    ERR_INVALID_LEGACY,
    ERR_INVALID_STATE,
    ERR_UNENUMERABLE,
    ERR_UNSTABLE_POSITION,
    ERR_SYNC_COUNTER_MISMATCH,
    ERR_SYNC_COUNTER_IMPOSSIBLE_REORDER,
    ERR_RETRY, // retrying the same call again may yet still work
    ERR_CUSTOM_ANY, // unspecified custom error, check get_last_error for a detailed string
    ERR_ENUM_DEFAULT_OFFSET, // not an error, start game method specific error enums at this offset
};

// returns not_general if the err is not a general error
const char* get_general_error_string(error_code err, const char* fallback);
// instead of returning an error code, one can return rerror(f,vf) which automatically manages fmt string buffer allocation for the error string
// call rerrorf or rerrorfv with fmt(or str)=NULL to free (*pbuf) (does not work on rerror)
error_code rerror(char** pbuf, error_code ec, const char* str, const char* str_end);
error_code rerrorf(char** pbuf, error_code ec, const char* fmt, ...);
error_code rerrorfv(char** pbuf, error_code ec, const char* fmt, va_list args);

typedef struct seed128_s {
    uint8_t bytes[16];
} seed128;

// anywhere a rng seed is use, SEED_NONE represents not using the rng
static const seed128 SEED128_NONE = (seed128){.bytes = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

// moves represent state transitions on the game board (and its internal state)
// actions represent sets of moves, i.e. sets of concrete moves (action instances / informed moves)
// every action can be encoded as a move
// a move that encodes an action is part of exactly that action set
// every concrete move can be encoded as a move
// every concrete move can be reduced to an action, i.e. a move
// e.g. flipping a coin is an action (i.e. flip the coin) whereupon the PLAYER_ENV decides the outcome of the flip
// e.g. laying down a hidden hand card facedown (i.e. keeping it hidden) is informed to other players through the action of playing *some card* from hand facedown, but the player doing it chooses one of the concrete moves specifying which card to play (as they can see their hand)
//TODO better name for "concrete_move"? maybe action instance / informed move

typedef uint64_t move_code;

// this can not just be a union over move_code and blob, because ls_move_data_serializer needs to be able to serialize this without the game as context
typedef struct move_data_s {
    union {
        move_code code; // FEATURE: !big_moves ; all values are moves
        size_t len; // FEATURE: big_moves ; if data is not NULL this must be >0
    } cl;

    // this functions as a tag for the cl union
    uint8_t* data; // MUST always be NULL for non big moves; if not NULL then this is a big move
} move_data; // unindexed move realization, switch by big_move feature flag

// NOTE: there exists no empty move! every move_data is *some* move
//TODO add ability to drop moves without using empty moves in the future

custom_serializer_t ls_move_data_serializer;

typedef struct move_data_sync_s {
    move_data md; // see unindexed move move_data
    uint64_t sync_ctr; // sync ctr from where this move is intended to be "made" (on the game) //TODO might rename to move_ctr
} move_data_sync;

extern const serialization_layout sl_move_data_sync[];

static const uint64_t SYNC_CTR_DEFAULT = 0; // this is already a valid sync ctr value (usually first for a fresh game)

//TODO !maybe! force every move to also include a player + move_to_player in the game_methods

typedef uint8_t player_id;
static const player_id PLAYER_NONE = 0x00;
static const player_id PLAYER_ENV = 0xFF; // this player moves random moves and forced moves

// random move: e.g. result of a dice roll
// forced move: e.g. showing another player a hidden card in your hand because they may peek
// forced moves are optional, but for very many games make the implementation easier because it can do what sync data normally does

typedef struct game_feature_flags_s {

    bool error_strings : 1;

    // options are passed together with the creation of the game data
    bool options : 1;

    // this is a binary serialization of the game, must be absolutely accurate representation of the game state
    bool serializable : 1;

    bool legacy : 1;

    // if a board has a seed then if any random moves happen there will only ever be one move available (the rigged one)
    // a board can be seeded at any time using the discretize feature
    // when a non seeded board offers random moves then all possible moves are available (according to the gathered info)
    // if the available random moves are too many to count, then instead they are unenumerable and get_random_move must be used
    // same result moves are treated as the same, i.e. 2 dice rolled at once will only offer 12 moves for their sum
    // probability distribution of the random moves is offered in get_concrete_move_probabilities
    bool random_moves : 1;

    bool hidden_information : 1;

    // FEATURE: hidden_information || simultaneous_moves
    // enable to signal that this game uses sync data
    // not all games with hidden information require/want to use the sync_data protocol
    bool sync_data : 1;

    // remember: the game owner must guarantee total move order on all moves!
    bool simultaneous_moves : 1;

    // FEATURE: simultaneous_moves
    // if this is used, the game manages the sync_ctr itself, and enables commutative simultaneous moves when it wants to
    bool sync_ctr : 1;

    // in a game that uses this feature flag, moves can be both small AND big moves
    // big moves use move_data.big, otherwise use move_data.code
    // see the appropriate helper functions for interacting with ambiguous moves
    bool big_moves : 1;

    bool move_ordering : 1;

    bool id : 1;

    bool eval : 1;

    // FEATURE: hidden_information || simultaneous_moves
    bool action_list : 1;

    // FEATURE: random_moves || hidden_information || simultaneous_moves
    bool discretize : 1;

    bool playout : 1;

    bool print : 1;

    // bool time : 1;

} game_feature_flags;

typedef struct sync_data_s {
    uint64_t player_c;
    uint8_t* players;
    blob b;
} sync_data;

extern const serialization_layout sl_sync_data[];

typedef enum GAME_INIT_SOURCE_TYPE_E {
    GAME_INIT_SOURCE_TYPE_DEFAULT = 0, // create a default game with the default options and default initial state
    GAME_INIT_SOURCE_TYPE_STANDARD, // create a game from some options, legacy and initial state
    GAME_INIT_SOURCE_TYPE_SERIALIZED, // (re)create a game from a serialization buffer
    GAME_INIT_SOURCE_TYPE_COUNT,
    GAME_INIT_SOURCE_TYPE_SIZE_MAX = UINT8_MAX,
} GAME_INIT_SOURCE_TYPE;

//TODO here, and in general, might want to remove some consts so a game_init can be kept in memory for editing, even through all the const-ness?

typedef struct game_init_standard_s {
    char* opts; // FEATURE: options ; may be NULL to use default
    uint64_t player_count; // use 0 to let the game initialize to default
    char* env_legacy; // FEATURE: legacy ; may be NULL to use default
    char** player_legacies; // FEATURE: legacy ; this has len of player_count, each legacy can be NULL to use default, legacies are supplied in order for player ids 1 to player_count, is player_legacies is NULL then all players are assumed to have a NULL legacy used here
    char* state; // may be null to use default
    uint64_t sync_ctr; // ignore this if you don't need it, otherwise this is used by the wrapper to set the initial sync ctr
} game_init_standard;

typedef struct game_init_serialized_s {
    // beware that the data given through serialized is UNTRUSTED and should be thouroughly checked for consistency
    blob b;
} game_init_serialized;

typedef struct game_init_s {
    GAME_INIT_SOURCE_TYPE source_type;

    union {
        game_init_standard standard; // use options, legacy, initial_state
        game_init_serialized serialized; // FEATURE: serialize ; use the given byte buffer to create the game data, NULL buffers are invalid
    } source;
} game_init;

extern const serialization_layout sl_game_init_info[];

void game_init_create_standard(game_init* init_info, const char* opts, uint8_t player_count, const char* env_legacy, const char* const* player_legacies, const char* state, uint64_t sync_ctr);

void game_init_create_serialized(game_init* init_info, blob b);

typedef struct timectlstage_s timectlstage; //TODO better name?

typedef struct game_s game; // forward declare the game for the game methods
typedef struct game_methods_s game_methods;

///////
// game method functions, usage comments are by the typedefs where the arguments are

// client-server multiplay move protocol:
// * client: sync board and play board
//   * play move on play board and send to server (it will only become valid on the sync board once the server sends it back)
//   * everything that arrives from server is made on sync board, if play board is desynced then reset it using the sync board
// * server: on move from client
//   * check move is legal -> convert move to action -> make move -> get sync data if any
//   * send to every player (in this order!!): sync data for all players this client serves + the action made (except the player who made the action, they get the true move)
// * client: on (sync data +) move from server
//   * import sync data to sync board
//   * make move on sync board
//   * (if req'd rollback and) update play board to be up to date with sync board
// * to send just sync data to other players without noticing, make a move that maps to the null move action, it gets dropped by the server
//   * //TODO alternative to this is to keep a sync ctr offset for every client, maybe do this in the future

// FEATURE: error_strings
// returns the error string complementing the most recent occured error (i.e. only available if != ERR_OK)
// returns NULL if there is no error string available for this error
// the string is still owned by the game method backend, do not free it
typedef const char* get_last_error_gf_t(game* self);

// construct and initialize a new game specific data object into self
// only one game can be created into a self at any one time (destroy it again to reuse with create)
// every create MUST ALWAYS be matched with a call to destroy
// !!! even if create fails, the game has to be destroyed before retrying create or releasing the self
// get_last_error will, if supported, be valid to check even if create fails (which is why destroy is required)
// the init_info provides details on what source is used, if any, and details about that source
// after creation, if successful, the game is always left in a valid and ready to use state
// the init_info is only read by the game, it is still owned externally, and no references are created during create
typedef error_code create_gf_t(game* self, game_init* init_info);

// deconstruct and release any (complex) game specific data, if it has been created already
// same for options specific data, if it exists
// even if this fails, there will never be an error string available (should generally only fail on corrupt games anyway)
typedef error_code destroy_gf_t(game* self);

// fills clone_target with a deep clone of the self game state
// undefined behaviour if self == clone_target
typedef error_code clone_gf_t(game* self, game* clone_target);

// deep clone the game state of other into self
// other is restricted to already created games using the same options, otherwise undefined behaviour
// undefined behaviour if self == other
typedef error_code copy_from_gf_t(game* self, game* other);

// FEATURE: options
// write this games options to a universal options string and returns a read only pointer to it
// returns the length of the options string written, excluding null character
// the options string encodes as much information as this game knows
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code export_options_gf_t(game* self, size_t* ret_size, const char** ret_str);

// returns the number of players participating in this game
typedef error_code player_count_gf_t(game* self, uint8_t* ret_count);

// writes the game state to a universal state string and returns a read only pointer to it
// returns the length of the state string written, excluding null character
// the state string encodes as much information as this game knows
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code export_state_gf_t(game* self, size_t* ret_size, const char** ret_str);

// load the game state from the given string, beware this may be called on running games
// if str is NULL then the initial position is loaded
// errors while parsing are handled (NOTE: avoid crashes)
// when this fails self is left unmodified from its previous state
typedef error_code import_state_gf_t(game* self, const char* str);

// FEATURE: serializable
// writes the game state and options to a game specific raw byte representation that is absolutely accurate to the state of the game and returns a read only pointer to it
// the serialization encodes as much information as this game knows
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
// considerations: the serialization contains the: sync_ctr, options, legacy, state, internals
typedef error_code serialize_gf_t(game* self, const blob** ret_blob);

// writes the player ids to move from this state and returns a read only pointer to them
// writes PLAYER_ENV if the current move branch is decided by randomness or the games pre determined state (i.e. forced move)
// writes no ids if the game is over
// returns the number of ids written
// written player ids are ordered from lowest to highest
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code players_to_move_gf_t(game* self, uint8_t* ret_count, const player_id** ret_players);

// writes the available moves for the player from this position and returns a read only pointer to them
// writes no moves if the game is over or the player is not to move
// if the game uses moves at this position, which can not be feasibly listed it can return ERR_UNENUMERABLE to signal this
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code get_concrete_moves_gf_t(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves);

// FEATURE: random_moves
// writes the probabilities [0,1] of each avilable move in get_concrete_moves(player=PLAYER_ENV) and returns a read only pointer to them (SUM=1)
// order is the same as get_concrete_moves
// only available if the the ptm is PLAYER_ENV
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code get_concrete_move_probabilities_gf_t(game* self, uint32_t* ret_count, const float** ret_move_probabilities);

// FEATURE: random_moves
// returns a legal random move from this position, for the PLAYER_ENV
// this will work even when the moves are ERR_UNENUMERABLE
// the move will always be chosen deterministically from the supplied seed
// SEED_NONE is a valid seed here and behaves just like all other possible values that a u64 can take
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code get_random_move_gf_t(game* self, seed128 seed, move_data_sync** ret_move);

// FEATURE: move_ordering
// writes the available moves for the player from this position and returns a read only pointer to them
// writes no moves if the game is over
// moves must be at least of count get_moves(NULL)
// moves written are ordered according to the game method, from perceived strongest to weakest
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code get_concrete_moves_ordered_gf_t(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves);

// FEATURE: action_list && (hidden_information || simultaneous_moves)
// this is mainly for engines, it is never needed in normal play
// writes the available action moves for the player from this position and returns a read only pointer to them
// writes no action moves if there are no action moves available
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code get_actions_gf_t(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves);

// returns whether or not this move would be legal to make on the current state
// only available if this ptm is to move
// for non SM games (or those that are totally ordered) equivalent to the fallback check of: move in list of get_moves?
// should be optimized by the game method if possible
// moves do not have to be valid
// the game only reads the move, the caller still has to clean it up
typedef error_code is_legal_move_gf_t(game* self, player_id player, move_data_sync move);

// FEATURE: hidden_information || simultaneous_moves
// returns the action representing the information set transformation of the (concrete) LEGAL move (action instance), into the target perspective specific action
// player may be in target_player, target_players can only contain ids of players who participate in the game
// use e.g. on server, send out the only the action to the client controlling the target_players
// for game implementations finding the greatest common knowledge among target_players should not be too difficult, if wanted however, a useful pattern would be to break up the move into parts such that each part only ever has two actions: its public identity and a generic action "move hidden"
// the game only reads the move, the caller still has to clean it up
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code move_to_action_gf_t(game* self, player_id player, move_data_sync move, uint8_t target_count, const player_id* target_players, move_data_sync** ret_action);

// make the legal move on the game state as player
// undefined behaviour if move is not within the get_moves list for this player (MAY CRASH)
// undefined behaviour if player is not within players_to_move (MAY CRASH)
// after a move is made, if it's action was not the null move (if applicable), the sync_ctr in the game must be incremented!
// the game only reads the move, the caller still has to clean it up
typedef error_code make_move_gf_t(game* self, player_id player, move_data_sync move);

// writes the result (winning) players and returns a read only pointer to them
// writes no ids if the game is not over yet or there are no result players
// returns the number of ids written
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code get_results_gf_t(game* self, uint8_t* ret_count, const player_id** ret_players);

// FEATURE: legacy
// while the game is running exports the used legacy strings at creation, or after the game is finished the resulting legacies to be used for the next game and returns a read only pointer to it
// player specifies the perspective player from which this is done, this serves for score legacies where players can drop and join games but keep their personal legacies intact, the game specific history is serialized for PLAYER_ENV
// the legacy str ptr returned is allowed to be NULL! this is a valid value and must be passed as such to future games
// NOTE: this does not include the used options, save them separately for reuse together with this in a future game
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code export_legacy_gf_t(game* self, player_id player, size_t* ret_size, const char** ret_str);

// FEATURE: legacy
// the options used for games of the same legacy or sharing player legacies MUST remain constant, a copy of them is supplied as opts_str
// env legacy is often NULL for e.g. point score counting legacy results
// winners are determined by the legacies supplied (e.g. point scores)
// writes the result (winning) players and returns a read only pointer to them
// NOTE: the result players written are identified by the idx of their legacy in the input player_legacies, these are NOT player_ids
// returns the number of idxs written
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code get_legacy_results_sgf_t(game_methods* methods, const char* opts_str, const char* env_legacy, uint16_t player_legacy_count, const char* const* player_legacies, uint16_t* ret_count, const uint16_t** ret_legacy_idxs);

// FEATURE: id
// state id, should be as conflict free as possible
// commutatively the same for equal board states
// optimally, any n bits of this should be functionally equivalent to a dedicated n-bit id
//TODO usefulness breaks down quickly for hidden info games? want perspective too?
typedef error_code id_gf_t(game* self, uint64_t* ret_id);

// FEATURE: eval
// evaluates the state comparatively against others
// higher evaluations correspond to a (game method) perceived better position for the player
// states with multiple players to move can be unstable, if their evaluations are worthless use ERR_UNSTABLE_POSITION to signal this
typedef error_code eval_gf_t(game* self, player_id player, float* ret_eval);

// FEATURE: (random_moves || hidden_information || simultaneous_moves) && discretize
// seed the game and collapse the hidden information and all that was inferred via play
// the resulting game state assigns possible values to all previously unknown information
// all random moves from here on will be pre-rolled from this seed
// here SEED_NONE can not be used
// NOTE: this would only really be used by engines, so in normal play you will never need this
typedef error_code discretize_gf_t(game* self, seed128 seed);

// FEATURE: playout
// playout the game by performing random moves for all players until it is over
// the random moves selected are determined by the seed
// here SEED_NONE can not be used
typedef error_code playout_gf_t(game* self, seed128 seed);

// FEATURE: random_moves || hidden_information || simultaneous_moves
// removes all but certain players hidden and public information from the internal state
// if PLAYER_ENV is not in players, then the seed (and all internal hidden information) is redacted as well
typedef error_code redact_keep_state_gf_t(game* self, uint8_t count, const player_id* players);

// FEATURE: (hidden_information || simultaneous_moves) && sync_data
// one sync data always describes the data to be sent to the given player array to sync up their state
// multiple sync data structs incur multiple events, i.e. multiple import_sync_data calls
// use this after making a move to test what sync data should be sent to which player clients before the action
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code export_sync_data_gf_t(game* self, uint32_t* ret_count, const sync_data** ret_sync_data);

// FEATURE: (hidden_information || simultaneous_moves) && sync_data
// import a sync data block received from another (more knowing) instance of this game (e.g. across the network)
typedef error_code import_sync_data_gf_t(game* self, blob b);

// returns the game method and state specific move data representing the move string at this position for this player
// if the move string does not represent a valid move this returns an error (NOTE: avoid crashes)
// this should accept at least the string given out by get_move_str, but is allowed to accept more strings as well
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code get_move_data_gf_t(game* self, player_id player, const char* str, move_data_sync** ret_move);

// writes the game method and state specific move string representing the move data for this player and returns a read only pointer to it
// returns number of characters written to string buffer on success, excluding null character
// the game only reads the move, the caller still has to clean it up
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code get_move_str_gf_t(game* self, player_id player, move_data_sync move, size_t* ret_size, const char** ret_str);

// FEATURE: print
// prints the game state into the str_buf and returns a read only pointer to it
// returns the number of characters written to string buffer on success, excluding null character
// the print string encodes as much information as this game knows
// the returned ptr is valid until the next call on this game, undefined behaviour if used after; it is still owned by the game
typedef error_code print_gf_t(game* self, size_t* ret_size, const char** ret_str);

// FEATURE: time
// informs the game that the game.time timestamp has moved
// the game may modify timectlstages, moves available, players to move and even end the game; so any caches of those are now potentially invalid
// if sleep_duration is zero (//TODO want helper?) the game does not request a wakeup
// otherwise, if nothing happened in between, time_ellapsed should be called at the latest after this duration
// typedef error_code time_ellapsed_gf_t(game* self, timestamp* sleep_duration);

typedef struct game_methods_s {
    // after any function was called on a game object, it must be destroyed before it can be safely released

    // the game methods functions work ONLY on the data supplied to it in the game
    // i.e. they are threadsafe across multiple games, but not within one game instance
    // use of lookup tables and similar constant read only game external structures is ok

    // except where explicitly permitted, methods should never cause a crash or undefined behaviour
    // when in doubt return an error code representing an unusable state and force deconstruction
    // also it is encouraged to report fails from malloc and similar calls

    // function pointers for functions marked with "FEATURE: ..." are valid if and only if the feature flag condition is met for the game method
    // functions with "NOTE: avoid crashes" should place extra care on proper input parsing
    // where applicable: size refers to bytes (incl. zero terminator for strings), count refers to a number of elements of a particular type

    // the concatenation of game_name+variant_name+impl_name+version uniquely identifies these game methods
    // minimum length of 1 character each, with allowed character set:
    // "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" additionally '-' and '_' but not at the start or end
    const char* game_name;
    const char* variant_name;
    const char* impl_name;
    const semver version;
    const game_feature_flags features; // these will never change depending on options (e.g. if randomness only really happens due to a specific option, the whole game methods are always marked as containing randomness)

    // the game method specific internal method struct, NULL if not available
    // use the {base,variant,impl} name and version to make sure you know exactly what this will be
    // e.g. these would expose get_cell and set_cell on a tictactoe board to enable rw-access to the state
    const void* internal_methods;

    // all the member function pointers using the typedefs from above
    get_last_error_gf_t* get_last_error;
    create_gf_t* create;
    destroy_gf_t* destroy;
    clone_gf_t* clone;
    copy_from_gf_t* copy_from;
    export_options_gf_t* export_options;
    player_count_gf_t* player_count;
    export_state_gf_t* export_state;
    import_state_gf_t* import_state;
    serialize_gf_t* serialize;
    players_to_move_gf_t* players_to_move;
    get_concrete_moves_gf_t* get_concrete_moves;
    get_concrete_move_probabilities_gf_t* get_concrete_move_probabilities;
    get_random_move_gf_t* get_random_move;
    get_concrete_moves_ordered_gf_t* get_concrete_moves_ordered;
    get_actions_gf_t* get_actions;
    is_legal_move_gf_t* is_legal_move;
    move_to_action_gf_t* move_to_action;
    make_move_gf_t* make_move;
    get_results_gf_t* get_results;
    export_legacy_gf_t* export_legacy;
    get_legacy_results_sgf_t* s_get_legacy_results;
    id_gf_t* id;
    eval_gf_t* eval;
    discretize_gf_t* discretize;
    playout_gf_t* playout;
    redact_keep_state_gf_t* redact_keep_state;
    export_sync_data_gf_t* export_sync_data;
    import_sync_data_gf_t* import_sync_data;
    get_move_data_gf_t* get_move_data;
    get_move_str_gf_t* get_move_str;
    print_gf_t* print;

} game_methods;

struct game_s {
    const game_methods* methods;
    void* data1; // owned by the game method
    void* data2; // owned by the game method
    uint64_t sync_ctr;

    // FEATURE: time
    // representation of the "current time" at function invocation, use to determine relative durations
    // the only allowed changes are monotonic increases (incl. no change)
    // timestamp time;

    // FEATURE: time
    // every participating player X has their stage at timectlstages[X], [0] is reserved
    // stages can change entirely after every game function call
    // if the game does not support / use timectl then this can be managed externally //TODO timectl helper api
    // timectlstage** timectlstages;
};

typedef enum TIMECTLSTAGE_TYPE_E {
    TIMECTLSTAGE_TYPE_NONE = 0,
    TIMECTLSTAGE_TYPE_TIME, // standard static time limit
    TIMECTLSTAGE_TYPE_BONUS, // add a persistent bonus AFTER every move
    TIMECTLSTAGE_TYPE_DELAY, // transient delay at the start of a move BEFORE the clock starts counting down, unused delay time is lost
    TIMECTLSTAGE_TYPE_BYO, // available time resets every move, timing out transfers to the next time control, can also be used for correspondence
    TIMECTLSTAGE_TYPE_UPCOUNT, // time counts upwards
    TIMECTLSTAGE_TYPE_COUNT,
} TIMECTLSTAGE_TYPE;

struct timectlstage_s {
    timectlstage* next_stage; //TODO want ALL stages? even past ones?
    TIMECTLSTAGE_TYPE type;
    bool discard_time; // if true: sets the remaining time to 0 before applying this time control
    bool chain; // for byo, if true: this and the previous time controls CHAIN up until a BYO will share a move counter for advancing to the end of the BYO chain
    uint32_t stage_time; // ms time available in this stage
    uint32_t stage_mod; // bonus/delay
    uint32_t stage_moves; // if > 0: moves to next time control
    uint32_t time; // ms left/accumulated for this player
    uint32_t moves; // already played moves with this time control
    const char* desc; // description
};

// game function wrap, use this instead of directly calling the game methods
const char* game_gname(game* self); // game name
const char* game_vname(game* self); // variant name
const char* game_iname(game* self); // impl name
const semver game_version(game* self);
const game_feature_flags game_ff(game* self);
get_last_error_gf_t game_get_last_error;
create_gf_t game_create;
destroy_gf_t game_destroy;
clone_gf_t game_clone;
copy_from_gf_t game_copy_from;
export_options_gf_t game_export_options;
player_count_gf_t game_player_count;
export_state_gf_t game_export_state;
import_state_gf_t game_import_state;
serialize_gf_t game_serialize;
players_to_move_gf_t game_players_to_move;
get_concrete_moves_gf_t game_get_concrete_moves;
get_concrete_move_probabilities_gf_t game_get_concrete_move_probabilities;
get_random_move_gf_t game_get_random_move;
get_concrete_moves_ordered_gf_t game_get_concrete_moves_ordered;
get_actions_gf_t game_get_actions;
is_legal_move_gf_t game_is_legal_move;
move_to_action_gf_t game_move_to_action;
make_move_gf_t game_make_move;
get_results_gf_t game_get_results;
export_legacy_gf_t game_export_legacy;
get_legacy_results_sgf_t game_s_get_legacy_results;
id_gf_t game_id;
eval_gf_t game_eval;
discretize_gf_t game_discretize;
playout_gf_t game_playout;
redact_keep_state_gf_t game_redact_keep_state;
export_sync_data_gf_t game_export_sync_data;
import_sync_data_gf_t game_import_sync_data;
get_move_data_gf_t game_get_move_data;
get_move_str_gf_t game_get_move_str;
print_gf_t game_print;
// extra utility for game funcs
move_data game_e_create_move_small(move_code move);
move_data game_e_create_move_big(size_t len, uint8_t* buf);
move_data_sync game_e_create_move_sync_small(game* self, move_code move);
move_data_sync game_e_create_move_sync_big(game* self, size_t len, uint8_t* buf);
move_data_sync game_e_move_make_sync(game* self, move_data move);
bool game_e_move_compare(move_data left, move_data right);
bool game_e_move_sync_compare(move_data_sync left, move_data_sync right);
bool game_e_move_copy(move_data* target_move, const move_data* source_move);
bool game_e_move_sync_copy(move_data_sync* target_move, const move_data_sync* source_move);
void game_e_move_destroy(move_data move);
void game_e_move_sync_destroy(move_data_sync move);
bool game_e_move_is_big(move_data move);
move_data_sync game_e_get_random_move_sync(game* self, seed128 seed); // from a position where PLAYER_ENV is ptm this uses the get_concrete_move_probabilities to copy a random move sync
bool game_e_player_to_move(game* self, player_id player); // returns true if this player is in ptm
uint32_t game_e_seed_rand_intn(seed128 seed, uint32_t n); // generates [0,n)

// game internal rerrorf: if your error string is self->data2 use this as a shorthand
error_code grerror(game* self, error_code ec, const char* str, const char* str_end);
error_code grerrorf(game* self, error_code ec, const char* fmt, ...);
error_code grerrorfv(game* self, error_code ec, const char* fmt, va_list args);

#ifdef __cplusplus
}
#endif
