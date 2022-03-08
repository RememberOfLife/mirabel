#include <cstdint>
#include <cstring>

#include "imgui.h"

// #include "state_control/client.hpp"
// #include "state_control/event_queue.hpp"
// #include "state_control/event.hpp"
// #include "state_control/guithread.hpp"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    struct TextFilters
    {
        // return 0 (pass) if the character is allowed
        static int FilterSanitizedTextLetters(ImGuiInputTextCallbackData* data)
        {
            if (data->EventChar < 256 && strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-", (char)data->EventChar))
                return 0;
            return 1;
        }
        static int FilterAddressLetters(ImGuiInputTextCallbackData* data)
        {
            if (data->EventChar < 256 && strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-./:", (char)data->EventChar))
                return 0;
            return 1;
        }
    };

    void connection_window(bool* p_open)
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(280, 300), ImGuiCond_FirstUseEver);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
        bool window_contents_visible = ImGui::Begin("Connection", p_open, window_flags);
        if (!window_contents_visible)
        {
            ImGui::End();
            return;
        }

        static bool connected = false;
        // draw game start,stop,restart
        // locks all pre loading input elements if game is running, stop is only available if running
        if (connected) {
            if (ImGui::Button("Disconnect", ImVec2(-1.0f, 0.0f))) {
                connected = false;
            }
        } else {
            if (ImGui::Button("Connect", ImVec2(-1.0f, 0.0f))) {
                connected = true;
            }
        }
        if (connected) {
            ImGui::BeginDisabled();
        }
        static char server_address[64] = "";
        ImGui::InputText("address", server_address, 64, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterAddressLetters);
        static char server_port[64] = "61801"; // default mirabel port
        ImGui::InputText("port", server_port, 64, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterSanitizedTextLetters);
        if (connected) {
            ImGui::EndDisabled();
        }
        ImGui::Separator();
        //TODO proper status line
        // when offline: "offline"
        // when trying to connect: "connecting.. (timeout in XXXXms)"
        // when connection failed, log the reason
        // on successful connection: log
        // while connected: "connected (LXXms HXXXX)" where L ist latency and H is time since last heartbeat
        ImGui::Text("Status: ");
        ImGui::SameLine();
        if (connected) {
            ImGui::TextColored(ImVec4(0.22, 0.85, 0.52, 1), "connected");
        } else {
            ImGui::TextColored(ImVec4(0.85, 0.52, 0.22, 1), "offline");
        }

        //TODO when the server was found we need to send our authentication request so it sends us a valid auth token to use in all future requests


        // static bool hide_pw = true;
        // static char password[64] = "";
        // ImGuiInputTextFlags password_flags = ImGuiInputTextFlags_CallbackCharFilter;
        // if (hide_pw) {
        //     password_flags |= ImGuiInputTextFlags_Password;
        // }
        // ImGui::InputText("password", password, 64, password_flags, TextFilters::FilterSanitizedTextLetters);
        // ImGui::SameLine();
        // if (ImGui::Button(hide_pw ? "Show" : "Hide")) {
        //     hide_pw = !hide_pw;
        // }
        // static char username[64] = "";
        // ImGui::InputText("username", username, 64, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterSanitizedTextLetters);

        ImGui::End();
    }

}
