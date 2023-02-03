#include <cstddef>
#include <cstdbool>
#include <cstdint>

#include "rosalia/noise.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"

#include "control/lobby_manager.hpp"

namespace Control {

    LobbyManager::LobbyManager()
    {
        //TODO
    }

    LobbyManager::~LobbyManager()
    {
        //TODO
    }

    void LobbyManager::HandleEvent(event_any e)
    {
        switch (e.base.type) {
            case EVENT_TYPE_NULL: {
                printf("[WARN] received impossible null event\n");
            } break;
            case EVENT_TYPE_GAME_LOAD:
            case EVENT_TYPE_GAME_UNLOAD:
            case EVENT_TYPE_GAME_STATE:
            case EVENT_TYPE_GAME_MOVE:
            case EVENT_TYPE_GAME_SYNC:
            case EVENT_TYPE_LOBBY_CHAT_MSG:
            case EVENT_TYPE_LOBBY_CHAT_DEL: {
                std::unordered_map<uint32_t, Lobby*>::iterator lobby_it = lobbies.find(e.base.lobby_id);
                if (lobby_it == lobbies.end()) {
                    // lobby not found send warning
                    event_any se;
                    event_create_logf(&se, "lobby id %u does not exist\n", e.base.lobby_id);
                    se.base.client_id = e.base.client_id;
                    event_queue_push(send_queue, &se);
                    break;
                }
                lobby_it->second->HandleEvent(e);
            } break;
            case EVENT_TYPE_LOBBY_CREATE: {
                if (e.lobby_create.lobby_name == NULL) {
                    event_any se;
                    event_create_logf(&se, "lobby name can not be empty\n");
                    se.base.client_id = e.base.client_id;
                    event_queue_push(send_queue, &se);
                    break;
                }
                if (e.lobby_create.max_users == 0) {
                    event_any se;
                    event_create_logf(&se, "lobby must have capacity for at least 1 user\n");
                    se.base.client_id = e.base.client_id;
                    event_queue_push(send_queue, &se);
                    break;
                }
                uint32_t new_id = strhash(e.lobby_create.lobby_name, NULL);
                std::unordered_map<uint32_t, Lobby*>::iterator lobby_it = lobbies.find(e.base.lobby_id);
                if (lobby_it != lobbies.end() || new_id == EVENT_LOBBY_NONE || new_id == EVENT_LOBBY_SPEC) {
                    event_any se;
                    event_create_logf(&se, "lobby name collision\n");
                    se.base.client_id = e.base.client_id;
                    event_queue_push(send_queue, &se);
                    break;
                }
                bool usepw = e.lobby_create.password != NULL;
                uint32_t pwhash = usepw ? strhash(e.lobby_create.password, NULL) : 0;
                lobbies.insert({new_id, new Lobby(new_id, e.lobby_create.lobby_name, usepw, pwhash, plugin_mgr, send_queue, e.lobby_create.max_users)});
                lobbies.at(new_id)->AddUser(e.base.client_id);
                event_any se;
                event_create_lobby_join(&se, e.lobby_create.lobby_name, NULL);
                se.base.client_id = e.base.client_id;
                se.base.lobby_id = lobbies.at(new_id)->id;
                event_queue_push(send_queue, &se);
            } break;
            case EVENT_TYPE_LOBBY_DESTROY: {
                std::unordered_map<uint32_t, Lobby*>::iterator lobby_it = lobbies.find(e.base.lobby_id);
                if (lobby_it == lobbies.end()) {
                    // lobby not found send warning
                    event_any se;
                    event_create_logf(&se, "lobby id %u does not exist\n", e.base.lobby_id);
                    se.base.client_id = e.base.client_id;
                    event_queue_push(send_queue, &se);
                    break;
                }
                //TODO send leave events to all members
                lobbies.erase(lobby_it);
            } break;
            case EVENT_TYPE_LOBBY_JOIN: {
                if (e.lobby_join.lobby_name == NULL) {
                    event_any se;
                    event_create_logf(&se, "lobby name can not be empty\n");
                    se.base.client_id = e.base.client_id;
                    event_queue_push(send_queue, &se);
                    break;
                }
                uint32_t target_id = strhash(e.lobby_create.lobby_name, NULL);
                std::unordered_map<uint32_t, Lobby*>::iterator lobby_it = lobbies.find(target_id);
                if (lobby_it == lobbies.end()) {
                    // lobby not found send warning
                    event_any se;
                    event_create_logf(&se, "lobby does not exist\n");
                    se.base.client_id = e.base.client_id;
                    event_queue_push(send_queue, &se);
                    break;
                }
                if (lobby_it->second->usepw == true) {
                    if (e.lobby_join.password == NULL || strhash(e.lobby_join.password, NULL) != lobby_it->second->pwhash) {
                        event_any se;
                        event_create_logf(&se, "lobby password incorrect\n");
                        se.base.client_id = e.base.client_id;
                        event_queue_push(send_queue, &se);
                        break;
                    }
                }
                lobby_it->second->AddUser(e.base.client_id);
                event_any se;
                event_create_lobby_join(&se, e.lobby_create.lobby_name, NULL);
                se.base.client_id = e.base.client_id;
                se.base.lobby_id = lobby_it->second->id;
                event_queue_push(send_queue, &se);
            } break;
            case EVENT_TYPE_LOBBY_LEAVE: {
                std::unordered_map<uint32_t, Lobby*>::iterator lobby_it = lobbies.find(e.base.lobby_id);
                if (lobby_it == lobbies.end()) {
                    // lobby not found send warning
                    event_any se;
                    event_create_logf(&se, "lobby id %u does not exist\n", e.base.lobby_id);
                    se.base.client_id = e.base.client_id;
                    event_queue_push(send_queue, &se);
                    break;
                }
                lobby_it->second->RemoveUser(e.base.client_id);
                event_any se;
                event_create_type_client(&se, EVENT_TYPE_LOBBY_LEAVE, e.base.client_id);
                se.base.lobby_id = lobby_it->second->id;
                event_queue_push(send_queue, &se);
            } break;
            case EVENT_TYPE_LOBBY_INFO: {

            } break;
            default: {
                printf("[WARN] unexpected event type %u in lobby manager\n", e.base.type);
            };
        }
    }

} // namespace Control
