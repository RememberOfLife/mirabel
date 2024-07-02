#pragma once

#include "mirabel/network_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct network_connection_s {
    network_adapter neta;
    //TODO progress + user authentication info about the connection
} network_connection;

#ifdef __cplusplus
}
#endif
