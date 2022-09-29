#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "SDL_net.h"
#include <openssl/ssl.h>
#include <openssl/x509v3.h> // for extensions and subj alt names

#include "network/util.hpp"

namespace Network {

    connection::connection(uint32_t client_id):
        state(PROTOCOL_CONNECTION_STATE_NONE),
        socket(NULL),
        client_id(client_id),
        ssl_session(NULL),
        send_bio(NULL),
        recv_bio(NULL)
    {}

    void connection::reset()
    {
        state = PROTOCOL_CONNECTION_STATE_NONE;
        socket = NULL;
        client_id = EVENT_CLIENT_NONE;
        ssl_session = NULL;
        send_bio = NULL;
        recv_bio = NULL;
    }

    SSL_CTX* util_ssl_ctx_init(UTIL_SSL_CTX_TYPE type, const char* chain_file, const char* key_file)
    {
        SSL_CTX* ctx = NULL;

        int r = 0;

        const SSL_METHOD* ctx_method;

        switch (type) {
            case UTIL_SSL_CTX_TYPE_CLIENT: {
                ctx_method = TLS_client_method();
            } break;
            case UTIL_SSL_CTX_TYPE_SERVER: {
                ctx_method = TLS_server_method();
            } break;
            default: {
                return NULL;
            } break;
        }

        ctx = SSL_CTX_new(ctx_method);
        if (!ctx) {
            return NULL;
        }

        r = SSL_CTX_set_ciphersuites(ctx, "TLS_AES_256_GCM_SHA384"); // strongest one according to $ openssl -v 'ALL:@STRENGTH'
        if (r != 1) {
            util_ssl_ctx_free(ctx);
            return NULL;
        }

        // enable partial chain trust to use letsencrypt certs
        X509_VERIFY_PARAM* vpm = SSL_CTX_get0_param(ctx); // do not free vpm, ssl owns this
        X509_VERIFY_PARAM_set_flags(vpm, X509_V_FLAG_PARTIAL_CHAIN);
        SSL_CTX_set1_param(ctx, vpm);

        SSL_CTX_set_verify_depth(ctx, 8); // allow longer verification chains

        // load ca cert stores as default root of trust
        SSL_CTX_set_default_verify_paths(ctx);

        // when using a NULL verify callback, everything just goes bad if the cert isnt verified correctly, i.e. manage error in own callback
        switch (type) {
            case UTIL_SSL_CTX_TYPE_CLIENT: {
                // client requests cert from server
                SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_peer_cb);
                // client doesnt load any certs
                return ctx;
            } break;
            case UTIL_SSL_CTX_TYPE_SERVER: {
                // server should not request cert from client
                SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, verify_peer_cb);
            } break;
        }

        // certificate fullchain file, also contains the public key
        r = SSL_CTX_use_certificate_chain_file(ctx, chain_file);
        if (r != 1) {
            util_ssl_ctx_free(ctx);
            return NULL;
        }

        // load private key
        r = SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM);
        if (r != 1) {
            util_ssl_ctx_free(ctx);
            return NULL;
        }

        // check if the private key is valid
        r = SSL_CTX_check_private_key(ctx);
        if (r != 1) {
            util_ssl_ctx_free(ctx);
            return NULL;
        }

        return ctx;
    }

    void util_ssl_ctx_free(SSL_CTX* ctx)
    {
        SSL_CTX_free(ctx);
    }

    bool util_ssl_session_init(SSL_CTX* ctx, connection* conn, UTIL_SSL_CTX_TYPE type)
    {
        conn->ssl_session = SSL_new(ctx);
        if (!conn->ssl_session) {
            return false;
        }
        conn->send_bio = BIO_new(BIO_s_mem());
        if (!conn->send_bio) {
            BIO_free(conn->send_bio);
            util_ssl_session_free(conn);
            return false;
        }
        // use 0 instead of -1 to get proper behaviour on empty bios, see: https://www.openssl.org/docs/crypto/BIO_s_mem.html
        BIO_set_mem_eof_return(conn->send_bio, 0);
        conn->recv_bio = BIO_new(BIO_s_mem());
        if (!conn->recv_bio) {
            BIO_free(conn->send_bio);
            BIO_free(conn->recv_bio);
            util_ssl_session_free(conn);
            return false;
        }
        BIO_set_mem_eof_return(conn->recv_bio, 0);
        SSL_set_bio(conn->ssl_session, conn->recv_bio, conn->send_bio);
        switch (type) {
            case UTIL_SSL_CTX_TYPE_CLIENT: {
                SSL_set_connect_state(conn->ssl_session);
            } break;
            case UTIL_SSL_CTX_TYPE_SERVER: {
                SSL_set_accept_state(conn->ssl_session);
            } break;
            default: {
                util_ssl_session_free(conn);
                return false;
            } break;
        }
        return true;
    }

    void util_ssl_session_free(connection* conn)
    {
        SSL_free(conn->ssl_session);
        conn->ssl_session = NULL;
        conn->send_bio = NULL;
        conn->recv_bio = NULL;
    }

    int verify_peer_cb(int ok, X509_STORE_CTX* cert_ctx)
    {
        // "ok" will never fail on its own if the diy check passes
        // if the user wants to keep a verification failing connection anyway, we can show a warning, and choose to ignore the fail here
        return 1;
    }

    size_t util_cert_get_subjects(X509* cert, char*** r_names, int* r_count)
    {
        char** ret_names = NULL;
        int ret_count = 0;
        size_t ret_totalsize = 0;

        size_t tmp_str_size;

        // adapted from https://github.com/iSECPartners/ssl-conservatory
        char* common_name_str = NULL;
        do {
            // find the position of the CN field in the subject field of the certificate
            int common_name_loc = X509_NAME_get_index_by_NID(X509_get_subject_name((X509*)cert), NID_commonName, -1);
            if (common_name_loc < 0) {
                break;
            }
            // extract the CN field
            X509_NAME_ENTRY* common_name_entry = X509_NAME_get_entry(X509_get_subject_name((X509*)cert), common_name_loc);
            if (common_name_entry == NULL) {
                break;
            }
            // convert the CN field to a c string
            ASN1_STRING* common_name_asn1 = X509_NAME_ENTRY_get_data(common_name_entry);
            if (common_name_asn1 == NULL) {
                break;
            }
            common_name_str = (char*)ASN1_STRING_get0_data(common_name_asn1); // ssl owned, do not free
            // make sure there isn't an embedded NUL character in the CN
            if (ASN1_STRING_length(common_name_asn1) != strlen(common_name_str)) {
                common_name_str = NULL; // malformed cert
            }
            // common name is fine
            ret_count++;
        } while (0);

        int san_names_count = 0;
        STACK_OF(GENERAL_NAME)* san_names = (STACK_OF(GENERAL_NAME)*)X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
        if (san_names != NULL) {
            san_names_count = sk_GENERAL_NAME_num(san_names);
            ret_count += san_names_count;
        }
        ret_names = (char**)malloc(sizeof(char*) * ret_count);
        ret_count = 0;
        if (common_name_str != NULL) {
            // copy common name into result
            tmp_str_size = strlen(common_name_str);
            ret_totalsize += tmp_str_size;
            ret_names[ret_count] = (char*)malloc(tmp_str_size + 1);
            strcpy(ret_names[ret_count++], common_name_str);
        }
        for (int i = 0; i < san_names_count; i++) {
            const GENERAL_NAME* curr_name = sk_GENERAL_NAME_value(san_names, i);
            if (curr_name->type == GEN_DNS) {
                // is a dns name, use it
                char* dns_name = (char*)ASN1_STRING_get0_data(curr_name->d.dNSName); // ssl owned, do not free
                if (ASN1_STRING_length(curr_name->d.dNSName) == strlen(dns_name)) {
                    // dns_name is a valid an, copy it into result
                    tmp_str_size = strlen(dns_name);
                    ret_totalsize += tmp_str_size;
                    ret_names[ret_count] = (char*)malloc(tmp_str_size + 1);
                    strcpy(ret_names[ret_count++], dns_name);
                }
            }
        }
        sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);

        *r_names = ret_names;
        *r_count = ret_count;
        return ret_totalsize;
    }

    void util_cert_free_subjects(char** r_names, int r_count)
    {
        for (int i = 0; i < r_count; i++) {
            free(r_names[i]);
        }
        free(r_names);
    }

} // namespace Network
