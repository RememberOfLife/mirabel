// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include <stdint.h>
#include <stdio.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL2/SDL_opengles2.h>
#else
#include <SDL2/SDL_opengl.h>
#endif
#include "nanovg_gl.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <dlfcn.h>

#include "mirabel/mirabel.h"
#include "mirabel/plugin.h"

uint32_t get_monotonic_time_ms()
{
    return SDL_GetTicks();
}

////////////////////////////////////////
/////// fake websocket /////////////////
////////////////////////////////////////

#if __EMSCRIPTEN__
#include <emscripten/websocket.h>
#else
//TODO
#endif
class asocket {
#if __EMSCRIPTEN__
    EMSCRIPTEN_WEBSOCKET_T socket;
#else
    int socket; //TODO
#endif

  public:

    int ctr = 0;
    bool available = false;
    bool connected = false;
    const char* err_line = NULL;
    asocket();
    ~asocket();
    void connect();
    void disconnect();
    void ping();
};
#if __EMSCRIPTEN__
EM_BOOL on_open(int eventType, const EmscriptenWebSocketOpenEvent* e, void* userData)
{
    printf("connected\n");
    return EM_TRUE;
}

EM_BOOL on_message(int eventType, const EmscriptenWebSocketMessageEvent* e, void* userData)
{
    if (e->isText) {
        printf("rx text: %s\n", e->data);
    } else {
        printf("rx bin len: %lu\n", e->numBytes);
    }
    return EM_TRUE;
}

EM_BOOL on_close(int eventType, const EmscriptenWebSocketCloseEvent* e, void* userData)
{
    asocket* mysock = (asocket*)userData;
    printf("connection closed: (%s) %s\n", e->wasClean ? "clean" : "unclean", e->reason);
    if (e->wasClean) {
        if (emscripten_websocket_delete(e->socket) != EMSCRIPTEN_RESULT_SUCCESS) {
            mysock->err_line = "failed to delete websocket";
            mysock->available = false;
            return EM_TRUE;
        }
        mysock->connected = false;
    }
    return EM_TRUE;
}

EM_BOOL on_error(int eventType, const EmscriptenWebSocketErrorEvent* e, void* userData)
{
    printf("socket error\n");
    return EM_TRUE;
}

asocket::asocket()
{
    available = emscripten_websocket_is_supported();
    if (!available) {
        err_line = "websockets not supported by platform";
        return;
    }
}

asocket::~asocket()
{
}

void asocket::connect()
{
    EmscriptenWebSocketCreateAttributes attr = {
        .url = "wss://echo.websocket.org/",
        .protocols = NULL,
        .createOnMainThread = false, // set to true only if threads other than this need to touch the socket
    };
    socket = emscripten_websocket_new(&attr);
    if (socket <= 0) {
        err_line = "failed to create new websocket";
        available = false;
        return;
    }
    connected = true;
    if (emscripten_websocket_set_onopen_callback(socket, this, on_open) != EMSCRIPTEN_RESULT_SUCCESS) {
        err_line = "failed to set callback onopen";
        available = false;
        return;
    }
    if (emscripten_websocket_set_onmessage_callback(socket, this, on_message) != EMSCRIPTEN_RESULT_SUCCESS) {
        err_line = "failed to set callback onmessage";
        available = false;
        return;
    }
    if (emscripten_websocket_set_onclose_callback(socket, this, on_close) != EMSCRIPTEN_RESULT_SUCCESS) {
        err_line = "failed to set callback onclose";
        available = false;
        return;
    }
    if (emscripten_websocket_set_onerror_callback(socket, this, on_error) != EMSCRIPTEN_RESULT_SUCCESS) {
        err_line = "failed to set callback onerror";
        available = false;
        return;
    }
}

void asocket::disconnect()
{
    if (emscripten_websocket_close(socket, 0, "closed") != EMSCRIPTEN_RESULT_SUCCESS) {
        err_line = "failed to close websocket";
        available = false;
        return;
    }
}

void asocket::ping()
{
    char msg[100];
    sprintf(msg, "ping/%i", ctr++);
    if (emscripten_websocket_send_utf8_text(socket, msg) != EMSCRIPTEN_RESULT_SUCCESS) {
        err_line = "failed to send message";
        available = false;
    }
}
#else
asocket::asocket()
{
    available = false;
    if (!available) {
        err_line = "websockets not supported by platform";
        return;
    }
}

asocket::~asocket()
{
}

void asocket::connect()
{
}

void asocket::disconnect()
{
}

void asocket::ping()
{
}
#endif
////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////

////////////////////////////////////////
/////////// dynamic linking ////////////
////////////////////////////////////////

plugin_draw_t* plugin_draw = NULL;

void setup_plugin(const char* path)
{
    printf("loading plugin\n");
    void* handle = dlopen(path, RTLD_LAZY);
    if (handle == NULL) {
        printf("rip no file\n");
        return;
    }
    plugin_draw = (plugin_draw_t*)dlsym(handle, "draw");
    if (plugin_draw != NULL) {
        printf("plugin loaded successfully\n");
    }
}

////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "examples/libs/emscripten/emscripten_mainloop_stub.h"
#endif

// Main code
int main(int argc, char** argv)
{
    printf("ARGS/BEGIN\n");
    for (int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }
    printf("ARGS/END\n");

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    if (argc > 1) {
        setup_plugin(argv[1]);
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("mirabel", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 500, 500, window_flags);
    if (window == nullptr) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(0); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& imgui_io = ImGui::GetIO();
    (void)imgui_io;
    imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    imgui_io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // IF using Docking Branch

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    NVGcontext* nvg_ctx;
#ifdef __EMSCRIPTEN__
    nvg_ctx = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#else
    nvg_ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#endif

    asocket mysock{};

    ImGuiViewport* imgui_viewport = ImGui::GetMainViewport();

    // Main loop
    bool done = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    imgui_io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                done = true;
                break;
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)) {
                done = true;
                break;
            }
            // imgui wants mouse: skip mouse events
            if (imgui_io.WantCaptureMouse && (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEWHEEL)) {
                continue;
            }
            // imgui wants keyboard: skip keyboard events
            if (imgui_io.WantCaptureKeyboard && (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)) {
                continue;
            }

            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                printf("resized to %i / %i\n", event.window.data1, event.window.data2);
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        float fbw = imgui_viewport->Size.x;
        float fbh = imgui_viewport->Size.y;

        ImGui::SetNextWindowSize(ImVec2{200, 200}, ImGuiCond_FirstUseEver);
        ImGui::Begin("info");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / imgui_io.Framerate, imgui_io.Framerate);
        ImGui::Separator();
        if (mysock.available) {
            if (!mysock.connected) {
                if (ImGui::Button("connect")) {
                    mysock.connect();
                }
            } else {
                if (ImGui::Button("disconnect")) {
                    mysock.disconnect();
                }
                if (ImGui::Button("ping")) {
                    mysock.ping();
                }
            }
        } else {
            ImGui::Text("<websockets unavailable>");
        }
        if (mysock.err_line != NULL) {
            ImGui::Text("error: %s", mysock.err_line);
        }
        ImGui::Separator();
        ImGui::Text("FB: %.0f / %.0f", fbw, fbh);
        ImGui::Text("v1");
        ImGui::End();

        glViewport(0, 0, (int)fbw, (int)fbh);
        // test nanovg
        nvgBeginFrame(nvg_ctx, fbw, fbh, 2);
        nvgSave(nvg_ctx);

        nvgBeginPath(nvg_ctx);
        nvgRect(nvg_ctx, -10, -10, fbw + 20, fbh + 20);
        nvgFillColor(nvg_ctx, nvgRGB(114, 140, 153));
        nvgFill(nvg_ctx);

        nvgRestore(nvg_ctx);
        nvgEndFrame(nvg_ctx);

        if (plugin_draw != NULL) {
            plugin_draw();
        }

        //glOrtho not available in GLES2

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
