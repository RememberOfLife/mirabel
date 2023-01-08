#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <SDL2/SDL.h>
#include "rosalia/config.h"
#include "rosalia/jobs.h"
#include "rosalia/semver.h"
#include "surena/engine.h"
#include "surena/game.h"
#include "surena/move_history.h"

#include "mirabel/event_queue.h"
#include "mirabel/event.h"

#ifdef __cplusplus
extern "C" {
#endif

//NOTE: updates to {config, event_queue, event, frontend, imgui_c_thin, job_queue, log, sound} will incur a version increase here
static const uint64_t MIRABEL_FRONTEND_API_VERSION = 16;

//TODO this mirrors a lot of the info that will be stored in the client lobby
typedef struct /*grand_unified_*/ frontend_display_data_s {
    //TODO move outbox to a frontend only receive queue, so errors dont propagate as much
    event_queue* outbox; // the frontend can place all outgoing interactions of the user here
    // the frontend is also able to start games by issuing the approproiate event here //TODO make sure meta gui combo boxes are adjusted accordinglys

    void* cfg_lock;
    cj_ovac* cfg; //TODO some kind of read/write limitations so a frontend will only ever write its own config?

    job_queue jobs;

    // all readonly:

    uint64_t ms_tick; // updated at the beginning of the frame before supplying events/inputs, and again before update

    // privacy view information
    player_id view;
    // framebuffer height and width
    float fbw;
    float fbh;
    // offset for drawing the frontend, origin is top left of the window
    float x;
    float y;
    float w;
    float h;

    // lobby info (player names, etc?)

    uint32_t time_ctl_count;
    time_control* time_ctl;
    uint8_t time_ctl_player_count;
    time_control_player* time_ctl_player;

    move_history* history;

    //TODO engine info
} frontend_display_data;

typedef struct frontend_feature_flags_s {
    bool options : 1;
} frontend_feature_flags;

typedef struct frontend_s frontend; // forward declare the frontend for the frontend methods

typedef struct frontend_methods_s {

    // minimum length of 1 character, with allowed character set:
    // "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" additionally '-' and '_' but not at the start or end
    const char* frontend_name;
    const semver version;
    const frontend_feature_flags features; // these will never change depending on options

    // the frontend method specific internal method struct, NULL if not available
    // use the frontend_name to make sure you know what this will be
    const void* internal_methods; //TODO any actual use for this?

    //TODO should load opts_create for frontends and game/engine wraps get a pointer ref, or return its own pointer?
    // FEATURE: options
    // load opts
    error_code (*opts_create)(void** options_struct);
    error_code (*opts_display)(void* options_struct);
    error_code (*opts_destroy)(void* options_struct);

    // returns the error string complementing the most recent occured error
    // returns NULL if there is error string available for this error
    // the string is still owned by the frontend method backend, do not free it
    const char* (*get_last_error)(frontend* self);

    // use the given options to create the frontend data
    // if options_struct is NULL then the default options are loaded
    // construct and initialize a new frontend specific data object into self
    // if any options exist, the defaults are used
    // if the load options feature is not supported then options_struct is NULL
    // a frontend can only be created once, must be matched with a call to destroy
    // !! even if create fails, the frontend has to be destroyed before releasing or creating again
    error_code (*create)(frontend* self, frontend_display_data* display_data, void* options_struct); //TODO make display data a pointer to const?

    // deconstruct and release any (complex) frontend specific data, if it has been created already
    // same for options specific data, if it exists
    error_code (*destroy)(frontend* self);

    //TODO make feature?
    error_code (*runtime_opts_display)(frontend* self);

    // calling chain is: all mirabel events, all sdl events, (opts + runtime_opts), update, render, (loop)

    // in general, passthrough all EVENT_TYPE_HEARTBEAT events untouched
    // a heartbeat id=0 can be used to assert that the frontend is up to date with all the queued input events (if it is buffering)  //TODO fine?
    // games will be passed to the frontend using EVENT_TYPE_GAME_LOAD_METHODS containing a pointer to the methods to be used
    // the frontend has to destroy the event copy it is passed
    error_code (*process_event)(frontend* self, event_any event);

    // the frontend is passed a shallow copy of the sdl event, it need not be released/freed in any way
    error_code (*process_input)(frontend* self, SDL_Event event);

    error_code (*update)(frontend* self);

    //TODO re-add background rendering, needs different xywh than normal rendering!

    //TODO render can take a bool wether the frontend is forced to redraw
    error_code (*render)(frontend* self);

    error_code (*is_game_compatible)(const game_methods* methods);

} frontend_methods;

struct frontend_s {
    const frontend_methods* methods;
    void* data1; // owned by the frontend method
    void* data2; // owned by the frontend method
};

#ifdef __cplusplus
}
#endif
