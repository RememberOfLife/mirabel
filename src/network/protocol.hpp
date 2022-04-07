#pragma once

#include <cstdint>

namespace Network {

    //will hold info for the state machine driven by EVENT_TYPE_NETWORK_PROTOCOL_*

    enum PROTOCOL_CONNECTION_STATE {
        PROTOCOL_CONNECTION_STATE_PRECLOSE, // close has been negotiated, expect the connection to actually close
        PROTOCOL_CONNECTION_STATE_NONE, // insecure, just tcp connected
        PROTOCOL_CONNECTION_STATE_INITIALIZING, // doing ssl handshake
        PROTOCOL_CONNECTION_STATE_WARNHELD, // ssl handshake done, verify failed
        PROTOCOL_CONNECTION_STATE_ACCEPTED, // ssl handshake done, either verify passed or user accepted warning
    };

}
