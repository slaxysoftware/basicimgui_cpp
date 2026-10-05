


#pragma once
#include "imgui.h"
#ifndef IMGUI_DISABLE


struct ImFontAtlas;
struct ImFontLoader;


typedef unsigned int ImGuiFreeTypeLoaderFlags;
enum ImGuiFreeTypeLoaderFlags_
{
    ImGuiFreeTypeLoaderFlags_NoHinting     = 1 << 0,
    ImGuiFreeTypeLoaderFlags_NoAutoHint    = 1 << 1,
    ImGuiFreeTypeLoaderFlags_ForceAutoHint = 1 << 2,
    ImGuiFreeTypeLoaderFlags_LightHinting  = 1 << 3,
    ImGuiFreeTypeLoaderFlags_MonoHinting   = 1 << 4,
    ImGuiFreeTypeLoaderFlags_Bold          = 1 << 5,
    ImGuiFreeTypeLoaderFlags_Oblique       = 1 << 6,
    ImGuiFreeTypeLoaderFlags_Monochrome    = 1 << 7,
    ImGuiFreeTypeLoaderFlags_LoadColor     = 1 << 8,
    ImGuiFreeTypeLoaderFlags_Bitmap        = 1 << 9,

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    ImGuiFreeTypeBuilderFlags_NoHinting     = ImGuiFreeTypeLoaderFlags_NoHinting,
    ImGuiFreeTypeBuilderFlags_NoAutoHint    = ImGuiFreeTypeLoaderFlags_NoAutoHint,
    ImGuiFreeTypeBuilderFlags_ForceAutoHint = ImGuiFreeTypeLoaderFlags_ForceAutoHint,
    ImGuiFreeTypeBuilderFlags_LightHinting  = ImGuiFreeTypeLoaderFlags_LightHinting,
    ImGuiFreeTypeBuilderFlags_MonoHinting   = ImGuiFreeTypeLoaderFlags_MonoHinting,
    ImGuiFreeTypeBuilderFlags_Bold          = ImGuiFreeTypeLoaderFlags_Bold,
    ImGuiFreeTypeBuilderFlags_Oblique       = ImGuiFreeTypeLoaderFlags_Oblique,
    ImGuiFreeTypeBuilderFlags_Monochrome    = ImGuiFreeTypeLoaderFlags_Monochrome,
    ImGuiFreeTypeBuilderFlags_LoadColor     = ImGuiFreeTypeLoaderFlags_LoadColor,
    ImGuiFreeTypeBuilderFlags_Bitmap        = ImGuiFreeTypeLoaderFlags_Bitmap,
#endif
};


#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
typedef ImGuiFreeTypeLoaderFlags_ ImGuiFreeTypeBuilderFlags_;
#endif

namespace ImGuiFreeType
{


    IMGUI_API const ImFontLoader*       GetFontLoader();


    IMGUI_API void                      SetAllocatorFunctions(void* (*alloc_func)(size_t sz, void* user_data), void (*free_func)(void* ptr, void* user_data), void* user_data = nullptr);


    IMGUI_API bool                      DebugEditFontLoaderFlags(ImGuiFreeTypeLoaderFlags* p_font_loader_flags);


#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS


#endif
}

#endif
