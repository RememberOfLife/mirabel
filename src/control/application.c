#include "mirabel/application.h"
#include "mirabel/method_registry.h"
#include "mirabel/log.h"

#ifdef __cplusplus
extern "C" {
#endif

/////
// public

app appi;

void app_create()
{
    appi = (app){
        .registry = method_registry_create(),
        .interface = NULL,
    };
}

void app_destroy()
{
    method_registry_destroy(appi.registry);
}

void app_args(int argc, char** argv)
{
    mirabel_slogf(LOGS_INFO, "ARGS (%i)", argc);
    for (int argi = 0; argi < argc; argi++) {
        mirabel_slogf(LOGS_NORM, "[%i]: %s", argi, argv[argi]);
    }

    // for (int i = 0; i < argc; i++) {
    //     if (strcmp(argv[i], "crl") == 0) {
    //         instance->ui = new CommandReadLine(); //TODO for now unsupported in the web, should be made unavailable via the registration manager
    //         break;
    //     } else if (strcmp(argv[i], "gim") == 0) {
    //         instance->ui = new GraphicalImmediateMode();
    //         break;
    //     }
    // }
    // if (instance->ui == NULL) {
    //     mirabel_slogf(LOGS_FATAL, "no interface set");
    //     exit(1);
    // }
}

bool app_mainloop()
{
    return true;
}

#ifdef __cplusplus
}
#endif
