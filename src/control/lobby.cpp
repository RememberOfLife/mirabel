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
            user_client_ids[i] = CLIENT_NONE;
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
            if (user_client_ids[i] == CLIENT_NONE) {
                user_client_ids[i] = client_id;
                if (the_game) {
                    // send sync info to user, load + state import
                    f_any_event e_load = f_event_game_load(base_game, game_variant);
                    e_load.client_id = client_id;
                    send_queue->push(e_load);
                    size_t game_state_buffer_len = the_game->sizer.state_str;
                    char* game_state_buffer = (char*)malloc(game_state_buffer_len);
                    the_game->methods->export_state(the_game, &game_state_buffer_len, game_state_buffer);
                    send_queue->push(f_event_game_state(client_id, game_state_buffer));
                } else {
                    send_queue->push(f_event(Control::EVENT_TYPE_GAME_UNLOAD, client_id));
                }
                char* msg_buf = (char*)malloc(32);
                sprintf(msg_buf, "client joined: %d\n", client_id);
                SendToAllButOne(f_event_chat_msg(lobby_msg_id_ctr++, CLIENT_SERVER, SDL_GetTicks64(), msg_buf), CLIENT_NONE);
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
                user_client_ids[i] = CLIENT_NONE;
                char* msg_buf = (char*)malloc(32);
                sprintf(msg_buf, "client left: %d\n", client_id);
                SendToAllButOne(f_event_chat_msg(lobby_msg_id_ctr++, CLIENT_SERVER, SDL_GetTicks64(), msg_buf), CLIENT_NONE);
                free(msg_buf);
                return;
            }
        }
        printf("[ERROR] could not find user to remove from lobby\n");
    }

    void Lobby::HandleEvent(f_any_event e)
    {
        switch (e.type) {
            //TODO code for LOAD+UNLOAD+IMPORT_STATE+MOVE is ripped from client, so comments may not match for now
            case EVENT_TYPE_GAME_LOAD: {
                auto ce = e.cast<f_event_game_load>();
                // reset everything in case we can't find the game later on
                if (the_game) {
                    the_game->methods->destroy(the_game);
                    free(the_game);
                }
                the_game = NULL;
                // find game in games catalogue by provided strings
                bool game_found = false;
                uint32_t base_game_idx = 0;
                const char* base_game_name = ce.base_name;
                uint32_t game_variant_idx = 0;
                const char* game_variant_name = ce.variant_name;
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
            case EVENT_TYPE_GAME_STATE: {
                auto ce = e.cast<f_event_game_state>();
                if (!the_game) {
                    printf("[WARN] attempted state import on null game\n");
                    break;
                }
                the_game->methods->import_state(the_game, ce.state);
                printf("[INFO] game state imported: %s\n", ce.state);
                // pass event to other clients in lobby
                SendToAllButOne(e, e.client_id);
            } break;
            case EVENT_TYPE_GAME_MOVE: {
                auto ce = e.cast<f_event_game_move>();
                if (!the_game) {
                    printf("[WARN] attempted move on null game\n");
                    break;
                }
                player_id pbuf[253];
                uint8_t pbuf_cnt = 253;
                the_game->methods->players_to_move(the_game, &pbuf_cnt, pbuf); //FIXME ptm
                the_game->methods->make_move(the_game, pbuf[0], ce.code);
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
                auto ce = e.cast<f_event_chat_msg>();
                printf("[INFO] chat message received from %d, broadcasting: %s\n", e.client_id, ce.text);
                ce.msg_id = lobby_msg_id_ctr++;
                ce.author_client_id = e.client_id;
                ce.timestamp = SDL_GetTicks64(); //TODO replace by non sdl function and something that is actually useful as a timestamp
                // send message to everyone
                SendToAllButOne(ce, CLIENT_NONE);
            } break;
            case EVENT_TYPE_LOBBY_CHAT_DEL: {
                SendToAllButOne(e, CLIENT_NONE);
            } break;
            default: {
                printf("[WARN] lobby received unexpected event type %d\n", e.type);
            } break;
        }
    }

    void Lobby::SendToAllButOne(f_any_event e, uint32_t excluded_client_id)
    {
        for (uint32_t i = 0; i < max_users; i++) {
            if (user_client_ids[i] == CLIENT_NONE || user_client_ids[i] == excluded_client_id) {
                continue;
            }
            e.client_id = user_client_ids[i];
            send_queue->push(e);
        }
    }

}
