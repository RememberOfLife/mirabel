#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>

#ifdef WIN32
#include <GL/glew.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "SDL_net.h"
#include "nanovg_gl.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "surena/game.h"

#include "control/client.hpp"
#include "control/event.hpp"
#include "control/event_queue.hpp"
#include "control/timeout_crash.hpp"
#include "frontends/empty_frontend.hpp"
#include "frontends/frontend_catalogue.hpp"
#include "games/game_catalogue.hpp"
#include "meta_gui/meta_gui.hpp"
#include "network/protocol.hpp"

#include "control/client.hpp"

namespace Control {

    Client* main_client = NULL;

    Client::Client()
    {
        main_client = this;

        // start watchdog so it can oversee explicit construction
        t_tc.start();
        tc_info = t_tc.register_timeout_item(&inbox, "guithread", 3000, 1000);

        // setup SDL
        if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER | SDL_INIT_VIDEO) != 0) {
            fprintf(stderr, "[FATAL] sdl init error: %s\n", SDL_GetError());
            exit(1);
        }
        // setup SDL_net
        if (SDLNet_Init() < 0) {
            SDL_Quit();
            fprintf(stderr, "[FATAL] sdl_net init error: %s\n", SDLNet_GetError());
            exit(1);
        }

        const char* glsl_version = "#version 130";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

        // create window with graphics context
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,16);
        SDL_GL_SetSwapInterval(1); // vsync with 1, possibly set after window creation
        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        sdl_window = SDL_CreateWindow("mirabel", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
        sdl_glcontext = SDL_GL_CreateContext(sdl_window);
        SDL_GL_MakeCurrent(sdl_window, sdl_glcontext);

#ifdef WIN32
        GLenum glew_err = glewInit();
        if (glew_err != GLEW_OK) {
            fprintf(stderr, "[FATAL] glew init error: %s\n", glewGetErrorString(glew_err));
            exit(1);
        }
#endif

        // setup imgui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        imgui_io = &ImGui::GetIO(); (void)imgui_io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // enable keyboard controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // enable gamepad controls

        // setup imgui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // setup platform/renderer backends
        ImGui_ImplSDL2_InitForOpenGL(sdl_window, sdl_glcontext);
        ImGui_ImplOpenGL3_Init(glsl_version);

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != NULL);

        // ImFontConfig font_config;
        // font_config.OversampleH = 4;
        // font_config.OversampleV = 4;
        // io.Fonts->AddFontFromFileTTF("../fonts/opensans/OpenSans-Regular.ttf", 20.0f, &font_config);

        glEnable(GL_MULTISAMPLE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);

        nanovg_ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
        int font_id0 = nvgCreateFont(nanovg_ctx, "df", "../res/fonts/opensans/OpenSans-Regular.ttf");
        if (font_id0 < 0) {
            printf("[ERROR] nvg failed to load font 0\n");
        }
        int font_id1 = nvgCreateFont(nanovg_ctx, "ff", "../res/fonts/opensans/OpenSans-ExtraBold.ttf");
        if (font_id1 < 0) {
            printf("[ERROR] nvg failed to load font 1\n");
        }

        // init default context
        frontend = new Frontends::EmptyFrontend();

        // init engine manager with a context queue to our inbox
        engine_mgr = new Engines::EngineManager(&inbox);
    }

    Client::~Client()
    {
        tc_info.pre_quit(2000);

        if (t_network) {
            t_network->close();
            delete t_network;
        }

        delete engine_mgr;

        delete frontend;
        free(the_game);

        //TODO delete loaded font images

        nvgDeleteGL3(nanovg_ctx);

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        SDL_GL_DeleteContext(sdl_glcontext);
        SDL_DestroyWindow(sdl_window);
        SDLNet_Quit();
        SDL_Quit();

        t_tc.unregister_timeout_item(tc_info.id);
        t_tc.inbox.push(f_event(EVENT_TYPE_EXIT));
        t_tc.join();
    }

    void Client::loop()
    {
        //TODO cleanup statics here
        bool show_hud = true;
        bool fullscreen = false;
        bool show_demo_window = false;
        ImGuiViewport* imgui_viewport = ImGui::GetMainViewport();
        float w_px = imgui_viewport->WorkSize.x;
        float h_px = imgui_viewport->WorkSize.y;
        float x_px = imgui_viewport->WorkPos.x;
        float y_px = imgui_viewport->WorkPos.y;

        bool ctrl_left = false;
        bool ctrl_right = false;

        int frame_work_ns = 0;
        const int frame_budget_ns = (1000 * 1000 * 1000)/60;
        bool quit = false;
        bool try_quit = false;
        while (!quit) {
            // sleep for the dead time that would be wasted by rendering
            // reduces input lag considerably by waiting up to the last possible moment to gather input events before action+rendering
            std::this_thread::sleep_for(std::chrono::nanoseconds(frame_budget_ns-frame_work_ns));
            // start measuring event + action and render time
            std::chrono::steady_clock::time_point frame_time_start = std::chrono::steady_clock::now();

            for (f_any_event e = inbox.pop(); e.type != Control::EVENT_TYPE_NULL; e = inbox.pop()) {
                // process event e
                // e.g. game updates, load other ctx or game, etc..
                switch (e.type) {
                    case EVENT_TYPE_EXIT: {
                        try_quit = true;
                        break;
                    } break;
                    case EVENT_TYPE_HEARTBEAT: {
                        tc_info.send_heartbeat();
                    } break;
                    case EVENT_TYPE_GAME_LOAD: {
                        auto ce = e.cast<f_event_game_load>();
                        // reset everything in case we can't find the game later on
                        frontend->set_game(NULL);
                        if (the_game) {
                            the_game->methods->destroy(the_game);
                            free(the_game);
                        }
                        the_game = NULL;
                        // find game in games catalogue by provided strings
                        //TODO should probably use an ordered map for the catalogue instead of a vector at this point
                        bool game_found = false;
                        uint32_t base_game_idx = 0;
                        uint32_t game_variant_idx = 0;
                        const char* base_game_name = ce.base_name;
                        const char* game_variant_name = ce.variant_name;
                        for (; base_game_idx < Games::game_catalogue.size(); base_game_idx++) {
                            if (strcmp(Games::game_catalogue[base_game_idx].name, base_game_name) == 0) {
                                game_found = true;
                                break;
                            }
                        }
                        if (!game_found) {
                            MetaGui::logf("#W guithread: failed to find base game: %s\n", base_game_name);
                            break;
                        }
                        game_found = false;
                        for (; game_variant_idx < Games::game_catalogue[base_game_idx].variants.size(); game_variant_idx++) {
                            if (strcmp(Games::game_catalogue[base_game_idx].variants[game_variant_idx]->name, game_variant_name) == 0) {
                                game_found = true;
                                break;
                            }
                        }
                        if (!game_found) {
                            MetaGui::logf("#W guithread: failed to find game variant: %s.%s\n", base_game_name, game_variant_name);
                            break;
                        }
                        // update metagui combobox selection
                        MetaGui::base_game_idx = base_game_idx;
                        MetaGui::game_variant_idx = game_variant_idx;
                        // actually load the game
                        the_game = Games::game_catalogue[base_game_idx].variants[game_variant_idx]->new_game(ce.options);
                        if (the_game->methods->export_options_str) {
                            // options have been set already by the catalogue through the game config, now export options for server
                            size_t options_len = the_game->sizer.options_str;
                            ce.options = (char*)malloc(options_len);
                            the_game->methods->export_options_str(the_game, &options_len, ce.options);
                        }
                        frontend->set_game(the_game);
                        // everything successful, pass to server
                        if (network_send_queue && ce.client_id == CLIENT_NONE) {
                            network_send_queue->push(ce);
                        }
                    } break;
                    case EVENT_TYPE_GAME_UNLOAD: {
                        frontend->set_game(NULL);
                        if (the_game) {
                            the_game->methods->destroy(the_game);
                            free(the_game);
                        }
                        the_game = NULL;
                        // everything successful, pass to server
                        if (network_send_queue && e.client_id == CLIENT_NONE) {
                            network_send_queue->push(e);
                        }
                    } break;
                    case EVENT_TYPE_GAME_STATE: {
                        auto ce = e.cast<f_event_game_state>();
                        if (!the_game) {
                            MetaGui::log("#W attempted state import on null game\n");
                            break;
                        }
                        the_game->methods->import_state(the_game, ce.state);
                        // everything successful, pass to server
                        if (network_send_queue && e.client_id == CLIENT_NONE) {
                            network_send_queue->push(e);
                        }
                    } break;
                    case EVENT_TYPE_GAME_MOVE: {
                        auto ce = e.cast<f_event_game_move>();
                        if (!the_game) {
                            MetaGui::log("#W attempted move on null game\n");
                            break;
                        }
                        player_id pbuf[253];
                        uint8_t pbuf_cnt = 253;
                        the_game->methods->players_to_move(the_game, &pbuf_cnt, pbuf);
                        the_game->methods->make_move(the_game, pbuf[0], ce.code); //FIXME ptm
                        the_game->methods->players_to_move(the_game, &pbuf_cnt, pbuf);
                        if (pbuf_cnt == 0) {
                            the_game->methods->get_results(the_game, &pbuf_cnt, pbuf);
                            if (pbuf_cnt == 0) {
                                pbuf[0] = PLAYER_NONE;
                            }
                            MetaGui::logf("game done: winner is player %d\n", pbuf[0]);
                        }
                        // everything successful, pass to server
                        if (network_send_queue && e.client_id == CLIENT_NONE) {
                            network_send_queue->push(e);
                        }
                    } break;
                    case EVENT_TYPE_FRONTEND_LOAD: {
                        auto ce = e.cast<f_event_frontend_load>();
                        delete frontend;
                        frontend = ce.frontend;
                        frontend->set_game(the_game);
                        MetaGui::running_few_idx = MetaGui::selected_few_idx;
                    } break;
                    case EVENT_TYPE_FRONTEND_UNLOAD: {
                        delete frontend;
                        frontend = new Frontends::EmptyFrontend();
                        MetaGui::running_few_idx = 0;
                    } break;
                    case EVENT_TYPE_LOBBY_CHAT_MSG: {
                        auto ce = e.cast<f_event_chat_msg>();
                        MetaGui::chat_msg_add(ce.msg_id, ce.author_client_id, ce.timestamp, ce.text);
                    } break;
                    case EVENT_TYPE_LOBBY_CHAT_DEL: {
                        auto ce = e.cast<f_event_chat_del>();
                        MetaGui::chat_msg_del(ce.msg_id);
                    } break;
                    /* skip EVENT_TYPE_NETWORK_ADAPTER_LOAD, t_network gets filled by the metagui connection window*/
                    case EVENT_TYPE_NETWORK_ADAPTER_SOCKET_CLOSED: // died while trying to connect
                    case EVENT_TYPE_NETWORK_ADAPTER_UNLOAD: { // metagui wants to disconnect
                        if (t_network == NULL) {
                            // need this to catch adapter recv runner socket close after proper unload
                            break;
                        }
                        network_send_queue = NULL;
                        t_network->close();
                        delete t_network;
                        t_network = NULL;
                        MetaGui::chat_clear();
                        MetaGui::connection_info_reset();
                    } break;
                    case EVENT_TYPE_NETWORK_ADAPTER_SOCKET_OPENED: {
                        // tcp opened
                        MetaGui::conn_info.adapter = MetaGui::RUNNING_STATE_DONE;
                        MetaGui::conn_info.connection = MetaGui::RUNNING_STATE_ONGOING;
                    } break;
                    case EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_ACCEPT: {
                        auto ce = e.cast<f_event_ssl_thumbprint>();
                        if (ce.thumbprint) {
                            free(MetaGui::conn_info.server_cert_thumbprint);
                            MetaGui::conn_info.server_cert_thumbprint = (uint8_t*)malloc(Network::SHA256_LEN);
                            memcpy(MetaGui::conn_info.server_cert_thumbprint, ce.thumbprint, Network::SHA256_LEN);
                        }
                        MetaGui::conn_info.connection = MetaGui::RUNNING_STATE_DONE;
                        // request auth info from server
                        t_network->send_queue.push(f_event_auth(EVENT_TYPE_USER_AUTHINFO, CLIENT_NONE, true, NULL, NULL));
                    } break;
                    case EVENT_TYPE_NETWORK_ADAPTER_CONNECTION_VERIFAIL: {
                        auto ce = e.cast<f_event_ssl_thumbprint>();
                        free(MetaGui::conn_info.server_cert_thumbprint);
                        MetaGui::conn_info.server_cert_thumbprint = (uint8_t*)malloc(Network::SHA256_LEN);
                        memcpy(MetaGui::conn_info.server_cert_thumbprint, ce.thumbprint, Network::SHA256_LEN);
                        free(MetaGui::conn_info.verifail_reason);
                        MetaGui::conn_info.verifail_reason = (char*)malloc(strlen((char*)ce.thumbprint) + 1);
                        strcpy(MetaGui::conn_info.verifail_reason, (char*)ce.thumbprint+Network::SHA256_LEN);
                    } break;
                    case EVENT_TYPE_NETWORK_ADAPTER_CLIENT_CONNECTED: {
                        // finalize connection by setting the sending queue, this transitions from initialization into usage
                        network_send_queue = &(t_network->send_queue);
                        // server sends its state as sync automatically, //TODO maybe we should reset it ourselves anyway?
                        MetaGui::chat_clear();
                    } break;
                    case EVENT_TYPE_NETWORK_ADAPTER_CLIENT_DISCONNECTED: {
                        //TODO we have been disconnected?
                        network_send_queue = NULL;
                        MetaGui::chat_clear();
                    } break;
                    case EVENT_TYPE_USER_AUTHINFO: {
                        auto ce = e.cast<f_event_auth>();
                        // we got the auth info from the server, set it up for display in the metagui conn info, also advance state
                        // if is_guest is true the server accepts guest logins, otherwise not
                        MetaGui::conn_info.auth_allow_guest = ce.is_guest;
                        // if username is NULL the server does NOT accept user logins
                        MetaGui::conn_info.auth_allow_login = (ce.username != NULL);
                        // if password is NULL the server does NOT require a server password for guests
                        MetaGui::conn_info.auth_want_guest_pw = (ce.password != NULL);
                        // if the server does not accept user AND guest logins wait for user to press guest login, enable pw input if wanted
                        MetaGui::conn_info.auth_info = true;
                    } break;
                    case EVENT_TYPE_USER_AUTHN: {
                        auto ce = e.cast<f_event_auth>();
                        // we received our authn credentials from the server
                        strcpy(MetaGui::conn_info.username, ce.username); // set username in authinfo, as received, may be assigned guest name
                        //TODO should probably store it somewhere else too
                        MetaGui::conn_info.authentication = MetaGui::RUNNING_STATE_DONE;
                        inbox.push(f_event(EVENT_TYPE_NETWORK_ADAPTER_CLIENT_CONNECTED));
                    } break;
                    case EVENT_TYPE_USER_AUTHFAIL: {
                        auto ce = e.cast<f_event_auth_fail>();
                        // server told us our authn failed / it signed us out after we requested logout
                        if (ce.reason) {
                            free(MetaGui::conn_info.authfail_reason);
                            MetaGui::conn_info.authfail_reason = ce.reason;
                            ce.reason = NULL;
                        }
                        MetaGui::conn_info.authentication = MetaGui::RUNNING_STATE_NONE;
                        inbox.push(f_event(EVENT_TYPE_NETWORK_ADAPTER_CLIENT_DISCONNECTED));
                    } break;
                    default: {
                        MetaGui::logf("#W guithread: received unexpected event, type: %d\n", e.type);
                    } break;
                }
            }
            
            // work through interface events: clicks, key presses, gui commands structs for updating interface elems
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                // pass event through imgui
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT) {
                    try_quit = true;
                    break;
                }
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(sdl_window)) {
                    try_quit = true;
                    break;
                }
                // if the confirm_exit_modal is up, pass CTRL+Q quit hotkey anyway
                if (MetaGui::show_confirm_exit_modal) {
                    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_q && (ctrl_left || ctrl_right)) {
                        try_quit = true;
                        break;
                    }
                }
                // imgui wants mouse: skip mouse events
                if (imgui_io->WantCaptureMouse && (
                    event.type == SDL_MOUSEMOTION ||
                    event.type == SDL_MOUSEBUTTONDOWN ||
                    event.type == SDL_MOUSEBUTTONUP ||
                    event.type == SDL_MOUSEWHEEL)) {
                    continue;
                }
                // imgui wants keyboard: skip keyboard events
                if (imgui_io->WantCaptureKeyboard && (
                    event.type == SDL_KEYDOWN ||
                    event.type == SDL_KEYUP)) {
                    continue;
                }
                // global window shortcuts
                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_F1) {
                        show_hud = !show_hud;
                    }
                    if (event.key.keysym.sym == SDLK_F3) {
                        MetaGui::show_stats_overlay = !MetaGui::show_stats_overlay;
                    }
                    if (event.key.keysym.sym == SDLK_F4) {
                        MetaGui::show_logs_window = !MetaGui::show_logs_window;
                    }
                    if (event.key.keysym.sym == SDLK_F5) {
                        show_demo_window = !show_demo_window;
                    }
                    if (event.key.keysym.sym == SDLK_F11) {
                        fullscreen = !fullscreen;
                        // borderless fullscreen
                        SDL_SetWindowFullscreen(sdl_window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                    }
                    if (event.key.keysym.sym == SDLK_c && (ctrl_left || ctrl_right)) {
                        MetaGui::show_connection_window = !MetaGui::show_connection_window;
                    }
                    if (event.key.keysym.sym == SDLK_g && (ctrl_left || ctrl_right)) {
                        MetaGui::show_game_config_window = !MetaGui::show_game_config_window;
                    }
                    if (event.key.keysym.sym == SDLK_f && (ctrl_left || ctrl_right)) {
                        MetaGui::show_frontend_config_window = !MetaGui::show_frontend_config_window;
                    }
                    if (event.key.keysym.sym == SDLK_e && (ctrl_left || ctrl_right)) {
                        MetaGui::show_engine_window = !MetaGui::show_engine_window;
                    }
                    if (event.key.keysym.sym == SDLK_t && (ctrl_left || ctrl_right)) {
                        MetaGui::show_chat_window = !MetaGui::show_chat_window;
                    }
                    if (MetaGui::show_chat_window && event.key.keysym.sym == SDLK_RETURN) {
                        MetaGui::focus_chat_input = true;
                    }
                    if (event.key.keysym.sym == SDLK_q && (ctrl_left || ctrl_right)) {
                        try_quit = true;
                        break;
                    }
                }
                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_LCTRL) {
                        ctrl_left = true;
                    } else if (event.key.keysym.sym == SDLK_RCTRL) {
                        ctrl_right = true;
                    }
                }
                if (event.type == SDL_KEYUP) {
                    if (event.key.keysym.sym == SDLK_LCTRL) {
                        ctrl_left = false;
                    } else if (event.key.keysym.sym == SDLK_RCTRL) {
                        ctrl_right = false;
                    }
                }
                frontend->process_event(event);
            }
            // user is trying to quit
            if (try_quit) {
                if (!the_game) {
                    quit = true;
                    break;
                } else {
                    // game running, display quit modal, or quit if already shown
                    if (MetaGui::show_confirm_exit_modal) {
                        quit = true;
                        break;
                    } else {
                        MetaGui::show_confirm_exit_modal = true;
                    }
                }
                try_quit = false;
            }

            // update engine containers by their queues
            engine_mgr->update();

            // start the dear imgui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            // show imgui windows
            //TODO since all of these only show when the bool is set, it doesnt really need to be an argument, they can just check themselves
            if (show_hud) {
                if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
                if (MetaGui::show_confirm_exit_modal) MetaGui::confirm_exit_modal(&MetaGui::show_confirm_exit_modal);
                if (MetaGui::show_main_menu_bar) MetaGui::main_menu_bar(&MetaGui::show_main_menu_bar);
                if (MetaGui::show_stats_overlay) MetaGui::stats_overlay(&MetaGui::show_stats_overlay);
                if (MetaGui::show_logs_window) MetaGui::logs_window(&MetaGui::show_logs_window);
                if (MetaGui::show_connection_window) MetaGui::connection_window(&MetaGui::show_connection_window);
                if (MetaGui::show_game_config_window) MetaGui::game_config_window(&MetaGui::show_game_config_window);
                if (MetaGui::show_frontend_config_window) MetaGui::frontend_config_window(&MetaGui::show_frontend_config_window);
                if (MetaGui::show_engine_window) MetaGui::engine_window(&MetaGui::show_engine_window);
                if (MetaGui::show_chat_window) MetaGui::chat_window(&MetaGui::show_chat_window);
            }


            //TODO put this in the sdl resize event, make a resize function on the context app
            w_px = imgui_viewport->WorkSize.x;
            h_px = imgui_viewport->WorkSize.y;
            x_px = imgui_viewport->WorkPos.x;
            y_px = imgui_viewport->WorkPos.y;
            frontend->w_px = w_px;
            frontend->h_px = h_px;
            frontend->x_px = x_px;
            frontend->y_px = y_px;
            // rendering
            glViewport(0, 0, (int)w_px, (int)h_px);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0.0, (GLdouble)w_px, (GLdouble)h_px, 0.0, -1, 1);

            frontend->update();
            nvgBeginFrame(nanovg_ctx, w_px, h_px, 2); //TODO use proper devicePixelRatio
            frontend->render(); // todo render should take the player perspective (information view) from which to draw
            nvgEndFrame(nanovg_ctx);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(sdl_window);

            std::chrono::steady_clock::time_point frame_time_stop = std::chrono::steady_clock::now();
            frame_work_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(frame_time_stop-frame_time_start).count();
        }
    }

}
