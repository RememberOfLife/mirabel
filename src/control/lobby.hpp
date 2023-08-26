#pragma once
//TODO maybe put client and server+this into specific directories, then control holds only general control logic

#include <cstdint>

#include "mirabel/game.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "control/plugins.hpp"

namespace Control {

    class Lobby {
      public:

        PluginManager* plugin_mgr;
        event_queue* send_queue;

        uint32_t id;
        char* name;
        bool usepw;
        uint32_t pwhash; //TODO obviously not secure, replace with more proper hashing when lobby prototypes are done
        //TODO lobby owner client id for admin actions

        char* game_base;
        char* game_variant;
        char* game_impl;
        char* game_options;
        game* the_game;
        // move_history* history; //TODO use this to send new players the game if it does not support serialization
        // bool game_trusted; // true if full game has only ever been on the server, i.e. no hidden state leaked, false if game is loaded from a user
        uint16_t max_users;

        struct lobby_user {
            uint32_t client_id;
            std::vector<player_id> plays_for;
        };

        std::vector<lobby_user> users; //TODO should use some user struct, for now just stores client ids of connected clients

        uint32_t lobby_msg_id_ctr = 1;

        Lobby(uint32_t id, const char* name, bool usepw, uint32_t pwhash, PluginManager* plugin_mgr, event_queue* send_queue, uint16_t max_users);
        ~Lobby();

        void AddUser(uint32_t client_id);
        void RemoveUser(uint32_t client_id);

        void HandleEvent(event_any e); // handle events that are specifically assigned to this lobby

        void SendToAllButOne(event_any e, uint32_t excluded_client_id);
    };

} // namespace Control
