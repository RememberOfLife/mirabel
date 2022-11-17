#include <cstdbool>
#include <cstddef>
#include <cstdint>

#include "imgui.h"

#include "mirabel/imgui_c_thin.h"

#ifdef __cplusplus
extern "C" {
#endif

void ImGuiThin_TextUnformatted(const char* str, const char* str_end)
{
    ImGui::TextUnformatted(str, str_end);
}

bool ImGuiThin_Button(const char* label)
{
    return ImGui::Button(label);
}

bool ImGuiThin_CheckBox(const char* label, bool* v)
{
    return ImGui::Checkbox(label, v);
}

bool ImGuiThin_InputText(const char* label, char* buf, size_t buf_size)
{
    return ImGui::InputText(label, buf, buf_size);
}

bool ImGuiThin_SliderScalar(const char* label, IMGUITHIN_DATATYPE type, void* v, void* min, void* max)
{
    return ImGui::SliderScalar(label, (ImGuiDataType)type, v, min, max);
}

bool ImGuiThin_InputScalar(const char* label, IMGUITHIN_DATATYPE type, void* v)
{
    return ImGui::InputScalar(label, (ImGuiDataType)type, v);
}

void ImGuiThin_BeginDisabled()
{
    ImGui::BeginDisabled();
}

void ImGuiThin_EndDisabled()
{
    ImGui::EndDisabled();
}

#ifdef __cplusplus
}
#endif
