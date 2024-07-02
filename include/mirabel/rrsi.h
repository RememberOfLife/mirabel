#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

// Request Response State and Request Response Tracking Indicator
// general purpose task indicator, optionally with association
// e.g. server <-> client : processing / loading state indicator
// e.g. request response indicator
// NOTE: for some clarity it may be useful to layout these such that you place things you need to know before the request above the indicator/tracker, and its possible results, after the indicator

typedef enum RUNNING_STATE_INDICATOR_E {
    RUNNING_STATE_INDICATOR_NONE = 0, //TODO want this?
    RUNNING_STATE_INDICATOR_IDLE,
    RUNNING_STATE_INDICATOR_WAITING,
    RUNNING_STATE_INDICATOR_DONE,

    RSI_NONE = RUNNING_STATE_INDICATOR_NONE, // uninitialized
    RSI_IDLE = RUNNING_STATE_INDICATOR_IDLE, // all fine
    RSI_WAITING = RUNNING_STATE_INDICATOR_WAITING, // thing did, e.g. request sent to server
    // response from server fills the respnse fields, //TODO and/or sets the state to idle if we can directly go again??
    RSI_DONE = RUNNING_STATE_INDICATOR_DONE, // thing finished, response exists
} RUNNING_STATE_INDICATOR;

typedef struct req_res_tracker_s {
    uint32_t association_id;
    RUNNING_STATE_INDICATOR state;
} req_res_tracker;

#ifdef __cplusplus
}
#endif
