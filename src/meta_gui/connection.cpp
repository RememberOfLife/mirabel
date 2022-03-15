#include <cstdint>
#include <cstring>

#include <SDL2/SDL.h>
#include "SDL_net.h"
#include "imgui.h"

#include "network/network_client.hpp"
#include "state_control/client.hpp"
#include "state_control/event_queue.hpp"
#include "state_control/event.hpp"
#include "state_control/guithread.hpp"

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
        //TODO put these in the metagui static space
        static char server_address[64] = "127.0.0.1"; //TODO for debugging purposes this is loopback
        static uint16_t server_port = 61801; // default mirabel port

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

        bool connected = (StateControl::main_client->network_send_queue != NULL);
        if (connected) {
            if (ImGui::Button("Disconnect", ImVec2(-1.0f, 0.0f))) {
                StateControl::main_client->t_gui.inbox.push(StateControl::event(StateControl::EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSE));
            }
        } else {
            if (ImGui::Button("Connect", ImVec2(-1.0f, 0.0f))) {
                Network::NetworkClient* net_client = new Network::NetworkClient();
                if (net_client->open(server_address, server_port)) {
                    net_client->recv_queue = &StateControl::main_client->t_gui.inbox;
                    StateControl::main_client->t_network = net_client;
                    StateControl::main_client->t_gui.inbox.push(StateControl::event(StateControl::EVENT_TYPE_NETWORK_ADAPTER_LOAD));
                } else {
                    delete net_client;
                }
            }
        }
        if (connected) {
            ImGui::BeginDisabled();
        }
        ImGui::InputText("address", server_address, 64, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterAddressLetters);
        ImGui::InputScalar("port", ImGuiDataType_U16, &server_port);
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

        //REMOVE
        if (connected && ImGui::Button("PING")) {
            StateControl::main_client->network_send_queue->push(StateControl::event(StateControl::EVENT_TYPE_NETWORK_PROTOCOL_PING));
        }
        //REMOVE

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
