#include <cstring>

#include "control/client.hpp"
#include "control/server.hpp"
#include "control/timeout_crash.hpp"

#include "control/event.hpp"
void sandbox()
{
    using namespace Control;

    // plain event

    f_any_event e = f_event_test_plain(EVENT_TYPE_TEST_PLAIN, 17);
    auto ep = event_cast<f_event_test_plain>(e);
    ep.client_id = 1;
    ep.lobby_id = 2;
    
    size_t ep_size = event_size(&ep);
    void* buf = malloc(ep_size);
    event_serialize(&ep, buf);
    printf("serialized size: %zu\n", ep_size);
    for (int i = 0; i < ep_size; i++) {
        if (i < sizeof(f_event)+sizeof(size_t) && i % 4 == 0 && i > 0) {
            printf(" ");
        }
        if (i == sizeof(size_t)) {
            printf("| ");
        }
        if (i == sizeof(f_event)+sizeof(size_t)) {
            printf(" + ");
        }
        printf("%02x", ((uint8_t*)buf)[i]);
    }
    printf("\n");

    // string event

    event_deserialize(&e, buf, (char*)buf + ep_size);
    auto ep2 = e.cast<f_event_test_plain>();
    printf("u32 W: %d\n    R: %d\n", ep.myown, ep2.myown);


    f_event_test_string es = f_event_test_string();
    es.type = Control::EVENT_TYPE_TEST_STRING;
    es.client_id = 1;
    es.lobby_id = 2;
    es.myother = 42;
    const char* thestr = "";//"0123456789 !!!!!!!!!! 0123456789";
    if (thestr) {
        es.mystring = (char*)malloc(strlen(thestr)+1);
        strcpy(es.mystring, thestr);
    } else {
        es.mystring = NULL;
    }
    const char* thestr2 = NULL;//"shortstr";
    if (thestr2) {
        es.mystring2 = (char*)malloc(strlen(thestr2)+1);
        strcpy(es.mystring2, thestr2);
    } else {
        es.mystring2 = NULL;
    }

    size_t es_size = event_size(&es);
    free(buf);
    buf = malloc(es_size);
    event_serialize(&es, buf);
    printf("serialized size: %zu\n", es_size);
    for (int i = 0; i < es_size; i++) {
        if (i < sizeof(f_event)+sizeof(size_t) && i % 4 == 0 && i > 0) {
            printf(" ");
        }
        if (i == sizeof(size_t)) {
            printf("| ");
        }
        if (i == sizeof(f_event)+sizeof(size_t)) {
            printf(" + ");
        }
        printf("%02x", ((uint8_t*)buf)[i]);
    }
    printf("\n");

    event_deserialize(&e, buf, (char*)buf + es_size);
    auto es2 = e.cast<f_event_test_string>();
    printf("u32 W: %d\n    R: %d\n", es.myother, es2.myother);
    printf("str W: \"%s\"\n    R: \"%s\"\n", es.mystring, es2.mystring);
    printf("str W: \"%s\"\n    R: \"%s\"\n", es.mystring2, es2.mystring2);

    exit(0);
}

int main(int argc, char **argv)
{
    //TODO use proper argparsing and offer some more sensible options, e.g. dont use watchdog, etc..
    if (argc == 2 && strcmp(argv[1], "server") == 0) {
        // start server
        Control::Server* the_server = new Control::Server();
        the_server->loop();
        delete the_server;
        return 0;
    }
    //TODO if launched in client mode, should still start the offline server, which is then paused if later connecting to another server
    Control::main_client = new Control::Client(); // instantiate the main client
    Control::main_client->loop(); // opengl + imgui has to run on the main thread
    delete Control::main_client; // destroy the client so everything cleans up nicely
    return 0;
}
