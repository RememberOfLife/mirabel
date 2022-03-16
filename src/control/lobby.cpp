#include <cstdint>
#include <cstring>

#include "surena/game.hpp"

#include "control/event_queue.hpp"
#include "control/event.hpp"
#include "control/lobby.hpp"
#include "games/game_catalogue.hpp"

namespace Control {

    Lobby::Lobby(event_queue* send_queue, uint16_t max_users):
        send_queue(send_queue),
        game(NULL),
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
    }

    void Lobby::AddUser(uint32_t client_id)
    {
        for (uint32_t i = 0; i < max_users; i++) {
            if (user_client_ids[i] == 0) {
                user_client_ids[i] = client_id;
                return;
            }
        }
        printf("[ERROR] could not add user to lobby\n");
    }

    void Lobby::HandleEvent(event e)
    {
        switch (e.type) {
            //TODO code for LOAD+UNLOAD+IMPORT_STATE+MOVE is ripped from client, so comments may not match for now
            case EVENT_TYPE_GAME_LOAD: {
                delete game;
                // reset everything in case we can't find the game later on
                game = NULL;
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
                game = Games::game_catalogue[base_game_idx].variants[game_variant_idx]->new_game();
                printf("[INFO] game loaded: %s.%s\n", base_game_name, game_variant_name);
                // pass event to other clients in lobby
                SendToAllButOne(e, e.client_id);
            } break;
            case EVENT_TYPE_GAME_UNLOAD: {
                delete game;
                game = NULL;
                printf("[INFO] game unloaded\n");
                // pass event to other clients in lobby
                SendToAllButOne(e, e.client_id);
            } break;
            case EVENT_TYPE_GAME_IMPORT_STATE: {
                if (!game) {
                    printf("[WARN] attempted state import on null game\n");
                    break;
                }
                game->import_state(static_cast<char*>(e.raw_data));
                printf("[INFO] game state imported: %s\n", static_cast<char*>(e.raw_data));
                // pass event to other clients in lobby
                SendToAllButOne(e, e.client_id);
            } break;
            case EVENT_TYPE_GAME_MOVE: {
                if (!game) {
                    printf("[WARN] attempted move on null game\n");
                    break;
                }
                game->apply_move(e.move.code);
                printf("[INFO] game move made\n");
                if (game->player_to_move() == 0) {
                    printf("[INFO] game done: winner is player %d\n", game->get_result());
                }
                // pass event to other clients in lobby
                SendToAllButOne(e, e.client_id);
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
