#include <cstdint>
#include <cstdio>
#include <cstring>

#include <SDL2/SDL.h>

#include "surena/game.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "control/plugins.hpp"
#include "games/game_catalogue.hpp"

#include "control/lobby.hpp"

namespace Control {

    Lobby::Lobby(PluginManager* plugin_mgr, f_event_queue* send_queue, uint16_t max_users):
        plugin_mgr(plugin_mgr),
        send_queue(send_queue),
        the_game(NULL),
        game_base(NULL),
        game_variant(NULL),
        game_impl(NULL),
        max_users(max_users),
        user_client_ids(static_cast<uint32_t*>(malloc(max_users * sizeof(uint32_t))))
    {
        for (uint32_t i = 0; i < max_users; i++) {
            user_client_ids[i] = F_EVENT_CLIENT_NONE;
        }
    }

    Lobby::~Lobby()
    {
        free(user_client_ids);
        free(game_impl);
        free(game_variant);
        free(game_base);
        delete the_game;
    }

    void Lobby::AddUser(uint32_t client_id)
    {
        for (uint32_t i = 0; i < max_users; i++) {
            if (user_client_ids[i] == F_EVENT_CLIENT_NONE) {
                user_client_ids[i] = client_id;
                f_event_any es;
                if (the_game) {
                    // send sync info to user, load + state import
                    f_event_create_game_load(&es, game_base, game_variant, game_impl, game_options);
                    es.base.client_id = client_id;
                    f_event_queue_push(send_queue, &es);
                    size_t game_state_buffer_len = the_game->sizer.state_str;
                    char* game_state_buffer = (char*)malloc(game_state_buffer_len);
                    the_game->methods->export_state(the_game, &game_state_buffer_len, game_state_buffer);
                    f_event_create_game_state(&es, client_id, game_state_buffer);
                    f_event_queue_push(send_queue, &es);
                } else {
                    f_event_create_type_client(&es, EVENT_TYPE_GAME_UNLOAD, client_id);
                    f_event_queue_push(send_queue, &es);
                }
                char* msg_buf = (char*)malloc(32);
                sprintf(msg_buf, "client joined: %d\n", client_id);
                f_event_create_chat_msg(&es, lobby_msg_id_ctr++, F_EVENT_CLIENT_SERVER, SDL_GetTicks64(), msg_buf);
                SendToAllButOne(es, F_EVENT_CLIENT_NONE);
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
                user_client_ids[i] = F_EVENT_CLIENT_NONE;
                char* msg_buf = (char*)malloc(32);
                sprintf(msg_buf, "client left: %d\n", client_id);
                f_event_any es;
                f_event_create_chat_msg(&es, lobby_msg_id_ctr++, F_EVENT_CLIENT_SERVER, SDL_GetTicks64(), msg_buf);
                SendToAllButOne(es, F_EVENT_CLIENT_NONE);
                free(msg_buf);
                return;
            }
        }
        printf("[ERROR] could not find user to remove from lobby\n");
    }

    void Lobby::HandleEvent(f_event_any e)
    {
        switch (e.base.type) {
            //TODO code for LOAD+UNLOAD+IMPORT_STATE+MOVE is ripped from client, so comments may not match for now
            case EVENT_TYPE_GAME_LOAD: {
                // reset everything in case we can't find the game later on
                if (the_game) {
                    the_game->methods->destroy(the_game);
                    free(the_game);
                }
                the_game = NULL;
                // find game in games catalogue by provided strings
                const char* base_name = e.game_load.base_name;
                const char* variant_name = e.game_load.variant_name;
                const char* impl_name = e.game_load.impl_name;
                uint32_t impl_idx = 0;
                if (!plugin_mgr->get_game_impl_idx(base_name, variant_name, impl_name, NULL, NULL, &impl_idx)) {
                    printf("[WARN] failed to find game: %s.%s.%s\n", base_name, variant_name, impl_name);
                    break;
                }
                game_options = e.game_load.options ? strdup(e.game_load.options) : NULL;
                the_game = plugin_mgr->impl_lookup[impl_idx]->new_game(NULL, game_options); //BUG make sure this actually works as inteded if game options is NULL on a game that support options
                // update game name strings
                game_base = strdup(base_name);
                game_variant = strdup(variant_name);
                game_impl = strdup(impl_name);
                printf("[INFO] game loaded: %s.%s.%s", base_name, variant_name, impl_name);
                if (game_options) {
                    printf(" with options: %s\n", game_options);
                } else {
                    printf("\n");
                }
                // pass event to other clients in lobby
                SendToAllButOne(e, e.base.client_id);
            } break;
            case EVENT_TYPE_GAME_UNLOAD: {
                if (the_game) {
                    the_game->methods->destroy(the_game);
                    free(the_game);
                }
                the_game = NULL;
                free(game_base);
                free(game_variant);
                free(game_options);
                game_base = NULL;
                game_variant = NULL;
                game_options = NULL;
                printf("[INFO] game unloaded\n");
                // pass event to other clients in lobby
                SendToAllButOne(e, e.base.client_id);
            } break;
            case EVENT_TYPE_GAME_STATE: {
                if (!the_game) {
                    printf("[WARN] attempted state import on null game\n");
                    break;
                }
                the_game->methods->import_state(the_game, e.game_state.state);
                printf("[INFO] game state imported: %s\n", e.game_state.state);
                // pass event to other clients in lobby
                SendToAllButOne(e, e.base.client_id);
            } break;
            case EVENT_TYPE_GAME_MOVE: {
                if (!the_game) {
                    printf("[WARN] attempted move on null game\n");
                    break;
                }
                player_id pbuf[253];
                uint8_t pbuf_cnt = 253;
                the_game->methods->players_to_move(the_game, &pbuf_cnt, pbuf); //FIXME ptm
                the_game->methods->make_move(the_game, pbuf[0], e.game_move.code);
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
                SendToAllButOne(e, e.base.client_id);
            } break;
            case EVENT_TYPE_LOBBY_CHAT_MSG: {
                printf("[INFO] chat message received from %d, broadcasting: %s\n", e.base.client_id, e.chat_msg.text);
                e.chat_msg.msg_id = lobby_msg_id_ctr++;
                e.chat_msg.author_client_id = e.base.client_id;
                e.chat_msg.timestamp = SDL_GetTicks64(); //TODO replace by non sdl function and something that is actually useful as a timestamp
                // send message to everyone
                SendToAllButOne(e, F_EVENT_CLIENT_NONE);
            } break;
            case EVENT_TYPE_LOBBY_CHAT_DEL: {
                SendToAllButOne(e, F_EVENT_CLIENT_NONE);
            } break;
            default: {
                printf("[WARN] lobby received unexpected event type %d\n", e.base.type);
            } break;
        }
    }

    void Lobby::SendToAllButOne(f_event_any e, uint32_t excluded_client_id)
    {
        for (uint32_t i = 0; i < max_users; i++) {
            if (user_client_ids[i] == F_EVENT_CLIENT_NONE || user_client_ids[i] == excluded_client_id) {
                continue;
            }
            e.base.client_id = user_client_ids[i];
            f_event_any es;
            f_event_copy(&es, &e);
            f_event_queue_push(send_queue, &es);
        }
    }

} // namespace Control
