#include <stdlib.h>

#include "mirabel/log.h"

#include "mirabel/client_interface.h"

/////
// public

const char* client_interface_get_last_error(client_interface* self)
{
    return self->methods->get_last_error(self);
}

error_code client_interface_create(client_interface* self)
{
    return self->methods->create(self);
}

void client_interface_destroy(client_interface* self)
{
    self->methods->destroy(self);
}

bool client_interface_mainloop(client_interface* self)
{
    return self->methods->mainloop(self);
}

void client_interface_log(client_interface* self, LOGS status, const char* str, const char* str_end)
{
    self->methods->log(self, status, str, str_end);
}

const char* client_interface_user_file_path_prompt(client_interface* self, const char* suggested_save_name)
{
    return self->methods->user_file_path_prompt(self, suggested_save_name);
}
