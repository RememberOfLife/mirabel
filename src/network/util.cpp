#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#ifdef WIN32
#include <io.h>
#define R_OK 4
#define access _access
#else
#include <unistd.h>
#endif

#include "SDL_net.h"
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h> // for extensions and subj alt names

#include "control/user_manager.hpp"

#include "network/util.hpp"

namespace Network {

    connection::connection(uint32_t client_id):
        state(PROTOCOL_CONNECTION_STATE_NONE),
        socket(NULL),
        client_id(client_id),
        ssl_session(NULL),
        send_bio(NULL),
        recv_bio(NULL),
        fragment_size_target(0),
        fragment_size(0),
        fragment_buf(NULL),
        user_id(Control::USER_ID_NONE)
    {}

    void connection::reset()
    {
        state = PROTOCOL_CONNECTION_STATE_NONE;
        socket = NULL;
        client_id = EVENT_CLIENT_NONE;
        ssl_session = NULL;
        send_bio = NULL;
        recv_bio = NULL;
        if (fragment_buf != NULL) {
            free(fragment_buf);
        }
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
                assert(0);
                return NULL;
            } break;
        }

        if (ctx_method == NULL) {
            return NULL;
        }

        ctx = SSL_CTX_new(ctx_method);
        if (ctx == NULL) {
            return NULL;
        }

        r = SSL_CTX_set_ciphersuites(ctx, "TLS_AES_256_GCM_SHA384"); // strongest one according to $ openssl ciphers 'ALL:@STRENGTH'
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
        r = SSL_CTX_set_default_verify_paths(ctx);
        if (r != 1) {
            util_ssl_ctx_free(ctx);
            return NULL;
        }

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

        // if no cert files found, initialize with ad-hoc certificates
        //TODO in proper this should be a setting of the server config
        if (access(chain_file, R_OK) == 0 && access(key_file, R_OK) == 0) {
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
        } else {
            EVP_PKEY* cert_pkey = EVP_EC_gen("P-521"); // largest prime curve

            X509* cert_x509 = X509_new();
            X509_set_version(cert_x509, X509_VERSION_3);
            ASN1_INTEGER_set(X509_get_serialNumber(cert_x509), 1);
            X509_gmtime_adj(X509_get_notBefore(cert_x509), 0);
            X509_gmtime_adj(X509_get_notAfter(cert_x509), 365 * 24 * 60 * 60); // valid for one year
            X509_set_pubkey(cert_x509, cert_pkey);

            X509_NAME* cert_name = X509_get_subject_name(cert_x509);
            X509_NAME_add_entry_by_txt(cert_name, "CN", MBSTRING_ASC, (const unsigned char*)"mirabel-debug", -1, -1, 0);

            X509_set_issuer_name(cert_x509, cert_name);
            X509_sign(cert_x509, cert_pkey, EVP_sha256());

            SSL_CTX_use_certificate(ctx, cert_x509);
            SSL_CTX_use_PrivateKey(ctx, cert_pkey);

            // should be safe to free here because the ssl ctx holds copies
            X509_free(cert_x509);
            EVP_PKEY_free(cert_pkey);
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
        if (conn->ssl_session == NULL) {
            return false;
        }

        conn->send_bio = BIO_new(BIO_s_mem());
        if (conn->send_bio == NULL) {
            BIO_free(conn->send_bio);
            util_ssl_session_free(conn);
            return false;
        }
        // sadly 0 doesn't work anymore here, so we have to use -1 and respect SSL_ERROR_WANT_* all the time
        BIO_set_mem_eof_return(conn->send_bio, -1);

        conn->recv_bio = BIO_new(BIO_s_mem());
        if (conn->recv_bio == NULL) {
            BIO_free(conn->send_bio);
            BIO_free(conn->recv_bio);
            util_ssl_session_free(conn);
            return false;
        }
        BIO_set_mem_eof_return(conn->recv_bio, -1);

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
