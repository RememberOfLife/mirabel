#pragma once

#include <cstdint>
#include <vector>

#include "surena/engine.h"

#include "control/event_queue.hpp"

#include "surena/engine.h"

namespace Engines {

    class EngineManager {

        public:

            struct engine_container {
                engine e; // contains the engine_id
                eevent_queue* eq;
            };

            //TODO figure out what can be private
            uint32_t log;

            Control::event_queue* client_inbox;

            eevent_queue engine_outbox; // this is where engines post their results

            std::vector<engine_container> engines;

            EngineManager(Control::event_queue* _client_inbox);

            ~EngineManager();

            void game_load(game* target_game); // clones the game
            void game_state(const char* state);
            void game_move(player_id player, move_code code);
            void game_sync(void* data_start, void* data_end);

            void update();

    };

}
