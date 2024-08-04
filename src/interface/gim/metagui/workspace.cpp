#include <cstdint>
#include <cstdio>

#include <GL/glew.h>
#include "imgui.h"
#include "rosalia/serialization.h"
#include "rosalia/vector.h"

#include "mirabel/application.h"
#include "mirabel/client.h"
#include "mirabel/methods_registry.h"
#include "mirabel/workspace.h"

#include "interface/gim/window.hpp"

struct ConnectionTextFilters {
    // return 0 (pass) if the character is allowed

    static int FilterAddressLetters(ImGuiInputTextCallbackData* data)
    {
        if (data->EventChar < 256 && strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-./:", (char)data->EventChar)) {
            return 0;
        }
        return 1;
    }

    static int FilterSanitizedTextLetters(ImGuiInputTextCallbackData* data)
    {
        if (data->EventChar < 256 && strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-", (char)data->EventChar)) {
            return 0;
        }
        return 1;
    }
};

void graphical_immediate_mode_interface::metagui_workspace_window(uint32_t workspace_idx)
{
    gim_workspace* gim_ws = &workspaces[workspace_idx];

    bool opened = true;
    char window_title[128];
    sprintf(window_title, "Workspace %u###workspace%u", gim_ws->client_workspace->id, gim_ws->client_workspace->id);
    ImVec2 mcenter = ImGui::GetMainViewport()->GetCenter(); //TODO better initial placement as center docked (on currently active workspace)
    ImGui::SetNextWindowPos(mcenter, ImGuiCond_FirstUseEver, ImVec2(0.5, 0.5));
    ImGui::SetNextWindowSize(ImVec2(1000, 750), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(window_title, &opened)) {

        ImGuiTabBarFlags tabbar_flags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable;
        if (ImGui::BeginTabBar("Breadcrumbs", tabbar_flags)) {
            //TODO draw out tabitems into separate functions
            if (ImGui::BeginTabItem("> Connection")) {

                ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
                ImVec2 parent_pos = ImGui::GetCursorScreenPos();
                ImVec2 insize = ImGui::GetContentRegionAvail();
                ImGui::SetNextWindowPos(ImVec2(parent_pos.x + insize.x / 2, parent_pos.y), ImGuiCond_None, ImVec2(0.5, 0));
                const float window_size_min = 450;
                float window_size = insize.x;
                if (window_size > window_size_min) {
                    window_size = window_size_min;
                }
                ImGui::BeginChild("Connection", ImVec2(window_size, 0), ImGuiChildFlags_None, window_flags);
                {
                    //TODO possibly draw out into separate functions

                    //TODO important: make a button for destroying connections
                    if (gim_ws->client_workspace->netc == NULL) {
                        if (ImGui::Button("New Connection", ImVec2(-1, 0))) {
                            gim_ws->connection_new();
                        } else {
                            ImGui::Separator();
                            float listbox_item_count = VEC_LEN(&appi.aclient->net_conns);
                            bool disable_existing_connection_listbox = listbox_item_count == 0;
                            if (disable_existing_connection_listbox) {
                                ImGui::BeginDisabled();
                            }
                            listbox_item_count++;
                            if (listbox_item_count < 3) {
                                listbox_item_count = 3;
                            }
                            if (listbox_item_count > 7) {
                                listbox_item_count = 7;
                            }
                            bool double_click_attach = false;
                            if (ImGui::BeginListBox("##ConnectionList", ImVec2(-FLT_MIN, listbox_item_count * ImGui::GetTextLineHeightWithSpacing()))) {
                                for (size_t connections_idx = 0; connections_idx < VEC_LEN(&appi.aclient->net_conns); connections_idx++) {
                                    const bool is_selected = (gim_ws->current_connection_idx == connections_idx);
                                    char selectable_connection_name[64];
                                    sprintf(selectable_connection_name, "Connection %zu", connections_idx); //TODO better name with context from connection; maybe only offer the ones that are actually online an up an running to be used?
                                    if (ImGui::Selectable(selectable_connection_name, is_selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                                        gim_ws->current_connection_idx = connections_idx;
                                        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                                            double_click_attach = true;
                                        }
                                    }
                                    if (is_selected) {
                                        ImGui::SetItemDefaultFocus();
                                    }
                                }
                                ImGui::EndListBox();
                            }
                            //TODO switch this and the new button, so accidental detach clicks do not automatically generate a new connection, also make it so that detaching, places the initial focus in the selectionbox on the current item
                            if (ImGui::Button("Attach to Connection", ImVec2(-1, 0)) || double_click_attach) {
                                gim_ws->connection_attach(appi.aclient->net_conns[gim_ws->current_connection_idx]);
                            }
                            if (disable_existing_connection_listbox) {
                                ImGui::EndDisabled();
                            }
                        }
                    } else {
                        if (ImGui::Button("Detach from Connection", ImVec2(-1, 0))) {
                            gim_ws->connection_detach();
                        } else {
                            ImGui::PushFont(fonts.imgui_mono);
                            ImGui::Text("Using: %p", gim_ws->client_workspace->netc); //TODO better name with context from connection; maybe only offer the ones that are actually online an up an running to be used?
                            ImGui::PopFont();

                            ImGui::Separator();
                            const network_adapter_methods* offline_adapter_methods = (const network_adapter_methods*)methods_registry_get(&appi.registry, "network_adapter_client", "offline");
                            bool offline_server_available = appi.aserver != NULL;
                            bool connection_setup_disabled = gim_ws->client_workspace->netc->adapter_state != RSI_IDLE;
                            if (!offline_server_available || connection_setup_disabled) {
                                ImGui::BeginDisabled();
                            }
                            if (ImGui::Button(">> Offline Server Connect <<", ImVec2(-1, 2 * ImGui::GetTextLineHeightWithSpacing()))) {
                                if (offline_adapter_methods) {
                                    gim_ws->client_workspace->netc->adapter.methods = offline_adapter_methods;
                                    network_connection_adapter_open(gim_ws->client_workspace->netc);
                                } else {
                                    mirabel_slogf(LOGS_WARN, "could not find offline adapter methods for client");
                                }
                            }
                            if (!offline_server_available || connection_setup_disabled) {
                                ImGui::EndDisabled();
                            }
                            if (false) {
                                ImGui::Separator();
                                ImGui::Text("Saved Servers");
                                ImGui::SameLine();
                                ImGui::SmallButton("Clear"); //TODO right align
                                //TODO automatically scale height of listbox up to some value, depending on the number of saved servers?
                                if (ImGui::BeginListBox("##ServerList", ImVec2(-FLT_MIN, 1 * ImGui::GetTextLineHeightWithSpacing()))) {
                                    static int item_current_idx = 0;
                                    for (int n = 0; n < 3; n++) {
                                        const bool is_selected = (item_current_idx == n);
                                        char selectable_server_name[64];
                                        sprintf(selectable_server_name, "Server %i", n);
                                        if (ImGui::Selectable(selectable_server_name, is_selected)) {
                                            item_current_idx = n;
                                        }
                                        if (is_selected) {
                                            ImGui::SetItemDefaultFocus();
                                        }
                                    }
                                    ImGui::EndListBox();
                                }
                            }
                            ImGui::Separator();
                            if (connection_setup_disabled) {
                                ImGui::BeginDisabled();
                            }
                            ImGui::Text("Connection");
                            const char* network_adapter_preview = "<none>";
                            if (gim_ws->client_workspace->netc->adapter.methods != NULL) {
                                network_adapter_preview = gim_ws->client_workspace->netc->adapter.methods->name;
                            }
                            uint32_t method_count = methods_registry_get_count(&appi.registry, "network_adapter_client");
                            bool disable_adapter_choice = (method_count == (0 + (offline_adapter_methods != NULL ? 1 : 0)));
                            if (disable_adapter_choice) {
                                ImGui::BeginDisabled();
                            }
                            if (ImGui::BeginCombo("Network Adapter", network_adapter_preview)) {
                                for (uint32_t method_idx = 0; method_idx < method_count; method_idx++) {
                                    const methods_entry* adapter_methods = methods_registry_get_entry_by_idx(&appi.registry, "network_adapter_client", method_idx);
                                    const bool is_selected = (adapter_methods->methods == gim_ws->client_workspace->netc->adapter.methods);
                                    if (adapter_methods->methods == offline_adapter_methods) {
                                        continue;
                                    }
                                    if (ImGui::Selectable(((const network_adapter_methods*)adapter_methods->methods)->name, is_selected)) {
                                        gim_ws->client_workspace->netc->adapter.methods = (const network_adapter_methods*)adapter_methods->methods;
                                    }
                                    if (is_selected) {
                                        ImGui::SetItemDefaultFocus();
                                    }
                                }
                                ImGui::EndCombo();
                            }
                            if (disable_adapter_choice) {
                                ImGui::EndDisabled();
                            }
                            bool no_adapter_selected = (gim_ws->client_workspace->netc->adapter.methods == NULL);
                            bool offline_adapter_selected = (gim_ws->client_workspace->netc->adapter.methods == offline_adapter_methods);
                            char* server_addr_buf = gim_ws->client_workspace->netc->adapter_server_address;
                            uint16_t* server_port = &gim_ws->client_workspace->netc->adapter_server_port;
                            if (!no_adapter_selected && !offline_adapter_selected) {
                                ImGui::InputText("Address", server_addr_buf, ADAPTER_SERVER_ADDRESS_SIZE, ImGuiInputTextFlags_CallbackCharFilter, ConnectionTextFilters::FilterAddressLetters);
                                ImGui::InputScalar("Port", ImGuiDataType_U16, server_port);
                            }
                            if (false && !no_adapter_selected) {
                                static bool server_saved = true;
                                ImGui::Checkbox("Save Server", &server_saved);
                                if (server_saved) {
                                    static char server_short_name[32] = "";
                                    ImGui::InputTextWithHint("saved name", server_addr_buf, server_short_name, sizeof(server_short_name));
                                }
                            }
                            if (connection_setup_disabled) {
                                ImGui::EndDisabled();
                            }
                            switch (gim_ws->client_workspace->netc->adapter_state) {
                                case RSI_NONE: {
                                    // unreachable
                                    assert(0);
                                } break;
                                case RSI_IDLE: {
                                    bool connect_unavailable = no_adapter_selected;
                                    if (connect_unavailable) {
                                        ImGui::BeginDisabled();
                                    }
                                    if (ImGui::Button("Connect", ImVec2(-1, 0))) {
                                        network_connection_adapter_open(gim_ws->client_workspace->netc);
                                    }
                                    if (connect_unavailable) {
                                        ImGui::EndDisabled();
                                    }
                                } break;
                                case RSI_WAITING: {
                                    ImGui::BeginDisabled();
                                    ImGui::Button("Connecting..", ImVec2(-1, 0));
                                    ImGui::EndDisabled();
                                } break;
                                case RSI_DONE: {
                                    if (ImGui::Button("Disconnect", ImVec2(-1, 0))) {
                                        network_connection_adapter_close(gim_ws->client_workspace->netc);
                                    }
                                } break;
                                default: {
                                    // unreachable
                                    assert(0);
                                } break;
                            }
                            if (gim_ws->client_workspace->netc->adapter_error != NULL) {
                                ImGui::Text("Adapter error:");
                                ImGui::SameLine();
                                ImGui::TextColored(imgui_cols.str_danger, "%s", gim_ws->client_workspace->netc->adapter_error);
                            }

                            ImGui::Separator();
                            ImGui::Text("Status:");
                            ImGui::SameLine(); // idiomatically we would want the samelines to be before the texts, but this is way shorter overall for the giant state switch here
                            switch (gim_ws->client_workspace->netc->adapter_state) {
                                case RSI_NONE: {
                                    // unreachable
                                    assert(0);
                                } break;
                                case RSI_IDLE: {
                                    ImGui::TextColored(imgui_cols.str_danger, "(offline)");
                                } break;
                                case RSI_WAITING: {
                                    ImGui::TextColored(imgui_cols.str_warn, "(connecting)");
                                } break;
                                case RSI_DONE: {
                                    ImGui::TextColored(imgui_cols.str_success, "connected");
                                    ImGui::SameLine();
                                    switch (gim_ws->client_workspace->netc->connection_state) {
                                        case RSI_NONE: {
                                            // unreachable
                                            assert(0);
                                        } break;
                                        case RSI_IDLE: {
                                            // unreachable
                                            assert(0);
                                        } break;
                                        case RSI_WAITING: {
                                            ImGui::TextColored(imgui_cols.str_warn, "+ (securing)");
                                        } break;
                                        case RSI_DONE: {
                                            ImGui::TextColored(imgui_cols.str_success, "+ secured");
                                            ImGui::SameLine();
                                            switch (gim_ws->client_workspace->netc->authinfo_state) {
                                                case RSI_NONE: {
                                                    // unreachable
                                                    assert(0);
                                                } break;
                                                case RSI_IDLE: {
                                                    // unreachable
                                                    assert(0);
                                                } break;
                                                case RSI_WAITING: {
                                                    ImGui::TextColored(imgui_cols.str_warn, "+ (authinfo?)");
                                                } break;
                                                case RSI_DONE: {
                                                    ImGui::TextColored(imgui_cols.str_success, "+ authinfo"); //TODO might not actually want this, maybe remove this and the sameline after it
                                                    ImGui::SameLine();
                                                    switch (gim_ws->client_workspace->netc->authn_state) {
                                                        case RSI_NONE: {
                                                            // unreachable
                                                            assert(0);
                                                        } break;
                                                        case RSI_IDLE: {
                                                            ImGui::TextColored(imgui_cols.str_danger, "+ (authn)");
                                                        } break;
                                                        case RSI_WAITING: {
                                                            ImGui::TextColored(imgui_cols.str_warn, "+ (authn)");
                                                        } break;
                                                        case RSI_DONE: {
                                                            ImGui::TextColored(imgui_cols.str_success, "+ authn");
                                                        } break;
                                                        default: {
                                                            // unreachable
                                                            assert(0);
                                                        } break;
                                                    }
                                                } break;
                                                default: {
                                                    // unreachable
                                                    assert(0);
                                                } break;
                                            }
                                        } break;
                                        default: {
                                            // unreachable
                                            assert(0);
                                        } break;
                                    }
                                } break;
                                default: {
                                    // unreachable
                                    assert(0);
                                } break;
                            }
                            if (gim_ws->client_workspace->netc->connection_state == RSI_DONE) {
                                if (ImGui::Button("PING", ImVec2(-1, 0))) {
                                    network_connection_ping(gim_ws->client_workspace->netc);
                                }
                            }
                            if (gim_ws->client_workspace->netc->connection_state >= RSI_WAITING) {
                                blob* thumbprint = &gim_ws->client_workspace->netc->connection_cert_thumb;
                                bool thumb_null = blob_is_null(thumbprint);
                                char thumb_preview[64];
                                if (thumb_null) {
                                    sprintf(thumb_preview, "Thumbprint: <unavailable>");
                                } else {
                                    char* w_thumb_preview = thumb_preview;
                                    w_thumb_preview += sprintf(w_thumb_preview, "Thumbprint: ");
                                    for (size_t col_idx = 0; col_idx < 8; col_idx++) {
                                        if (col_idx > 0) {
                                            w_thumb_preview += sprintf(w_thumb_preview, ":");
                                        }
                                        w_thumb_preview += sprintf(w_thumb_preview, "%02x", ((uint8_t*)thumbprint->data)[col_idx]);
                                    }
                                }
                                if (thumb_null) {
                                    ImGui::BeginDisabled();
                                }
                                if (ImGui::CollapsingHeader(thumb_preview)) {
                                    ImGui::PushFont(fonts.imgui_mono);
                                    for (size_t row_idx = 0; row_idx < thumbprint->len / 8; row_idx++) {
                                        for (size_t col_idx = 0; col_idx < 8; col_idx++) {
                                            if (col_idx > 0) {
                                                ImGui::SameLine();
                                            }
                                            ImGui::Text("%02x", ((uint8_t*)thumbprint->data)[row_idx * 8 + col_idx]);
                                        }
                                    }
                                    size_t remaining_thumb_bytes = thumbprint->len % 8;
                                    if (remaining_thumb_bytes > 0) {
                                        size_t row_idx = thumbprint->len / 8 + 1;
                                        for (size_t col_idx = 0; col_idx < remaining_thumb_bytes; col_idx++) {
                                            if (col_idx > 0) {
                                                ImGui::SameLine();
                                            }
                                            ImGui::Text("%02x", ((uint8_t*)thumbprint->data)[row_idx * 8 + col_idx]);
                                        }
                                    }
                                    ImGui::PopFont();
                                }
                                if (thumb_null) {
                                    ImGui::EndDisabled();
                                }
                            }

                            if (gim_ws->client_workspace->netc->connection_verifail_reason != NULL) {
                                if (gim_ws->client_workspace->netc->connection_state == RSI_WAITING) {
                                    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, IM_COL32(226, 74, 117, 255));
                                    ImGui::BeginTable("sidebar_table", 1, ImGuiTableFlags_BordersV, ImVec2(-1, 0)); //TODO need to do -1 horizontal size, otherwise the right border doesnt show up somehow..
                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    {
                                        ImGui::Text("Server cert verification failed:");
                                        ImGui::PushFont(fonts.imgui_bold);
                                        ImGui::TextUnformatted(" ");
                                        ImGui::SameLine();
                                        ImGui::TextColored(imgui_cols.str_danger, "%s", gim_ws->client_workspace->netc->connection_verifail_reason);
                                        ImGui::PopFont();
                                        metagui_util_push_button_colors(METAGUI_UTIL_BUTTON_TYPE_DANGER);
                                        if (ImGui::Button("Accept Insecure Connection", ImVec2(-1.0f, 0.0f))) {
                                            network_connection_veriaccept(gim_ws->client_workspace->netc);
                                        }
                                        metagui_util_pop_button_colors();
                                    }
                                    ImGui::EndTable();
                                    ImGui::PopStyleColor();
                                } else if (gim_ws->client_workspace->netc->connection_state == RSI_DONE) {
                                    ImGui::Text("Server verification accepted:");
                                    //TODO right aligned clear button to make it go away, IF we event want to allow this at all..
                                    ImGui::TextUnformatted(" ");
                                    ImGui::SameLine();
                                    ImGui::TextColored(imgui_cols.str_success, "%s", gim_ws->client_workspace->netc->connection_verifail_reason);
                                }
                            }

                            if (gim_ws->client_workspace->netc->authinfo_state == RSI_DONE) {
                                ImGui::Separator();
                                ImGui::Text("Authentication");

                                bool disable_login = !gim_ws->client_workspace->netc->authinfo_allow_login;
                                bool disable_guest = !gim_ws->client_workspace->netc->authinfo_allow_guest && !disable_login;
                                bool disable_un = disable_login && !gim_ws->client_workspace->netc->authinfo_allow_guest;
                                bool disable_pw = disable_login && !gim_ws->client_workspace->netc->authinfo_want_guest_pw;

                                bool disable_authn_panel = gim_ws->client_workspace->netc->authn_state > RSI_IDLE;
                                if (disable_authn_panel) {
                                    ImGui::BeginDisabled();
                                }
                                if (disable_un) {
                                    ImGui::BeginDisabled();
                                }
                                ImGui::InputText("username", gim_ws->client_workspace->netc->authn_username, CONNECTION_AUTHN_USERNAME_SIZE, ImGuiInputTextFlags_CallbackCharFilter, ConnectionTextFilters::FilterSanitizedTextLetters);
                                if (disable_un) {
                                    ImGui::EndDisabled();
                                }
                                static bool hide_pw = true;
                                ImGuiInputTextFlags password_flags = ImGuiInputTextFlags_CallbackCharFilter;
                                if (hide_pw || disable_authn_panel) {
                                    password_flags |= ImGuiInputTextFlags_Password;
                                }
                                if (disable_pw) {
                                    ImGui::BeginDisabled();
                                }
                                ImGui::InputText("password", gim_ws->client_workspace->netc->authn_password, CONNECTION_AUTHN_PASSWORD_SIZE, password_flags, ConnectionTextFilters::FilterSanitizedTextLetters);
                                ImGui::SameLine();
                                if (ImGui::SmallButton(hide_pw ? "S" : "H")) {
                                    hide_pw = !hide_pw;
                                }
                                if (disable_pw) {
                                    ImGui::EndDisabled();
                                }
                                if (disable_authn_panel) {
                                    ImGui::EndDisabled();
                                }
                                switch (gim_ws->client_workspace->netc->authn_state) {
                                    case RSI_NONE: {
                                        // unreachable
                                        assert(0);
                                    } break;
                                    case RSI_IDLE: {
                                        float btn_width = ImGui::CalcItemWidth();
                                        if (disable_login) {
                                            ImGui::BeginDisabled();
                                        }
                                        if (ImGui::Button("Login", ImVec2(btn_width, 0.0f))) {
                                            network_connection_authn_login(gim_ws->client_workspace->netc, false);
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
                                            network_connection_authn_login(gim_ws->client_workspace->netc, true);
                                        }
                                        if (disable_guest) {
                                            ImGui::EndDisabled();
                                        }
                                    } break;
                                    case RSI_WAITING: {
                                        ImGui::BeginDisabled();
                                        ImGui::Button("Authenticating..", ImVec2(-1.0f, 0.0f));
                                        ImGui::EndDisabled();
                                    } break;
                                    case RSI_DONE: {
                                        if (ImGui::Button("Logout", ImVec2(-1.0f, 0.0f))) {
                                            network_connection_authn_logout(gim_ws->client_workspace->netc);
                                        }
                                    } break;
                                    default: {
                                        // unreachable
                                        assert(0);
                                    } break;
                                }
                                if (gim_ws->client_workspace->netc->authn_fail_reason != NULL) {
                                    //TODO small clear button somewhere to make it go away..
                                    ImGui::Text("AuthN fail:");
                                    ImGui::SameLine();
                                    ImGui::TextColored(ImVec4(0.85, 0.52, 0.22, 1), "%s", gim_ws->client_workspace->netc->authn_fail_reason);
                                }
                            }
                        }
                    }
                }
                ImGui::EndChild();

                ImGui::EndTabItem();
            }
            if (false && ImGui::BeginTabItem("> User")) {
                ImGui::EndTabItem();
            }
            if (false && ImGui::BeginTabItem("> Lobby")) {
                ImGui::EndTabItem();
            }
            if (false && ImGui::BeginTabItem("> Frontend")) {

                /////////

                ImVec2 content_size = ImGui::GetContentRegionAvail();
                float new_fbw = content_size.x;
                float new_fbh = content_size.y;
                bool skip_draw = false;
                if (new_fbw <= 0 || new_fbh <= 0) {
                    skip_draw = true; // dont draw if invisible, otherwise FBO errors
                }
                if (!skip_draw) {
                    if (new_fbw != fedd.fbw || new_fbh != fedd.fbh) {
                        // resized, change them
                        glBindFramebuffer(GL_FRAMEBUFFER, fedd.frontend_fbo);
                        glBindTexture(GL_TEXTURE_2D, fedd.frontend_tex);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, new_fbw, new_fbh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fedd.frontend_tex, 0);
                        glBindRenderbuffer(GL_RENDERBUFFER, fedd.frontend_rbo);
                        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fedd.fbw, fedd.fbh);
                        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fedd.frontend_rbo);
                        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                            mirabel_slogf(LOGS_ERR, "frontend framebuffer incomplete"); //TODO handle error
                        }
                        glBindFramebuffer(GL_FRAMEBUFFER, 0);
                        fedd.fbw = new_fbw;
                        fedd.fbh = new_fbh;
                    }
                    fedd.fex = 0;
                    fedd.fey = 0;
                    fedd.few = fedd.fbw;
                    fedd.feh = fedd.fbh;

                    glBindFramebuffer(GL_FRAMEBUFFER, fedd.frontend_fbo);
                    glViewport(0, 0, fedd.fbw, fedd.fbh);
                    metagui_empty_frontend();
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);

                    ImGui::Image((void*)(intptr_t)fedd.frontend_tex, ImVec2(fedd.fbw, fedd.fbh), ImVec2(0, 1), ImVec2(1, 0));
                }

                /////////

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void graphical_immediate_mode_interface::metagui_new_workspace()
{
    workspace* new_client_ws = client_add_workspace(appi.aclient);
    workspaces.push_back(gim_workspace(new_client_ws->id));
}
