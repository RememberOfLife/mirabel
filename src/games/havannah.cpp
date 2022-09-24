#include <cstdint>
#include <cstdlib>

#include "imgui.h"
#include "surena/games/havannah.h"
#include "surena/game.h"

#include "mirabel/game_wrap.h"

#include "games/game_catalogue.hpp"

namespace {

    error_code opts_create(void** options_struct)
    {
        havannah_options* opts = (havannah_options*)malloc(sizeof(havannah_options));
        opts->size = 8;
        *options_struct = opts;
        return ERR_OK;
    }

    error_code opts_display(void* options_struct)
    {
        havannah_options* opts = (havannah_options*)options_struct;
        const uint32_t min = 4;
        const uint32_t max = 10;
        ImGui::SliderScalar("size", ImGuiDataType_U32, &opts->size, &min, &max, "%u", ImGuiSliderFlags_AlwaysClamp);
        return ERR_OK;
    }

    error_code opts_destroy(void* options_struct)
    {
        free(options_struct);
        return ERR_OK;
    }

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
        // white is actually displayed red per default, but because that might be configurable it is kept uniform here
        const char* move_options[4] = {"-", "WHITE", "BLACK", "-"}; // needs 2 dashes for none AND invalid
        const char* result_options[4] = {"DRAW", "WHITE", "BLACK", "-"}; // needs 2 dashes for none AND invalid
        player_id pbuf;
        uint8_t pbuf_c;
        rgame->methods->players_to_move(rgame, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
            pbuf = HAVANNAH_PLAYER_NONE;
        }
        ImGui::Text("player to move: %s", move_options[pbuf]);
        rgame->methods->get_results(rgame, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
            pbuf = HAVANNAH_PLAYER_INVALID;
        }
        ImGui::Text("result: %s", result_options[pbuf]);
        //TODO expose winningcondition
        return ERR_OK;
    }

    error_code runtime_destroy(void* runtime_struct)
    {
        //TODO
        return ERR_OK;
    }

} // namespace

const game_wrap havannah_gw{
    .game_api_version = SURENA_GAME_API_VERSION,
    .backend = &havannah_gbe,
    .features = (game_wrap_feature_flags){
        .options = true,
        .runtime = true,
    },

    .opts_create = opts_create,
    .opts_display = opts_display,
    .opts_destroy = opts_destroy,

    .opts_bin_to_str = NULL,

    .runtime_create = runtime_create,
    .runtime_display = runtime_display,
    .runtime_destroy = runtime_destroy,
};
