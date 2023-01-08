#include <cstdint>
#include <cstdlib>

#include "imgui.h"
#include "surena/games/chess.h"
#include "surena/game.h"

#include "mirabel/game_wrap.h"

#include "games/game_catalogue.hpp"

namespace {

    error_code runtime_create(game* rgame, void** runtime_struct)
    {
        //TODO
        *runtime_struct = rgame->data1; // fill runtime struct with a spoofed pointer
        return ERR_OK;
    }

    error_code runtime_display(game* rgame, void* runtime_struct)
    {
        //TODO expose state string
        //TODO proper state editor
        const char* check_options[4] = {"-", "WHITE", "BLACK"};
        uint8_t pbuf_c;
        const player_id* pbuf;
        player_id player_out;
        game_players_to_move(rgame, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
            player_out = PLAYER_NONE;
        } else {
            player_out = pbuf[0];
        }
        ImGui::Text("player to move: %s", check_options[player_out]);
        game_get_results(rgame, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
            player_out = PLAYER_NONE;
        } else {
            player_out = pbuf[0];
        }
        ImGui::Text("result: %s", check_options[player_out]);
        //TODO expose winningcondition
        return ERR_OK;
    }

    error_code runtime_destroy(void* runtime_struct)
    {
        //TODO
        return ERR_OK;
    }

} // namespace

const game_wrap chess_gw{
    .game_api_version = SURENA_GAME_API_VERSION,
    .backend = &chess_standard_gbe,
    .features = (game_wrap_feature_flags){
        .options = false,
        .runtime = true,
    },

    .opts_create = NULL,
    .opts_display = NULL,
    .opts_destroy = NULL,

    .opts_bin_to_str = NULL,

    .runtime_create = runtime_create,
    .runtime_display = runtime_display,
    .runtime_destroy = runtime_destroy,
};
