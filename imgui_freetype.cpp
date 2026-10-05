


#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_freetype.h"
#include "imgui_internal.h"
#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H
#include FT_GLYPH_H
#include FT_SIZES_H
#include FT_SYNTHESIS_H


#if defined(IMGUI_ENABLE_FREETYPE_LUNASVG) && defined(IMGUI_ENABLE_FREETYPE_PLUTOSVG)
#error "Cannot enable both IMGUI_ENABLE_FREETYPE_LUNASVG and IMGUI_ENABLE_FREETYPE_PLUTOSVG"
#endif
#ifdef  IMGUI_ENABLE_FREETYPE_LUNASVG
#include FT_OTSVG_H
#include FT_BBOX_H
#include <lunasvg.h>
#endif
#ifdef  IMGUI_ENABLE_FREETYPE_PLUTOSVG
#include <plutosvg.h>
#endif
#if defined(IMGUI_ENABLE_FREETYPE_LUNASVG) || defined (IMGUI_ENABLE_FREETYPE_PLUTOSVG)
#if !((FREETYPE_MAJOR >= 2) && (FREETYPE_MINOR >= 12))
#error IMGUI_ENABLE_FREETYPE_PLUTOSVG or IMGUI_ENABLE_FREETYPE_LUNASVG requires FreeType version >= 2.12
#endif
#endif

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4505)
#pragma warning (disable: 26812)
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunused-function"
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
#endif
#endif


static void* ImGuiFreeTypeDefaultAllocFunc(size_t size, void* user_data) { IM_UNUSED(user_data); return IM_ALLOC(size); }
static void  ImGuiFreeTypeDefaultFreeFunc(void* ptr, void* user_data) { IM_UNUSED(user_data); IM_FREE(ptr); }


static void* (*GImGuiFreeTypeAllocFunc)(size_t size, void* user_data) = ImGuiFreeTypeDefaultAllocFunc;
static void  (*GImGuiFreeTypeFreeFunc)(void* ptr, void* user_data) = ImGuiFreeTypeDefaultFreeFunc;
static void* GImGuiFreeTypeAllocatorUserData = nullptr;


#ifdef IMGUI_ENABLE_FREETYPE_LUNASVG
static FT_Error ImGuiLunasvgPortInit(FT_Pointer* state);
static void     ImGuiLunasvgPortFree(FT_Pointer* state);
static FT_Error ImGuiLunasvgPortRender(FT_GlyphSlot slot, FT_Pointer* _state);
static FT_Error ImGuiLunasvgPortPresetSlot(FT_GlyphSlot slot, FT_Bool cache, FT_Pointer* _state);
#endif


#define FT_CEIL(X)      (((X + 63) & -64) / 64)
#define FT_SCALEFACTOR  64.0f


struct ImGui_ImplFreeType_Data
{
    FT_Library                      Library;
    FT_MemoryRec_                   MemoryManager;
    ImGui_ImplFreeType_Data()       { memset((void*)this, 0, sizeof(*this)); }
};


struct ImGui_ImplFreeType_FontSrcData
{

    bool                            InitFont(FT_Library ft_library, ImFontConfig* src, ImGuiFreeTypeLoaderFlags extra_user_flags);
    void                            CloseFont();
    ImGui_ImplFreeType_FontSrcData()   { memset((void*)this, 0, sizeof(*this)); }
    ~ImGui_ImplFreeType_FontSrcData()  { CloseFont(); }


    FT_Face                         FtFace;
    ImGuiFreeTypeLoaderFlags        UserFlags;
    FT_Int32                        LoadFlags;
    ImFontBaked*                    BakedLastActivated;
};


struct ImGui_ImplFreeType_FontSrcBakedData
{
    FT_Size     FtSize;
    ImGui_ImplFreeType_FontSrcBakedData() { memset((void*)this, 0, sizeof(*this)); }
};

bool ImGui_ImplFreeType_FontSrcData::InitFont(FT_Library ft_library, ImFontConfig* src, ImGuiFreeTypeLoaderFlags extra_font_loader_flags)
{
    FT_Error error = FT_New_Memory_Face(ft_library, (uint8_t*)src->FontData, (FT_Long)src->FontDataSize, (FT_Long)src->FontNo, &FtFace);
    if (error != 0)
        return false;
    error = FT_Select_Charmap(FtFace, FT_ENCODING_UNICODE);
    if (error != 0)
        return false;


    UserFlags = (ImGuiFreeTypeLoaderFlags)(src->FontLoaderFlags | extra_font_loader_flags);

    LoadFlags = 0;
    if ((UserFlags & ImGuiFreeTypeLoaderFlags_Bitmap) == 0)
        LoadFlags |= FT_LOAD_NO_BITMAP;

    if (UserFlags & ImGuiFreeTypeLoaderFlags_NoHinting)
        LoadFlags |= FT_LOAD_NO_HINTING;
    else
        src->PixelSnapH = true;

    if (UserFlags & ImGuiFreeTypeLoaderFlags_NoAutoHint)
        LoadFlags |= FT_LOAD_NO_AUTOHINT;
    if (UserFlags & ImGuiFreeTypeLoaderFlags_ForceAutoHint)
        LoadFlags |= FT_LOAD_FORCE_AUTOHINT;
    if (UserFlags & ImGuiFreeTypeLoaderFlags_LightHinting)
        LoadFlags |= FT_LOAD_TARGET_LIGHT;
    else if (UserFlags & ImGuiFreeTypeLoaderFlags_MonoHinting)
        LoadFlags |= FT_LOAD_TARGET_MONO;
    else
        LoadFlags |= FT_LOAD_TARGET_NORMAL;

    if (UserFlags & ImGuiFreeTypeLoaderFlags_LoadColor)
        LoadFlags |= FT_LOAD_COLOR;

    return true;
}

void ImGui_ImplFreeType_FontSrcData::CloseFont()
{
    if (FtFace)
    {
        FT_Done_Face(FtFace);
        FtFace = nullptr;
    }
}

static const FT_Glyph_Metrics* ImGui_ImplFreeType_LoadGlyph(ImGui_ImplFreeType_FontSrcData* src_data, uint32_t codepoint)
{
    uint32_t glyph_index = FT_Get_Char_Index(src_data->FtFace, codepoint);
    if (glyph_index == 0)
        return nullptr;


    FT_Error error = FT_Load_Glyph(src_data->FtFace, glyph_index, src_data->LoadFlags);
    if (error)
        return nullptr;


    FT_GlyphSlot slot = src_data->FtFace->glyph;
#if defined(IMGUI_ENABLE_FREETYPE_LUNASVG) || defined(IMGUI_ENABLE_FREETYPE_PLUTOSVG)
    IM_ASSERT(slot->format == FT_GLYPH_FORMAT_OUTLINE || slot->format == FT_GLYPH_FORMAT_BITMAP || slot->format == FT_GLYPH_FORMAT_SVG);
#else
#if ((FREETYPE_MAJOR >= 2) && (FREETYPE_MINOR >= 12))
    IM_ASSERT(slot->format != FT_GLYPH_FORMAT_SVG && "The font contains SVG glyphs, you'll need to enable IMGUI_ENABLE_FREETYPE_PLUTOSVG or IMGUI_ENABLE_FREETYPE_LUNASVG in imconfig.h and install required libraries in order to use this font");
#endif
    IM_ASSERT(slot->format == FT_GLYPH_FORMAT_OUTLINE || slot->format == FT_GLYPH_FORMAT_BITMAP);
#endif


    if (src_data->UserFlags & ImGuiFreeTypeLoaderFlags_Bold)
        FT_GlyphSlot_Embolden(slot);
    if (src_data->UserFlags & ImGuiFreeTypeLoaderFlags_Oblique)
    {
        FT_GlyphSlot_Oblique(slot);


    }

    return &slot->metrics;
}

static void ImGui_ImplFreeType_BlitGlyph(const FT_Bitmap* ft_bitmap, uint32_t* dst, uint32_t dst_pitch)
{
    IM_ASSERT(ft_bitmap != nullptr);
    const uint32_t w = ft_bitmap->width;
    const uint32_t h = ft_bitmap->rows;
    const uint8_t* src = ft_bitmap->buffer;
    const uint32_t src_pitch = ft_bitmap->pitch;

    switch (ft_bitmap->pixel_mode)
    {
    case FT_PIXEL_MODE_GRAY:
        {
            for (uint32_t y = 0; y < h; y++, src += src_pitch, dst += dst_pitch)
                for (uint32_t x = 0; x < w; x++)
                    dst[x] = IM_COL32(255, 255, 255, src[x]);
            break;
        }
    case FT_PIXEL_MODE_MONO:
        {
            for (uint32_t y = 0; y < h; y++, src += src_pitch, dst += dst_pitch)
            {
                uint8_t bits = 0;
                const uint8_t* bits_ptr = src;
                for (uint32_t x = 0; x < w; x++, bits <<= 1)
                {
                    if ((x & 7) == 0)
                        bits = *bits_ptr++;
                    dst[x] = IM_COL32(255, 255, 255, (bits & 0x80) ? 255 : 0);
                }
            }
            break;
        }
    case FT_PIXEL_MODE_BGRA:
        {

            #define DE_MULTIPLY(color, alpha) ImMin((ImU32)(255.0f * (float)color / (float)(alpha + FLT_MIN) + 0.5f), 255u)
            for (uint32_t y = 0; y < h; y++, src += src_pitch, dst += dst_pitch)
                for (uint32_t x = 0; x < w; x++)
                {
                    uint8_t r = src[x * 4 + 2], g = src[x * 4 + 1], b = src[x * 4], a = src[x * 4 + 3];
                    dst[x] = IM_COL32(DE_MULTIPLY(r, a), DE_MULTIPLY(g, a), DE_MULTIPLY(b, a), a);
                }
            #undef DE_MULTIPLY
            break;
        }
    default:
        IM_ASSERT(0 && "FreeTypeFont::BlitGlyph(): Unknown bitmap pixel mode!");
    }
}


static void* FreeType_Alloc(FT_Memory , long size)
{
    return GImGuiFreeTypeAllocFunc((size_t)size, GImGuiFreeTypeAllocatorUserData);
}

static void FreeType_Free(FT_Memory , void* block)
{
    GImGuiFreeTypeFreeFunc(block, GImGuiFreeTypeAllocatorUserData);
}

static void* FreeType_Realloc(FT_Memory , long cur_size, long new_size, void* block)
{

    if (block == nullptr)
        return GImGuiFreeTypeAllocFunc((size_t)new_size, GImGuiFreeTypeAllocatorUserData);

    if (new_size == 0)
    {
        GImGuiFreeTypeFreeFunc(block, GImGuiFreeTypeAllocatorUserData);
        return nullptr;
    }

    if (new_size > cur_size)
    {
        void* new_block = GImGuiFreeTypeAllocFunc((size_t)new_size, GImGuiFreeTypeAllocatorUserData);
        memcpy(new_block, block, (size_t)cur_size);
        GImGuiFreeTypeFreeFunc(block, GImGuiFreeTypeAllocatorUserData);
        return new_block;
    }

    return block;
}

static bool ImGui_ImplFreeType_LoaderInit(ImFontAtlas* atlas)
{
    IM_ASSERT(atlas->FontLoaderData == nullptr);
    ImGui_ImplFreeType_Data* bd = IM_NEW(ImGui_ImplFreeType_Data)();


    bd->MemoryManager.user = nullptr;
    bd->MemoryManager.alloc = &FreeType_Alloc;
    bd->MemoryManager.free = &FreeType_Free;
    bd->MemoryManager.realloc = &FreeType_Realloc;


    FT_Error error = FT_New_Library(&bd->MemoryManager, &bd->Library);
    if (error != 0)
    {
        IM_DELETE(bd);
        return false;
    }


    FT_Add_Default_Modules(bd->Library);

#ifdef IMGUI_ENABLE_FREETYPE_LUNASVG


    SVG_RendererHooks hooks = { ImGuiLunasvgPortInit, ImGuiLunasvgPortFree, ImGuiLunasvgPortRender, ImGuiLunasvgPortPresetSlot };
    FT_Property_Set(bd->Library, "ot-svg", "svg-hooks", &hooks);
#endif
#ifdef IMGUI_ENABLE_FREETYPE_PLUTOSVG

    FT_Property_Set(bd->Library, "ot-svg", "svg-hooks", plutosvg_ft_svg_hooks());
#endif


    atlas->FontLoaderData = (void*)bd;

    return true;
}

static void ImGui_ImplFreeType_LoaderShutdown(ImFontAtlas* atlas)
{
    ImGui_ImplFreeType_Data* bd = (ImGui_ImplFreeType_Data*)atlas->FontLoaderData;
    IM_ASSERT(bd != nullptr);
    FT_Done_Library(bd->Library);
    IM_DELETE(bd);
    atlas->FontLoaderData = nullptr;
}

static bool ImGui_ImplFreeType_FontSrcInit(ImFontAtlas* atlas, ImFontConfig* src)
{
    ImGui_ImplFreeType_Data* bd = (ImGui_ImplFreeType_Data*)atlas->FontLoaderData;
    ImGui_ImplFreeType_FontSrcData* bd_font_data = IM_NEW(ImGui_ImplFreeType_FontSrcData);
    IM_ASSERT(src->FontLoaderData == nullptr);
    src->FontLoaderData = bd_font_data;

    if (!bd_font_data->InitFont(bd->Library, src, (ImGuiFreeTypeLoaderFlags)atlas->FontLoaderFlags))
    {
        IM_DELETE(bd_font_data);
        src->FontLoaderData = nullptr;
        return false;
    }

    return true;
}

static void ImGui_ImplFreeType_FontSrcDestroy(ImFontAtlas* atlas, ImFontConfig* src)
{
    IM_UNUSED(atlas);
    ImGui_ImplFreeType_FontSrcData* bd_font_data = (ImGui_ImplFreeType_FontSrcData*)src->FontLoaderData;
    IM_DELETE(bd_font_data);
    src->FontLoaderData = nullptr;
}

static bool ImGui_ImplFreeType_FontBakedInit(ImFontAtlas* atlas, ImFontConfig* src, ImFontBaked* baked, void* loader_data_for_baked_src)
{
    IM_UNUSED(atlas);
    float size = baked->Size;
    if (src->MergeMode && src->SizePixels != 0.0f)
        size *= (src->SizePixels / baked->OwnerFont->Sources[0]->SizePixels);

    ImGui_ImplFreeType_FontSrcData* bd_font_data = (ImGui_ImplFreeType_FontSrcData*)src->FontLoaderData;
    bd_font_data->BakedLastActivated = baked;


    ImGui_ImplFreeType_FontSrcBakedData* bd_baked_data = (ImGui_ImplFreeType_FontSrcBakedData*)loader_data_for_baked_src;
    IM_ASSERT(bd_baked_data != nullptr);
    IM_PLACEMENT_NEW(bd_baked_data) ImGui_ImplFreeType_FontSrcBakedData();

    FT_New_Size(bd_font_data->FtFace, &bd_baked_data->FtSize);
    FT_Activate_Size(bd_baked_data->FtSize);


    const float rasterizer_density = src->RasterizerDensity * baked->RasterizerDensity;
    FT_Size_RequestRec req;
    req.type = (bd_font_data->UserFlags & ImGuiFreeTypeLoaderFlags_Bitmap) ? FT_SIZE_REQUEST_TYPE_NOMINAL : FT_SIZE_REQUEST_TYPE_REAL_DIM;
    req.width = 0;
    req.height = (uint32_t)(size * 64 * rasterizer_density);
    req.horiResolution = 0;
    req.vertResolution = 0;
    FT_Request_Size(bd_font_data->FtFace, &req);


    if (src->MergeMode == false)
    {

        FT_Size_Metrics metrics = bd_baked_data->FtSize->metrics;
        const float scale = 1.0f / rasterizer_density;
        baked->Ascent     = (float)FT_CEIL(metrics.ascender) * scale;
        baked->Descent    = (float)FT_CEIL(metrics.descender) * scale;


    }
    return true;
}

static void ImGui_ImplFreeType_FontBakedDestroy(ImFontAtlas* atlas, ImFontConfig* src, ImFontBaked* baked, void* loader_data_for_baked_src)
{
    IM_UNUSED(atlas);
    IM_UNUSED(baked);
    IM_UNUSED(src);
    ImGui_ImplFreeType_FontSrcBakedData* bd_baked_data = (ImGui_ImplFreeType_FontSrcBakedData*)loader_data_for_baked_src;
    IM_ASSERT(bd_baked_data != nullptr);
    FT_Done_Size(bd_baked_data->FtSize);
    bd_baked_data->~ImGui_ImplFreeType_FontSrcBakedData();
}

static bool ImGui_ImplFreeType_FontBakedLoadGlyph(ImFontAtlas* atlas, ImFontConfig* src, ImFontBaked* baked, void* loader_data_for_baked_src, ImWchar codepoint, ImFontGlyph* out_glyph, float* out_advance_x)
{
    ImGui_ImplFreeType_FontSrcData* bd_font_data = (ImGui_ImplFreeType_FontSrcData*)src->FontLoaderData;
    uint32_t glyph_index = FT_Get_Char_Index(bd_font_data->FtFace, codepoint);
    if (glyph_index == 0)
        return false;

    if (bd_font_data->BakedLastActivated != baked)
    {

        ImGui_ImplFreeType_FontSrcBakedData* bd_baked_data = (ImGui_ImplFreeType_FontSrcBakedData*)loader_data_for_baked_src;
        FT_Activate_Size(bd_baked_data->FtSize);
        bd_font_data->BakedLastActivated = baked;
    }

    const FT_Glyph_Metrics* metrics = ImGui_ImplFreeType_LoadGlyph(bd_font_data, codepoint);
    if (metrics == nullptr)
        return false;

    FT_Face face = bd_font_data->FtFace;
    FT_GlyphSlot slot = face->glyph;
    const float rasterizer_density = src->RasterizerDensity * baked->RasterizerDensity;


    const float advance_x = (slot->advance.x / FT_SCALEFACTOR) / rasterizer_density;
    if (out_advance_x != NULL)
    {
        IM_ASSERT(out_glyph == NULL);
        *out_advance_x = advance_x;
        return true;
    }


    FT_Render_Mode render_mode = (bd_font_data->UserFlags & ImGuiFreeTypeLoaderFlags_Monochrome) ? FT_RENDER_MODE_MONO : FT_RENDER_MODE_NORMAL;
    FT_Error error = FT_Render_Glyph(slot, render_mode);
    const FT_Bitmap* ft_bitmap = &slot->bitmap;
    if (error != 0 || ft_bitmap == nullptr)
        return false;

    const int w = (int)ft_bitmap->width;
    const int h = (int)ft_bitmap->rows;
    const bool is_visible = (w != 0 && h != 0);


    out_glyph->Codepoint = codepoint;
    out_glyph->AdvanceX = advance_x;


    if (is_visible)
    {
        ImFontAtlasRectId pack_id = ImFontAtlasPackAddRect(atlas, w, h);
        if (pack_id == ImFontAtlasRectId_Invalid)
        {

            IM_ASSERT(pack_id != ImFontAtlasRectId_Invalid && "Out of texture memory.");
            return false;
        }
        ImTextureRect* r = ImFontAtlasPackGetRect(atlas, pack_id);


        atlas->Builder->TempBuffer.resize(w * h * 4);
        uint32_t* temp_buffer = (uint32_t*)atlas->Builder->TempBuffer.Data;
        ImGui_ImplFreeType_BlitGlyph(ft_bitmap, temp_buffer, w);

        const float ref_size = baked->OwnerFont->Sources[0]->SizePixels;
        const float offsets_scale = (ref_size != 0.0f) ? (baked->Size / ref_size) : 1.0f;
        float font_off_x = (src->GlyphOffset.x * offsets_scale);
        float font_off_y = (src->GlyphOffset.y * offsets_scale) + baked->Ascent;
        if (src->PixelSnapH)
            font_off_x = IM_ROUND(font_off_x);
        if (src->PixelSnapV)
            font_off_y = IM_ROUND(font_off_y);
        float recip_h = 1.0f / rasterizer_density;
        float recip_v = 1.0f / rasterizer_density;


        float glyph_off_x = (float)face->glyph->bitmap_left;
        float glyph_off_y = (float)-face->glyph->bitmap_top;
        out_glyph->X0 = glyph_off_x * recip_h + font_off_x;
        out_glyph->Y0 = glyph_off_y * recip_v + font_off_y;
        out_glyph->X1 = (glyph_off_x + w) * recip_h + font_off_x;
        out_glyph->Y1 = (glyph_off_y + h) * recip_v + font_off_y;
        out_glyph->Visible = true;
        out_glyph->Colored = (ft_bitmap->pixel_mode == FT_PIXEL_MODE_BGRA);
        out_glyph->PackId = pack_id;
        ImFontAtlasBakedSetFontGlyphBitmap(atlas, baked, src, out_glyph, r, (const unsigned char*)temp_buffer, ImTextureFormat_RGBA32, w * 4);
    }

    return true;
}

static bool ImGui_ImplFreetype_FontSrcContainsGlyph(ImFontAtlas* atlas, ImFontConfig* src, ImWchar codepoint)
{
    IM_UNUSED(atlas);
    ImGui_ImplFreeType_FontSrcData* bd_font_data = (ImGui_ImplFreeType_FontSrcData*)src->FontLoaderData;
    int glyph_index = FT_Get_Char_Index(bd_font_data->FtFace, codepoint);
    return glyph_index != 0;
}

const ImFontLoader* ImGuiFreeType::GetFontLoader()
{
    static ImFontLoader loader;
    loader.Name = "FreeType";
    loader.LoaderInit = ImGui_ImplFreeType_LoaderInit;
    loader.LoaderShutdown = ImGui_ImplFreeType_LoaderShutdown;
    loader.FontSrcInit = ImGui_ImplFreeType_FontSrcInit;
    loader.FontSrcDestroy = ImGui_ImplFreeType_FontSrcDestroy;
    loader.FontSrcContainsGlyph = ImGui_ImplFreetype_FontSrcContainsGlyph;
    loader.FontBakedInit = ImGui_ImplFreeType_FontBakedInit;
    loader.FontBakedDestroy = ImGui_ImplFreeType_FontBakedDestroy;
    loader.FontBakedLoadGlyph = ImGui_ImplFreeType_FontBakedLoadGlyph;
    loader.FontBakedSrcLoaderDataSize = sizeof(ImGui_ImplFreeType_FontSrcBakedData);
    return &loader;
}

void ImGuiFreeType::SetAllocatorFunctions(void* (*alloc_func)(size_t sz, void* user_data), void (*free_func)(void* ptr, void* user_data), void* user_data)
{
    GImGuiFreeTypeAllocFunc = alloc_func;
    GImGuiFreeTypeFreeFunc = free_func;
    GImGuiFreeTypeAllocatorUserData = user_data;
}

bool ImGuiFreeType::DebugEditFontLoaderFlags(unsigned int* p_font_loader_flags)
{
    bool edited = false;
    edited |= ImGui::CheckboxFlags("NoHinting",    p_font_loader_flags, ImGuiFreeTypeLoaderFlags_NoHinting);
    edited |= ImGui::CheckboxFlags("NoAutoHint",   p_font_loader_flags, ImGuiFreeTypeLoaderFlags_NoAutoHint);
    edited |= ImGui::CheckboxFlags("ForceAutoHint",p_font_loader_flags, ImGuiFreeTypeLoaderFlags_ForceAutoHint);
    edited |= ImGui::CheckboxFlags("LightHinting", p_font_loader_flags, ImGuiFreeTypeLoaderFlags_LightHinting);
    edited |= ImGui::CheckboxFlags("MonoHinting",  p_font_loader_flags, ImGuiFreeTypeLoaderFlags_MonoHinting);
    edited |= ImGui::CheckboxFlags("Bold",         p_font_loader_flags, ImGuiFreeTypeLoaderFlags_Bold);
    edited |= ImGui::CheckboxFlags("Oblique",      p_font_loader_flags, ImGuiFreeTypeLoaderFlags_Oblique);
    edited |= ImGui::CheckboxFlags("Monochrome",   p_font_loader_flags, ImGuiFreeTypeLoaderFlags_Monochrome);
    edited |= ImGui::CheckboxFlags("LoadColor",    p_font_loader_flags, ImGuiFreeTypeLoaderFlags_LoadColor);
    edited |= ImGui::CheckboxFlags("Bitmap",       p_font_loader_flags, ImGuiFreeTypeLoaderFlags_Bitmap);
    return edited;
}

#ifdef IMGUI_ENABLE_FREETYPE_LUNASVG


struct LunasvgPortState
{
    FT_Error            err = FT_Err_Ok;
    lunasvg::Matrix     matrix;
    std::unique_ptr<lunasvg::Document> svg = nullptr;
};

static FT_Error ImGuiLunasvgPortInit(FT_Pointer* _state)
{
    *_state = IM_NEW(LunasvgPortState)();
    return FT_Err_Ok;
}

static void ImGuiLunasvgPortFree(FT_Pointer* _state)
{
    IM_DELETE(*(LunasvgPortState**)_state);
}

static FT_Error ImGuiLunasvgPortRender(FT_GlyphSlot slot, FT_Pointer* _state)
{
    LunasvgPortState* state = *(LunasvgPortState**)_state;


    if (state->err != FT_Err_Ok)
        return state->err;


    lunasvg::Bitmap bitmap((uint8_t*)slot->bitmap.buffer, slot->bitmap.width, slot->bitmap.rows, slot->bitmap.pitch);
#if LUNASVG_VERSION_MAJOR >= 3
    state->svg->render(bitmap, state->matrix);
#else
    state->svg->setMatrix(state->svg->matrix().identity());
    state->svg->render(bitmap, state->matrix);
#endif
    state->err = FT_Err_Ok;
    return state->err;
}

static FT_Error ImGuiLunasvgPortPresetSlot(FT_GlyphSlot slot, FT_Bool cache, FT_Pointer* _state)
{
    FT_SVG_Document   document = (FT_SVG_Document)slot->other;
    LunasvgPortState* state = *(LunasvgPortState**)_state;
    FT_Size_Metrics&  metrics = document->metrics;


    if (cache)
        return state->err;

    state->svg = lunasvg::Document::loadFromData((const char*)document->svg_document, document->svg_document_length);
    if (state->svg == nullptr)
    {
        state->err = FT_Err_Invalid_SVG_Document;
        return state->err;
    }

#if LUNASVG_VERSION_MAJOR >= 3
    lunasvg::Box box = state->svg->boundingBox();
#else
    lunasvg::Box box = state->svg->box();
#endif
    double scale = std::min(metrics.x_ppem / box.w, metrics.y_ppem / box.h);
    double xx = (double)document->transform.xx / (1 << 16);
    double xy = -(double)document->transform.xy / (1 << 16);
    double yx = -(double)document->transform.yx / (1 << 16);
    double yy = (double)document->transform.yy / (1 << 16);
    double x0 = (double)document->delta.x / 64 * box.w / metrics.x_ppem;
    double y0 = -(double)document->delta.y / 64 * box.h / metrics.y_ppem;

#if LUNASVG_VERSION_MAJOR >= 3

    state->matrix = lunasvg::Matrix::translated(-box.x, -box.y);
    state->matrix.multiply(lunasvg::Matrix(xx, xy, yx, yy, x0, y0));
    state->matrix.scale(scale, scale);


    box.transform(state->matrix);
#else

    state->matrix.identity();
    state->matrix.scale(scale, scale);
    state->matrix.transform(xx, xy, yx, yy, x0, y0);
    state->svg->setMatrix(state->matrix);


    state->matrix.translate(-box.x, -box.y);


    box = state->svg->box();
#endif


    slot->bitmap_left = FT_Int(box.x);
    slot->bitmap_top = FT_Int(-box.y);
    slot->bitmap.rows = (unsigned int)(ImCeil((float)box.h));
    slot->bitmap.width = (unsigned int)(ImCeil((float)box.w));
    slot->bitmap.pitch = slot->bitmap.width * 4;
    slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;


    double metrics_width = box.w;
    double metrics_height = box.h;
    double horiBearingX = box.x;
    double horiBearingY = -box.y;
    double vertBearingX = slot->metrics.horiBearingX / 64.0 - slot->metrics.horiAdvance / 64.0 / 2.0;
    double vertBearingY = (slot->metrics.vertAdvance / 64.0 - slot->metrics.height / 64.0) / 2.0;
    slot->metrics.width = FT_Pos(IM_ROUND(metrics_width * 64.0));
    slot->metrics.height = FT_Pos(IM_ROUND(metrics_height * 64.0));
    slot->metrics.horiBearingX = FT_Pos(horiBearingX * 64);
    slot->metrics.horiBearingY = FT_Pos(horiBearingY * 64);
    slot->metrics.vertBearingX = FT_Pos(vertBearingX * 64);
    slot->metrics.vertBearingY = FT_Pos(vertBearingY * 64);

    if (slot->metrics.vertAdvance == 0)
        slot->metrics.vertAdvance = FT_Pos(metrics_height * 1.2 * 64.0);

    state->err = FT_Err_Ok;
    return state->err;
}

#endif


#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning (pop)
#endif

#endif
