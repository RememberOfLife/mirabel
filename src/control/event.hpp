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
        EVENT_TYPE_HEARTBEAT, // purely local event between queueholder and timeoutcrash, networking uses protocol_ping events
        EVENT_TYPE_HEARTBEAT_PREQUIT, // theoretically functions as a HEARTBEAT + HEARTBEAT_SET_TIMEOUT would
        EVENT_TYPE_HEARTBEAT_RESET,
        EVENT_TYPE_EXIT, // queueholder object stop runners and prepares itself for deconstruction by e.g. join
        // normal events
        EVENT_TYPE_GAME_LOAD,
        EVENT_TYPE_GAME_UNLOAD,
        EVENT_TYPE_GAME_IMPORT_STATE,
        EVENT_TYPE_GAME_MOVE,
        // client only events
        EVENT_TYPE_FRONTEND_LOAD,
        EVENT_TYPE_FRONTEND_UNLOAD,
        EVENT_TYPE_ENGINE_LOAD,
        EVENT_TYPE_ENGINE_UNLOAD,
        // networking events: internal events
        EVENT_TYPE_NETWORK_INTERNAL_SSL_WRITE,
        // networking events: adapter events; work with adapter<->main_queue
        EVENT_TYPE_NETWORK_ADAPTER_LOAD,
        EVENT_TYPE_NETWORK_ADAPTER_UNLOAD,
        EVENT_TYPE_NETWORK_ADAPTER_SOCKET_OPENED,
        EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED,
        EVENT_TYPE_NETWORK_ADAPTER_CLIENT_CONNECTED,
        EVENT_TYPE_NETWORK_ADAPTER_CLIENT_DISCONNECTED,
        // networking events: protocol events; work with adapter<->adapter, they should not reach the main queue, and ignored if they do
        EVENT_TYPE_NETWORK_PROTOCOL_OK,
        EVENT_TYPE_NETWORK_PROTOCOL_NOK,
        EVENT_TYPE_NETWORK_PROTOCOL_PING,
        EVENT_TYPE_NETWORK_PROTOCOL_PONG,
        EVENT_TYPE_NETWORK_PROTOCOL_CLIENT_ID_SET,
        // lobby events: deal with client/server communication
        EVENT_TYPE_LOBBY_CHAT_MSG, // contains msgId(for removal),client_id(who sent the message),timestamp,text
        EVENT_TYPE_LOBBY_CHAT_DEL,
    };

    struct heartbeat_event {
        uint32_t id;
        uint32_t time;
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

    struct msg_del_event {
        uint32_t msg_id;
    };

    //TODO pack with
    // #pragma pack(1)
    // typedef struct {
    //     data goes here...
    // } __atribute__((aligned(4))) event_netdata;
    // then use:
    // struct event : public event_netdata { ... }

    struct event {
        uint32_t type;
        uint32_t client_id;
        // uint32_t lobby_id; //TODO will go here instead of padding
        // raw data segment, owned by the event
        // when serializing over network this is read and sent together with the packet
        uint32_t raw_length; // len could be stuffed into the first 4 bytes of raw_data, some way to keep two accessors?
        void* raw_data;
        // char _padding[8-sizeof(size_t)]; // when using padding
        union {
            heartbeat_event heartbeat;
            move_event move;
            frontend_event frontend;
            engine_event engine;
            msg_del_event msg_del;
        };
        event();
        event(uint32_t type);
        event(uint32_t type, uint32_t raw_length, void* raw_data);
        event(uint32_t type, uint32_t client_id);
        event(uint32_t type, uint32_t client_id, uint32_t raw_length, void* raw_data);
        event(const event& other); // copy construct
        event(event&& other); // move construct
        event& operator=(const event& other); // copy assign
        event& operator=(event&& other); // move assign
        ~event();
        static event create_heartbeat_event(uint32_t type, uint32_t id, uint32_t time = 0);
        static event create_game_event(uint32_t type, const char* base_game, const char* base_game_variant);
        static event create_move_event(uint32_t type, uint64_t code);
        static event create_frontend_event(uint32_t type, Frontends::Frontend* frontend);
        static event create_engine_event(uint32_t type, surena::Engine* frontend);
        static event create_chat_msg_event(uint32_t type, uint32_t msg_id, uint32_t client_id, uint64_t timestamp, const char* text);
        static event create_chat_del_event(uint32_t type, uint32_t msg_id);
    };

}
