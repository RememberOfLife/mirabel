#include <cstdint>

#include "imgui.h"

#include "control/client.hpp"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    //TODO same as in conn and what chat should use, dont dupe maybe
    struct TextFilters {
        // return 0 (pass) if the character is allowed
        static int FilterSanitizedTextLetters(ImGuiInputTextCallbackData* data)
        {
            if (data->EventChar < 256 && strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-", (char)data->EventChar))
                return 0;
            return 1;
        }
    };

    void lobby_window(bool* p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(450, 550), ImGuiCond_FirstUseEver);
        bool window_contents_visible = ImGui::Begin("Lobby Config", p_open);
        if (!window_contents_visible) {
            ImGui::End();
            return;
        }

        if (Control::main_client->network_send_queue == NULL) {
            ImGui::Text("lobbies only available on server");
            ImGui::End();
            return;
        }

        //TODO join screen will move to different tab with search and listings
        bool disable_joincreate = the_lobby_info.id != EVENT_LOBBY_NONE;
        if (disable_joincreate) {
            ImGui::BeginDisabled();
        }
        ImGui::InputText("name", the_lobby_info.name, sizeof(lobby_info::name));
        static bool hide_pw = true;
        ImGuiInputTextFlags password_flags = ImGuiInputTextFlags_CallbackCharFilter;
        if (hide_pw) {
            password_flags |= ImGuiInputTextFlags_Password;
        }
        ImGui::InputText("password", the_lobby_info.password, sizeof(lobby_info::password), password_flags, TextFilters::FilterSanitizedTextLetters);
        ImGui::SameLine();
        if (disable_joincreate) {
            ImGui::EndDisabled();
        }
        if (ImGui::SmallButton(hide_pw ? "S" : "H")) {
            hide_pw = !hide_pw;
        }
        //TODO these buttons could use the running state just like connection buttons
        if (disable_joincreate == false) {
            if (ImGui::Button("Create")) {
                event_any es;
                event_create_lobby_create(&es, the_lobby_info.name, strlen(the_lobby_info.password) == 0 ? NULL : the_lobby_info.password, 8);
                event_queue_push(&Control::main_client->t_network->send_queue, &es);
            }
            ImGui::SameLine();
            if (ImGui::Button("Join", ImVec2(-1.0f, 0.0f))) {
                event_any es;
                event_create_lobby_join(&es, the_lobby_info.name, strlen(the_lobby_info.password) == 0 ? NULL : the_lobby_info.password);
                event_queue_push(&Control::main_client->t_network->send_queue, &es);
            }
        } else {
            //TODO only show destroy to lobby admin, and then ONLY show destroy
            if (ImGui::Button("Destroy")) {
                event_any es;
                event_create_type(&es, EVENT_TYPE_LOBBY_DESTROY);
                event_queue_push(&Control::main_client->t_network->send_queue, &es);
            }
            ImGui::SameLine();
            if (ImGui::Button("Leave", ImVec2(-1.0f, 0.0f))) {
                event_any es;
                event_create_type(&es, EVENT_TYPE_LOBBY_LEAVE);
                event_queue_push(&Control::main_client->t_network->send_queue, &es);
            }
        }

        ImGui::Separator();

        if (the_lobby_info.id != EVENT_LOBBY_NONE) {
            //TODO this is a table, on the right side 1/4 is the user list, everthing following is on the left:
            //TODO lobby settings, client kinda needs offline lobby for that to work properly.. //TODO offline server :/
            //TODO game config
            //TODO client<->playerid mappings
        }

        ImGui::End();
    }

} // namespace MetaGui
