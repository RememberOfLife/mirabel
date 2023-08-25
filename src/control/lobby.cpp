#include <cstdint>
#include <cstdio>
#include <cstring>

#include <SDL2/SDL.h>

#include "mirabel/game.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "control/plugins.hpp"

#include "control/lobby.hpp"

namespace Control {

    Lobby::Lobby(uint32_t id, const char* name, bool usepw, uint32_t pwhash, PluginManager* plugin_mgr, event_queue* send_queue, uint16_t max_users):
        id(id),
        name(strdup(name)),
        usepw(usepw),
        pwhash(pwhash),
        plugin_mgr(plugin_mgr),
        send_queue(send_queue),
        the_game(NULL),
        game_base(NULL),
        game_variant(NULL),
        game_impl(NULL),
        game_options(NULL),
        max_users(max_users),
        users(max_users)
    {
    }

    Lobby::~Lobby()
    {
        free(game_options);
        free(game_impl);
        free(game_variant);
        free(game_base);
        delete the_game;
        free(name);
    }

    void Lobby::AddUser(uint32_t client_id)
    {
        if (users.size() >= max_users) {
            printf("[ERROR] could not add user to lobby, full\n");
            return;
        }
        users.push_back((lobby_user){.client_id = client_id});
        event_any es;
        if (the_game) {
            // send sync info to user, load + state import
            size_t game_state_buffer_len;
            const char* game_state_buffer;
            game_export_state(the_game, &game_state_buffer_len, &game_state_buffer);
            game_init init_info = (game_init){
                .source_type = GAME_INIT_SOURCE_TYPE_STANDARD,
                .source{
                    .standard{
                        .opts = game_options,
                        .player_count = 2, //TODO //HACK needs proper optionable
                        .env_legacy = NULL,
                        .player_legacies = NULL,
                        .state = game_state_buffer,
                        .sync_ctr = the_game->sync_ctr,
                    },
                },
            };
            event_create_game_load(&es, game_base, game_variant, game_impl, init_info);
            es.base.client_id = client_id;
        } else {
            event_create_type_client(&es, EVENT_TYPE_GAME_UNLOAD, client_id);
        }
        es.base.lobby_id = id;
        event_queue_push(send_queue, &es);
        char* msg_buf = (char*)malloc(32);
        sprintf(msg_buf, "client joined: %d\n", client_id);
        event_create_chat_msg(&es, lobby_msg_id_ctr++, EVENT_CLIENT_SERVER, SDL_GetTicks64(), msg_buf);
        SendToAllButOne(es, EVENT_CLIENT_NONE);
        free(msg_buf);
    }

    void Lobby::RemoveUser(uint32_t client_id)
    {
        for (size_t i = 0; i < users.size(); i++) {
            if (users[i].client_id == client_id) {
                char* msg_buf = (char*)malloc(32);
                sprintf(msg_buf, "client left: %d\n", client_id);
                event_any es;
                event_create_chat_msg(&es, lobby_msg_id_ctr++, EVENT_CLIENT_SERVER, SDL_GetTicks64(), msg_buf);
                SendToAllButOne(es, EVENT_CLIENT_NONE);
                free(msg_buf);
                users.erase(users.begin() + i);
                return;
            }
        }
        printf("[ERROR] could not find user to remove from lobby\n");
    }

    void Lobby::HandleEvent(event_any e)
    {
        switch (e.base.type) {
            case EVENT_TYPE_GAME_LOAD: {
                // reset everything in case we can't find the game later on
                if (the_game) {
                    game_destroy(the_game);
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
                the_game = plugin_mgr->impl_lookup[impl_idx]->new_game(e.game_load.init_info);
                if (the_game == NULL) {
                    printf("[WARN] failed to create game: %s.%s.%s\n", base_name, variant_name, impl_name);
                    // as server, need to inform of failed create, unload all clients games
                    event_any s1;
                    event_create_type(&s1, EVENT_TYPE_GAME_UNLOAD);
                    SendToAllButOne(s1, EVENT_CLIENT_NONE);
                    event_destroy(&s1);
                    break;
                }
                // export opts from the loaded game
                game_options = NULL;
                if (game_ff(the_game).options) {
                    size_t size_fill;
                    const char* game_options_local;
                    game_export_options(the_game, &size_fill, &game_options_local);
                    game_options = strdup(game_options_local);
                }
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
                    game_destroy(the_game);
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
                game_import_state(the_game, e.game_state.state);
                printf("[INFO] game state imported: %s\n", e.game_state.state);
                // pass event to other clients in lobby
                SendToAllButOne(e, e.base.client_id);
            } break;
            case EVENT_TYPE_GAME_MOVE: {
                if (!the_game) {
                    printf("[WARN] attempted move on null game\n");
                    break;
                }
                uint8_t pbuf_cnt;
                const player_id* pbuf;
                game_make_move(the_game, e.game_move.player, e.game_move.data);
                game_players_to_move(the_game, &pbuf_cnt, &pbuf);
                printf("[INFO] game move made\n");
                if (pbuf_cnt == 0) {
                    game_get_results(the_game, &pbuf_cnt, &pbuf);
                    char winners_str[1024] = "000";
                    char* winners_str_acc = winners_str;
                    for (uint8_t pbuf_i = 0; pbuf_i < pbuf_cnt; pbuf_i++) {
                        winners_str_acc += sprintf(winners_str_acc, "%03hhu", pbuf[pbuf_i]);
                    }
                    printf("game done: results %s\n", winners_str);
                }
                // pass event to other clients in lobby
                SendToAllButOne(e, e.base.client_id);
            } break;
            case EVENT_TYPE_GAME_SYNC: {
                event_any se;
                event_create_log(&se, "game sync event is server to client only\n", NULL);
                se.base.client_id = e.base.client_id;
                se.base.lobby_id = id;
                event_queue_push(send_queue, &se);
            } break;
            case EVENT_TYPE_LOBBY_CHAT_MSG: {
                printf("[INFO] chat message received from %d, broadcasting: %s\n", e.base.client_id, e.chat_msg.text);
                e.chat_msg.msg_id = lobby_msg_id_ctr++;
                e.chat_msg.author_client_id = e.base.client_id;
                e.chat_msg.timestamp = SDL_GetTicks64(); //TODO replace by non sdl function and something that is actually useful as a timestamp
                // send message to everyone
                SendToAllButOne(e, EVENT_CLIENT_NONE);
            } break;
            case EVENT_TYPE_LOBBY_CHAT_DEL: {
                SendToAllButOne(e, EVENT_CLIENT_NONE);
            } break;
            default: {
                printf("[WARN] lobby received unexpected event type %d\n", e.base.type);
            } break;
        }
    }

    void Lobby::SendToAllButOne(event_any e, uint32_t excluded_client_id)
    {
        for (size_t i = 0; i < users.size(); i++) {
            if (users[i].client_id == EVENT_CLIENT_NONE || users[i].client_id == excluded_client_id) {
                continue;
            }
            e.base.client_id = users[i].client_id;
            event_any es;
            event_copy(&es, &e);
            es.base.lobby_id = id;
            event_queue_push(send_queue, &es);
        }
    }

} // namespace Control
