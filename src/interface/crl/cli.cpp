#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>

#include "crossline.h"
#include "rosalia/semver.h"

#include "mirabel/alloc.h"

#include "interface/crl/cli.h"

#include "interface/crl/cli.hpp"

CommandReadLine::CommandReadLine()
{
    input_thread = std::thread(&CommandReadLine::input_thread_func, this);
}

CommandReadLine::~CommandReadLine()
{
    //TODO somehow make the crl quit..
    input_thread.join();
}

bool CommandReadLine::mainloop()
{
    input_mut.lock();
    if (input_buf != NULL) {
        // process input
        mirabel_slogf(LOGS_NORM, "crl-echo: \"%s\"\n", input_buf);
    }
    input_buf = NULL;
    input_mut.unlock();
    input_cv.notify_all();
    //TODO after the prompt is put up we do not want to log anything else as that displaces the prompt, i.e. store the logs and push them later, or, "restore" the prompt if possible everytime in our logging function..
    if (input_quit) {
        mirabel_slogf(LOGS_NORM, "crl-done");
        return true;
    }
    return false;
}

void CommandReadLine::input_thread_func()
{
    char* buf = (char*)mirabel_malloc(input_size);
    while (true) {
        char* rb = crossline_readline("mirabel > ", buf, input_size);
        std::unique_lock<std::mutex> lock(input_mut);
        if (rb == NULL || strcmp(rb, "exit") == 0 || strcmp(rb, "quit") == 0) {
            input_quit = true;
            break;
        }
        input_buf = buf;
        input_cv.wait(lock);
        if (input_quit) {
            break;
        }
    }
    mirabel_free(buf);
}

/////
// methods wrapper

#ifdef __cplusplus
extern "C" {
#endif

static const char* get_last_error_cif(app_interface* self)
{
    return NULL;
}

static error_code create_cif(app_interface* self)
{
    self->data = new CommandReadLine();
    return APP_INTERFACE_ERR_OK;
}

static void destroy_cif(app_interface* self)
{
    delete (CommandReadLine*)self->data;
}

static bool mainloop_cif(app_interface* self)
{
    return ((CommandReadLine*)self->data)->mainloop();
}

static void log_cif(app_interface* self, LOGS status, const char* str, const char* str_end)
{
    //TODO
}

static const char* user_file_path_prompt_cif(app_interface* self, const char* suggested_save_name)
{
    //TODO
    return NULL;
}

const app_interface_methods cli_app_interface{
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
