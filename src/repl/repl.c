#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rosalia/base64.h"
#include "rosalia/noise.h"
#include "rosalia/serialization.h"
#include "rosalia/timestamp.h"

#include "surena/games/chess.h"
#include "surena/games/havannah.h"
#include "surena/games/quasar.h"
#include "surena/games/rockpaperscissors.h"
#include "surena/games/tictactoe_ultimate.h"
#include "surena/games/tictactoe.h"
#include "surena/games/twixt_pp.h"
#include "surena/game_plugin.h"
#include "surena/game.h"
#include "surena/move_history.h"

#include "repl.h"

// string pointer to first char occurence
// returns either ptr to the first occurence of c in str, or ptr to the zero terminator if c was not found in str
const char* strpfc(const char* str, char c)
{
    const char* wstr = str;
    while (*wstr != c && *wstr != '\0') {
        wstr++;
    }
    return wstr;
}

// if str is NULL, then this frees the content of argv
// you still need to provide argc
void strargsplit(const char* str, int* argc, char*** argv)
{
    if (str == NULL) {
        // the first element of *argv points to the start of the allocation used
        // for all strings in *argv
        if (*argc > 0) {
            free((*argv)[0]);
        }
        free(*argv);
        return;
    }
    //TODO want do backslash escapes for \ + (\,",',n,t,b,f,r,u,/) just like json
    //TODO allow both quote modes with " and ', remember only the outer most one used
    char* newstr = (char*)malloc(strlen(str) + 2); // sadly need 1 extra space for the very last terminator //TODO might not even want that feature
    char* ostr = newstr;
    const char* wstr = str;
    bool is_empty = true;
    bool quoted = false;
    int cnt = 0;
    while (*wstr != '\0') {
        if (*wstr == ' ' && quoted == false) {
            *ostr = '\0';
            if (is_empty == false) {
                ostr++;
                cnt++;
            }
            is_empty = true;
        } else if (*wstr == '\"') {
            quoted = !quoted;
        } else {
            is_empty = false;
            *ostr = *wstr;
            ostr++;
        }
        wstr++;
    }
    *ostr = '\0';
    if (is_empty == false) {
        cnt++;
    }
    ostr++;
    *ostr = '\0';
    *argc = cnt;
    *argv = (char**)malloc(sizeof(char*) * cnt);
    char* sstr = newstr;
    for (int i = 0; i < cnt; i++) {
        (*argv)[i] = sstr;
        while (*sstr != '\0') {
            sstr++;
        }
        sstr++;
    }
    // if cnt is 0, no reference to the newstr allocation will remain
    // hence, free newstr now for this case
    if (cnt == 0) {
        free(newstr);
    }
}

const game_methods* load_plugin_game_methods(const char* file, uint32_t idx)
{
    void* dll_handle = dlopen(file, RTLD_LAZY);
    plugin_get_game_capi_version_t version = (plugin_get_game_capi_version_t)dlsym(dll_handle, "plugin_get_game_capi_version");
    if (version == NULL || version() != SURENA_GAME_API_VERSION) {
        return NULL;
    }
    void (*init)() = (void (*)())dlsym(dll_handle, "plugin_init_game");
    if (init == NULL) {
        return NULL;
    }
    init();
    plugin_get_game_methods_t get_games = (plugin_get_game_methods_t)dlsym(dll_handle, "plugin_get_game_methods");
    if (get_games == NULL) {
        return NULL;
    }
    uint32_t method_cnt;
    get_games(&method_cnt, NULL);
    if (method_cnt == 0) {
        return NULL;
    }
    const game_methods** method_buf = (const game_methods**)malloc(sizeof(game_methods*) * method_cnt);
    get_games(&method_cnt, method_buf);
    if (method_cnt == 0) {
        return NULL;
    }
    if (idx >= method_cnt) {
        return NULL;
    }
    const game_methods* ret_method = method_buf[idx];
    free(method_buf);
    return ret_method;
}

//TODO keep this ordered via a sort and compare func, for now use hierarchical
const game_methods* static_game_methods[] = {
    &chess_standard_gbe,
    &havannah_standard_gbe,
    &quasar_standard_gbe,
    &rockpaperscissors_standard_gbe,
    &tictactoe_standard_gbe,
    &tictactoe_ultimate_gbe,
    &twixt_pp_gbe,
};

const game_methods* load_static_game_methods(const char* composite_id)
{
    const int GNAME_IDX = 0;
    const int VNAME_IDX = 1;
    const int INAME_IDX = 2;
    char id_name[3][64] = {'\0', '\0', '\0'};
    // split composite id into proper g/v/i names
    const char* wstr = composite_id;
    const char* estr = composite_id;
    for (int i = 0; i < 3; i++) {
        if (*estr == '\0') {
            break;
        }
        if (i > 0) {
            estr++;
        }
        wstr = estr;
        estr = strpfc(estr, '.');
        if (estr - wstr >= 64) {
            return NULL;
        }
        strncpy(id_name[i], wstr, estr - wstr);
    }
    if (strlen(id_name[GNAME_IDX]) == 0) {
        return NULL;
    }
    if (strlen(id_name[VNAME_IDX]) == 0) {
        strcpy(id_name[VNAME_IDX], "Standard");
    }
    // find correctly named game method
    size_t statics_cnt = sizeof(static_game_methods) / sizeof(game_methods*);
    for (size_t i = 0; i < statics_cnt; i++) {
        if (strcmp(id_name[GNAME_IDX], static_game_methods[i]->game_name) == 0 &&
            strcmp(id_name[VNAME_IDX], static_game_methods[i]->variant_name) == 0 &&
            (strlen(id_name[INAME_IDX]) == 0 || strcmp(id_name[INAME_IDX], static_game_methods[i]->impl_name) == 0)) {
            return static_game_methods[i];
        }
    }
    return NULL;
}

void print_game_error(game* g, error_code ec)
{
    if (ec == ERR_OK) {
        return;
    }
    if (game_ff(g).error_strings == true) {
        printf("[WARN] game error #%u (%s): %s\n", ec, get_general_error_string(ec, "unknown specific"), game_get_last_error(g));
    } else {
        printf("[WARN] game error #%u (%s)\n", ec, get_general_error_string(ec, "unknown specific"));
    }
}

// read up to buf_size characters from stdin into buf, guaranteeing a zero terminator after them and cutting off any excess characers
// automatically clears stdin after getting the characters, so further calls will not read them
// returns true if input was truncated
bool readbufsafe(char* buf, size_t buf_size)
{
    size_t read_pos = 0;
    bool read_stop = false;
    bool read_clear = false;
    while (read_stop == false) {
        int tc = getc(stdin);
        if (tc == EOF) {
            printf("\nerror/eof\n");
            exit(EXIT_FAILURE);
        }
        if (tc == '\n' || read_pos >= buf_size - 1) {
            tc = '\0';
            read_stop = true;
            if (read_pos >= buf_size - 1) {
                read_clear = true;
            }
        }
        buf[read_pos++] = tc;
    }
    if (read_clear) {
        // got input str up to buffer length, now clear read buffer until newline char, only really works for stdin on cli but fine for now
        int tc;
        do {
            tc = getc(stdin);
        } while (tc != '\n' && tc != EOF);
    }
    return read_clear;
}

typedef struct repl_state_s {
    bool exit;
    //TODO set and get variable array and enum type + string tag list
    const game_methods* g_methods;
    char* g_c_options;
    char* g_c_legacy;
    char* g_c_initial_state;
    char* g_c_b64_serialized;
    blob g_c_serialized;
    game g;
    move_history* history;
    move_history* history_head;
    player_id pov;
} repl_state;

typedef enum REPL_CMD_E {
    REPL_CMD_NONE = 0,
    REPL_CMD_M_HELP,
    //TODO REPL_CMD_M_LIST_STATIC,
    REPL_CMD_M_LOAD_STATIC,
    REPL_CMD_M_LOAD_PLUGIN, //TODO support unloading?
    REPL_CMD_M_EXIT,
    REPL_CMD_M_GET,
    REPL_CMD_M_SET,
    REPL_CMD_M_POV,
    //TODO REPL_CMD_M_RS,
    //TODO REPL_CMD_M_GINFO,
    REPL_CMD_G_CREATE,
    REPL_CMD_G_DESTROY,
    // REPL_CMD_G_EXPORT_OPTIONS,
    // REPL_CMD_G_PLAYER_COUNT,
    // REPL_CMD_G_EXPORT_STATE,
    // REPL_CMD_G_IMPORT_STATE,
    // REPL_CMD_G_SERIALIZE,
    REPL_CMD_G_PLAYERS_TO_MOVE,
    REPL_CMD_G_GET_CONCRETE_MOVES,
    // REPL_CMD_G_GET_CONCRETE_MOVE_PROBABILITIES,
    // REPL_CMD_G_GET_RANDOM_MOVE,
    // REPL_CMD_G_GET_CONCRETE_MOVES_ORDERED,
    // REPL_CMD_G_GET_ACTIONS,
    // REPL_CMD_G_IS_LEGAL_MOVE,
    // REPL_CMD_G_MOVE_TO_ACTION,
    REPL_CMD_G_MAKE_MOVE,
    REPL_CMD_G_GET_RESULTS,
    // REPL_CMD_G_EXPORT_LEGACY,
    // REPL_CMD_G_S_GET_LEGACY_RESULTS,
    // REPL_CMD_G_GET_SCORES,
    // REPL_CMD_G_ID,
    // REPL_CMD_G_EVAL,
    // REPL_CMD_G_DISCRETIZE,
    // REPL_CMD_G_PLAYOUT,
    // REPL_CMD_G_REDACT_KEEP_STATE,
    // REPL_CMD_G_EXPORT_SYNC_DATA,
    // REPL_CMD_G_IMPORT_SYNC_DATA,
    // REPL_CMD_G_GET_MOVE_DATA,
    // REPL_CMD_G_GET_MOVE_STR,
    REPL_CMD_G_PRINT,
    REPL_CMD_GS_HISTORY,
    REPL_CMD_GS_RESOLVE_RANDOM,
    REPL_CMD_COUNT,
} REPL_CMD;

// a repl_cmd_func_t must print help information if rs is NULL
typedef void repl_cmd_func_t(repl_state* rs, int argc, char** argv);

repl_cmd_func_t repl_cmd_handle_m_help;
repl_cmd_func_t repl_cmd_handle_m_load_static;
repl_cmd_func_t repl_cmd_handle_m_load_plugin;
repl_cmd_func_t repl_cmd_handle_m_exit;
repl_cmd_func_t repl_cmd_handle_m_get;
repl_cmd_func_t repl_cmd_handle_m_set;
repl_cmd_func_t repl_cmd_handle_m_pov;
repl_cmd_func_t repl_cmd_handle_g_create;
repl_cmd_func_t repl_cmd_handle_g_destroy;
repl_cmd_func_t repl_cmd_handle_g_players_to_move;
repl_cmd_func_t repl_cmd_handle_g_get_concrete_moves;
repl_cmd_func_t repl_cmd_handle_g_make_move;
repl_cmd_func_t repl_cmd_handle_g_get_results;
repl_cmd_func_t repl_cmd_handle_g_print;
repl_cmd_func_t repl_cmd_handle_gs_history;
repl_cmd_func_t repl_cmd_handle_gs_resolve_random;

typedef struct game_command_info_s {
    char* text; //TODO support multiple alias via "abc\0def\0"
    repl_cmd_func_t* handler;
} game_command_info;

//TODO separate commands for game and meta like repl state and info and load mehtods
game_command_info game_command_infos[REPL_CMD_COUNT] = {
    [REPL_CMD_NONE] = {"noop", NULL},
    [REPL_CMD_M_HELP] = {"help", repl_cmd_handle_m_help},
    [REPL_CMD_M_LOAD_STATIC] = {"load_static", repl_cmd_handle_m_load_static},
    [REPL_CMD_M_LOAD_PLUGIN] = {"load_plugin", repl_cmd_handle_m_load_plugin},
    [REPL_CMD_M_EXIT] = {"exit", repl_cmd_handle_m_exit},
    [REPL_CMD_M_GET] = {"get", repl_cmd_handle_m_get},
    [REPL_CMD_M_SET] = {"set", repl_cmd_handle_m_set},
    [REPL_CMD_M_POV] = {"pov", repl_cmd_handle_m_pov},
    [REPL_CMD_G_CREATE] = {"create", repl_cmd_handle_g_create},
    [REPL_CMD_G_DESTROY] = {"destroy", repl_cmd_handle_g_destroy},
    // [REPL_CMD_G_EXPORT_OPTIONS] = {"export_options", NULL},
    // [REPL_CMD_G_PLAYER_COUNT] = {"player_count", NULL},
    // [REPL_CMD_G_EXPORT_STATE] = {"export_state", NULL},
    // [REPL_CMD_G_IMPORT_STATE] = {"import_state", NULL},
    // [REPL_CMD_G_SERIALIZE] = {"serialize", NULL},
    [REPL_CMD_G_PLAYERS_TO_MOVE] = {"players_to_move", repl_cmd_handle_g_players_to_move},
    [REPL_CMD_G_GET_CONCRETE_MOVES] = {"get_concrete_moves", repl_cmd_handle_g_get_concrete_moves},
    // [REPL_CMD_G_GET_CONCRETE_MOVE_PROBABILITIES] = {"get_concrete_move_probabilities", NULL},
    // [REPL_CMD_G_GET_RANDOM_MOVE] = {"get_random_move", NULL},
    // [REPL_CMD_G_GET_CONCRETE_MOVES_ORDERED] = {"get_concrete_moves_ordered", NULL},
    // [REPL_CMD_G_GET_ACTIONS] = {"get_actions", NULL},
    // [REPL_CMD_G_IS_LEGAL_MOVE] = {"is_legal_move", NULL},
    // [REPL_CMD_G_MOVE_TO_ACTION] = {"move_to_action", NULL},
    [REPL_CMD_G_MAKE_MOVE] = {"make_move", repl_cmd_handle_g_make_move},
    [REPL_CMD_G_GET_RESULTS] = {"get_results", repl_cmd_handle_g_get_results},
    // [REPL_CMD_G_EXPORT_LEGACY] = {"export_legacy", NULL},
    // [REPL_CMD_G_S_GET_LEGACY_RESULTS] = {"s_get_legacy_results", NULL},
    // [REPL_CMD_G_GET_SCORES] = {"get_scores", NULL},
    // [REPL_CMD_G_ID] = {"id", NULL},
    // [REPL_CMD_G_EVAL] = {"eval", NULL},
    // [REPL_CMD_G_DISCRETIZE] = {"discretize", NULL},
    // [REPL_CMD_G_PLAYOUT] = {"playout", NULL},
    // [REPL_CMD_G_REDACT_KEEP_STATE] = {"redact_keep_state", NULL},
    // [REPL_CMD_G_EXPORT_SYNC_DATA] = {"export_sync_data", NULL},
    // [REPL_CMD_G_IMPORT_SYNC_DATA] = {"import_sync_data", NULL},
    // [REPL_CMD_G_GET_MOVE_DATA] = {"get_move_data", NULL},
    // [REPL_CMD_G_GET_MOVE_STR] = {"get_move_str", NULL},
    [REPL_CMD_G_PRINT] = {"print", repl_cmd_handle_g_print},
    [REPL_CMD_GS_HISTORY] = {"history", repl_cmd_handle_gs_history},
    [REPL_CMD_GS_RESOLVE_RANDOM] = {"resolve_random", repl_cmd_handle_gs_resolve_random},
};

REPL_CMD get_cmd_type(const char* str, const char* str_end)
{
    const char* wstr = str;
    const char* estr = (str_end == NULL ? str + strlen(str) : str_end);
    size_t cmp_len = estr - wstr;
    REPL_CMD cmd_type = REPL_CMD_NONE;
    for (REPL_CMD ct = 0; ct < REPL_CMD_COUNT; ct++) {
        if (strncmp(wstr, game_command_infos[ct].text, cmp_len) == 0) {
            cmd_type = ct;
            break;
        }
    }
    return cmd_type;
}

void handle_command(repl_state* rs, char* str)
{
    /*TODO ideally cmd list wanted:
    :set option=value // use for things like print_after_move=off and similarly :set options="8+" and :set pov=1
    :get option
    :create [options] [legacy] [initial_state] // want default params here?
    :destroy
    :move
    :ptm
    :get_moves{_prob,_ordered}
    :info // prints lots of things about the game
    */
    const char* wstr = str;
    const char* estr = strpfc(str, ' ');
    REPL_CMD cmd_type = get_cmd_type(wstr, estr);
    if (*estr != '\0') {
        estr++;
    }
    wstr = estr;
    if (cmd_type != REPL_CMD_NONE && cmd_type < REPL_CMD_COUNT) {
        int argc;
        char** argv;
        strargsplit(wstr, &argc, &argv);
        game_command_infos[cmd_type].handler(rs, argc, argv);
        strargsplit(NULL, &argc, &argv);
    } else {
        printf("unknown command\n");
    }
}

char cmd_prefix = '/';

int repl()
{
    repl_state rs = (repl_state){
        .exit = false,
        .g_methods = NULL,
        .g_c_options = NULL,
        .g_c_legacy = NULL,
        .g_c_initial_state = NULL,
        .g_c_b64_serialized = NULL,
        .g = (game){
            .methods = NULL,
            .data1 = NULL,
            .data2 = NULL,
            .sync_ctr = SYNC_CTR_DEFAULT,
        },
        .history = move_history_create(),
        .history_head = NULL,
        .pov = PLAYER_NONE,
    };
    rs.history_head = rs.history;
    const size_t read_buf_size = 4096;
    char* read_buf = malloc(read_buf_size);
    while (rs.exit == false) {
        // read input
        printf("(pov%03hhu)> ", rs.pov);
        bool input_truncated = readbufsafe(read_buf, read_buf_size);
        if (input_truncated) {
            printf("[WARN] input was truncated to %zu characters\n", read_buf_size - 1);
        }
        // eval
        if (*read_buf == cmd_prefix) {
            // is a command
            handle_command(&rs, read_buf + 1);
        } else {
            // default "move"
            int move_argc;
            char** move_argv;
            strargsplit(read_buf, &move_argc, &move_argv);
            game_command_infos[REPL_CMD_G_MAKE_MOVE].handler(&rs, move_argc, move_argv);
            strargsplit(NULL, &move_argc, &move_argv);
        }
        // print
        /*TODO
        auto print results on end
        auto print board/id/state/eval/ptm after move
        auto print sync data after move? or show some info about it
        */
    }
    free(read_buf);

    return 0;
}

//TODO for all of these, check that arg count isnt too many! since some vary in function depending on args, also help should show this per command!

void repl_cmd_handle_m_help(repl_state* rs, int argc, char** argv)
{
    if (argc == 0) {
        printf("usage: help [cmd]\n");
        printf("use with command to see its usage\n");
        printf("available commands:");
        for (REPL_CMD cmd_type = REPL_CMD_NONE; cmd_type < REPL_CMD_COUNT; cmd_type++) {
            printf(" %s", game_command_infos[cmd_type].text);
        }
        printf("\n");
    } else if (argc == 1) {
        printf("showing help for \"%s\"\n", argv[0]);
        REPL_CMD cmd_type = get_cmd_type(argv[0], NULL);
        if (cmd_type != REPL_CMD_NONE && cmd_type < REPL_CMD_COUNT) {
            game_command_infos[cmd_type].handler(NULL, 0, NULL);
        } else {
            printf("unknown command type\n");
        }
    } else {
        printf("too many args for help cmd\n");
    }
}

void repl_cmd_handle_m_load_static(repl_state* rs, int argc, char** argv)
{
    if (rs == NULL) { // print help
        printf("usage: load_static <composite id>\n");
        printf("format is BaseName.VariantName.ImplName, if variant is omitted it is assumed \"Standard\", if impl is omitted the first found is accepted\n");
        return;
    }
    if (argc < 1) {
        printf("game name composite missing\n");
        return;
    }
    const game_methods* gm = load_static_game_methods(argv[0]);
    if (gm == NULL) {
        printf("game methods \"%s\" not found\n", argv[0]);
    } else {
        printf("found: %s.%s.%s v%u.%u.%u\n", gm->game_name, gm->variant_name, gm->impl_name, gm->version.major, gm->version.minor, gm->version.patch);
        rs->g_methods = gm;
    }
}

void repl_cmd_handle_m_load_plugin(repl_state* rs, int argc, char** argv)
{
    if (rs == NULL) { // print help
        printf("usage: load_plugin <path> [idx]\n");
        printf("if index is omitted, 0 is used\n");
        return;
    }
    if (argc < 1) {
        printf("plugin file path missing\n");
        return;
    }
    uint32_t load_idx = 0;
    if (argc >= 2) {
        int sc = sscanf(argv[1], "%u", &load_idx);
        if (sc != 1) {
            printf("could not parse index as u32\n");
            return;
        }
    }
    const game_methods* gm = load_plugin_game_methods(argv[0], load_idx);
    if (gm == NULL) {
        printf("plugin did not provide at least one game, ignoring\n");
        printf("note: relative plugin paths MUST be prefixed with \"./\"\n");
    } else {
        printf("loaded method %u: %s.%s.%s v%u.%u.%u\n", load_idx, gm->game_name, gm->variant_name, gm->impl_name, gm->version.major, gm->version.minor, gm->version.patch);
        rs->g_methods = gm;
    }
}

void repl_cmd_handle_m_exit(repl_state* rs, int argc, char** argv)
{
    if (rs == NULL) { // print help
        printf("usage: exit\n");
        printf("quit the program\n");
        return;
    }
    rs->exit = true;
}

void repl_cmd_handle_m_get(repl_state* rs, int argc, char** argv)
{
    if (rs == NULL) { // print help
        //TODO
        return;
    }
    //TODO
    printf("feature unsupported");
}

void repl_cmd_handle_m_set(repl_state* rs, int argc, char** argv)
{
    if (rs == NULL) { // print help
        //TODO
        return;
    }
    //TODO
    printf("feature unsupported");
}

void repl_cmd_handle_m_pov(repl_state* rs, int argc, char** argv)
{
    if (rs == NULL) { // print help
        //TODO
        return;
    }
    if (rs->g.methods == NULL) {
        printf("no game running\n");
        return;
    }
    player_id pov = rs->pov;
    if (argc == 0) {
        printf("pov: %03hhu\n", pov);
        return;
    }
    //TODO allow none and env as string shorthands for 0 and 255
    int sc = sscanf(argv[0], "%hhu", &pov);
    if (sc != 1) {
        printf("could not parse pov as u8\n");
        return;
    }
    uint8_t pnum;
    error_code ec = game_player_count(&rs->g, &pnum);
    if (ec != ERR_OK) {
        print_game_error(&rs->g, ec);
        printf("[ERROR] unexpected game player count error\n");
        return;
    }
    if (pov > pnum && pov != PLAYER_ENV) {
        printf("invalid pov for %u players\n", pnum);
        return;
    }
    rs->pov = pov;
}

void repl_cmd_handle_g_create(repl_state* rs, int argc, char** argv)
{
    if (rs == NULL) { // print help
        // printf("usage: create\n"); //TODO future feature
        printf("usage: create def\n");
        printf("usage: create std <[O][P][S]> [options] [player_count] [state]\n");
        printf("usage: create ser <b64>\n");
        printf("create a game from default, standard or serialization source\n"); //TODO if no source is specified the cached one rs->g_c_* should be used
        printf("standard sources can specify which parts are supplied, other are assumed NULL\n");
        printf("e.g. to create with opts and default state: /create std O \"myopts\"\n");
        return;
    }
    if (rs->g_methods == NULL) {
        printf("can not create game: no methods selected\n");
        return;
    }
    game_init game_init_info;
    if (argc < 1) {
        printf("can not create game without specifying a source\n"); //TODO this is a future feature
        return;
    }
    const char* source_type = argv[0];
    if (strcmp(source_type, "def") == 0) {
        game_init_info.source_type = GAME_INIT_SOURCE_TYPE_DEFAULT;
    } else if (strcmp(source_type, "std") == 0) {
        game_init_info = (game_init){
            .source_type = GAME_INIT_SOURCE_TYPE_STANDARD,
            .source = {
                .standard = {
                    .opts = NULL,
                    .player_count = 0,
                    .env_legacy = NULL,
                    .player_legacies = NULL,
                    .state = NULL,
                    .sync_ctr = SYNC_CTR_DEFAULT,
                },
            },
        };
        //TODO add support for E and L, i.e. env and player legacies
        // if no specifiers supplied then just use defaults everywhere
        if (argc > 1) { // must be in order otherwise it might be confusing
            const int C_OPTIONS_IDX = 0;
            const int C_PLAYER_COUNT_IDX = 1;
            const int C_ENV_LEGACY_IDX = 2;
            const int C_PLAYER_LEGACIES_IDX = 3;
            const int C_STATE_IDX = 4;
            bool expecting[5] = {false, false, false, false, false};
            char* gotstr[5] = {NULL, NULL, NULL, NULL, NULL};
            const char* announcing = argv[1];
            for (int ai = 0; announcing[ai] != '\0'; ai++) {
                if (ai > 2) {
                    printf("too many announced infos for standard source\n");
                    return;
                }
                switch (announcing[ai]) {
                    case 'O': {
                        if (rs->g_methods->features.options == false) {
                            printf("game does not support options\n");
                            return;
                        }
                        expecting[C_OPTIONS_IDX] = true;
                    } break;
                    case 'P': {
                        expecting[C_PLAYER_COUNT_IDX] = true;
                    } break;
                    // case 'E': {
                    //     if (rs->g_methods->features.options == false) {
                    //         printf("game does not support legacy\n");
                    //         return;
                    //     }
                    //     expecting[C_ENV_LEGACY_IDX] = true;
                    // } break;
                    // case 'L': {
                    //     if (rs->g_methods->features.options == false) {
                    //         printf("game does not support legacy\n");
                    //         return;
                    //     }
                    //     expecting[C_PLAYER_LEGACIES_IDX] = true;
                    // } break;
                    case 'S': {
                        expecting[C_STATE_IDX] = true;
                    } break;
                    default: {
                        printf("unkown announced source string for standard source\n");
                        return;
                    }
                }
            }
            // go through the rest of the supplied args and fill in everywhere where expecting first before going on
            int wargc = argc - 2;
            char** wargv = argv + 2;
            for (int ei = 0; ei < 5; ei++) {
                if (expecting[ei] == false) {
                    continue;
                }
                if (wargc < 1) {
                    printf("missing expected source string\n");
                    return;
                }
                gotstr[ei] = strdup(wargv[0]);
                wargc--;
                wargv++;
            }
            // process all the copied info strings, if applicable set or parse and free them
            game_init_info.source.standard.opts = gotstr[C_OPTIONS_IDX];
            {
                int ec = sscanf(gotstr[C_PLAYER_COUNT_IDX], "%hhu", &game_init_info.source.standard.player_count);
                free(gotstr[C_PLAYER_COUNT_IDX]);
                if (ec != 1) {
                    //TODO proper error and free everything
                    printf("could not parse player count as u8, using 0");
                    game_init_info.source.standard.player_count = 0;
                }
            }
            game_init_info.source.standard.env_legacy = gotstr[C_ENV_LEGACY_IDX];
            game_init_info.source.standard.player_legacies = NULL; // gotstr[C_PLAYER_LEGACIES_IDX] should be a multi string
            game_init_info.source.standard.state = gotstr[C_STATE_IDX];
        }
    } else if (strcmp(source_type, "ser") == 0) {
        if (rs->g_methods->features.serializable == false) {
            printf("game does not support serialization source\n");
            return;
        }
        game_init_info.source_type = GAME_INIT_SOURCE_TYPE_SERIALIZED;
        if (argc < 2) {
            printf("need to supply serialization to use for create\n");
            return;
        }
        free(rs->g_c_b64_serialized);
        rs->g_c_b64_serialized = NULL;
        blob_destroy(&rs->g_c_serialized);
        rs->g_c_b64_serialized = strdup(argv[1]);
        size_t rsize = b64_decode_size(argv[1]);
        blob_create(&rs->g_c_serialized, rsize);
        rsize = b64_decode(rs->g_c_serialized.data, argv[1]);
        game_init_info.source.serialized.b = rs->g_c_serialized;
    } else {
        printf("unknown source type\n");
        return;
    }
    error_code ec;
    if (rs->g.methods != NULL) { // destroy old game
        ec = game_destroy(&rs->g);
        rs->g = (game){
            .methods = NULL,
            .data1 = NULL,
            .data2 = NULL,
            .sync_ctr = SYNC_CTR_DEFAULT,
        };
        if (ec != ERR_OK) {
            print_game_error(&rs->g, ec);
            printf("[ERROR] unexpected game destruction error\n");
        }
    }
    // swap out move history for new one
    move_history_destroy(rs->history);
    rs->history = move_history_create();
    rs->history_head = rs->history;
    // create new game
    rs->g.methods = rs->g_methods;
    ec = game_create(&rs->g, &game_init_info);
    if (ec != ERR_OK) {
        print_game_error(&rs->g, ec);
        //TODO destroy on failed create could be optional
        ec = game_destroy(&rs->g);
        rs->g = (game){
            .methods = NULL,
            .data1 = NULL,
            .data2 = NULL,
            .sync_ctr = SYNC_CTR_DEFAULT,
        };
        if (ec != ERR_OK) {
            print_game_error(&rs->g, ec);
            printf("[ERROR] unexpected game destruction error\n");
        }
    }
    rs->pov = PLAYER_NONE;
}

void repl_cmd_handle_g_destroy(repl_state* rs, int argc, char** argv)
{
    if (rs == NULL) { // print help
        printf("usage: destroy\n");
        printf("destroy the currently running game, if it exists\n");
        return;
    }
    if (rs->g.methods == NULL) {
        printf("no game running\n");
        return;
    }
    error_code ec = game_destroy(&rs->g);
    rs->g = (game){
        .methods = NULL,
        .data1 = NULL,
        .data2 = NULL,
        .sync_ctr = SYNC_CTR_DEFAULT,
    };
    if (ec != ERR_OK) {
        print_game_error(&rs->g, ec);
        printf("[ERROR] unexpected game destruction error\n");
    }
    rs->pov = PLAYER_NONE;
    move_history_destroy(rs->history);
    rs->history = move_history_create();
    rs->history_head = rs->history;
}

void repl_cmd_handle_g_players_to_move(repl_state* rs, int argc, char** argv)
{
    if (rs == NULL) { // print help
        printf("usage: players_to_move\n");
        printf("list all the players to move from this position\n");
        return;
    }
    if (rs->g.methods == NULL) {
        printf("no game running\n");
        return;
    }
    error_code ec;
    uint8_t size_fill;
    const player_id* ptm;
    ec = game_players_to_move(&rs->g, &size_fill, &ptm);
    if (ec != ERR_OK) {
        print_game_error(&rs->g, ec);
        printf("[ERROR] unexpected game players to move error\n");
        return;
    }
    printf("players to move (%hhu):", size_fill);
    for (uint8_t i = 0; i < size_fill; i++) {
        if (ptm[i] == PLAYER_ENV) {
            printf(" ENV");
        } else {
            printf(" %03hhu", ptm[i]);
        }
    }
    printf("\n");
}

void repl_cmd_handle_g_get_concrete_moves(repl_state* rs, int argc, char** argv)
{
    if (rs == NULL) { // print help
        printf("usage: get_concrete_moves [pov]\n");
        printf("list all available concrete moves for the pov, uses the current pov by default\n");
        return;
    }
    if (rs->g.methods == NULL) {
        printf("no game running\n");
        return;
    }
    error_code ec;
    player_id pov = rs->pov;
    if (argc > 0) {
        //TODO should be able to parse ENV here
        int sc = sscanf(argv[0], "%hhu", &pov);
        if (sc != 1) {
            printf("could not parse pov as u8\n");
            return;
        }
        uint8_t pnum;
        ec = game_player_count(&rs->g, &pnum);
        if (ec != ERR_OK) {
            print_game_error(&rs->g, ec);
            printf("[ERROR] unexpected game player count error\n");
            return;
        }
        if (pov > pnum && pov != PLAYER_ENV) {
            printf("invalid pov for %u players\n", pnum);
            return;
        }
    }
    uint8_t ptm_c;
    const player_id* ptm;
    ec = game_players_to_move(&rs->g, &ptm_c, &ptm);
    if (ec != ERR_OK) {
        print_game_error(&rs->g, ec);
        printf("[ERROR] unexpected game players to move error\n");
        return;
    }
    bool pov_valid = false;
    for (uint8_t i = 0; i < ptm_c; i++) {
        if (ptm[i] == pov) {
            pov_valid = true;
            break;
        }
    }
    if (pov_valid == false) {
        printf("[ERROR] player is not to move\n");
        return;
    }

    uint32_t moves_c;
    const move_data* moves_out;
    ec = game_get_concrete_moves(&rs->g, pov, &moves_c, &moves_out);
    if (ec != ERR_OK) {
        print_game_error(&rs->g, ec);
        printf("[ERROR] unexpected game get concrete moves error\n");
        return;
    }
    move_data* moves = (move_data*)malloc(sizeof(move_data) * moves_c);
    char** move_strs = (char**)malloc(sizeof(char*) * moves_c);
    uint32_t copystr_idx = 0;
    bool copystr_fail = false;
    for (; copystr_idx < moves_c; copystr_idx++) {
        game_e_move_copy(&moves[copystr_idx], moves_out + copystr_idx); //TODO this can fail oom
        size_t size_fill;
        const char* move_str;
        ec = game_get_move_str(&rs->g, pov, game_e_move_make_sync(&rs->g, moves[copystr_idx]), &size_fill, &move_str);
        if (ec != ERR_OK) {
            print_game_error(&rs->g, ec);
            printf("[ERROR] unexpected game get move str error\n");
            game_e_move_destroy(moves[copystr_idx]);
            copystr_fail = true;
            break;
        }
        move_strs[copystr_idx] = strdup(move_str);
    }
    if (copystr_fail == false) {
        printf("concrete moves pov%03hhu (%u):", pov, moves_c);
        for (uint32_t i = 0; i < moves_c; i++) {
            printf(" %s", move_strs[i]);
        }
        printf("\n");
    }
    for (uint32_t i = 0; i < copystr_idx; i++) {
        game_e_move_destroy(moves[i]);
        free(move_strs[i]);
    }
    free(moves);
    free(move_strs);
}

void repl_cmd_handle_g_make_move(repl_state* rs, int argc, char** argv)
{
    if (rs == NULL) { // print help
        printf("usage: make_move [pov] <move string>\n");
        printf("check if the move is legal, and make it one the board if it is, using the current pov or the supplied override\n");
        return;
    }
    if (rs->g.methods == NULL) {
        printf("no game running\n");
        return;
    }
    error_code ec;
    player_id pov = rs->pov;
    char* movestr;
    if (argc == 0) {
        printf("at least a move string is required to make a move\n");
        return;
    } else if (argc == 1) {
        movestr = argv[0];
    } else if (argc == 2) {
        //TODO should be able to parse ENV here
        int sc = sscanf(argv[0], "%hhu", &pov);
        if (sc != 1) {
            printf("could not parse pov as u8\n");
            return;
        }
        uint8_t pnum;
        ec = game_player_count(&rs->g, &pnum);
        if (ec != ERR_OK) {
            print_game_error(&rs->g, ec);
            printf("[ERROR] unexpected game player count error\n");
            return;
        }
        if (pov > pnum && pov != PLAYER_ENV) {
            printf("invalid pov for %u players\n", pnum);
            return;
        }
        movestr = argv[1];
    } else {
        printf("too many arguments for make move\n");
        return;
    }
    if (pov == PLAYER_NONE) {
        printf("can not make move as PLAYER_NONE\n");
        return;
    }
    //TODO check if pov is to move before continuing?
    move_data_sync* fill_move;
    ec = game_get_move_data(&rs->g, pov, movestr, &fill_move);
    if (ec != ERR_OK) {
        print_game_error(&rs->g, ec);
        printf("could not get move data\n");
        return;
    }
    move_data_sync move;
    game_e_move_sync_copy(&move, fill_move); //TODO this can fail oom
    ec = game_is_legal_move(&rs->g, pov, move);
    if (ec != ERR_OK) {
        print_game_error(&rs->g, ec);
        printf("move is not legal to make\n");
        return;
    }
    ec = game_make_move(&rs->g, pov, move);
    if (ec != ERR_OK) {
        print_game_error(&rs->g, ec);
        printf("[ERROR] unexpected move making error\n");
        return;
    }
    rs->history_head = move_history_insert(rs->history_head, NULL, pov, move, movestr);
}

void repl_cmd_handle_g_get_results(repl_state* rs, int argc, char** argv)
{
    if (rs == NULL) { // print help
        printf("usage: get_results\n");
        printf("list all the winning players\n");
        return;
    }
    if (rs->g.methods == NULL) {
        printf("no game running\n");
        return;
    }
    error_code ec;
    uint8_t size_fill;
    const player_id* res;
    ec = game_get_results(&rs->g, &size_fill, &res);
    if (ec != ERR_OK) {
        print_game_error(&rs->g, ec);
        printf("[ERROR] unexpected game get results error\n");
        return;
    }
    printf("results (%hhu):", size_fill);
    for (uint8_t i = 0; i < size_fill; i++) {
        printf(" %03hhu", res[i]);
    }
    printf("\n");
}

void repl_cmd_handle_g_print(repl_state* rs, int argc, char** argv)
{
    if (rs == NULL) { // print help
        printf("usage: print\n");
        printf("print the current board state\n");
        return;
    }
    if (rs->g.methods == NULL) {
        printf("no game running\n");
        return;
    }
    if (game_ff(&rs->g).print == false) {
        printf("game does not support feature: print\n");
        return;
    }
    error_code ec;
    size_t print_size;
    const char* print_str;
    ec = game_print(&rs->g, &print_size, &print_str);
    if (ec != ERR_OK) {
        print_game_error(&rs->g, ec);
        return;
    }
    printf("%s", print_str);
}

void repl_cmd_handle_gs_history(repl_state* rs, int argc, char** argv)
{
    if (rs == NULL) { // print help
        printf("usage: history\n");
        printf("show the move history that lead to this state\n");
        //TODO manipulation features planned for moving around the tree
        return;
    }
    if (rs->g.methods == NULL) {
        printf("no game running\n");
        return;
    }
    move_history* hn = rs->history->left_child;
    while (hn != NULL) {
        printf("%03hhu: %s\n", hn->player, hn->move_str);
        hn = hn->left_child;
    }
}

void repl_cmd_handle_gs_resolve_random(repl_state* rs, int argc, char** argv)
{
    if (rs == NULL) { // print help
        printf("usage: resolve_random [count] [seed]\n");
        printf("resolve the next count many random moves, or by default (0), until all random moves are done\n");
        return;
    }
    if (rs->g.methods == NULL) {
        printf("no game running\n");
        return;
    }
    if (game_ff(&rs->g).random_moves == false) {
        printf("game does not support feature: random_moves\n");
        return;
    }
    uint32_t random_count = 0;
    if (argc >= 1) {
        int sc = sscanf(argv[0], "%u", &random_count);
        if (sc != 1) {
            printf("could not parse count as u32\n");
            return;
        }
    }
    uint64_t random_seed = timestamp_get_ns64();
    if (argc >= 2) {
        int sc = sscanf(argv[1], "%lu", &random_seed);
        if (sc != 1) {
            printf("could not parse seed as u64\n");
            return;
        }
    }
    while (true) {
        random_seed++;
        error_code ec;
        // check that ptm random player is actually to move
        uint8_t ptm_c;
        const player_id* ptm;
        ec = game_players_to_move(&rs->g, &ptm_c, &ptm);
        if (ec != ERR_OK) {
            print_game_error(&rs->g, ec);
            printf("[ERROR] unexpected game player to move error\n");
            return;
        }
        if (ptm_c == 0 || ptm[0] != PLAYER_ENV) {
            break;
        }
        // TODO generate a better bigger seed, especially if not given by user
        seed128 big_seed = SEED128_NONE;
        ((uint64_t*)big_seed.bytes)[0] = random_seed;
        ((uint64_t*)big_seed.bytes)[1] = ~random_seed;
        // use get_random_move to get a random move
        move_data_sync random_move = game_e_get_random_move_sync(&rs->g, big_seed);
        // print chosen move
        size_t size_fill;
        const char* move_str;
        ec = game_get_move_str(&rs->g, PLAYER_ENV, random_move, &size_fill, &move_str);
        if (ec != ERR_OK) {
            print_game_error(&rs->g, ec);
            printf("[ERROR] unexpected game get move str error\n");
            return;
        }
        // printf("playing random move: %s\n", move_str); //TODO this needs a setting
        // play chosen move
        ec = game_make_move(&rs->g, PLAYER_ENV, random_move);
        if (ec != ERR_OK) {
            print_game_error(&rs->g, ec);
            printf("[ERROR] unexpected game make move error\n");
            return;
        }
        if (random_count > 0 && --random_count == 0) {
            break;
        }
    }
}
