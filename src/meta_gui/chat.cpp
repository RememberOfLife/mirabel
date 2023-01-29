#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <SDL2/SDL.h>
#include "imgui.h"
#include "rosalia/noise.h"

#include "control/client.hpp"
#include "mirabel/event_queue.h"
#include "mirabel/event.h"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    // "Chat"
    // - chat of the current lobby
    // - how to make text colored but copyable?
    // - a button on every message to enable copyability?!

    struct chat_msg {
        uint32_t msg_id;
        uint32_t client_id;
        uint64_t timestamp;
        char* text;
    };

    std::vector<chat_msg> chat_log;
    bool chat_autoscroll = true;
    bool window_has_focus = false;
    bool focus_chat_input = false;
    uint64_t local_msg_id_ctr = 1;
    char* popup_str_id_buf = (char*)malloc(9);

    void chat_process_cmd(const char* msg); // forward decl because we dont want this public in metagui

    void chat_window(bool* p_open)
    {
        ImGui::SetNextWindowPos(ImVec2(50, 500), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
        // set transparencies
        ImVec4 window_title_bg = ImGui::GetStyleColorVec4(ImGuiCol_TitleBg);
        ImVec4 window_title_bga = ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive);
        ImVec4 window_title_bgc = ImGui::GetStyleColorVec4(ImGuiCol_TitleBgCollapsed);
        if (!window_has_focus) {
            ImGui::SetNextWindowBgAlpha(0.2f);
            window_title_bg.w = 0.2f;
            window_title_bga.w = 0.2f;
            window_title_bgc.w = 0.2f;
        }
        ImGui::PushStyleColor(ImGuiCol_TitleBg, window_title_bg);
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, window_title_bga);
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, window_title_bgc);
        // begin actual window
        if (focus_chat_input) {
            ImGui::SetNextWindowFocus();
        }
        bool window_contents_visible = ImGui::Begin("Chat", p_open, window_flags);
        window_has_focus = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        if (!window_contents_visible) {
            ImGui::End();
            ImGui::PopStyleColor(3);
            focus_chat_input = false;
            return;
        }

        // show chat settings
        ImGui::Checkbox("auto-scroll", &chat_autoscroll);

        ImGui::Separator();

        // reserve enough left-over height for 1 separator + 1 input text
        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
        // display chat history
        for (int i = 0; i < chat_log.size(); i++) {
            bool colored = false;
            if (chat_log[i].client_id == EVENT_CLIENT_NONE || chat_log[i].msg_id == UINT32_MAX) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180, 180, 180, 255)); // self/deleted messages are gray
                colored = true;
            } else if (chat_log[i].client_id == EVENT_CLIENT_SERVER) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(88, 255, 113, 255)); // server messages are green
                colored = true;
            }
            if (chat_log[i].client_id == EVENT_CLIENT_SERVER) {
                ImGui::TextWrapped("[%09lu] %s", chat_log[i].timestamp, chat_log[i].text);
            } else {
                ImGui::TextWrapped("[%09lu] %d: %s", chat_log[i].timestamp, chat_log[i].client_id, chat_log[i].text);
            }
            //TODO should only offer popup if we are either msg owner or have mod perms in the lobby
            sprintf(popup_str_id_buf, "%x", chat_log[i].msg_id); //HACK keep the id of the msg stable
            if (chat_log[i].msg_id < UINT32_MAX && ImGui::BeginPopupContextItem(popup_str_id_buf)) {
                if (ImGui::Selectable("Delete")) {
                    if (Control::main_client->network_send_queue) {
                        event_any es;
                        event_create_chat_del(&es, chat_log[i].msg_id);
                        event_queue_push(Control::main_client->network_send_queue, &es);
                    } else {
                        chat_msg_del(chat_log[i].msg_id);
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            if (colored) {
                ImGui::PopStyleColor();
            }
        }
        // auto scroll
        if (chat_autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        ImGui::Separator();

        // show input line
        static char input_buf[2048];
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::PushID("input");
        bool reclaim_focus = false;
        if (ImGui::InputTextWithHint("", "type here to chat", input_buf, 2048, ImGuiInputTextFlags_EnterReturnsTrue)) {
            // user has pressed enter, lose focus from input line and send message
            ImGui::SetWindowFocus(NULL);
            // trim whitespace back and front
            char* str_end = input_buf + strlen(input_buf);
            while (str_end > input_buf && str_end[-1] == ' ') {
                str_end--;
            }
            *str_end = 0;
            char* msg_start = input_buf;
            while (*msg_start == ' ') {
                msg_start++;
            }
            if (strlen(msg_start) > 0) {
                if (msg_start[0] == '/') {
                    // try as command and log to local
                    chat_msg_add(UINT32_MAX, 0, SDL_GetTicks64(), msg_start);
                    chat_process_cmd(msg_start);
                } else {
                    if (Control::main_client->network_send_queue) {
                        event_any es;
                        event_create_chat_msg(&es, 0, 0, 0, msg_start);
                        event_queue_push(Control::main_client->network_send_queue, &es);
                    } else {
                        chat_msg_add(local_msg_id_ctr++, 0, SDL_GetTicks64(), msg_start);
                    }
                }
            }
            input_buf[0] = '\0';
        }
        ImGui::PopID();
        // auto-focus text on window apparition and when refocussing the window by enter
        ImGui::SetItemDefaultFocus();
        if (focus_chat_input) {
            ImGui::SetKeyboardFocusHere(-1);
        }

        ImGui::End();
        ImGui::PopStyleColor(3);
        focus_chat_input = false;
    }

    void chat_msg_add(uint32_t msg_id, uint32_t client_id, uint64_t timestamp, const char* text)
    {
        chat_msg new_msg{msg_id, client_id, timestamp, (char*)malloc(strlen(text) + 1)};
        strcpy(new_msg.text, text);
        chat_log.push_back(new_msg);
    }

    void chat_msg_del(uint32_t msg_id)
    {
        for (int i = 0; i < chat_log.size(); i++) {
            if (chat_log[i].msg_id == msg_id) {
                chat_log[i].text = (char*)realloc(chat_log[i].text, 18);
                sprintf(chat_log[i].text, "<message deleted>");
                chat_log[i].msg_id = UINT32_MAX;
                return;
            }
        }
    }

    void chat_clear()
    {
        for (int i = 0; i < chat_log.size(); i++) {
            free(chat_log[i].text);
        }
        chat_log.clear();
    }

    //TODO coin and roll and others should really be handles by the server and not here
    void chat_process_cmd(const char* msg)
    {
        size_t msg_len = strlen(msg);
        if (msg_len < 1 || msg[0] != '/') {
            return;
        }
        const char* cmsg = msg + 1;
        if (strncmp(cmsg, "coin", 4) == 0) {
            const char* coin_result = squirrelnoise5(Control::main_client->dd.ms_tick, 0) % 2 == 0 ? "HEADS" : "TAILS";
            char coin_msg[100];
            sprintf(coin_msg, "coin flipped: %s\n", coin_result);
            if (Control::main_client->network_send_queue) {
                event_any es;
                event_create_chat_msg(&es, 0, 0, 0, coin_msg);
                event_queue_push(Control::main_client->network_send_queue, &es);
            } else {
                chat_msg_add(local_msg_id_ctr++, 0, SDL_GetTicks64(), coin_msg);
            }
            return;
        }
        if (strncmp(cmsg, "move", 4) == 0) {
            if (msg_len <= 6) {
                chat_msg_add(UINT32_MAX, 0, SDL_GetTicks64(), "~ usage: /move <move_str>");
                return;
            }
            cmsg = msg + 6;
            game* tg = Control::main_client->the_game;
            if (tg == NULL || tg->methods == NULL) {
                chat_msg_add(UINT32_MAX, 0, SDL_GetTicks64(), "~ illegal move attempt on null game");
                return;
            }
            uint8_t ptm_c;
            const player_id* ptm;
            game_players_to_move(tg, &ptm_c, &ptm);
            if (ptm_c == 0) {
                chat_msg_add(UINT32_MAX, 0, SDL_GetTicks64(), "~ illegal move attempt on finished game");
                return;
            }
            //TODO offer positional arg for player id instead of just using dd.view always
            player_id the_ptm = Control::main_client->dd.view;
            if (the_ptm == PLAYER_NONE) {
                chat_msg_add(UINT32_MAX, 0, SDL_GetTicks64(), "~ can not move as PLAYER_NONE");
                return;
            }
            if (the_ptm == PLAYER_RAND && game_ff(Control::main_client->the_game).random_moves == false) {
                chat_msg_add(UINT32_MAX, 0, SDL_GetTicks64(), "~ can not move as PLAYER_RAND on game without random moves");
                return;
            }
            move_data_sync* mc;
            error_code ec = game_get_move_data(tg, the_ptm, cmsg, &mc);
            if (ec != ERR_OK) {
                char err_msg[64];
                sprintf(err_msg, "~ illegal move attempt, error: (%d) %s\n", ec, game_ff(tg).error_strings ? game_get_last_error(tg) : "");
                chat_msg_add(UINT32_MAX, 0, SDL_GetTicks64(), err_msg);
                return;
            }
            event_any es;
            event_create_game_move(&es, the_ptm, *mc);
            event_queue_push(&Control::main_client->inbox, &es);
            return;
        }
        if (strncmp(cmsg, "roll", 4) == 0) {
            // roll n
            // roll ndm
            // to roll 1 die [1,n] or n dice [1,m] and print individual results as well as point sum
            if (msg_len <= 6) {
                chat_msg_add(UINT32_MAX, 0, SDL_GetTicks64(), "~ usage: /roll <n> or /roll <n>d<m>");
                return;
            }
            cmsg = msg + 6; //TODO why is this 6 and not 5???
            uint32_t die_count;
            uint32_t die_faces;
            int rc = sscanf(cmsg, "%ud%u", &die_count, &die_faces);
            if (rc != 2) {
                die_count = 1;
                rc = sscanf(cmsg, "%u", &die_faces);
                if (rc != 1) {
                    chat_msg_add(UINT32_MAX, 0, SDL_GetTicks64(), "~ can not parse die face count as u32");
                    return;
                }
            }
            if (die_count == 0 || die_faces == 0 || die_faces > 100) {
                chat_msg_add(UINT32_MAX, 0, SDL_GetTicks64(), "~ die count and faces must be > 0 and die count <= 100");
                return;
            }

            char* roll_msg = (char*)malloc(die_count * 12 + 20);
            uint32_t sum = 0;
            char* roll_cbuf = roll_msg;
            for (uint32_t i = 0; i < die_count; i++) {
                uint32_t part = squirrelnoise5(i, Control::main_client->dd.ms_tick) % die_faces + 1; //TODO //BUG //HACK this has modulo bias
                roll_cbuf += sprintf(roll_cbuf, "%u", part);
                sum += part;
                if (i < die_count - 1) {
                    roll_cbuf += sprintf(roll_cbuf, "+");
                }
            }
            roll_cbuf += sprintf(roll_cbuf, "=%u", sum);

            if (Control::main_client->network_send_queue) {
                event_any es;
                event_create_chat_msg(&es, 0, 0, 0, roll_msg);
                event_queue_push(Control::main_client->network_send_queue, &es);
            } else {
                chat_msg_add(local_msg_id_ctr++, 0, SDL_GetTicks64(), roll_msg);
            }
            free(roll_msg);
            return;
        }
        chat_msg_add(UINT32_MAX, 0, SDL_GetTicks64(), "~ unknown command\n");
        return;
    }

} // namespace MetaGui
