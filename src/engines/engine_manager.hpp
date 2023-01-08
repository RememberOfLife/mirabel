#pragma once

#include <cstdint>
#include <vector>

#include "surena/engine.h"
#include "surena/game.h"

#include "mirabel/event_queue.h"

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
            bool search_constraints_timectl;
            bool search_constraints_open;
            bool searching;
            // no stop opts because we always want all score infos and all move scores if available
            ee_engine_searchinfo searchinfo; //TODO when to reset this? start/stop?, also on game load/move etc?
            //TODO score info
            //TODO line info
            ee_engine_bestmove bestmove;
            std::vector<char*> bestmove_strings;
            //TODO movescore

            //TODO draw line info table all the way at the bottom, use movescores to give evals, if no lines create 1 for every move
            //TODO display score info in the same table as bestmove, just lut eval there too

            //TODO maybe some search_info update time struct so we can visually highlight new search info as it comes in
            //TODO error log nums etc and DISPLAY ID IN THE ENGINE METAGUI WINDOW!!!!

            engine_container(char* _name, player_id _ai_slot, uint32_t engine_id); // takes ownership of name
            engine_container(const engine_container& other) = delete; // copy construct
            engine_container(engine_container&& other) = delete; // move construct
            engine_container& operator=(const engine_container& other) = delete; // copy assign
            engine_container& operator=(engine_container&& other) = delete; // move assign
            ~engine_container();

            void start_search();
            void search_poll_bestmove();
            void stop_search();

            void submit_option(ee_engine_option* option);
            void destroy_option(ee_engine_option* option);
        };

        //TODO figure out what can be private
        uint32_t log;

        event_queue* client_inbox;

        eevent_queue engine_outbox; // this is where engines post their results

        std::vector<engine_container*> engines;

        // methods

        EngineManager(event_queue* _client_inbox);

        ~EngineManager();

        void game_load(game* target_game); // clones the game, unload is just game == NULL
        void game_state(const char* state);
        void game_move(player_id player, move_data_sync data);
        void game_sync(void* data_start, void* data_end);

        void update();

        engine_container* container_by_engine_id(uint32_t engine_id);

        void add_container(player_id ai_slot);
        void rename_container(uint32_t container_idx);
        void start_container(uint32_t container_idx);
        void stop_container(uint32_t container_idx);
        void remove_container(uint32_t container_idx); // the container will only be removed next update
    };

} // namespace Engines
