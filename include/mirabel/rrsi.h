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

typedef enum RSI_E {
    RSI_NONE = 0, // uninitialized
    RSI_IDLE, // all fine
    RSI_WAITING, // thing did, e.g. request sent to server, e.g. lock input fields and only show cancel button
    // response from server fills the respnse fields, //TODO and/or sets the state to idle if we can directly go again??
    RSI_DONE, // thing finished, response exists
} RSI; //TODO this is so big in caps it looks weird when among other fields like req_res_tracker, rename to e.g. run_state_tracker ?

typedef struct req_res_tracker_s {
    uint32_t association_id;
    RSI state;
} req_res_tracker;

#ifdef __cplusplus
}
#endif
