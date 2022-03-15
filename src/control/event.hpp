#pragma once

#include <cstdint>

#include "SDL_net.h"

#include "surena/engine.hpp"
#include "surena/game.hpp"

#include "frontends/frontend_catalogue.hpp"

namespace Control {

    //TODO types of keepalive checks:
    // heartbeat = direct queue object holder alive check
    // protocol ping just pings the connected network adapter, do not place in recv_queue
    // adapter ping pings into the recv_queue of the connected adapter, i.e. gets to the server main loop where protocol ping gets swallowed by the adapter

    enum EVENT_TYPE : uint32_t {
        // special events
        EVENT_TYPE_NULL = 0, // ignored event
        EVENT_TYPE_HEARTBEAT, //TODO this should be a universal thing taking a queue where to put the heartbeat response into, i.e. PING+PONG
        EVENT_TYPE_EXIT, // queueholder object stop runners and prepares itself for deconstruction by e.g. join
        // normal events
        EVENT_TYPE_GAME_LOAD,
        EVENT_TYPE_GAME_UNLOAD,
        EVENT_TYPE_GAME_MOVE,
        EVENT_TYPE_GAME_INTERNAL_UPDATE,
        // client only events
        EVENT_TYPE_FRONTEND_LOAD,
        EVENT_TYPE_FRONTEND_UNLOAD,
        EVENT_TYPE_ENGINE_LOAD,
        EVENT_TYPE_ENGINE_UNLOAD,
        // networking events
        // adapter events work with adapter<->main_queue
        // protocol events work with adapter<->adapter, they should not reach the main queue, and ignored if they do
        EVENT_TYPE_NETWORK_ADAPTER_LOAD,
        EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSE,
        EVENT_TYPE_NETWORK_PROTOCOL_OK,
        EVENT_TYPE_NETWORK_PROTOCOL_NOK,
        EVENT_TYPE_NETWORK_PROTOCOL_PING,
        EVENT_TYPE_NETWORK_PROTOCOL_PONG,
        EVENT_TYPE_NETWORK_PROTOCOL_CLIENT_ID_SET,
    };

    struct game_event {
        surena::Game* game;
    };

    struct move_event {
        uint64_t code;
    };

    struct frontend_event {
        Frontends::Frontend* frontend;
    };

    struct engine_event {
        surena::Engine* engine;
    };

    struct event {
        uint32_t type;
        uint32_t client_id;
        // raw data segment, owned by the event
        // when serializing over network this is read and sent together with the packet
        uint32_t raw_length; // len could be stuffed into the first 4 bytes of raw_data
        void* raw_data;
        union {
            game_event game;
            move_event move;
            frontend_event frontend;
            engine_event engine;
        };
        event();
        event(uint32_t type);
        event(uint32_t type, uint32_t client_id);
        event(const event& other); // copy construct
        event(event&& other); // move construct
        event& operator=(const event& other); // copy assign
        event& operator=(event&& other); // move assign
        ~event();
        static event create_game_event(uint32_t type, surena::Game* game);
        static event create_move_event(uint32_t type, uint64_t code);
        static event create_frontend_event(uint32_t type, Frontends::Frontend* frontend);
        static event create_engine_event(uint32_t type, surena::Engine* frontend);
    };

}
