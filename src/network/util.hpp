#pragma once

#include <cstdint>

#include "SDL_net.h"
#include <openssl/ssl.h>

#include "network/protocol.hpp"

namespace Network {

    // a few things are prefixed with util_ to avoid confusion with similarly named openssl functions

    enum UTIL_SSL_CTX_TYPE {
        UTIL_SSL_CTX_TYPE_CLIENT,
        UTIL_SSL_CTX_TYPE_SERVER,
    };

    struct connection {
        PROTOCOL_CONNECTION_STATE state;
        TCPsocket socket = NULL;
        IPaddress peer_addr;
        uint32_t client_id;
        SSL* ssl_session;
        BIO* send_bio; // ssl writes into this, we read and send out over the socket
        BIO* recv_bio; // we dump socket recv data here and make ssl read from this
        connection(uint32_t client_id = 0); // construct empty and NULL
        void reset();
    };

    // client uses this with files both NULL
    // server uses this with appropriate file paths
    SSL_CTX* util_ssl_ctx_init(UTIL_SSL_CTX_TYPE type, const char* chain_file, const char* key_file);
    void util_ssl_ctx_free(SSL_CTX* ctx);

    bool util_ssl_session_init(SSL_CTX* ctx, connection* conn, UTIL_SSL_CTX_TYPE type);
    void util_ssl_session_free(connection* conn);

    // this verify callback always passes, allowing even sessions that do not verify to connect
    int verify_peer_cb(int ok, X509_STORE_CTX* cert_ctx);

    // returns into r_names and r_count, first the subject name, and following that any alternative names, if any
    // returns the total byte size over all names, not including the NUL terminator
    size_t util_cert_get_subjects(X509* cert, char*** r_names, int* r_count);
    void util_cert_free_subjects(char** r_names, int r_count); // helper function for freeing the mess

}
