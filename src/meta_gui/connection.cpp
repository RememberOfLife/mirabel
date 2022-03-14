#include <cstdint>
#include <cstring>

#include <SDL2/SDL.h>
#include "SDL_net.h"
#include "imgui.h"

#include "state_control/server.hpp"

#include "meta_gui/meta_gui.hpp"

namespace MetaGui {

    //TODO move all the ugly net code to the network adapter

    //REMOVE
    using StateControl::BP;
    static IPaddress server_ip;
    static TCPsocket my_sock = NULL;
    static SDLNet_SocketSet socketset = SDLNet_AllocSocketSet(1);
    static char* current_message = (char*)malloc(512);
    //REMOVE

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

        //REMOVE
        // check socket
        SDLNet_CheckSockets(socketset, 0);
        if (SDLNet_SocketReady(my_sock)) {
            uint8_t in_data[512]; // data buffer for incoming data
            if (SDLNet_TCP_Recv(my_sock, in_data, 512) <= 0) {
                // connection closed
                sprintf(current_message, "connection closed\n");
                SDLNet_TCP_DelSocket(socketset, my_sock);
                SDLNet_TCP_Close(my_sock);
                my_sock = NULL;
            } else {
                switch (in_data[0]) {
                    case BP::BP_PONG: {
                        sprintf(current_message, "received pong");
                    } break;
                    case BP::BP_OK: {
                        sprintf(current_message, "received OK");
                    } break;
                    case BP::BP_NOK: {
                        sprintf(current_message, "received NOK");
                    } break;
                    default: {
                        sprintf(current_message, "received unexpected packet type\n");
                    } break;
                }
            }
        }
        //REMOVE

        static bool connected = false;
        connected = (my_sock != NULL); //REMOVE
        // draw game start,stop,restart
        // locks all pre loading input elements if game is running, stop is only available if running
        if (connected) {
            if (ImGui::Button("Disconnect", ImVec2(-1.0f, 0.0f))) {
                //REMOVE
                char out_data[20] = "_goodbye!";
                out_data[0] = StateControl::BP_TEXT;
                SDLNet_TCP_Send(my_sock, out_data, 20);
                SDLNet_TCP_DelSocket(socketset, my_sock);
                SDLNet_TCP_Close(my_sock);
                my_sock = NULL;
                sprintf(current_message, "disconnected");
                //REMOVE
            }
        } else {
            if (ImGui::Button("Connect", ImVec2(-1.0f, 0.0f))) {
                //REMOVE
                my_sock = SDLNet_TCP_Open(&server_ip);
                if (my_sock != NULL) {
                    SDLNet_TCP_AddSocket(socketset, my_sock);
                    sprintf(current_message, "socket created\n");
                }
                //REMOVE
            }
        }
        if (connected) {
            ImGui::BeginDisabled();
        }
        static char server_address[64] = "127.0.0.1"; //TODO for debugging purposes this is loopback
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

        //REMOVE
        ImGui::Text("last socket event: %s", current_message);
        if (connected && ImGui::Button("PING")) {
            uint8_t out_data = StateControl::BP_PING;
            SDLNet_TCP_Send(my_sock, &out_data, 1);
            sprintf(current_message, "sent ping");
        }
        if (!connected && ImGui::Button("resolve address")) {
            SDLNet_ResolveHost(&server_ip, server_address, 61801);
            uint8_t address_dec[4];
            for (int i = 0; i < 4; i++) {
                address_dec[i] = (server_ip.host>>(8*(3-i)))&0xFF;
            }
            sprintf(current_message, "ip %d.%d.%d.%d:%d\n", address_dec[0], address_dec[1], address_dec[2], address_dec[3], server_ip.port);
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
