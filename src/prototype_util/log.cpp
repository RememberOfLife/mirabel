#include <cstdint>

#include "meta_gui/meta_gui.hpp"

#include "mirabel/imgui_c_thin.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t mirabel_log_register(const char* name)
{
    return MetaGui::log_register(name);
}

void mirabel_log_unregister(uint32_t log_id)
{
    MetaGui::log_unregister(log_id);
}

void mirabel_log_clear(uint32_t log_id)
{
    MetaGui::log_clear(log_id);
}

void mirabel_log(const char* str, const char* str_end)
{
    MetaGui::log(str, str_end);
}

void mirabel_logh(uint32_t log_id, const char* str, const char* str_end)
{
    MetaGui::log(log_id, str, str_end);
}

#ifdef __cplusplus
}
#endif
