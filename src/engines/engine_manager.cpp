#include <cstdint>
#include <cstring>
#include <vector>

#include "surena/engine.h"
#include "surena/game.h"

#include "control/event_queue.hpp"
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
        searching(false)
    {
        name_swap = (char*)malloc(STR_BUF_MAX);
        strcpy(name_swap, name);
    }

    EngineManager::engine_container::engine_container(const engine_container& other) // copy construct
    {
        memcpy(this, &other, sizeof(engine_container));
        name = (char*)malloc(STR_BUF_MAX);
        strcpy(name, other.name);
        name_swap = (char*)malloc(STR_BUF_MAX);
        strcpy(name_swap, other.name_swap);
    }

    EngineManager::engine_container::engine_container(engine_container&& other) // move construct
    {
        memcpy(this, &other, sizeof(engine_container));
        name = other.name;
        other.name = NULL;
        name_swap = other.name_swap;
        other.name_swap = NULL;
    }
    
    EngineManager::engine_container& EngineManager::engine_container::operator=(const engine_container& other) // copy assign
    {
        memcpy(this, &other, sizeof(engine_container));
        name = (char*)malloc(STR_BUF_MAX);
        strcpy(name, other.name);
        name_swap = (char*)malloc(STR_BUF_MAX);
        strcpy(name_swap, other.name_swap);
        return *this;
    }
    
    EngineManager::engine_container& EngineManager::engine_container::operator=(engine_container&& other) // move assign
    {
        memcpy(this, &other, sizeof(engine_container));
        name = other.name;
        other.name = NULL;
        name_swap = other.name_swap;
        other.name_swap = NULL;
        return *this;
    }
    
    EngineManager::engine_container::~engine_container()
    {
        free(name);
        name = NULL;
        free(name_swap);
        name_swap = NULL;
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
            switch (e.type) {
                //TODO
                case EE_TYPE_EXIT: {
                    engine_container& tec = *container_by_engine_id(e.engine_id);
                    tec.e.methods->destroy(&tec.e);
                    tec.remove = true;
                } break;
            }
        } while (e.type != EE_TYPE_NULL);
        for (int i = 0; i < engines.size(); ) {
            if (engines[i].remove) {
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

    void EngineManager::remove_container(uint32_t container_idx)
    {
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

    void EngineManager::rename_container(uint32_t container_idx)
    {
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

}
