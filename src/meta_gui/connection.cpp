#include <cstdint>
#include <cstring>

#include "imgui.h"

#include "control/client.hpp"
#include "control/event_queue.hpp"
#include "control/event.hpp"
#include "control/timeout_crash.hpp"
#include "network/network_client.hpp"
#include "network/protocol.hpp"

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

        switch (conn_info.adapter) {
            case RUNNING_STATE_NONE: {
                if (ImGui::Button("Connect", ImVec2(-1.0f, 0.0f))) {
                    Network::NetworkClient* net_client = new Network::NetworkClient(&Control::main_client->t_tc);
                    net_client->recv_queue = &Control::main_client->inbox;
                    if (net_client->open(conn_info.server_address, conn_info.server_port)) {
                        Control::main_client->t_network = net_client;
                        conn_info.adapter = RUNNING_STATE_ONGOING;
                    } else {
                        delete net_client;
                    }
                }
            } break;
            case RUNNING_STATE_ONGOING: {
                ImGui::BeginDisabled();
                ImGui::Button("Connecting..", ImVec2(-1.0f, 0.0f));
                ImGui::EndDisabled();
            } break;
            case RUNNING_STATE_DONE: {
                if (ImGui::Button("Disconnect", ImVec2(-1.0f, 0.0f))) {
                    Control::main_client->inbox.push(Control::event(Control::EVENT_TYPE_NETWORK_ADAPTER_UNLOAD));
                }
            } break;
        }
        bool disable_address = conn_info.adapter > RUNNING_STATE_NONE;
        if (disable_address) {
            ImGui::BeginDisabled();
        }
        ImGui::InputText("address", conn_info.server_address, sizeof(connection_info::server_address), ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterAddressLetters);
        ImGui::InputScalar("port", ImGuiDataType_U16, &conn_info.server_port);
        if (disable_address) {
            ImGui::EndDisabled();
        }
        ImGui::Separator();

        //TODO make this a table of status lines
        //TODO make this a sidebar box like the cert verification box?
        //TODO proper status line, show state of adapter,ssl-conn,auth
        // when offline: "offline"
        // when trying to connect: "connecting.. (timeout in XXXXms)"
        // when connection failed, log the reason
        // on successful connection: log
        // while connected: "connected (LXXms HXXXX)" where L ist latency and H is time since last heartbeat
        ImGui::Text("Status:");
        ImGui::SameLine();
        switch (conn_info.connection) {
            case RUNNING_STATE_NONE: {
                ImGui::TextColored(ImVec4(0.85, 0.52, 0.22, 1), "offline");
            } break;
            case RUNNING_STATE_ONGOING: {            
                ImGui::TextColored(ImVec4(0.85, 0.52, 0.22, 1), "connecting..");
            } break;
            case RUNNING_STATE_DONE: {
                ImGui::TextColored(ImVec4(0.22, 0.85, 0.52, 1), "connected");
            } break;
        }
        if (conn_info.server_cert_thumbprint) {
            //TODO how does this get here? extra event?
            ImGui::Text("Thumbprint:");
            ImGui::SameLine();
            ImGui::Text("%s", conn_info.server_cert_thumbprint);
        }
        ImGui::Separator();

        if (conn_info.connection == RUNNING_STATE_ONGOING && conn_info.verifail_reason != NULL) {
            //TODO after a connection was established, if verification failed show info + accept
            //TODO any way to make the border bigger? and maybe give the whole box a lighter background tone?
            ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, IM_COL32(226, 74, 117, 255));
            ImGui::BeginTable("sidebar_table", 1, ImGuiTableFlags_BordersV);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            {
                ImGui::Text("Server cert verification failed:");
                ImGui::TextColored(ImVec4(0.89, 0.29, 0.46, 1), " %s", conn_info.verifail_reason);
                
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(154, 58, 58, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(212, 81, 81, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(226, 51, 51, 255));
                if (ImGui::Button("Accept Insecure Connection", ImVec2(-1.0f, 0.0f))) {
                    // accept connection
                    Control::main_client->t_network->send_queue.push(Control::event(Control::EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_ACCEPT));
                }
                ImGui::PopStyleColor(3);
            }
            ImGui::EndTable();
            ImGui::PopStyleColor();
        }

        if (false && conn_info.connection == RUNNING_STATE_DONE) { //REWORK put in when auth gets here
            bool disable_authentication = conn_info.authentication > RUNNING_STATE_NONE;
            if (disable_authentication) {
                ImGui::BeginDisabled();
            }
            ImGui::InputText("username", conn_info.username, sizeof(connection_info::username), ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterSanitizedTextLetters);
            static bool hide_pw = true;
            ImGuiInputTextFlags password_flags = ImGuiInputTextFlags_CallbackCharFilter;
            if (hide_pw || disable_authentication) {
                password_flags |= ImGuiInputTextFlags_Password;
            }
            ImGui::InputText("password", conn_info.password, sizeof(connection_info::password), password_flags, TextFilters::FilterSanitizedTextLetters);
            ImGui::SameLine();
            if (ImGui::SmallButton(hide_pw ? "S" : "H")) {
                hide_pw = !hide_pw;
            }
            if (disable_authentication) {
                ImGui::EndDisabled();
            }
            switch (conn_info.authentication) {
                case RUNNING_STATE_NONE: {
                    if (ImGui::Button("Guest")) {
                        //TODO
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Login", ImVec2(-1.0f, 0.0f))) {
                        //TODO
                    }
                } break;
                case RUNNING_STATE_ONGOING: {
                    ImGui::BeginDisabled();
                    ImGui::Button("...", ImVec2(-1.0f, 0.0f));
                    ImGui::EndDisabled();
                } break;
                case RUNNING_STATE_DONE: {
                    if (ImGui::Button("Logout", ImVec2(-1.0f, 0.0f))) {
                        //TODO
                    }
                } break;
            }
        }

        //REWORK
        if (Control::main_client->network_send_queue && ImGui::Button("PING")) {
            Control::main_client->network_send_queue->push(Control::event(Control::EVENT_TYPE_NETWORK_PROTOCOL_PING));
        }
        //REWORK

        ImGui::End();
    }

    void connection_info_reset()
    {
        conn_info.adapter = RUNNING_STATE_NONE;
        conn_info.connection = RUNNING_STATE_NONE;
        free(conn_info.server_cert_thumbprint);
        conn_info.server_cert_thumbprint = NULL;
        free(conn_info.verifail_reason);
        conn_info.verifail_reason = NULL;
        conn_info.username[0] = '\0';
        conn_info.password[0] = '\0';
        conn_info.authentication = RUNNING_STATE_NONE;
    }

}
