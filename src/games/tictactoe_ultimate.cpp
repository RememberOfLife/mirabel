#include <cstdint>
#include <cstdlib>

#include "imgui.h"
#include "surena/games/tictactoe_ultimate.h"
#include "surena/game.h"

#include "mirabel/game_wrap.h"

#include "games/game_catalogue.hpp"

namespace {

    error_code runtime_create(game* rgame, void** runtime_struct)
    {
        //TODO
        return ERR_OK;
    }

    error_code runtime_display(game* rgame, void* runtime_struct)
    {
        //TODO expose state string
        //TODO proper state editor
        const char* check_options[3] = {"-", "X", "O"};
        player_id pbuf;
        uint8_t pbuf_c;
        rgame->methods->players_to_move(rgame, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
            pbuf = PLAYER_NONE;
        }
        ImGui::Text("player to move: %s", check_options[pbuf]);
        rgame->methods->get_results(rgame, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
            pbuf = PLAYER_NONE;
        }
        ImGui::Text("result: %s", check_options[pbuf]);
        return ERR_OK;
    }

    error_code runtime_destroy(game* rgame, void* runtime_struct)
    {
        //TODO
        return ERR_OK;
    }

    const game_wrap tictactoe_gw{
        .game_api_version = SURENA_GAME_API_VERSION,
        .backend = &tictactoe_ultimate_gbe,

        .opts_create = NULL,
        .opts_display = NULL,
        .opts_destroy = NULL,

        .runtime_create = runtime_create,
        .runtime_display = runtime_display,
        .runtime_destroy = runtime_destroy,
    };

}
