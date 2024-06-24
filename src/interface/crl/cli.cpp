#include <cstddef>
#include <cstdio>
#include <cstring>

#include "crossline.h"
#include "rosalia/semver.h"

#include "interface/crl/cli.h"

#include "interface/crl/cli.hpp"

CommandReadLine::CommandReadLine()
{
}

CommandReadLine::~CommandReadLine()
{
}

bool CommandReadLine::mainloop()
{
    char buf[256];

    bool quit = crossline_readline("mirabel > ", buf, sizeof(buf)) == NULL;
    if (quit || strcmp(buf, "exit") == 0 || strcmp(buf, "quit") == 0) {
        printf("DONE\n");
        return true;
    }

    printf("echo: \"%s\"\n", buf);

    return false;
}

/////
// methods wrapper

#ifdef __cplusplus
extern "C" {
#endif

static const char* get_last_error_cif(client_interface* self)
{
    return NULL;
}

static error_code create_cif(client_interface* self)
{
    self->data = new CommandReadLine();
    return CLIENT_INTERFACE_ERR_OK;
}

static error_code destroy_cif(client_interface* self)
{
    delete (CommandReadLine*)self->data;
    return CLIENT_INTERFACE_ERR_OK;
}

static bool mainloop_cif(client_interface* self)
{
    return ((CommandReadLine*)self->data)->mainloop();
}

static void log_cif(client_interface* self, LOGS status, const char* str, const char* str_end)
{
    //TODO
}

static const char* user_file_path_prompt_cif(client_interface* self, const char* suggested_save_name)
{
    //TODO
    return NULL;
}

const client_interface_methods cli_client_interface{
    .name = "crl",
    .version = (semver){
        .major = 0,
        .minor = 0,
        .patch = 0,
    },
    .get_last_error = get_last_error_cif,
    .create = create_cif,
    .destroy = destroy_cif,
    .mainloop = mainloop_cif,
    .log = log_cif,
    .user_file_path_prompt = user_file_path_prompt_cif,
};

#ifdef __cplusplus
}
#endif
