#include <cstdint>
#include <cstdio>

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#ifndef __EMSCRIPTEN__
#include "SDL_net.h"
#endif
#include "nanovg.h"
#ifndef __EMSCRIPTEN__
#include <openssl/ssl.h>
#endif
#include "imgui.h"

#include "mirabel/application.h"
#include "mirabel/game.h"
#include "generated/git_commit_hash.h"

#include "interface/gim/window.hpp"

bool init_sdl_version = false;
SDL_version sdl_version_compiled;
SDL_version sdl_version_linked;

// bool init_sdl_net_version = false;
// SDLNet_version sdl_net_version_compiled;
// SDLNet_version sdl_net_version_linked;

void graphical_immediate_mode_interface::metagui_about_info()
{
    //TODO make this a popup or fullscreen window, if we try to exit it is auto replaced by the fullscreen exit popup

    if (!show_about_info) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(600, 380), ImGuiCond_FirstUseEver);
    bool window_contents_visible = ImGui::Begin("About", &show_about_info);
    if (!window_contents_visible) {
        ImGui::End();
        return;
    }

    ImGui::Text("MIRABEL"); // make bigger
    ImGui::Text("version: %u.%u.%u", app_version.major, app_version.minor, app_version.patch);
    ImGui::Text(
        "git commit hash: %s%s",
        GIT_COMMIT_HASH == NULL ? "<no commit info available>" : GIT_COMMIT_HASH,
        GIT_COMMIT_HASH != NULL && GIT_COMMIT_DIRTY ? " (dirty)" : ""
    );
    ImGui::Text(
        "mirabel api versions:\n"
        "\tgame(%lu)",
        MIRABEL_GAME_API_VERSION
    );
    ImGui::Separator();
    {
        // GLEW
        ImGui::Text("GLEW version: %s", glewGetString(GLEW_VERSION));
    }
    {
        // SDL
        if (init_sdl_version == false) {
            SDL_VERSION(&sdl_version_compiled);
            SDL_GetVersion(&sdl_version_linked);
            init_sdl_version = true;
        }
        ImGui::Text("SDL version: %u.%u.%u", sdl_version_linked.major, sdl_version_linked.minor, sdl_version_linked.patch);
        if (memcmp(&sdl_version_linked, &sdl_version_compiled, sizeof(SDL_version)) != 0) {
            ImGui::SameLine();
            ImGui::Text("(compiled %u.%u.%u)", sdl_version_compiled.major, sdl_version_compiled.minor, sdl_version_compiled.patch);
        }
    }
    {
        // OpenGL
        GLint gl_vmaj;
        GLint gl_vmin;
        glGetIntegerv(GL_MAJOR_VERSION, &gl_vmaj);
        glGetIntegerv(GL_MINOR_VERSION, &gl_vmin);
        ImGui::Text("OpenGL version: %i.%i", gl_vmaj, gl_vmin); // only works for 3 upwards //TODO require strictly >=3 in cmakelists
    }
    {
        /*
        // SDL_net
        if (init_sdl_net_version == false) {
            SDL_NET_VERSION(&sdl_net_version_compiled);
            const SDLNet_version* lv = SDLNet_Linked_Version();
            memcpy(&sdl_net_version_linked, lv, sizeof(SDLNet_version));
            init_sdl_net_version = true;
        }
        ImGui::Text("SDL_net version: %u.%u.%u", sdl_net_version_linked.major, sdl_net_version_linked.minor, sdl_net_version_linked.patch);
        if (memcmp(&sdl_net_version_linked, &sdl_net_version_compiled, sizeof(SDLNet_version)) != 0) {
            ImGui::SameLine();
            ImGui::Text("(compiled %u.%u.%u)", sdl_net_version_compiled.major, sdl_net_version_compiled.minor, sdl_net_version_compiled.patch);
        }
        */
    }
    {
#ifndef __EMSCRIPTEN__
        // OpenSSL
        ImGui::Text("OpenSSL version: %s", OPENSSL_FULL_VERSION_STR);
#endif
    }
    //TODO nanovg possible?
    {
        // dear imgui
        ImGui::Text("dear imgui version: %s", IMGUI_VERSION);
    }
    {
        // rosalia
        // ImGui::Text("rosalia version: ?"); //TODO theres a lot of apis there, show all or just the commit hash?
    }
    ImGui::Separator();
    ImGui::TextUnformatted("repository: https://github.com/RememberOfLife/mirabel");
    /*
    ImGui::SameLine();
    if (ImGui::SmallButton("open link")) {
#if ISMSVC
        ShellExecuteA(NULL, "open", "https://github.com/RememberOfLife/mirabel", NULL, NULL, SW_SHOWNORMAL)
#else
        signal(SIGCHLD, SIG_IGN); // so we dont have to clean up the child
        pid_t child = fork();
        if (child == -1) {
            //TODO fail case
        } else if (child == 0) {
            execlp("xdg-open", "xdg-open", "https://github.com/RememberOfLife/mirabel", NULL);
        }
#endif
    }
    */

    ImGui::End();
}
