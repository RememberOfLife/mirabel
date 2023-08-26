#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

#include <SDL.h>

#include "mirabel/engine.h"
#include "mirabel/event_queue.h"
#include "mirabel/game.h"
#include "control/client.hpp"
#include "control/plugins.hpp"
#include "meta_gui/meta_gui.hpp"

#include "engines/engine_manager.hpp"

namespace Engines {

    EngineManager::engine_container::engine_container(char* _name, player_id _ai_slot, uint32_t engine_id):
        open(true),
        remove(false),
        catalogue_idx(0),
        load_options(NULL),
        name(_name),
        swap_names(false),
        ai_slot(_ai_slot),
        e((engine){
            .methods = NULL,
            .engine_id = engine_id,
            .data1 = NULL,
            .data2 = NULL,
        }),
        eq(NULL),
        stopping(false),
        heartbeat_next_id(0),
        heartbeat_last_response(0),
        heartbeat_last_ticks(0),
        id_name(NULL),
        id_author(NULL),
        search_constraints((ee_engine_start){
            .player = PLAYER_NONE,
            .timeout = 0,
            .ponder = false,
            .time_ctl_count = 0,
            .time_ctl = NULL,
        }),
        search_constraints_timectl(false),
        search_constraints_open(false),
        searching(false),
        searchinfo((ee_engine_searchinfo){
            .flags = 0,
        }),
        bestmove((ee_engine_bestmove){
            .count = 0,
            .player = NULL,
            .move = NULL,
            .confidence = NULL,
        })
    {
        name_swap = (char*)malloc(STR_BUF_MAX);
        strcpy(name_swap, name);
    }

    EngineManager::engine_container::~engine_container()
    {
        free(name);
        name = NULL;
        free(name_swap);
        name_swap = NULL;
        free(id_name);
        id_name = NULL;
        free(id_author);
        id_author = NULL;
        for (int i = 0; i < options.size(); i++) {
            destroy_option(&options[i]);
        }
        options.clear();
        free(bestmove.player);
        free(bestmove.move);
        free(bestmove.confidence);
        bestmove = (ee_engine_bestmove){
            .count = 0,
            .player = NULL,
            .move = NULL,
            .confidence = NULL,
        };
        for (int i = 0; i < bestmove_strings.size(); i++) {
            free(bestmove_strings[i]);
        }
        bestmove_strings.clear();
    }

    void EngineManager::engine_container::start_search()
    {
        searchinfo.flags = 0; // for now reset flags at least before a new search
        engine_event te;
        eevent_create_start(&te, e.engine_id, search_constraints.player, search_constraints.timeout, search_constraints.ponder, 0, 0); //TODO timectl
        eevent_queue_push(eq, &te);
        eevent_destroy(&te);
    }

    void EngineManager::engine_container::search_poll_bestmove()
    {
        engine_event te;
        eevent_create_bestmove_empty(&te, e.engine_id);
        eevent_queue_push(eq, &te);
        eevent_destroy(&te);
    }

    void EngineManager::engine_container::stop_search()
    {
        engine_event te;
        eevent_create_stop(&te, e.engine_id, true, true);
        eevent_queue_push(eq, &te);
        eevent_destroy(&te);
    }

    void EngineManager::engine_container::submit_option(ee_engine_option* option)
    {
        engine_event te;
        switch (option->type) {
            case EE_OPTION_TYPE_CHECK: {
                eevent_create_option_check(&te, e.engine_id, option->name, option->value.check);
            } break;
            case EE_OPTION_TYPE_SPIN: {
                eevent_create_option_spin(&te, e.engine_id, option->name, option->value.spin);
            } break;
            case EE_OPTION_TYPE_COMBO: {
                eevent_create_option_combo(&te, e.engine_id, option->name, option->value.combo);
            } break;
            case EE_OPTION_TYPE_BUTTON: {
                eevent_create_option_button(&te, e.engine_id, option->name);
            } break;
            case EE_OPTION_TYPE_STRING: {
                eevent_create_option_string(&te, e.engine_id, option->name, option->value.str);
            } break;
            case EE_OPTION_TYPE_SPIND: {
                eevent_create_option_spind(&te, e.engine_id, option->name, option->value.spind);
            } break;
            case EE_OPTION_TYPE_U64: {
                eevent_create_option_u64(&te, e.engine_id, option->name, option->value.u64);
            } break;
        }
        eevent_queue_push(eq, &te);
        eevent_destroy(&te);
    }

    void EngineManager::engine_container::destroy_option(ee_engine_option* option)
    {
        free(option->name);
        option->name = NULL;
        if (option->type == EE_OPTION_TYPE_COMBO) {
            // do not destroy value.combo, it points to within l.v.var
            free(option->l.v.var);
        }
        if (option->type == EE_OPTION_TYPE_STRING) {
            free(option->value.str);
        }
        option->type = EE_OPTION_TYPE_NONE;
    }

    EngineManager::EngineManager(event_queue* _client_inbox):
        client_inbox(_client_inbox)
    {
        log = MetaGui::log_register("engines");
        eevent_queue_create(&engine_outbox);
    }

    EngineManager::~EngineManager()
    {
        //TODO send exit to all engines, wait, destroy all containers
        eevent_queue_destroy(&engine_outbox);
        MetaGui::log_unregister(log);
    }

    void EngineManager::game_load(game* target_game)
    {
        engine_event e;
        for (int i = 0; i < engines.size(); i++) {
            if (engines[i]->eq) {
                if (target_game != NULL) {
                    eevent_create_load(&e, engines[i]->e.engine_id, target_game);
                } else {
                    eevent_create(&e, engines[i]->e.engine_id, EE_TYPE_GAME_UNLOAD);
                }
                eevent_queue_push(engines[i]->eq, &e);
                eevent_destroy(&e);
            }
        }
    }

    void EngineManager::game_state(const char* state)
    {
        engine_event e;
        for (int i = 0; i < engines.size(); i++) {
            if (engines[i]->eq) {
                eevent_create_state(&e, engines[i]->e.engine_id, state);
                eevent_queue_push(engines[i]->eq, &e);
                eevent_destroy(&e);
            }
        }
    }

    void EngineManager::game_move(player_id player, move_data_sync data)
    {
        engine_event e;
        for (int i = 0; i < engines.size(); i++) {
            if (engines[i]->eq) {
                eevent_create_move(&e, engines[i]->e.engine_id, player, code);
                eevent_queue_push(engines[i]->eq, &e);
                eevent_destroy(&e);
            }
        }
    }

    void EngineManager::game_sync(void* data_start, void* data_end)
    {
        engine_event e;
        for (int i = 0; i < engines.size(); i++) {
            if (engines[i]->eq) {
                eevent_create_sync(&e, engines[i]->e.engine_id, data_start, data_end);
                eevent_queue_push(engines[i]->eq, &e);
                eevent_destroy(&e);
            }
        }
    }

    void EngineManager::update()
    {
        engine_event e = (engine_event){
            .type = EE_TYPE_NULL};
        do {
            eevent_destroy(&e);
            eevent_queue_pop(&engine_outbox, &e, 0);
            if (e.type == EE_TYPE_NULL) {
                break;
            }
            engine_container* tec_p = container_by_engine_id(e.engine_id);
            if (tec_p == NULL) {
                MetaGui::logf(log, "#W E%u container does not exists, eevent discarded\n");
                continue;
            }
            engine_container& tec = *tec_p;
            switch (e.type) {
                case EE_TYPE_NULL: {
                    // pass
                } break;
                case EE_TYPE_EXIT: {
                    MetaGui::logf(log, "E%u exitted\n", e.engine_id);
                    tec.e.methods->destroy(&tec.e);
                    tec.eq = NULL;
                    tec.stopping = false;
                    tec.heartbeat_next_id = 0;
                    tec.heartbeat_last_response = 0;
                    tec.heartbeat_last_ticks = 0;
                    free(tec.id_name);
                    tec.id_name = NULL;
                    free(tec.id_author);
                    tec.id_author = NULL;
                    tec.search_constraints = (ee_engine_start){
                        .player = PLAYER_NONE,
                        .timeout = 0,
                        .ponder = false,
                        .time_ctl_count = 0,
                        .time_ctl = NULL,
                    };
                    tec.search_constraints_timectl = false;
                    tec.search_constraints_open = false;
                    tec.searching = false;
                    tec.searchinfo = (ee_engine_searchinfo){
                        .flags = 0,
                    };
                    for (int i = 0; i < tec.options.size(); i++) {
                        tec.destroy_option(&tec.options[i]);
                    }
                    tec.options.clear();
                    free(tec.bestmove.player);
                    free(tec.bestmove.move);
                    free(tec.bestmove.confidence);
                    tec.bestmove = (ee_engine_bestmove){
                        .count = 0,
                        .player = NULL,
                        .move = NULL,
                        .confidence = NULL,
                    };
                    for (int i = 0; i < tec.bestmove_strings.size(); i++) {
                        free(tec.bestmove_strings[i]);
                    }
                    tec.bestmove_strings.clear();
                    if (tec.open == false) {
                        tec.remove = true;
                    }
                } break;
                case EE_TYPE_LOG: {
                    if (e.log.ec == ERR_OK) {
                        MetaGui::logf(log, "E%u LOG ec#%u %s\n", e.engine_id, e.log.ec, e.log.text);
                    } else {
                        MetaGui::logf(log, "#W E%u LOG ec#%u %s\n", e.engine_id, e.log.ec, e.log.text);
                    }
                } break;
                case EE_TYPE_HEARTBEAT: {
                    if (e.heartbeat.id > tec.heartbeat_last_response) {
                        tec.heartbeat_last_response = e.heartbeat.id;
                    }
                } break;
                case EE_TYPE_GAME_LOAD:
                case EE_TYPE_GAME_UNLOAD:
                case EE_TYPE_GAME_STATE:
                case EE_TYPE_GAME_MOVE:
                case EE_TYPE_GAME_SYNC: {
                    assert(0);
                } break;
                case EE_TYPE_GAME_DRAW: {
                    MetaGui::logf("#I E%u offers%s a draw\n", e.engine_id, e.draw.accept ? "/accepts" : "");
                } break;
                case EE_TYPE_GAME_RESIGN: {
                    MetaGui::logf("#I E%u wants to resign\n", e.engine_id);
                } break;
                case EE_TYPE_ENGINE_ID: {
                    tec.id_name = e.id.name ? strdup(e.id.name) : NULL;
                    tec.id_author = e.id.author ? strdup(e.id.author) : NULL;
                } break;
                case EE_TYPE_ENGINE_OPTION: {
                    tec.options.push_back((ee_engine_option){
                        .name = strdup(e.option.name),
                        .type = e.option.type,
                    });
                    tec.options_changed.push_back(false);
                    switch (e.option.type) {
                        case EE_OPTION_TYPE_NONE: {
                            if (e.option.name == NULL) {
                                break;
                            }
                            for (int i = 0; i < tec.options.size(); i++) {
                                if (strcmp(tec.options[i].name, e.option.name) == 0) {
                                    tec.destroy_option(&tec.options[i]);
                                    tec.options.erase(tec.options.begin() + 1);
                                    tec.options_changed.erase(tec.options_changed.begin() + 1);
                                    break;
                                }
                            }
                        } break;
                        case EE_OPTION_TYPE_CHECK: {
                            tec.options.back().value.check = e.option.value.check;
                        } break;
                        case EE_OPTION_TYPE_SPIN: {
                            tec.options.back().value.spin = e.option.value.spin;
                            tec.options.back().l.mm = e.option.l.mm;
                        } break;
                        case EE_OPTION_TYPE_COMBO: {
                            // the options combo points to the selected element in the v.var
                            {
                                size_t varsize = 1;
                                const char* varp = e.option.l.v.var;
                                while (*varp != '\0') {
                                    size_t svs = strlen(varp) + 1;
                                    varsize += svs;
                                    varp += svs;
                                }
                                tec.options.back().l.v.var = (char*)malloc(varsize);
                                memcpy(tec.options.back().l.v.var, e.option.l.v.var, varsize);
                            }
                            tec.options.back().value.combo = NULL;
                            char* p_sel_combo = tec.options.back().l.v.var;
                            while (*p_sel_combo != '\0') {
                                MetaGui::logf("%s %s\n", p_sel_combo, e.option.value.combo);
                                if (strcmp(e.option.value.combo, p_sel_combo) == 0) {
                                    tec.options.back().value.combo = p_sel_combo;
                                    break;
                                }
                                p_sel_combo += strlen(p_sel_combo) + 1;
                            }
                            if (tec.options.back().value.combo == NULL) {
                                // default combo selection not among the given options, abort adding this option
                                free(tec.options.back().l.v.var);
                                free(tec.options.back().name);
                                tec.options.pop_back();
                                tec.options_changed.pop_back();
                                MetaGui::logf("#W E%u sent an invalid combobox selection\n", e.engine_id);
                                break;
                            }
                        } break;
                        case EE_OPTION_TYPE_BUTTON: {
                            // pass
                        } break;
                        case EE_OPTION_TYPE_STRING: {
                            tec.options.back().value.str = (char*)malloc(STR_BUF_MAX);
                            strcpy(tec.options.back().value.str, e.option.value.str);
                        } break;
                        case EE_OPTION_TYPE_SPIND: {
                            tec.options.back().value.spind = e.option.value.spind;
                            tec.options.back().l.mmd = e.option.l.mmd;
                        } break;
                        case EE_OPTION_TYPE_U64: {
                            tec.options.back().value.u64 = e.option.value.u64;
                            tec.options.back().l.mm = e.option.l.mm;
                        } break;
                        default: {
                            free(tec.options.back().name);
                            tec.options.pop_back();
                            tec.options_changed.pop_back();
                            MetaGui::logf(log, "#W E%u sent invalid option type %u\n", e.engine_id, e.option.type);
                        } break;
                    }
                } break;
                case EE_TYPE_ENGINE_START: {
                    tec.searching = true;
                } break;
                case EE_TYPE_ENGINE_SEARCHINFO: {
                    tec.searchinfo.flags |= e.searchinfo.flags;
                    if (e.searchinfo.flags & EE_SEARCHINFO_FLAG_TYPE_TIME) {
                        tec.searchinfo.time = e.searchinfo.time;
                    }
                    if (e.searchinfo.flags & EE_SEARCHINFO_FLAG_TYPE_DEPTH) {
                        tec.searchinfo.depth = e.searchinfo.depth;
                    }
                    if (e.searchinfo.flags & EE_SEARCHINFO_FLAG_TYPE_SELDEPTH) {
                        tec.searchinfo.seldepth = e.searchinfo.seldepth;
                    }
                    if (e.searchinfo.flags & EE_SEARCHINFO_FLAG_TYPE_NODES) {
                        tec.searchinfo.nodes = e.searchinfo.nodes;
                    }
                    if (e.searchinfo.flags & EE_SEARCHINFO_FLAG_TYPE_NPS) {
                        tec.searchinfo.nps = e.searchinfo.nps;
                    }
                    if (e.searchinfo.flags & EE_SEARCHINFO_FLAG_TYPE_HASHFULL) {
                        tec.searchinfo.hashfull = e.searchinfo.hashfull;
                    }
                } break;
                case EE_TYPE_ENGINE_SCOREINFO: {
                    //TODO
                    MetaGui::logf(log, "E%u recv scoreinfo\n", e.engine_id);
                } break;
                case EE_TYPE_ENGINE_LINEINFO: {
                    //TODO
                    MetaGui::logf(log, "E%u recv lineinfo\n", e.engine_id);
                } break;
                case EE_TYPE_ENGINE_STOP: {
                    tec.searching = false;
                } break;
                case EE_TYPE_ENGINE_BESTMOVE: {
                    if (Control::main_client->the_game == NULL) {
                        break;
                    }
                    free(tec.bestmove.player);
                    free(tec.bestmove.move);
                    free(tec.bestmove.confidence);
                    tec.bestmove = (ee_engine_bestmove){
                        .count = 0,
                        .player = NULL,
                        .move = NULL,
                        .confidence = NULL,
                    };
                    for (int i = 0; i < tec.bestmove_strings.size(); i++) {
                        free(tec.bestmove_strings[i]);
                    }
                    tec.bestmove_strings.clear();
                    tec.bestmove.count = e.bestmove.count;
                    tec.bestmove.player = (player_id*)malloc(sizeof(player_id) * tec.bestmove.count);
                    tec.bestmove.move = (move_code*)malloc(sizeof(move_code) * tec.bestmove.count);
                    tec.bestmove.confidence = (float*)malloc(sizeof(float) * tec.bestmove.count);
                    memcpy(tec.bestmove.player, e.bestmove.player, sizeof(player_id) * tec.bestmove.count);
                    memcpy(tec.bestmove.move, e.bestmove.move, sizeof(move_code) * tec.bestmove.count);
                    memcpy(tec.bestmove.confidence, e.bestmove.confidence, sizeof(float) * tec.bestmove.count);
                    for (int i = 0; i < tec.bestmove.count; i++) {
                        char* move_str = (char*)malloc(Control::main_client->the_game->sizer.move_str);
                        size_t move_len = 0;
                        Control::main_client->the_game->methods->get_move_str(Control::main_client->the_game, tec.bestmove.player[i], tec.bestmove.move[i], &move_len, move_str);
                        tec.bestmove_strings.push_back(move_str);
                    }
                } break;
                case EE_TYPE_ENGINE_MOVESCORE: {
                    //TODO
                    MetaGui::logf(log, "E%u recv movescore\n", e.engine_id);
                } break;
                default: {
                    assert(0);
                } break;
            }
        } while (e.type != EE_TYPE_NULL);
        uint64_t sdl_ticks = SDL_GetTicks64();
        for (int i = 0; i < engines.size();) {
            engine_container& tec = *engines[i];
            if (tec.eq != NULL && sdl_ticks - tec.heartbeat_last_ticks > 3000) { // 3000ms bewteen forced heartbeats
                eevent_create_heartbeat(&e, tec.e.engine_id, tec.heartbeat_next_id++);
                eevent_queue_push(tec.eq, &e);
                eevent_destroy(&e);
                tec.heartbeat_last_ticks = sdl_ticks;
            }
            if (tec.remove) {
                delete engines[i];
                engines.erase(engines.begin() + i);
            } else {
                i++;
            }
        }
    }

    EngineManager::engine_container* EngineManager::container_by_engine_id(uint32_t engine_id)
    {
        for (int i = 0; i < engines.size(); i++) {
            if (engines[i]->e.engine_id == engine_id) {
                return engines[i];
            }
        }
        return NULL;
    }

    void EngineManager::add_container(player_id ai_slot)
    {
        char* name = (char*)malloc(STR_BUF_MAX);
        if (ai_slot == PLAYER_NONE) {
            sprintf(name, "E%d", next_engine_id);
        } else {
            sprintf(name, "P%03hhu", ai_slot);
        }
        engines.push_back(new engine_container(name, ai_slot, next_engine_id++));
    }

    void EngineManager::rename_container(uint32_t container_idx)
    {
        assert(container_idx < engines.size());
        engine_container& tec = *engines[container_idx];
        if (strlen(tec.name_swap) < 2) {
            strcpy(tec.name_swap, tec.name);
            return;
        }
        for (int i = 0; i < engines.size(); i++) {
            if (strcmp(tec.name_swap, engines[i]->name) == 0) {
                strcpy(tec.name_swap, tec.name);
                return;
            }
        }
        strcpy(tec.name, tec.name_swap);
        tec.swap_names = false;
    }

    void EngineManager::start_container(uint32_t container_idx)
    {
        assert(container_idx < engines.size());
        engine_container& tec = *engines[container_idx];
        tec.e.methods = Control::main_client->plugin_mgr.engine_lookup[tec.catalogue_idx]->get_methods();
        {
            //TODO handle engine errors
            //tec.e.methods->create_with_opts_bin(&tec.e, tec.e.engine_id, &engine_outbox, &tec.eq, tec.load_options);
            // binary opts engines wrap needs to provide bin to str
            //TODO //BUG
            tec.e.methods->create(&tec.e, tec.e.engine_id, &engine_outbox, &tec.eq, NULL);
        }
        engine_event e;
        eevent_create_heartbeat(&e, tec.e.engine_id, tec.heartbeat_next_id++);
        eevent_queue_push(tec.eq, &e);
        eevent_destroy(&e);
        tec.heartbeat_last_ticks = SDL_GetTicks64();
        // send the engine the currently running game, if any
        if (Control::main_client->the_game != NULL) {
            eevent_create_load(&e, tec.e.engine_id, Control::main_client->the_game);
            eevent_queue_push(tec.eq, &e);
            eevent_destroy(&e);
        }
    }

    void EngineManager::stop_container(uint32_t container_idx)
    {
        assert(container_idx < engines.size());
        engine_container& tec = *engines[container_idx];
        engine_event e;
        eevent_create(&e, tec.e.engine_id, EE_TYPE_EXIT);
        eevent_queue_push(tec.eq, &e);
        eevent_destroy(&e);
        tec.stopping = true;
    }

    void EngineManager::remove_container(uint32_t container_idx)
    {
        assert(container_idx < engines.size());
        engine_container& tec = *engines[container_idx];
        tec.open = false;
        if (tec.eq != NULL) {
            stop_container(container_idx);
        } else {
            tec.remove = true;
        }
    }

} // namespace Engines
