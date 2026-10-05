#include "imgui_text_renderer.h"

#include <cfloat>

namespace KrxSlaxy {
    extern ImFont* g_FontRegular;
    extern ImFont* g_FontBold;
    extern ImFont* g_PixelFont;
    extern ImFont* g_FontTitle;
    extern ImFont* g_FontBadge;
}

TextRenderer g_TextRenderer;
TextFont g_TextFont;

TextRenderer::TextRenderer()
    : m_Device(nullptr)
    , m_Context(nullptr)
{
}

TextRenderer::~TextRenderer()
{
    Shutdown();
}

bool TextRenderer::Init(ID3D11Device* device, ID3D11DeviceContext* context)
{
    m_Device = device;
    m_Context = context;
    return true;
}

void TextRenderer::Shutdown()
{
    ClearCache();
    m_Device = nullptr;
    m_Context = nullptr;
}

void TextRenderer::ClearCache()
{
}

ImFont* TextRenderer::ResolveFont(const TextFont& font) const
{
    if (font.size <= 11 && KrxSlaxy::g_FontBadge)
        return KrxSlaxy::g_FontBadge;

    if (font.size <= 10 && KrxSlaxy::g_PixelFont)
        return KrxSlaxy::g_PixelFont;

    if (font.size >= 15 && KrxSlaxy::g_FontTitle)
        return KrxSlaxy::g_FontTitle;

    if (font.weight >= FW_BOLD && KrxSlaxy::g_FontBold)
        return KrxSlaxy::g_FontBold;

    if (KrxSlaxy::g_FontRegular)
        return KrxSlaxy::g_FontRegular;

    return ImGui::GetFont();
}

float TextRenderer::ResolveFontSize(const TextFont& font, ImFont* imguiFont) const
{
    if (imguiFont)
        return imguiFont->LegacySize;

    if (font.size > 0)
        return static_cast<float>(font.size);

    return ImGui::GetFontSize();
}

ImVec2 TextRenderer::MeasureText(const std::string& text, const TextFont& font)
{
    if (text.empty())
        return ImVec2(0.0f, 0.0f);

    ImFont* imguiFont = ResolveFont(font);
    if (!imguiFont)
        return ImGui::CalcTextSize(text.c_str());

    const float fontSize = ResolveFontSize(font, imguiFont);
    return imguiFont->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text.c_str());
}

void TextRenderer::RenderText(ImDrawList* drawList, const ImVec2& pos, const std::string& text,
    ImU32 color, const TextFont& font, unsigned int flags)
{
    if (!drawList || text.empty())
        return;

    ImFont* imguiFont = ResolveFont(font);
    const float fontSize = ResolveFontSize(font, imguiFont);
    const int alpha = (color >> IM_COL32_A_SHIFT) & 0xFF;

    if (flags & FLAG_OUTLINED)
    {
        const int outlineAlpha = (alpha < 220) ? alpha : 220;
        const ImU32 outlineColor = IM_COL32(0, 0, 0, outlineAlpha);
        drawList->AddText(imguiFont, fontSize, ImVec2(pos.x - 1.0f, pos.y), outlineColor, text.c_str());
        drawList->AddText(imguiFont, fontSize, ImVec2(pos.x + 1.0f, pos.y), outlineColor, text.c_str());
        drawList->AddText(imguiFont, fontSize, ImVec2(pos.x, pos.y - 1.0f), outlineColor, text.c_str());
        drawList->AddText(imguiFont, fontSize, ImVec2(pos.x, pos.y + 1.0f), outlineColor, text.c_str());
    }

    if (flags & FLAG_DROPSHADOW)
    {
        const int shadowAlpha = (alpha < 180) ? alpha : 180;
        const ImU32 shadowColor = IM_COL32(0, 0, 0, shadowAlpha);
        drawList->AddText(imguiFont, fontSize, ImVec2(pos.x + 1.0f, pos.y + 1.0f), shadowColor, text.c_str());
    }

    drawList->AddText(imguiFont, fontSize, pos, color, text.c_str());
}


