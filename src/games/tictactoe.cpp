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
        *runtime_struct = rgame->data1; // fill runtime struct with a spoofed pointer
        return ERR_OK;
    }

    error_code runtime_display(game* rgame, void* runtime_struct)
    {
        //TODO
        tictactoe_internal_methods* ag_int = (tictactoe_internal_methods*)rgame->methods->internal_methods;
        const char* check_options[3] = {"-", "X", "O"};
        float check_width = 1.2 * ImGui::CalcTextSize("XO").x;
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
                    game* gclone = (game*)malloc(sizeof(game));
                    game_clone(rgame, gclone);
                    ((tictactoe_internal_methods*)gclone->methods->internal_methods)->set_cell(gclone, ix, iy, imgui_check);
                    size_t game_state_buffer_len;
                    const char* game_state_buffer;
                    game_export_state(gclone, PLAYER_NONE, &game_state_buffer_len, &game_state_buffer);
                    event_any es;
                    event_create_game_state(&es, EVENT_CLIENT_NONE, game_state_buffer);
                    event_queue_push(&Control::main_client->inbox, &es);
                    game_destroy(gclone);
                    free(gclone);
                }
            }
        }
        // edit: player to move
        uint8_t pbuf_c;
        const player_id* pbuf;
        game_players_to_move(rgame, &pbuf_c, &pbuf);
        player_id board_current;
        if (pbuf_c == 0) {
            board_current = PLAYER_NONE;
        } else {

            board_current = pbuf[0];
        }
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
            game* gclone = (game*)malloc(sizeof(game));
            game_clone(rgame, gclone);
            ((tictactoe_internal_methods*)gclone->methods->internal_methods)->set_current_player(gclone, imgui_current);
            size_t game_state_buffer_len;
            const char* game_state_buffer;
            game_export_state(gclone, PLAYER_NONE, &game_state_buffer_len, &game_state_buffer);
            event_any es;
            event_create_game_state(&es, EVENT_CLIENT_NONE, game_state_buffer);
            event_queue_push(&Control::main_client->inbox, &es);
            game_destroy(gclone);
            free(gclone);
        }
        // edit: result
        game_get_results(rgame, &pbuf_c, &pbuf);
        player_id board_result;
        if (pbuf_c == 0) {
            board_result = PLAYER_NONE;
        } else {
            board_result = pbuf[0];
        }
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
            game* gclone = (game*)malloc(sizeof(game));
            game_clone(rgame, gclone);
            ((tictactoe_internal_methods*)gclone->methods->internal_methods)->set_result(gclone, imgui_result);
            size_t game_state_buffer_len;
            const char* game_state_buffer;
            game_export_state(gclone, PLAYER_NONE, &game_state_buffer_len, &game_state_buffer);
            event_any es;
            event_create_game_state(&es, EVENT_CLIENT_NONE, game_state_buffer);
            event_queue_push(&Control::main_client->inbox, &es);
            game_destroy(gclone);
            free(gclone);
        }
        return ERR_OK;
    }

    error_code runtime_destroy(void* runtime_struct)
    {
        //TODO
        return ERR_OK;
    }

} // namespace

const game_wrap tictactoe_gw{
    .game_api_version = SURENA_GAME_API_VERSION,
    .backend = &tictactoe_standard_gbe,
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
