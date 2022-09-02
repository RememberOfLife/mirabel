#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//WARNING updates here incur updates to the versioning of: {engine_wrap, frontend, game_wrap}

//TODO text

// if str_end is NULL, uses '\0' to determine string end
void ImGuiThin_TextUnformatted(const char* str, const char* str_end);

bool ImGuiThin_Button(const char* label); //TODO size / fullwidth bool?

bool ImGuiThin_CheckBox(const char* label, bool* v);

bool ImGuiThin_InputText(const char* label, char* buf, size_t buf_size);

typedef enum IMGUITHIN_DATATYPE_E {
    IMGUITHIN_DATATYPE_S8 = 0,
    IMGUITHIN_DATATYPE_U8,
    IMGUITHIN_DATATYPE_S16,
    IMGUITHIN_DATATYPE_U16,
    IMGUITHIN_DATATYPE_S32,
    IMGUITHIN_DATATYPE_U32,
    IMGUITHIN_DATATYPE_S64,
    IMGUITHIN_DATATYPE_U64,
    IMGUITHIN_DATATYPE_FLOAT,
    IMGUITHIN_DATATYPE_DOUBLE,
    IMGUITHIN_DATATYPE_COUNT,
} IMGUITHIN_DATATYPE;

//TODO offer always clamp option built in and display precision format
bool ImGuiThin_SliderScalar(const char* label, IMGUITHIN_DATATYPE type, void* v, void* min, void* max);

//TODO offer always clamp option built in and display precision format and step and step buttons
bool ImGuiThin_InputScalar(const char* label, IMGUITHIN_DATATYPE type, void* v);

#ifdef __cplusplus
}
#endif
