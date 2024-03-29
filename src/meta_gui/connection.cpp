#include <cstdint>
#include <cstring>

#include "imgui.h"

#include "control/client.hpp"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"
#include "control/timeout_crash.hpp"
#include "network/network_client.hpp"
#include "network/protocol.hpp"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    struct TextFilters {
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
        if (!window_contents_visible) {
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
                    event_any es;
                    event_create_type(&es, EVENT_TYPE_NETWORK_ADAPTER_UNLOAD);
                    event_queue_push(&Control::main_client->inbox, &es);
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

        //TODO proper status line
        // when trying to connect: "connecting.. (timeout in XXXXms)"
        // while connected: "connected (LXXms HXXXX)" where L ist latency and H is time since last heartbeat
        ImGui::Text("Status:");
        ImGui::SameLine();
        switch (conn_info.adapter) {
            case RUNNING_STATE_NONE: {
                ImGui::TextColored(ImVec4(0.85, 0.52, 0.22, 1), "offline");
            } break;
            case RUNNING_STATE_ONGOING: {
                ImGui::TextColored(ImVec4(0.85, 0.52, 0.22, 1), "connecting..");
            } break;
            case RUNNING_STATE_DONE: {
                ImGui::TextColored(ImVec4(0.22, 0.85, 0.52, 1), "connected");
                switch (conn_info.connection) {
                    case RUNNING_STATE_NONE:
                        break;
                    case RUNNING_STATE_ONGOING: {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.85, 0.52, 0.22, 1), " (ssl)");
                    } break;
                    case RUNNING_STATE_DONE: {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.22, 0.85, 0.52, 1), "+ ssl");
                        ImGui::SameLine();
                        if (!conn_info.auth_info) {
                            ImGui::TextDisabled(" (auth)");
                            break;
                        }
                        switch (conn_info.authentication) {
                            case RUNNING_STATE_NONE: {
                                ImGui::Text(" (auth)");
                            } break;
                            case RUNNING_STATE_ONGOING: {
                                ImGui::TextColored(ImVec4(0.85, 0.52, 0.22, 1), " (auth)");
                            } break;
                            case RUNNING_STATE_DONE: {
                                ImGui::TextColored(ImVec4(0.22, 0.85, 0.52, 1), "+ auth");
                            } break;
                        }
                    } break;
                }
            } break;
        }
        if (Control::main_client->network_send_queue) {
            ImGui::SameLine();
            if (ImGui::SmallButton("PING")) {
                event_any es;
                event_create_type(&es, EVENT_TYPE_NETWORK_PROTOCOL_PING);
                event_queue_push(Control::main_client->network_send_queue, &es);
            }
        }

        if (conn_info.server_cert_thumbprint) {
            // show only first n bytes, then on click show everything in a matrix
            ImGui::Text("Thumbprint: ");
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, -1)); //TODO can we just skip proper vertical spacing here?
            const int show_bytes = 8;
            for (int i = 0; i < show_bytes; i++) {
                ImGui::SameLine();
                ImGui::Text("%02x", *(conn_info.server_cert_thumbprint + i));
                if (i < show_bytes - 1) {
                    ImGui::SameLine();
                    ImGui::TextUnformatted(":");
                }
            }
            ImGui::PopStyleVar();
            ImGui::SameLine();
            if (ImGui::SmallButton("F")) {
                ImGui::OpenPopup("full_thumbprint");
            }
            if (ImGui::BeginPopup("full_thumbprint")) {
                if (ImGui::SmallButton("Close")) {
                    ImGui::CloseCurrentPopup();
                }
                const int show_bytes_x = 8;
                int show_bytes_y = Network::SHA256_LEN / show_bytes_x;
                int show_bytes_x_overflow = Network::SHA256_LEN - (show_bytes_x * show_bytes_y);
                if (show_bytes_x_overflow > 1) {
                    show_bytes_y++;
                }
                for (int i = 0; i < show_bytes_y; i++) {
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, -1)); //TODO can we just skip proper vertical spacing here?
                    int show_bytes_x_cur = (i == show_bytes_y - 1 && show_bytes_x_overflow > 0 ? show_bytes_x_overflow : show_bytes_x);
                    for (int j = 0; j < show_bytes_x_cur; j++) {
                        ImGui::Text("%02x", *(conn_info.server_cert_thumbprint + i * show_bytes_x + j));
                        ImGui::SameLine();
                        if (j < show_bytes_x_cur - 1) {
                            ImGui::TextUnformatted(":");
                            ImGui::SameLine();
                        }
                    }
                    ImGui::PopStyleVar();
                    if (i < show_bytes_y - 1) {
                        ImGui::NewLine();
                    }
                }
                ImGui::EndPopup();
            }
        }
        ImGui::Separator();

        if (conn_info.connection == RUNNING_STATE_ONGOING && conn_info.verifail_reason != NULL) {
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
                    event_any es;
                    event_create_ssl_thumbprint(&es, EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_ACCEPT);
                    event_queue_push(&Control::main_client->t_network->send_queue, &es);
                }
                ImGui::PopStyleColor(3);
            }
            ImGui::EndTable();
            ImGui::PopStyleColor();
        }

        bool disable_login = !conn_info.auth_allow_login;
        bool disable_guest = !conn_info.auth_allow_guest && !disable_login;
        bool disable_un = disable_login && !conn_info.auth_allow_guest;
        bool disable_pw = disable_login && !conn_info.auth_want_guest_pw;

        if (conn_info.connection == RUNNING_STATE_DONE && conn_info.auth_info) {
            bool disable_authentication = conn_info.authentication > RUNNING_STATE_NONE;
            if (disable_authentication) {
                ImGui::BeginDisabled();
            }
            if (disable_un) {
                ImGui::BeginDisabled();
            }
            ImGui::InputText("username", conn_info.username, sizeof(connection_info::username), ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterSanitizedTextLetters);
            if (disable_un) {
                ImGui::EndDisabled();
            }
            static bool hide_pw = true;
            ImGuiInputTextFlags password_flags = ImGuiInputTextFlags_CallbackCharFilter;
            if (hide_pw || disable_authentication) {
                password_flags |= ImGuiInputTextFlags_Password;
            }
            if (disable_pw) {
                ImGui::BeginDisabled();
            }
            ImGui::InputText("password", conn_info.password, sizeof(connection_info::password), password_flags, TextFilters::FilterSanitizedTextLetters);
            ImGui::SameLine();
            if (ImGui::SmallButton(hide_pw ? "S" : "H")) {
                hide_pw = !hide_pw;
            }
            if (disable_pw) {
                ImGui::EndDisabled();
            }
            if (disable_authentication) {
                ImGui::EndDisabled();
            }
            switch (conn_info.authentication) {
                case RUNNING_STATE_NONE: {
                    float btn_width = ImGui::CalcItemWidth();
                    if (disable_login) {
                        ImGui::BeginDisabled();
                    }
                    if (ImGui::Button("Login", ImVec2(btn_width, 0.0f))) {
                        event_any es;
                        event_create_auth(&es, EVENT_TYPE_USER_AUTHN, EVENT_CLIENT_NONE, false, conn_info.username, conn_info.password);
                        event_queue_push(&Control::main_client->t_network->send_queue, &es);
                        conn_info.authentication = RUNNING_STATE_ONGOING;
                        free(conn_info.authfail_reason);
                        conn_info.authfail_reason = NULL;
                    }
                    if (disable_login) {
                        ImGui::EndDisabled();
                    }
                    ImGui::SameLine();
                    btn_width = ImGui::GetContentRegionAvail().x;
                    if (disable_guest) {
                        ImGui::BeginDisabled();
                    }
                    if (ImGui::Button("Guest", ImVec2(btn_width, 0.0f))) {
                        event_any es;
                        event_create_auth(&es, EVENT_TYPE_USER_AUTHN, EVENT_CLIENT_NONE, true, conn_info.username, conn_info.password);
                        event_queue_push(&Control::main_client->t_network->send_queue, &es);
                        conn_info.authentication = RUNNING_STATE_ONGOING;
                        free(conn_info.authfail_reason);
                        conn_info.authfail_reason = NULL;
                    }
                    if (disable_guest) {
                        ImGui::EndDisabled();
                    }
                } break;
                case RUNNING_STATE_ONGOING: {
                    ImGui::BeginDisabled();
                    ImGui::Button("Authenticating..", ImVec2(-1.0f, 0.0f));
                    ImGui::EndDisabled();
                } break;
                case RUNNING_STATE_DONE: {
                    if (ImGui::Button("Logout", ImVec2(-1.0f, 0.0f))) {
                        event_any es;
                        event_create_auth_fail(&es, EVENT_TYPE_USER_AUTHFAIL, NULL);
                        event_queue_push(&Control::main_client->t_network->send_queue, &es);
                    }
                } break;
            }
            if (conn_info.authfail_reason) {
                ImGui::TextColored(ImVec4(0.85, 0.52, 0.22, 1), "%s", conn_info.authfail_reason);
            }
        }

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
        conn_info.auth_info = false;
        conn_info.authentication = RUNNING_STATE_NONE;
        conn_info.auth_allow_login = false;
        conn_info.auth_allow_guest = false;
        conn_info.auth_want_guest_pw = false;
        conn_info.username[0] = '\0';
        conn_info.password[0] = '\0';
        free(conn_info.authfail_reason);
        conn_info.authfail_reason = NULL;
    }

} // namespace MetaGui
