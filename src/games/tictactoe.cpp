#include <cstdint>
#include <cstdlib>

#include "imgui.h"
#include "surena/games/tictactoe.h"
#include "surena/game.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "mirabel/game_wrap.h"
#include "control/client.hpp"

#include "games/game_catalogue.hpp"

namespace {

    error_code runtime_create(game* rgame, void** runtime_struct)
    {
        //TODO
        return ERR_OK;
    }

    error_code runtime_display(game* rgame, void* runtime_struct)
    {
        //TODO
        tictactoe_internal_methods* ag_int = (tictactoe_internal_methods*)rgame->methods->internal_methods;
        const char* check_options[3] = {"-", "X", "O"};
        float check_width = 1.2*ImGui::CalcTextSize("XO").x;
        int imgui_id = 0;
        // edit: board state
        ImGui::Text("board state:");
        for (int iy = 2; iy >= 0; iy--) {
            for (int ix = 0; ix < 3; ix++) {
                if (ix > 0) {
                    ImGui::SameLine();
                }
                player_id board_state;
                ag_int->get_cell(rgame, ix, iy, &board_state);
                player_id imgui_check = board_state;
                ImGui::PushID(imgui_id++);
                ImGui::PushItemWidth(check_width);
                if (ImGui::BeginCombo("", check_options[imgui_check], ImGuiComboFlags_NoArrowButton)) {
                    for (int i = 0; i < 3; i++) {
                        bool is_selected = (imgui_check == i);
                        if (ImGui::Selectable(check_options[i], is_selected)) {
                            imgui_check = i;
                        }
                        // set the initial focus when opening the combo
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
                ImGui::PopID();
                if (imgui_check != board_state) {
                    // modified state via imgui, clone+edit+load
                    game* game_clone = (game*)malloc(sizeof(game));
                    rgame->methods->clone(rgame, game_clone);
                    ((tictactoe_internal_methods*)game_clone->methods->internal_methods)->set_cell(game_clone, ix, iy, imgui_check);
                    size_t game_state_buffer_len = game_clone->sizer.state_str;
                    char* game_state_buffer = (char*)malloc(game_state_buffer_len);
                    game_clone->methods->export_state(game_clone, &game_state_buffer_len, game_state_buffer);
                    f_event_any es;
                    f_event_create_game_state(&es, F_EVENT_CLIENT_NONE, game_state_buffer);
                    f_event_queue_push(&Control::main_client->inbox, &es);
                    game_clone->methods->destroy(game_clone);
                    free(game_clone);
                }
            }
        }
        // edit: player to move
        player_id pbuf;
        uint8_t pbuf_c;
        rgame->methods->players_to_move(rgame, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
            pbuf = PLAYER_NONE;
        }
        player_id board_current = pbuf;
        player_id imgui_current = board_current;
        ImGui::PushID(imgui_id++);
        ImGui::PushItemWidth(check_width);
        if (ImGui::BeginCombo("player to move", check_options[imgui_current], ImGuiComboFlags_NoArrowButton)) {
            for (int i = 0; i < 3; i++) {
                bool is_selected = (imgui_current == i);
                if (ImGui::Selectable(check_options[i], is_selected)) {
                    imgui_current = i;
                }
                // set the initial focus when opening the combo
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::PopID();
        if (imgui_current != board_current) {
            // modified state via imgui, clone+edit+load
            game* game_clone = (game*)malloc(sizeof(game));
            rgame->methods->clone(rgame, game_clone);
            ((tictactoe_internal_methods*)game_clone->methods->internal_methods)->set_current_player(game_clone, imgui_current);
            size_t game_state_buffer_len = game_clone->sizer.state_str;
            char* game_state_buffer = (char*)malloc(game_state_buffer_len);
            game_clone->methods->export_state(game_clone, &game_state_buffer_len, game_state_buffer);
            f_event_any es;
            f_event_create_game_state(&es, F_EVENT_CLIENT_NONE, game_state_buffer);
            f_event_queue_push(&Control::main_client->inbox, &es);
            game_clone->methods->destroy(game_clone);
            free(game_clone);
        }
        // edit: result
        rgame->methods->get_results(rgame, &pbuf_c, &pbuf);
        if (pbuf_c == 0) {
            pbuf = PLAYER_NONE;
        }
        player_id board_result = pbuf;
        player_id imgui_result = board_result;
        ImGui::PushID(imgui_id++);
        ImGui::PushItemWidth(check_width);
        if (ImGui::BeginCombo("result", check_options[imgui_result], ImGuiComboFlags_NoArrowButton)) {
            for (int i = 0; i < 3; i++) {
                bool is_selected = (imgui_result == i);
                if (ImGui::Selectable(check_options[i], is_selected)) {
                    imgui_result = i;
                }
                // set the initial focus when opening the combo
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::PopID();
        if (imgui_result != board_result) {
            // modified state via imgui, clone+edit+load
            game* game_clone = (game*)malloc(sizeof(game));
            rgame->methods->clone(rgame, game_clone);
            ((tictactoe_internal_methods*)game_clone->methods->internal_methods)->set_result(game_clone, imgui_result);
            size_t game_state_buffer_len = game_clone->sizer.state_str;
            char* game_state_buffer = (char*)malloc(game_state_buffer_len);
            game_clone->methods->export_state(game_clone, &game_state_buffer_len, game_state_buffer);
            f_event_any es;
            f_event_create_game_state(&es, F_EVENT_CLIENT_NONE, game_state_buffer);
            f_event_queue_push(&Control::main_client->inbox, &es);
            game_clone->methods->destroy(game_clone);
            free(game_clone);
        }
        return ERR_OK;
    }

    error_code runtime_destroy(void* runtime_struct)
    {
        //TODO
        return ERR_OK;
    }

}

const game_wrap tictactoe_gw{
    .game_api_version = SURENA_GAME_API_VERSION,
    .backend = &tictactoe_gbe,
    .features = (game_wrap_feature_flags){
        .options = false,
        .initial_state = false,
        .runtime = true,
    },

    .opts_create = NULL,
    .opts_display = NULL,
    .opts_destroy = NULL,

    .opts_bin_to_str = NULL,
    .opts_initial_state = NULL,

    .runtime_create = runtime_create,
    .runtime_display = runtime_display,
    .runtime_destroy = runtime_destroy,
};
