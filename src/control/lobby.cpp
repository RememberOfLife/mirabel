#include <cstdint>
#include <cstdio>
#include <cstring>

#include <SDL2/SDL.h>

#include "surena/game.h"

#include "control/event_queue.hpp"
#include "control/event.hpp"
#include "control/lobby.hpp"
#include "games/game_catalogue.hpp"

namespace Control {

    Lobby::Lobby(event_queue* send_queue, uint16_t max_users):
        send_queue(send_queue),
        the_game(NULL),
        base_game(NULL),
        game_variant(NULL),
        max_users(max_users),
        user_client_ids(static_cast<uint32_t*>(malloc(max_users*sizeof(uint32_t))))
    {
        for (uint32_t i = 0; i < max_users; i++) {
            user_client_ids[i] = 0;
        }
    }

    Lobby::~Lobby()
    {
        free(user_client_ids);
        free(game_variant);
        free(base_game);
        delete the_game;
    }

    void Lobby::AddUser(uint32_t client_id)
    {
        for (uint32_t i = 0; i < max_users; i++) {
            if (user_client_ids[i] == 0) {
                user_client_ids[i] = client_id;
                if (the_game) {
                    // send sync info to user, load + state import
                    event e_load = event::create_game_event(EVENT_TYPE_GAME_LOAD, base_game, game_variant);
                    e_load.client_id = client_id;
                    send_queue->push(e_load);
                    size_t game_state_buffer_len;
                    the_game->methods->export_state(the_game, &game_state_buffer_len, NULL);
                    char* game_state_buffer = (char*)malloc(game_state_buffer_len);
                    the_game->methods->export_state(the_game, &game_state_buffer_len, game_state_buffer);
                    send_queue->push(Control::event(Control::EVENT_TYPE_GAME_IMPORT_STATE, client_id, game_state_buffer_len, game_state_buffer));
                } else {
                    send_queue->push(Control::event(Control::EVENT_TYPE_GAME_UNLOAD, client_id));
                }
                char* msg_buf = (char*)malloc(32);
                sprintf(msg_buf, "client joined: %d\n", client_id);
                SendToAllButOne(event::create_chat_msg_event(EVENT_TYPE_LOBBY_CHAT_MSG, lobby_msg_id_ctr++, UINT32_MAX, SDL_GetTicks64(), msg_buf), 0);
                free(msg_buf);
                return;
            }
        }
        printf("[ERROR] could not add user to lobby\n");
    }

    void Lobby::RemoveUser(uint32_t client_id)
    {
        for (uint32_t i = 0; i < max_users; i++) {
            if (user_client_ids[i] == client_id) {
                user_client_ids[i] = 0;
                char* msg_buf = (char*)malloc(32);
                sprintf(msg_buf, "client left: %d\n", client_id);
                SendToAllButOne(event::create_chat_msg_event(EVENT_TYPE_LOBBY_CHAT_MSG, lobby_msg_id_ctr++, UINT32_MAX, SDL_GetTicks64(), msg_buf), 0);
                free(msg_buf);
                return;
            }
        }
        printf("[ERROR] could not find user to remove from lobby\n");
    }

    void Lobby::HandleEvent(event e)
    {
        switch (e.type) {
            //TODO code for LOAD+UNLOAD+IMPORT_STATE+MOVE is ripped from client, so comments may not match for now
            case EVENT_TYPE_GAME_LOAD: {
                // reset everything in case we can't find the game later on
                if (the_game) {
                    the_game->methods->destroy(the_game);
                    free(the_game);
                }
                the_game = NULL;
                // find game in games catalogue by provided strings
                bool game_found = false;
                uint32_t base_game_idx = 0;
                const char* base_game_name = static_cast<char*>(e.raw_data);
                uint32_t game_variant_idx = 0;
                const char* game_variant_name = static_cast<char*>(e.raw_data)+strlen(base_game_name)+1;
                for (; base_game_idx < Games::game_catalogue.size(); base_game_idx++) {
                    if (strcmp(Games::game_catalogue[base_game_idx].name, base_game_name) == 0) {
                        game_found = true;
                        break;
                    }
                }
                if (!game_found) {
                    printf("[WARN] failed to find base game: %s\n", base_game_name);
                    break;
                }
                game_found = false;
                for (; game_variant_idx < Games::game_catalogue[base_game_idx].variants.size(); game_variant_idx++) {
                    if (strcmp(Games::game_catalogue[base_game_idx].variants[game_variant_idx]->name, game_variant_name) == 0) {
                        game_found = true;
                        break;
                    }
                }
                if (!game_found) {
                    printf("[WARN] failed to find game variant: %s.%s\n", base_game_name, game_variant_name);
                    break;
                }
                the_game = Games::game_catalogue[base_game_idx].variants[game_variant_idx]->new_game();
                // update game name strings
                free(base_game);
                free(game_variant);
                base_game = (char*)malloc(strlen(base_game_name)+1);
                game_variant = (char*)malloc(strlen(game_variant_name)+1);
                strcpy(base_game, base_game_name);
                strcpy(game_variant, game_variant_name);
                printf("[INFO] game loaded: %s.%s\n", base_game_name, game_variant_name);
                // pass event to other clients in lobby
                SendToAllButOne(e, e.client_id);
            } break;
            case EVENT_TYPE_GAME_UNLOAD: {
                if (the_game) {
                    the_game->methods->destroy(the_game);
                    free(the_game);
                }
                the_game = NULL;
                printf("[INFO] game unloaded\n");
                // pass event to other clients in lobby
                SendToAllButOne(e, e.client_id);
            } break;
            case EVENT_TYPE_GAME_IMPORT_STATE: {
                if (!the_game) {
                    printf("[WARN] attempted state import on null game\n");
                    break;
                }
                the_game->methods->import_state(the_game, static_cast<char*>(e.raw_data));
                printf("[INFO] game state imported: %s\n", static_cast<char*>(e.raw_data));
                // pass event to other clients in lobby
                SendToAllButOne(e, e.client_id);
            } break;
            case EVENT_TYPE_GAME_MOVE: {
                if (!the_game) {
                    printf("[WARN] attempted move on null game\n");
                    break;
                }
                player_id pbuf[253];
                uint8_t pbuf_cnt = 253;
                the_game->methods->players_to_move(the_game, &pbuf_cnt, pbuf); //FIXME ptm
                the_game->methods->make_move(the_game, pbuf[0], e.move.code);
                the_game->methods->players_to_move(the_game, &pbuf_cnt, pbuf);
                printf("[INFO] game move made\n");
                if (pbuf_cnt == 0) {
                    the_game->methods->get_results(the_game, &pbuf_cnt, pbuf);
                    if (pbuf_cnt == 0) {
                        pbuf[0] = PLAYER_NONE;
                    }
                    printf("[INFO] game done: winner is player %d\n", pbuf[0]);
                }
                // pass event to other clients in lobby
                SendToAllButOne(e, e.client_id);
            } break;
            case EVENT_TYPE_LOBBY_CHAT_MSG: {
                // get data from msg
                //TODO this should use the future get event info struct thing
                //TODO also these casts are hideous
                uint32_t* m_msg_id = reinterpret_cast<uint32_t*>(static_cast<char*>(e.raw_data));
                uint32_t* m_client_id = reinterpret_cast<uint32_t*>(static_cast<char*>(e.raw_data)+sizeof(uint32_t));
                uint64_t* m_timestamp = reinterpret_cast<uint64_t*>(static_cast<char*>(e.raw_data)+sizeof(uint32_t)*2);
                char* m_text = static_cast<char*>(e.raw_data)+sizeof(uint32_t)*2+sizeof(uint64_t);
                printf("[INFO] chat message received from %d, broadcasting: %s\n", e.client_id, m_text);
                *m_msg_id = lobby_msg_id_ctr++;
                *m_client_id = e.client_id;
                *m_timestamp = SDL_GetTicks64(); //TODO replace by non sdl function and something that is actually useful as a timestamp
                // send message to everyone
                SendToAllButOne(e, 0);
            } break;
            case EVENT_TYPE_LOBBY_CHAT_DEL: {
                SendToAllButOne(e, 0);
            } break;
        }
    }

    void Lobby::SendToAllButOne(event e, uint32_t excluded_client_id)
    {
        for (uint32_t i = 0; i < max_users; i++) {
            if (user_client_ids[i] == 0 || user_client_ids[i] == excluded_client_id) {
                continue;
            }
            e.client_id = user_client_ids[i];
            send_queue->push(e); //TODO make sure this is copied and not moved
        }
    }

}
