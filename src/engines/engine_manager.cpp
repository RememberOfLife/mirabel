#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

#include <SDL2/SDL.h>
#include "surena/engine.h"
#include "surena/game.h"

#include "control/event_queue.hpp"
#include "engines/engine_catalogue.hpp"
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
        search_constraints_open(false),
        searching(false),
        stop_opts((ee_engine_stop){
            .all_score_infos = true,
            .all_move_scores = false,
        }),
        searchinfo((ee_engine_searchinfo){
            .flags = 0,
        })
    {
        name_swap = (char*)malloc(STR_BUF_MAX);
        strcpy(name_swap, name);
    }

    //TODO move bulk of this code into a construct from existing, and remove redundant code here
    //BUG does this all even work with the two vectors?! likely not due to memcpy

    EngineManager::engine_container::engine_container(const engine_container& other) // copy construct
    {
        memcpy(this, &other, sizeof(engine_container));
        name = (char*)malloc(STR_BUF_MAX);
        strcpy(name, other.name);
        name_swap = (char*)malloc(STR_BUF_MAX);
        strcpy(name_swap, other.name_swap);
        id_name = other.id_name ? strdup(other.id_name) : NULL;
        id_author = other.id_author ? strdup(other.id_author) : NULL;
    }

    EngineManager::engine_container::engine_container(engine_container&& other) // move construct
    {
        memcpy(this, &other, sizeof(engine_container));
        name = other.name;
        other.name = NULL;
        name_swap = other.name_swap;
        other.name_swap = NULL;
        id_name = other.id_name;
        other.id_name = NULL;
        id_author = other.id_author;
        other.id_author = NULL;
    }
    
    EngineManager::engine_container& EngineManager::engine_container::operator=(const engine_container& other) // copy assign
    {
        memcpy(this, &other, sizeof(engine_container));
        name = (char*)malloc(STR_BUF_MAX);
        strcpy(name, other.name);
        name_swap = (char*)malloc(STR_BUF_MAX);
        strcpy(name_swap, other.name_swap);
        id_name = other.id_name ? strdup(other.id_name) : NULL;
        id_author = other.id_author ? strdup(other.id_author) : NULL;
        return *this;
    }
    
    EngineManager::engine_container& EngineManager::engine_container::operator=(engine_container&& other) // move assign
    {
        memcpy(this, &other, sizeof(engine_container));
        name = other.name;
        other.name = NULL;
        name_swap = other.name_swap;
        other.name_swap = NULL;
        id_name = other.id_name;
        other.id_name = NULL;
        id_author = other.id_author;
        other.id_author = NULL;
        return *this;
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
    }

    EngineManager::EngineManager(Control::event_queue* _client_inbox)
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
        //TODO
    }

    void EngineManager::game_state(const char* state)
    {
        //TODO
    }
    
    void EngineManager::game_move(player_id player, move_code code)
    {
        //TODO
    }
    
    void EngineManager::game_sync(void* data_start, void* data_end)
    {
        //TODO
    }
    
    void EngineManager::update()
    {
        engine_event e = (engine_event){
            .type = EE_TYPE_NULL
        };
        do {
            eevent_destroy(&e);
            eevent_queue_pop(&engine_outbox, &e, 0);
            if (e.type == EE_TYPE_NULL) {
                break;
            }
            engine_container* tec_p = container_by_engine_id(e.engine_id);
            if (tec_p == NULL) {
                MetaGui::logf("#W E%u container does not exists, eevent discarded\n");
                continue;
            }
            engine_container& tec = *tec_p;
            switch (e.type) { //TODO
                case EE_TYPE_NULL: {
                    // pass
                } break;
                case EE_TYPE_EXIT: {
                    MetaGui::logf(log, "E%u exitted\n");
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
                    tec.options.clear();
                    tec.options_changed.clear();
                    tec.search_constraints = (ee_engine_start){
                        .player = PLAYER_NONE,
                        .timeout = 0,
                        .ponder = false,
                        .time_ctl_count = 0,
                        .time_ctl = NULL,
                    };
                    tec.search_constraints_open = false;
                    tec.searching = false;
                    tec.stop_opts = (ee_engine_stop){
                        .all_score_infos = true,
                        .all_move_scores = false,
                    };
                    tec.searchinfo = (ee_engine_searchinfo){
                        .flags = 0,
                    };
                    if (tec.open != true) {
                        tec.remove = true;
                    }
                } break;
                case EE_TYPE_LOG: {
                    if (e.log.ec == ERR_OK) {
                        MetaGui::logf(log, "E%u: ec#%u %s\n", e.engine_id, e.log.ec, e.log.text);
                    } else {
                        MetaGui::logf(log, "#W E%u: ec#%u %s\n", e.engine_id, e.log.ec, e.log.text);
                    }
                } break;
                case EE_TYPE_HEARTBEAT: {
                    MetaGui::logf(log, "E%u heartbeat response %u\n", e.engine_id, e.heartbeat.id);
                    //TODO instead of loggin here, show a marker on the gui how fresh the engine heartbeat is
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
                        case EE_OPTION_TYPE_CHECK: {
                            tec.options.back().value.check = e.option.value.check;
                        } break;
                        case EE_OPTION_TYPE_SPIN: {
                            tec.options.back().value.spin = e.option.value.spin;
                            tec.options.back().mm = e.option.mm;
                        } break;
                        case EE_OPTION_TYPE_COMBO: {
                            tec.options.back().value.str = (char*)malloc(STR_BUF_MAX);
                            strcpy(tec.options.back().value.str, e.option.value.str);
                            tec.options.back().v.var = strdup(e.option.v.var);
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
                            tec.options.back().mmd = e.option.mmd;
                        } break;
                        case EE_OPTION_TYPE_U64: {
                            tec.options.back().value.u64 = e.option.value.u64;
                            tec.options.back().mm = e.option.mm;
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
                    assert(0);
                } break;
                case EE_TYPE_ENGINE_SEARCHINFO: {

                } break;
                case EE_TYPE_ENGINE_SCOREINFO: {

                } break;
                case EE_TYPE_ENGINE_LINEINFO: {

                } break;
                case EE_TYPE_ENGINE_STOP: {
                    assert(0);
                } break;
                case EE_TYPE_ENGINE_BESTMOVE: {

                } break;
                case EE_TYPE_ENGINE_MOVESCORE: {

                } break;
                default: {
                    assert(0);
                } break;
            }
        } while (e.type != EE_TYPE_NULL);
        uint64_t sdl_ticks = SDL_GetTicks64();
        for (int i = 0; i < engines.size(); ) {
            engine_container& tec = engines[i];
            if (tec.eq != NULL && sdl_ticks - tec.heartbeat_last_ticks > 3000) { // 3000ms bewteen forced heartbeats
                eevent_create_heartbeat(&e, tec.e.engine_id, tec.heartbeat_next_id++);
                eevent_queue_push(tec.eq, &e);
                eevent_destroy(&e);
                tec.heartbeat_last_ticks = sdl_ticks;
            }
            if (tec.remove) {
                engines.erase(engines.begin() + i);
            } else {
                i++;
            }
        }
    }
    
    EngineManager::engine_container* EngineManager::container_by_engine_id(uint32_t engine_id)
    {
        for (int i = 0; i < engines.size(); i++) {
            if (engines[i].e.engine_id == engine_id) {
                return &engines[i];
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
        engines.emplace_back(name, ai_slot, next_engine_id++);
    }

    void EngineManager::rename_container(uint32_t container_idx)
    {
        assert(container_idx < engines.size());
        engine_container& tec = engines[container_idx];
        for (int i = 0; i < engines.size(); i++) {
            if (strcmp(tec.name_swap, engines[i].name) == 0) {
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
        engine_container& tec = engines[container_idx];
        tec.e.methods = engine_catalogue[tec.catalogue_idx];
        //TODO handle engine errors
        if (tec.load_options == NULL) {
            tec.e.methods->create_default(&tec.e, tec.e.engine_id, &engine_outbox, &tec.eq);
        } else {
            tec.e.methods->create_with_opts_bin(&tec.e, tec.e.engine_id, &engine_outbox, &tec.eq, tec.load_options);
        }
        engine_event e;
        eevent_create_heartbeat(&e, tec.e.engine_id, tec.heartbeat_next_id++);
        eevent_queue_push(tec.eq, &e);
        eevent_destroy(&e);
        tec.heartbeat_last_ticks = SDL_GetTicks64();
    }

    void EngineManager::stop_container(uint32_t container_idx)
    {
        assert(container_idx < engines.size());
        engine_container& tec = engines[container_idx];
        engine_event e;
        eevent_create(&e, tec.e.engine_id, EE_TYPE_EXIT);
        eevent_queue_push(tec.eq, &e);
        eevent_destroy(&e);
        tec.stopping = true;
    }

    void EngineManager::remove_container(uint32_t container_idx)
    {
        assert(container_idx < engines.size());
        engine_container& tec = engines[container_idx];
        tec.open = false;
        if (tec.eq != NULL) {
            engine_event e;
            eevent_create(&e, tec.e.engine_id, EE_TYPE_EXIT);
            eevent_queue_push(tec.eq, &e);
            eevent_destroy(&e);
        } else {
            tec.remove = true;
        }
    }

}
