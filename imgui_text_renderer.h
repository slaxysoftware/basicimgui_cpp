#pragma once

#include "imgui.h"
#include <d3d11.h>
#include <string>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

struct TextFont
{
    std::string fontFamily;
    int size;
    int weight;
    bool italic;
    bool antialiased;

    TextFont()
        : fontFamily("Verdana")
        , size(13)
        , weight(FW_NORMAL)
        , italic(false)
        , antialiased(true)
    {
    }
};

enum ImFontFlagsCustom : int
{
    FLAG_DROPSHADOW = 1 << 0,
    FLAG_OUTLINED = 1 << 1,
};

class TextRenderer
{
public:
    TextRenderer();
    ~TextRenderer();

    bool Init(ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    void RenderText(ImDrawList* drawList, const ImVec2& pos, const std::string& text,
        ImU32 color, const TextFont& font, unsigned int flags = 0);

    ImVec2 MeasureText(const std::string& text, const TextFont& font);
    void ClearCache();

private:
    ID3D11Device* m_Device;
    ID3D11DeviceContext* m_Context;

    ImFont* ResolveFont(const TextFont& font) const;
    float ResolveFontSize(const TextFont& font, ImFont* imguiFont) const;
};

extern TextRenderer g_TextRenderer;
extern TextFont g_TextFont;


