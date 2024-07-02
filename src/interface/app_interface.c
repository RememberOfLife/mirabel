#include <stdlib.h>

#include "mirabel/event_queue.h"
#include "mirabel/log.h"

#include "mirabel/app_interface.h"

/////
// public

const char* app_interface_get_last_error(app_interface* self)
{
    return self->methods->get_last_error(self);
}

error_code app_interface_create(app_interface* self)
{
    event_queue_create(&self->inbox);
    return self->methods->create(self);
}

void app_interface_destroy(app_interface* self)
{
    self->methods->destroy(self);
    event_queue_destroy(&self->inbox);
}

bool app_interface_mainloop(app_interface* self)
{
    return self->methods->mainloop(self);
}

void app_interface_log(app_interface* self, LOGS status, const char* str, const char* str_end)
{
    self->methods->log(self, status, str, str_end);
}

const char* app_interface_user_file_path_prompt(app_interface* self, const char* suggested_save_name)
{
    return self->methods->user_file_path_prompt(self, suggested_save_name);
}
