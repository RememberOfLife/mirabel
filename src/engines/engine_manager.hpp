#pragma once

#include <cstdint>
#include <vector>

#include "surena/engine.h"
#include "surena/game.h"

#include "control/event_queue.hpp"

#include "surena/engine.h"

namespace Engines {

    class EngineManager {

        public:

            static const size_t STR_BUF_MAX = 512;

            uint32_t next_engine_id = 1;

            struct engine_container {
                bool open;
                bool remove;
                uint32_t catalogue_idx;
                void* load_options;
                char* name; // tab name
                char* name_swap; // editing tab name
                bool swap_names;
                player_id ai_slot; // ai_slot != 0 to make the tab unclosable and mark with dot to show connection to an ai
                engine e; // contains the engine_id, which uniquely identifies this tab
                eevent_queue* eq; // also used to easily test if an engine exists here
                bool stopping;
                uint32_t heartbeat_next_id;
                uint32_t heartbeat_last_response;
                uint64_t heartbeat_last_ticks;
                char* id_name;
                char* id_author;
                std::vector<ee_engine_option> options; // deletion is costly, but sparse
                std::vector<bool> options_changed;
                ee_engine_start search_constraints; //TODO integrate player and timectl
                bool search_constraints_open;
                bool searching;
                ee_engine_stop stop_opts;
                ee_engine_searchinfo searchinfo;
                //TODO score info
                //TODO line info
                //TODO bestmove
                //TODO movescore

                //TODO maybe some search_info update time struct so we can visually highlight new search info as it comes in
                //TODO error log nums etc and DISPLAY ID IN THE ENGINE METAGUI WINDOW!!!!

                engine_container(char* _name, player_id _ai_slot, uint32_t engine_id); // takes ownership of name
                engine_container(const engine_container& other); // copy construct
                engine_container(engine_container&& other); // move construct
                engine_container& operator=(const engine_container& other); // copy assign
                engine_container& operator=(engine_container&& other); // move assign
                ~engine_container();
            };

            //TODO figure out what can be private
            uint32_t log;

            Control::event_queue* client_inbox;

            eevent_queue engine_outbox; // this is where engines post their results

            std::vector<engine_container> engines;

            // methods

            EngineManager(Control::event_queue* _client_inbox);

            ~EngineManager();

            void game_load(game* target_game); // clones the game
            void game_state(const char* state);
            void game_move(player_id player, move_code code);
            void game_sync(void* data_start, void* data_end);

            void update();

            engine_container* container_by_engine_id(uint32_t engine_id);

            void add_container(player_id ai_slot);
            void rename_container(uint32_t container_idx);
            void start_container(uint32_t container_idx);
            void stop_container(uint32_t container_idx);
            void remove_container(uint32_t container_idx); // the container will only be removed next update

    };

}
