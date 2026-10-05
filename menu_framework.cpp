#include "menu_framework.h"
#include "imgui_text_renderer.h"
#include <cstdio>
#include <ctime>
#include <set>
#include <Windows.h>

extern HWND g_hWnd;

namespace KrxSlaxy {
    Config g_Config;
    ImFont* g_IconFont = nullptr;
    ImFont* g_FontAwesome = nullptr;
    ImFont* g_FontRegular = nullptr;
    ImFont* g_FontBold = nullptr;
    ImFont* g_PixelFont = nullptr;
    ImFont* g_FontTitle = nullptr;
    ImFont* g_FontBadge = nullptr;
    void* g_BackgroundTexture = nullptr;
    int g_BackgroundWidth = 0;
    int g_BackgroundHeight = 0;
    void* g_LogoTexture = nullptr;
    int g_LogoWidth = 0;
    int g_LogoHeight = 0;

    bool Keybind::IsActive() const {
        if (key == 0) return false;

        switch (type) {
            case KeybindType::Hold:
                return (GetAsyncKeyState(key) & 0x8000) != 0;
            case KeybindType::Toggle:
                return active;
            case KeybindType::Always:
                return true;
        }
        return false;
    }

    const char* Keybind::GetKeyName() const {
        if (key == 0) return "None";
        if (key == VK_LBUTTON) return "Mouse 1";
        if (key == VK_RBUTTON) return "Mouse 2";
        if (key == VK_MBUTTON) return "Mouse 3";
        if (key == VK_XBUTTON1) return "Mouse 4";
        if (key == VK_XBUTTON2) return "Mouse 5";
        if (key == VK_SPACE) return "Space";
        if (key == VK_SHIFT) return "Shift";
        if (key == VK_CONTROL) return "Ctrl";
        if (key == VK_MENU) return "Alt";
        if (key == VK_TAB) return "Tab";
        if (key == VK_CAPITAL) return "Caps";
        if (key == VK_ESCAPE) return "Esc";
        if (key == VK_RETURN) return "Enter";
        if (key == VK_BACK) return "Backspace";
        if (key == VK_INSERT) return "Insert";
        if (key == VK_DELETE) return "Delete";
        if (key == VK_HOME) return "Home";
        if (key == VK_END) return "End";
        if (key == VK_PRIOR) return "PgUp";
        if (key == VK_NEXT) return "PgDn";
        if (key == VK_UP) return "Up";
        if (key == VK_DOWN) return "Down";
        if (key == VK_LEFT) return "Left";
        if (key == VK_RIGHT) return "Right";
        if (key >= VK_F1 && key <= VK_F12) {
            static char fkey[4];
            snprintf(fkey, sizeof(fkey), "F%d", key - VK_F1 + 1);
            return fkey;
        }
        if (key >= '0' && key <= '9') {
            static char num[2];
            num[0] = (char)key;
            num[1] = 0;
            return num;
        }
        if (key >= 'A' && key <= 'Z') {
            static char letter[2];
            letter[0] = (char)key;
            letter[1] = 0;
            return letter;
        }

        return "Unknown";
    }

    const char* Keybind::GetTypeName(KeybindType t) {
        switch (t) {
            case KeybindType::Hold: return "Hold";
            case KeybindType::Toggle: return "Toggle";
            case KeybindType::Always: return "Always";
        }
        return "Hold";
    }
    void ProcessKeybinds() {
        static std::map<std::string, bool> prevKeyState;

        for (auto& pair : g_Config.keybinds) {
            Keybind& kb = pair.second;
            if (kb.key == 0 || kb.type != KeybindType::Toggle) continue;

            bool isPressed = (GetAsyncKeyState(kb.key) & 0x8000) != 0;
            bool wasPressed = prevKeyState[pair.first];
            if (isPressed && !wasPressed) {
                kb.active = !kb.active;
            }

            prevKeyState[pair.first] = isPressed;
        }
    }

    namespace GUI {
        static struct {
            ImVec2 groupStartPos;
            float groupWidth;
            float groupHeight;
            float groupContentY;
            int columnCount;
            int currentColumn;
            float columnWidths[8];
            float columnX[8];
        } s_LayoutState;

        std::string ToLower(const char* text) {
            std::string result(text);
            std::transform(result.begin(), result.end(), result.begin(), ::tolower);
            return result;
        }

        void Initialize() {
            State::Get().drawList = nullptr;
        }

        void BeginFrame(const ImVec2& pos, const ImVec2& size) {
            State& s = State::Get();
            s.windowPos = pos;
            s.windowSize = size;
            s.cursorPos = pos;
            s.drawList = ImGui::GetWindowDrawList();
        }

        void EndFrame() {
        }

        void DrawDiagonalPattern(ImDrawList* dl, ImVec2 min, ImVec2 max, float spacing, float lineWidth, ImU32 color) {
            float width = max.x - min.x;
            float height = max.y - min.y;
            float diagonal = width + height;

            dl->PushClipRect(min, max, true);

            for (float offset = -height; offset < width; offset += spacing) {
                ImVec2 p1(min.x + offset, max.y);
                ImVec2 p2(min.x + offset + height, min.y);
                dl->AddLine(p1, p2, color, lineWidth);
            }

            dl->PopClipRect();
        }

        void DrawCrosshatchPattern(ImDrawList* dl, ImVec2 min, ImVec2 max, float spacing, float lineWidth, ImU32 color) {
            float width = max.x - min.x;
            float height = max.y - min.y;

            dl->PushClipRect(min, max, true);
            for (float offset = -height; offset < width; offset += spacing) {
                ImVec2 p1(min.x + offset, max.y);
                ImVec2 p2(min.x + offset + height, min.y);
                dl->AddLine(p1, p2, color, lineWidth);
            }
            for (float offset = 0; offset < width + height; offset += spacing) {
                ImVec2 p1(min.x + offset, min.y);
                ImVec2 p2(min.x + offset - height, max.y);
                dl->AddLine(p1, p2, color, lineWidth);
            }

            dl->PopClipRect();
        }
        void DrawCarbonFiberPattern(ImDrawList* dl, ImVec2 min, ImVec2 max) {
            float cellW = 8.0f;
            float cellH = 12.0f;
            float gap = 2.0f;

            ImU32 darkColor = IM_COL32(20, 20, 20, 255);
            ImU32 lightColor = IM_COL32(30, 30, 30, 255);

            dl->PushClipRect(min, max, true);

            int row = 0;
            for (float y = min.y; y < max.y; y += cellH + gap) {
                float xOffset = (row % 2) * (cellW * 0.5f + gap * 0.5f);

                for (float x = min.x - cellW + xOffset; x < max.x + cellW; x += cellW + gap) {
                    ImVec2 cellMin(x, y);
                    ImVec2 cellMax(x + cellW, y + cellH);
                    dl->AddRectFilled(cellMin, cellMax, darkColor, 1.0f);
                    dl->AddLine(
                        ImVec2(cellMin.x + 1, cellMin.y + 1),
                        ImVec2(cellMax.x - 1, cellMin.y + 1),
                        lightColor, 1.0f
                    );
                }
                row++;
            }

            dl->PopClipRect();
        }

        void DrawGearIcon(ImDrawList* dl, ImVec2 center, float size, ImU32 color) {
            const float outerRadius = size * 0.5f;
            const float innerRadius = size * 0.25f;
            const float toothDepth = size * 0.15f;
            const int numTeeth = 8;
            for (int i = 0; i < numTeeth; i++) {
                float angle1 = (float)i / numTeeth * IM_PI * 2.0f - IM_PI / numTeeth * 0.5f;
                float angle2 = (float)i / numTeeth * IM_PI * 2.0f + IM_PI / numTeeth * 0.5f;

                ImVec2 p1(center.x + cosf(angle1) * outerRadius, center.y + sinf(angle1) * outerRadius);
                ImVec2 p2(center.x + cosf(angle2) * outerRadius, center.y + sinf(angle2) * outerRadius);
                ImVec2 p3(center.x + cosf(angle2) * (outerRadius + toothDepth), center.y + sinf(angle2) * (outerRadius + toothDepth));
                ImVec2 p4(center.x + cosf(angle1) * (outerRadius + toothDepth), center.y + sinf(angle1) * (outerRadius + toothDepth));

                dl->AddQuadFilled(p1, p2, p3, p4, color);
            }
            dl->AddCircleFilled(center, outerRadius, color, 16);
            dl->AddCircleFilled(center, innerRadius, Colors::Background(), 12);
        }

        void DrawColorPreview(ImDrawList* dl, ImVec2 pos, float width, float height, float* colorPtr) {
            if (!colorPtr) return;
            int alpha = (int)(255 * Colors::GlobalAlpha());
            ImU32 color = IM_COL32(
                (int)(colorPtr[0] * 255.0f),
                (int)(colorPtr[1] * 255.0f),
                (int)(colorPtr[2] * 255.0f),
                alpha
            );
            dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), color, 0.0f);
            dl->AddRect(pos, ImVec2(pos.x + width, pos.y + height), Colors::Border(), 0.0f, 0, 1.0f);
        }

        void DrawTabIcon(ImDrawList* dl, ImVec2 center, float size, int iconType, ImU32 color) {
            const float r = size * 0.5f;

            switch (iconType) {
                case 0:
                {
                    dl->AddCircle(center, r, color, 16, 1.5f);
                    dl->AddCircleFilled(center, r * 0.2f, color, 8);
                    dl->AddLine(ImVec2(center.x - r, center.y), ImVec2(center.x - r * 0.4f, center.y), color, 1.5f);
                    dl->AddLine(ImVec2(center.x + r * 0.4f, center.y), ImVec2(center.x + r, center.y), color, 1.5f);
                    dl->AddLine(ImVec2(center.x, center.y - r), ImVec2(center.x, center.y - r * 0.4f), color, 1.5f);
                    dl->AddLine(ImVec2(center.x, center.y + r * 0.4f), ImVec2(center.x, center.y + r), color, 1.5f);
                    break;
                }
                case 1:
                {
                    ImVec2 points[12];
                    for (int i = 0; i < 12; i++) {
                        float t = (float)i / 11.0f;
                        float angle = t * IM_PI;
                        points[i] = ImVec2(
                            center.x + cosf(angle) * r,
                            center.y + sinf(angle) * r * 0.5f
                        );
                    }
                    dl->AddPolyline(points, 12, color, ImDrawFlags_None, 1.5f);
                    for (int i = 0; i < 12; i++) {
                        float t = (float)i / 11.0f;
                        float angle = IM_PI + t * IM_PI;
                        points[i] = ImVec2(
                            center.x + cosf(angle) * r,
                            center.y + sinf(angle) * r * 0.5f
                        );
                    }
                    dl->AddPolyline(points, 12, color, ImDrawFlags_None, 1.5f);
                    dl->AddCircleFilled(center, r * 0.35f, color, 12);
                    break;
                }
                case 2:
                {
                    dl->AddCircle(center, r, color, 16, 1.5f);
                    dl->AddLine(ImVec2(center.x - r, center.y), ImVec2(center.x + r, center.y), color, 1.0f);
                    for (int i = 0; i <= 12; i++) {
                        float angle = (float)i / 12.0f * IM_PI * 2.0f;
                        float x = center.x + cosf(angle) * r * 0.4f;
                        float y = center.y + sinf(angle) * r;
                        if (i > 0) {
                            float prevAngle = (float)(i-1) / 12.0f * IM_PI * 2.0f;
                            float px = center.x + cosf(prevAngle) * r * 0.4f;
                            float py = center.y + sinf(prevAngle) * r;
                            dl->AddLine(ImVec2(px, py), ImVec2(x, y), color, 1.0f);
                        }
                    }
                    break;
                }
                case 3:
                {
                    dl->AddLine(
                        ImVec2(center.x - r * 0.7f, center.y + r * 0.7f),
                        ImVec2(center.x + r * 0.5f, center.y - r * 0.5f),
                        color, 2.0f
                    );
                    ImVec2 tip(center.x + r * 0.5f, center.y - r * 0.5f);
                    dl->AddLine(ImVec2(tip.x - r * 0.3f, tip.y), ImVec2(tip.x + r * 0.3f, tip.y), color, 1.0f);
                    dl->AddLine(ImVec2(tip.x, tip.y - r * 0.3f), ImVec2(tip.x, tip.y + r * 0.3f), color, 1.0f);
                    break;
                }
                case 4:
                {
                    DrawGearIcon(dl, center, size, color);
                    break;
                }
            }
        }

        void DrawKrxSlaxyEmblem(ImDrawList* dl, ImVec2 center, float radius, ImU32 mainColor, ImU32 glowColor) {
            (void)glowColor;
            float scale = radius / 14.0f;
            float anim_time = (float)ImGui::GetTime();
            float pulse = (sinf(anim_time * 2.8f) + 1.0f) * 0.5f;

            float alpha = Colors::GlobalAlpha();
            ImU32 headshot_red = IM_COL32(255, 35, 65, (int)(255 * alpha));
            ImU32 reticle_dim  = Colors::WithAlpha(Colors::TextInactive(), (0.65f + 0.15f * pulse) * alpha);
            ImU32 outerGlow    = Colors::WithAlpha(mainColor, (0.15f + 0.18f * pulse) * alpha);

            dl->AddCircleFilled(center, 13.5f * scale, Colors::WithAlpha(Colors::BackgroundDark(), 0.85f * alpha), 32);
            dl->AddCircle(center, (15.5f + pulse * 1.5f) * scale, outerGlow, 32, 1.2f * scale);
            dl->AddCircle(center, 13.5f * scale, mainColor, 32, 1.8f * scale);
            dl->AddCircle(center, 6.5f * scale, reticle_dim, 24, 1.0f * scale);

            dl->AddLine(ImVec2(center.x, center.y - 18.0f * scale), ImVec2(center.x, center.y - 9.0f * scale), mainColor, 2.0f * scale);
            dl->AddLine(ImVec2(center.x, center.y + 9.0f * scale),   ImVec2(center.x, center.y + 18.0f * scale), mainColor, 2.0f * scale);
            dl->AddLine(ImVec2(center.x - 18.0f * scale, center.y), ImVec2(center.x - 9.0f * scale, center.y), mainColor, 2.0f * scale);
            dl->AddLine(ImVec2(center.x + 9.0f * scale, center.y),   ImVec2(center.x + 18.0f * scale, center.y), mainColor, 2.0f * scale);

            dl->AddCircleFilled(center, (3.8f + pulse * 1.2f) * scale, IM_COL32(255, 35, 65, (int)((55 + 65 * pulse) * alpha)));
            dl->AddCircleFilled(center, 2.5f * scale, headshot_red);
            dl->AddCircleFilled(center, 1.0f * scale, IM_COL32(255, 255, 255, (int)(255 * alpha)));
        }

        void RenderHeader(const char* title) {
            (void)title;

            State& s = State::Get();
            ImDrawList* dl = s.drawList;
            Style& style = Style::Get();

            ImVec2 headerMin = s.windowPos;
            ImVec2 headerMax(s.windowPos.x + s.windowSize.x, s.windowPos.y + style.headerHeight);
            dl->AddRectFilled(headerMin, headerMax, Colors::TopBar(), 8.0f, ImDrawFlags_RoundCornersTop);

            float gradientWidth = s.windowSize.x * 0.40f;
            for (int i = 0; i < (int)s.windowSize.x; i++) {
                ImU32 lineColor;
                if (i < (int)gradientWidth) {
                    float t = (float)i / gradientWidth;
                    lineColor = Colors::LerpColor(Colors::Accent(), Colors::Border(), t);
                } else {
                    lineColor = Colors::Border();
                }
                dl->AddLine(
                    ImVec2(headerMin.x + i, headerMax.y),
                    ImVec2(headerMin.x + i + 1, headerMax.y),
                    lineColor, 1.5f
                );
            }

            float currentX = headerMin.x + 20.0f;
            float logoRadius = 14.0f;
            ImVec2 logoCenter(currentX + logoRadius, headerMin.y + style.headerHeight * 0.5f);
            DrawKrxSlaxyEmblem(dl, logoCenter, logoRadius, Colors::Accent(), Colors::AccentHover());
            currentX += logoRadius * 2.0f + 14.0f;

            TextFont titleFont;
            titleFont.size = 15;
            titleFont.weight = FW_BOLD;
            ImVec2 krxSize = g_TextRenderer.MeasureText("KRX ", titleFont);
            ImVec2 titlePos(currentX, headerMin.y + (style.headerHeight - krxSize.y) * 0.5f);
            g_TextRenderer.RenderText(dl, titlePos, "KRX ", Colors::TextActive(), titleFont, FLAG_DROPSHADOW);
            g_TextRenderer.RenderText(dl, ImVec2(titlePos.x + krxSize.x, titlePos.y), "SLAXY", Colors::Accent(), titleFont, FLAG_DROPSHADOW);

        }

        bool Tab(const char* label, const char* icon, int tabId, int* activeTab) {
            State& s = State::Get();
            ImDrawList* dl = s.drawList;
            Style& style = Style::Get();

            const float tabWidth = 85.0f;
            const float tabsStartX = s.windowPos.x + 220.0f;

            ImVec2 tabMin(tabsStartX + tabId * (tabWidth + 6.0f), s.windowPos.y + 12.0f);
            ImVec2 tabMax(tabMin.x + tabWidth, s.windowPos.y + style.headerHeight - 12.0f);

            bool isActive = (*activeTab == tabId);
            bool isHovered = ImGui::IsMouseHoveringRect(tabMin, tabMax);
            bool clicked = false;
            static std::map<int, float> s_tabHoverAnim;
            float targetAnim = (isHovered || isActive) ? 1.0f : 0.0f;
            float& hoverAnim = s_tabHoverAnim[tabId];
            hoverAnim = ImLerp(hoverAnim, targetAnim, ImGui::GetIO().DeltaTime * 12.0f);
            if (isHovered) {
                g_Config.blockWindowDrag = true;
                if (ImGui::IsMouseDown(0)) {
                    g_Config.interactiveMouseDown = true;
                }
            }
            if (isHovered && ImGui::IsMouseClicked(0)) {
                if (*activeTab != tabId) {
                    g_Config.previousTab = *activeTab;
                    g_Config.tabTransitionAnim = 1.0f;
                }
                *activeTab = tabId;
                clicked = true;
            }
            ImU32 tabTextColor = isActive ? Colors::TextActive() : Colors::LerpColor(Colors::TextInactive(), Colors::TextActive(), hoverAnim);

            if (hoverAnim > 0.01f) {
                ImU32 bgPill = Colors::WithAlpha(Colors::AccentDim(), 0.35f * hoverAnim);
                dl->AddRectFilled(tabMin, tabMax, bgPill, 4.0f);
                if (isActive) {
                    dl->AddRect(tabMin, tabMax, Colors::Accent(), 4.0f, 0, 1.0f);
                }
            }

            std::string lowerLabel = ToLower(label);
            TextFont tabFont;
            tabFont.size = 13;
            tabFont.weight = isActive ? FW_BOLD : FW_NORMAL;
            ImVec2 textSize = g_TextRenderer.MeasureText(lowerLabel, tabFont);
            ImVec2 textPos(tabMin.x + (tabWidth - textSize.x) * 0.5f, tabMin.y + (tabMax.y - tabMin.y - textSize.y) * 0.5f);
            g_TextRenderer.RenderText(dl, textPos, lowerLabel, tabTextColor, tabFont);

            static std::map<int, float> s_underlineAnim;
            float targetUnderline = isActive ? 1.0f : 0.0f;
            float& underlineAnim = s_underlineAnim[tabId];
            underlineAnim = ImLerp(underlineAnim, targetUnderline, ImGui::GetIO().DeltaTime * 10.0f);

            if (underlineAnim > 0.01f) {
                float fullWidth = tabWidth - 16.0f;
                float animatedWidth = fullWidth * underlineAnim;
                float centerX = tabMin.x + tabWidth * 0.5f;
                float lineY = tabMax.y - 1.0f;
                float halfWidth = animatedWidth * 0.5f;
                dl->AddLine(ImVec2(centerX - halfWidth, lineY), ImVec2(centerX + halfWidth, lineY), Colors::Accent(), 2.0f);
            }

            return clicked;
        }

        bool SubTab(const char* label, int tabId, int* activeTab) {
            State& s = State::Get();
            ImDrawList* dl = s.drawList;
            Style& style = Style::Get();

            const float subTabWidth = 120.0f;
            const float footerY = s.windowPos.y + s.windowSize.y - style.footerHeight;
            const float totalWidth = 3 * subTabWidth;
            const float startX = s.windowPos.x + (s.windowSize.x - totalWidth) * 0.5f;

            ImVec2 tabMin(startX + tabId * subTabWidth, footerY + 5.0f);
            ImVec2 tabMax(tabMin.x + subTabWidth, footerY + style.footerHeight - 5.0f);

            bool isActive = (*activeTab == tabId);
            bool isHovered = ImGui::IsMouseHoveringRect(tabMin, tabMax);
            bool clicked = false;
            static std::map<int, float> s_subTabHoverAnim;
            float targetAnim = (isHovered || isActive) ? 1.0f : 0.0f;
            float& hoverAnim = s_subTabHoverAnim[tabId];
            hoverAnim = ImLerp(hoverAnim, targetAnim, ImGui::GetIO().DeltaTime * 12.0f);
            if (isHovered) {
                g_Config.blockWindowDrag = true;
                if (ImGui::IsMouseDown(0)) {
                    g_Config.interactiveMouseDown = true;
                }
            }

            if (isHovered && ImGui::IsMouseClicked(0)) {
                if (*activeTab != tabId) {
                    g_Config.previousSubTab = *activeTab;
                    g_Config.subTabTransitionAnim = 1.0f;
                }
                *activeTab = tabId;
                clicked = true;
            }
            ImU32 textColor = isActive ? Colors::TextActive() : Colors::LerpColor(Colors::TextInactive(), Colors::TextActive(), hoverAnim);
            std::string lowerLabel = ToLower(label);
            TextFont subTabFont;
            subTabFont.size = 13;
            subTabFont.weight = isActive ? FW_BOLD : FW_NORMAL;
            ImVec2 textSize = g_TextRenderer.MeasureText(lowerLabel, subTabFont);
            ImVec2 textPos(
                tabMin.x + (subTabWidth - textSize.x) * 0.5f,
                tabMin.y + (style.footerHeight - 10.0f - textSize.y) * 0.5f
            );
            g_TextRenderer.RenderText(dl, textPos, lowerLabel, textColor, subTabFont);
            static std::map<int, float> s_subUnderlineAnim;
            float targetUnderline = isActive ? 1.0f : 0.0f;
            float& underlineAnim = s_subUnderlineAnim[tabId];
            underlineAnim = ImLerp(underlineAnim, targetUnderline, ImGui::GetIO().DeltaTime * 10.0f);

            if (underlineAnim > 0.01f) {
                float fullWidth = subTabWidth - 20.0f;
                float animatedWidth = fullWidth * underlineAnim;
                float centerX = tabMin.x + subTabWidth * 0.5f;
                float lineY = tabMax.y - 2.0f;
                float halfWidth = animatedWidth * 0.5f;
                for (int i = 0; i < (int)animatedWidth; i++) {
                    float x = centerX - halfWidth + i;
                    float distFromCenter = fabsf((float)i - halfWidth) / halfWidth;
                    float lineAlpha = (1.0f - distFromCenter * distFromCenter) * underlineAnim;
                    dl->AddLine(
                        ImVec2(x, lineY),
                        ImVec2(x + 1, lineY),
                        Colors::WithAlpha(Colors::Accent(), lineAlpha), 1.0f
                    );
                }
            }

            return clicked;
        }

        void BeginGroup(const char* name, float widthPercent, float heightPercent) {
            State& s = State::Get();
            ImDrawList* dl = s.drawList;
            Style& style = Style::Get();
            float availableWidth = s.windowSize.x - style.panelPadding * 3;
            float availableHeight = s.windowSize.y - style.headerHeight - style.footerHeight - style.panelPadding * 2;

            float groupWidth = availableWidth * widthPercent;
            float groupHeight = heightPercent > 0 ? availableHeight * heightPercent : availableHeight;

            s_LayoutState.groupWidth = groupWidth;
            s_LayoutState.groupHeight = groupHeight;
            s_LayoutState.groupStartPos = s.cursorPos;

            ImVec2 groupMin = s.cursorPos;
            ImVec2 groupMax(groupMin.x + groupWidth, groupMin.y + groupHeight);
            dl->AddRectFilled(groupMin, groupMax, Colors::PanelBg(), style.panelRounding);
            dl->AddRect(groupMin, groupMax, Colors::Border(), style.panelRounding);
            ImVec2 headerMin = groupMin;
            ImVec2 headerMax(groupMax.x, groupMin.y + style.panelHeaderHeight);
            dl->AddRectFilled(headerMin, headerMax, Colors::PanelHeader(), style.panelRounding);
            std::string lowerName = ToLower(name);
            ImVec2 textSize = g_TextRenderer.MeasureText(lowerName, g_TextFont);
            ImVec2 textPos(
                headerMin.x + (groupWidth - textSize.x) * 0.5f,
                headerMin.y + (style.panelHeaderHeight - g_TextFont.size) * 0.5f
            );
            g_TextRenderer.RenderText(dl, textPos, lowerName, Colors::TextInactive(), g_TextFont);
            s_LayoutState.groupContentY = headerMax.y + style.itemSpacing;
            s.currentGroupWidth = groupWidth;
            s.currentGroupX = groupMin.x;
        }

        void EndGroup() {
            State& s = State::Get();
            s.cursorPos.x = s_LayoutState.groupStartPos.x + s_LayoutState.groupWidth + Style::Get().panelPadding;
        }

        void BeginGroupFixed(const char* name, float width, float height) {
            State& s = State::Get();
            ImDrawList* dl = s.drawList;
            Style& style = Style::Get();

            s_LayoutState.groupWidth = width;
            s_LayoutState.groupHeight = height;
            s_LayoutState.groupStartPos = s.cursorPos;

            ImVec2 groupMin = s.cursorPos;
            ImVec2 groupMax(groupMin.x + width, groupMin.y + height);
            dl->AddRectFilled(groupMin, groupMax, Colors::PanelBg(), style.panelRounding);
            dl->AddRect(groupMin, groupMax, Colors::Border(), style.panelRounding);
            ImVec2 headerMin = groupMin;
            ImVec2 headerMax(groupMax.x, groupMin.y + style.panelHeaderHeight);
            dl->AddRectFilled(headerMin, headerMax, Colors::PanelHeader(), style.panelRounding, ImDrawFlags_RoundCornersTop);
            dl->AddRect(headerMin, headerMax, Colors::Border(), style.panelRounding, ImDrawFlags_RoundCornersTop);
            std::string lowerName = ToLower(name);
            TextFont boldFont = g_TextFont;
            boldFont.weight = FW_BOLD;
            ImVec2 textSize = g_TextRenderer.MeasureText(lowerName, boldFont);
            ImVec2 textPos(
                headerMin.x + (width - textSize.x) * 0.5f,
                headerMin.y + (style.panelHeaderHeight - boldFont.size) * 0.5f
            );
            g_TextRenderer.RenderText(dl, textPos, lowerName, Colors::TextInactive(), boldFont);
            s_LayoutState.groupContentY = headerMax.y + style.itemSpacing;
            s.currentGroupWidth = width;
            s.currentGroupX = groupMin.x;
        }

        bool Toggle(const char* label, bool* v, bool showGear, float* colorPtr) {
            State& s = State::Get();
            ImDrawList* dl = s.drawList;
            Style& style = Style::Get();

            const float itemHeight = style.toggleHeight;
            const float toggleSize = style.toggleBoxSize;
            const float gearSize = style.gearIconSize;
            const float colorSize = style.colorPreviewSize;
            const float padding = 8.0f;
            const float rightPadding = 12.0f;
            ImVec2 itemMin(s.currentGroupX + padding, s_LayoutState.groupContentY);
            ImVec2 itemMax(s.currentGroupX + s_LayoutState.groupWidth - padding, itemMin.y + itemHeight);
            s_LayoutState.groupContentY = itemMax.y + 2.0f;
            if (s_LayoutState.groupContentY > s_LayoutState.groupStartPos.y + s_LayoutState.groupHeight - padding) {
                return false;
            }

            bool isHovered = ImGui::IsMouseHoveringRect(itemMin, itemMax);
            bool clicked = false;
            bool valueChanged = false;
            static std::map<std::string, float> s_toggleHoverAnim;
            float targetAnim = isHovered ? 1.0f : 0.0f;
            float& hoverAnim = s_toggleHoverAnim[label];
            hoverAnim = ImLerp(hoverAnim, targetAnim, ImGui::GetIO().DeltaTime * 12.0f);
            if (isHovered) {
                g_Config.blockWindowDrag = true;
                if (ImGui::IsMouseDown(0)) {
                    g_Config.interactiveMouseDown = true;
                }
            }
            float rightX = itemMax.x - rightPadding;
            ImVec2 colorPreviewPos(0, 0);
            bool colorHovered = false;
            const float colorWidth = 24.0f;
            const float colorHeight = colorSize;
            if (colorPtr) {
                colorPreviewPos = ImVec2(rightX - colorWidth, itemMin.y + (itemHeight - colorHeight) * 0.5f);
                rightX -= colorWidth + 6.0f;

                colorHovered = ImGui::IsMouseHoveringRect(colorPreviewPos, ImVec2(colorPreviewPos.x + colorWidth, colorPreviewPos.y + colorHeight));

                DrawColorPreview(dl, colorPreviewPos, colorWidth, colorHeight, colorPtr);
                if (colorHovered && ImGui::IsMouseClicked(0) && !g_Config.dropdownConsumedClick && !g_Config.IsPopupBlocking()) {
                    g_Config.showColorPicker = true;
                    g_Config.activeColorEdit = colorPtr;
                    g_Config.colorPickerAnchor = ImVec2(colorPreviewPos.x + colorWidth * 0.5f, colorPreviewPos.y + colorHeight + 4.0f);
                    g_Config.colorPickerOpenFrame = ImGui::GetFrameCount();
                }
            }
            ImVec2 togglePos(rightX - toggleSize, itemMin.y + (itemHeight - toggleSize) * 0.5f);
            ImVec2 toggleEnd(togglePos.x + toggleSize, togglePos.y + toggleSize);
            rightX -= toggleSize + 6.0f;

            bool toggleHovered = ImGui::IsMouseHoveringRect(togglePos, toggleEnd);
            static std::map<std::string, float> s_checkboxAnim;
            float targetCheckAnim = *v ? 1.0f : 0.0f;
            float& checkAnim = s_checkboxAnim[label];
            checkAnim = ImLerp(checkAnim, targetCheckAnim, ImGui::GetIO().DeltaTime * 15.0f);
            ImU32 toggleOffColor = toggleHovered ? Colors::WithAlpha(Colors::ToggleOff(), 200) : Colors::ToggleOff();
            ImU32 toggleOnColor = toggleHovered ? Colors::AccentHover() : Colors::ToggleOn();
            ImU32 toggleBgColor = Colors::LerpColor(toggleOffColor, toggleOnColor, checkAnim);

            dl->AddRectFilled(togglePos, toggleEnd, toggleBgColor, 0.0f);
            dl->AddRect(togglePos, toggleEnd, Colors::Border(), 0.0f, 0, 1.0f);
            if (checkAnim > 0.01f) {
                ImVec2 checkCenter(togglePos.x + toggleSize * 0.5f, togglePos.y + toggleSize * 0.5f);
                float cs = toggleSize * 0.25f * checkAnim;
                ImVec2 points[3] = {
                    ImVec2(checkCenter.x - cs * 0.9f, checkCenter.y),
                    ImVec2(checkCenter.x - cs * 0.2f, checkCenter.y + cs * 0.7f),
                    ImVec2(checkCenter.x + cs, checkCenter.y - cs * 0.6f)
                };
                ImU32 checkColor = Colors::WithAlpha(Colors::TextActive(), checkAnim);
                dl->AddPolyline(points, 3, checkColor, ImDrawFlags_None, 1.2f);
            }
            if (toggleHovered && ImGui::IsMouseClicked(0) && !g_Config.dropdownConsumedClick && !g_Config.IsPopupBlocking()) {
                *v = !*v;
                valueChanged = true;
            }
            bool gearHovered = false;
            if (showGear) {
                ImVec2 gearMin(rightX - gearSize, itemMin.y + (itemHeight - gearSize) * 0.5f);
                ImVec2 gearMax(gearMin.x + gearSize, gearMin.y + gearSize);
                rightX -= gearSize + 6.0f;

                gearHovered = ImGui::IsMouseHoveringRect(gearMin, gearMax);
                bool hasKey = (g_Config.keybinds.find(label) != g_Config.keybinds.end() && g_Config.keybinds[label].key != 0);
                ImU32 gearColor = hasKey ? (gearHovered ? Colors::AccentHover() : Colors::Accent()) : (gearHovered ? Colors::GearIconHover() : Colors::GearIcon());
                if (g_FontAwesome) {
                    float iconFontSize = g_FontAwesome->LegacySize;
                    ImVec2 iconSize = g_FontAwesome->CalcTextSizeA(iconFontSize, FLT_MAX, 0.0f, Icons::Gear);
                    ImVec2 iconPos(gearMin.x + (gearSize - iconSize.x) * 0.5f, gearMin.y + (gearSize - iconSize.y) * 0.5f);
                    g_FontAwesome->RenderText(dl, iconFontSize, iconPos, gearColor, ImVec4(0, 0, 9999, 9999), Icons::Gear, Icons::Gear + strlen(Icons::Gear));
                } else {
                    ImVec2 gearCenter(gearMin.x + gearSize * 0.5f, gearMin.y + gearSize * 0.5f);
                    DrawGearIcon(dl, gearCenter, gearSize, gearColor);
                }
                if (gearHovered && ImGui::IsMouseClicked(0) && !g_Config.dropdownConsumedClick && !g_Config.IsPopupBlocking()) {
                    g_Config.showKeybindPopup = true;
                    g_Config.activeKeybindLabel = label;
                    g_Config.keybindPopupAnchor = ImVec2(gearMin.x + gearSize * 0.5f, gearMax.y + 4.0f);
                    g_Config.keybindPopupOpenFrame = ImGui::GetFrameCount();
                }
            }
            std::string lowerLabel = ToLower(label);
            ImU32 textColor = *v ? Colors::TextActive() : Colors::TextInactive();
            if (hoverAnim > 0.5f) {
                textColor = Colors::TextActive();
            }

            float slideOffset = 3.0f * hoverAnim;
            ImVec2 textPos(itemMin.x + padding + slideOffset, itemMin.y + (itemHeight - g_TextFont.size) * 0.5f);
            g_TextRenderer.RenderText(dl, textPos, lowerLabel, textColor, g_TextFont);
            if (isHovered && !toggleHovered && !gearHovered && !colorHovered && ImGui::IsMouseClicked(0) && !g_Config.dropdownConsumedClick && !g_Config.IsPopupBlocking()) {
                *v = !*v;
                valueChanged = true;
            }

            return valueChanged;
        }

        bool Button(const char* label) {
            State& s = State::Get();
            ImDrawList* dl = s.drawList;
            Style& style = Style::Get();

            const float itemHeight = 28.0f;
            const float padding = 8.0f;
            ImVec2 itemMin(s.currentGroupX + padding, s_LayoutState.groupContentY);
            ImVec2 itemMax(s.currentGroupX + s_LayoutState.groupWidth - padding, itemMin.y + itemHeight);
            s_LayoutState.groupContentY = itemMax.y + 4.0f;
            if (s_LayoutState.groupContentY > s_LayoutState.groupStartPos.y + s_LayoutState.groupHeight - padding) {
                return false;
            }

            bool isHovered = ImGui::IsMouseHoveringRect(itemMin, itemMax);
            bool isPressed = isHovered && ImGui::IsMouseDown(0);
            bool clicked = false;
            static std::map<std::string, float> s_buttonHoverAnim;
            static std::map<std::string, float> s_buttonPressAnim;

            float targetHover = isHovered ? 1.0f : 0.0f;
            float targetPress = isPressed ? 1.0f : 0.0f;
            float& hoverAnim = s_buttonHoverAnim[label];
            float& pressAnim = s_buttonPressAnim[label];
            hoverAnim = ImLerp(hoverAnim, targetHover, ImGui::GetIO().DeltaTime * 12.0f);
            pressAnim = ImLerp(pressAnim, targetPress, ImGui::GetIO().DeltaTime * 20.0f);
            if (isHovered) {
                g_Config.blockWindowDrag = true;
                if (ImGui::IsMouseDown(0)) {
                    g_Config.interactiveMouseDown = true;
                }
            }
            ImU32 bgColor = Colors::LerpColor(
                Colors::LerpColor(IM_COL32(30, 30, 30, (int)(255 * Colors::GlobalAlpha())),
                                  IM_COL32(40, 40, 40, (int)(255 * Colors::GlobalAlpha())), hoverAnim),
                IM_COL32(25, 25, 25, (int)(255 * Colors::GlobalAlpha())), pressAnim
            );
            ImU32 borderColor = Colors::LerpColor(Colors::Border(), Colors::Accent(), hoverAnim * 0.5f);
            dl->AddRectFilled(itemMin, itemMax, bgColor, 3.0f);
            dl->AddRect(itemMin, itemMax, borderColor, 3.0f);
            std::string lowerLabel = ToLower(label);
            ImVec2 textSize = g_TextRenderer.MeasureText(lowerLabel, g_TextFont);
            float textX = itemMin.x + (itemMax.x - itemMin.x - textSize.x) * 0.5f;
            float textY = itemMin.y + (itemHeight - g_TextFont.size) * 0.5f;
            ImU32 textColor = Colors::LerpColor(Colors::TextInactive(), Colors::TextActive(), hoverAnim);
            float pressOffset = pressAnim * 1.0f;
            g_TextRenderer.RenderText(dl, ImVec2(textX, textY + pressOffset), lowerLabel, textColor, g_TextFont);
            if (isHovered && ImGui::IsMouseClicked(0) && !g_Config.dropdownConsumedClick && !g_Config.IsPopupBlocking()) {
                clicked = true;
            }

            return clicked;
        }

        bool TextInput(const char* label, char* buffer, size_t bufferSize) {
            State& s = State::Get();
            ImDrawList* dl = s.drawList;
            Style& style = Style::Get();
            ImGuiIO& io = ImGui::GetIO();

            const float itemHeight = 28.0f;
            const float padding = 8.0f;
            const float inputWidth = 120.0f;
            const float inputHeight = 20.0f;
            ImVec2 itemMin(s.currentGroupX + padding, s_LayoutState.groupContentY);
            ImVec2 itemMax(s.currentGroupX + s_LayoutState.groupWidth - padding, itemMin.y + itemHeight);
            s_LayoutState.groupContentY = itemMax.y + 2.0f;
            if (s_LayoutState.groupContentY > s_LayoutState.groupStartPos.y + s_LayoutState.groupHeight - padding) {
                return false;
            }
            float inputX = itemMax.x - padding - inputWidth;
            float inputY = itemMin.y + (itemHeight - inputHeight) * 0.5f;
            ImVec2 inputMin(inputX, inputY);
            ImVec2 inputMax(inputX + inputWidth, inputY + inputHeight);

            bool isHovered = ImGui::IsMouseHoveringRect(itemMin, itemMax);
            bool inputHovered = ImGui::IsMouseHoveringRect(inputMin, inputMax);
            bool valueChanged = false;
            static std::map<std::string, bool> s_inputFocused;
            static std::map<std::string, float> s_cursorBlink;
            static std::map<std::string, int> s_cursorPos;
            static std::map<std::string, int> s_selectionStart;
            static std::map<std::string, bool> s_isDragging;

            bool& isFocused = s_inputFocused[label];
            float& cursorBlink = s_cursorBlink[label];
            int& cursorPos = s_cursorPos[label];
            int& selectionStart = s_selectionStart[label];
            bool& isDragging = s_isDragging[label];
            static std::map<std::string, float> s_inputHoverAnim;
            static std::map<std::string, float> s_inputFocusAnim;

            float targetHover = (isHovered || inputHovered) ? 1.0f : 0.0f;
            float targetFocus = isFocused ? 1.0f : 0.0f;
            float& hoverAnim = s_inputHoverAnim[label];
            float& focusAnim = s_inputFocusAnim[label];
            hoverAnim = ImLerp(hoverAnim, targetHover, io.DeltaTime * 12.0f);
            focusAnim = ImLerp(focusAnim, targetFocus, io.DeltaTime * 12.0f);
            float textPadding = 6.0f;
            ImVec2 textStart(inputMin.x + textPadding, inputMin.y + (inputHeight - g_TextFont.size) * 0.5f);
            auto getCursorPosFromMouse = [&](float mouseX) -> int {
                int len = (int)strlen(buffer);
                float relX = mouseX - textStart.x;
                if (relX <= 0) return 0;
                for (int i = 1; i <= len; i++) {
                    std::string sub(buffer, i);
                    ImVec2 size = g_TextRenderer.MeasureText(sub, g_TextFont);
                    if (relX < size.x) {
                        std::string prevSub(buffer, i - 1);
                        ImVec2 prevSize = g_TextRenderer.MeasureText(prevSub, g_TextFont);
                        return (relX - prevSize.x < size.x - relX) ? i - 1 : i;
                    }
                }
                return len;
            };
            if (isHovered || isFocused) {
                g_Config.blockWindowDrag = true;
                if (ImGui::IsMouseDown(0)) {
                    g_Config.interactiveMouseDown = true;
                }
            }
            if (ImGui::IsMouseClicked(0)) {
                if (inputHovered && !g_Config.dropdownConsumedClick && !g_Config.IsPopupBlocking()) {
                    isFocused = true;
                    cursorPos = getCursorPosFromMouse(io.MousePos.x);
                    selectionStart = cursorPos;
                    isDragging = true;
                    cursorBlink = 0.0f;
                } else if (!inputHovered) {
                    isFocused = false;
                    selectionStart = -1;
                    isDragging = false;
                }
            }
            if (isDragging && ImGui::IsMouseDown(0) && isFocused) {
                int newPos = getCursorPosFromMouse(io.MousePos.x);
                cursorPos = newPos;
                cursorBlink = 0.0f;
            }
            if (isDragging && !ImGui::IsMouseDown(0)) {
                isDragging = false;
                if (selectionStart == cursorPos) {
                    selectionStart = -1;
                }
            }
            auto hasSelection = [&]() -> bool {
                return selectionStart >= 0 && selectionStart != cursorPos;
            };
            auto getSelectionRange = [&]() -> std::pair<int, int> {
                int start = selectionStart < cursorPos ? selectionStart : cursorPos;
                int end = selectionStart > cursorPos ? selectionStart : cursorPos;
                return {start, end};
            };
            auto deleteSelection = [&]() {
                if (!hasSelection()) return;
                auto [start, end] = getSelectionRange();
                int len = (int)strlen(buffer);
                for (int i = start; i <= len - (end - start); i++) {
                    buffer[i] = buffer[i + (end - start)];
                }
                cursorPos = start;
                selectionStart = -1;
                valueChanged = true;
            };
            if (isFocused) {
                cursorBlink += io.DeltaTime;
                if (cursorBlink > 1.0f) cursorBlink = 0.0f;

                int len = (int)strlen(buffer);
                bool ctrlHeld = io.KeyCtrl;
                bool shiftHeld = io.KeyShift;
                if (ctrlHeld && ImGui::IsKeyPressed(ImGuiKey_A)) {
                    selectionStart = 0;
                    cursorPos = len;
                    cursorBlink = 0.0f;
                }
                for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
                    ImWchar c = io.InputQueueCharacters[i];
                    if (c >= 32 && c < 127) {
                        if (hasSelection()) {
                            deleteSelection();
                            len = (int)strlen(buffer);
                        }
                        if (len < (int)bufferSize - 1) {
                            for (int j = len; j >= cursorPos; j--) {
                                buffer[j + 1] = buffer[j];
                            }
                            buffer[cursorPos] = (char)c;
                            cursorPos++;
                            valueChanged = true;
                        }
                    }
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
                    if (hasSelection()) {
                        deleteSelection();
                    } else if (cursorPos > 0) {
                        len = (int)strlen(buffer);
                        for (int j = cursorPos - 1; j < len; j++) {
                            buffer[j] = buffer[j + 1];
                        }
                        cursorPos--;
                        valueChanged = true;
                    }
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                    if (hasSelection()) {
                        deleteSelection();
                    } else {
                        len = (int)strlen(buffer);
                        if (cursorPos < len) {
                            for (int j = cursorPos; j < len; j++) {
                                buffer[j] = buffer[j + 1];
                            }
                            valueChanged = true;
                        }
                    }
                }
                if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && cursorPos > 0) {
                    if (shiftHeld) {
                        if (selectionStart < 0) selectionStart = cursorPos;
                    } else {
                        selectionStart = -1;
                    }
                    cursorPos--;
                    cursorBlink = 0.0f;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && cursorPos < len) {
                    if (shiftHeld) {
                        if (selectionStart < 0) selectionStart = cursorPos;
                    } else {
                        selectionStart = -1;
                    }
                    cursorPos++;
                    cursorBlink = 0.0f;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
                    if (shiftHeld) {
                        if (selectionStart < 0) selectionStart = cursorPos;
                    } else {
                        selectionStart = -1;
                    }
                    cursorPos = 0;
                    cursorBlink = 0.0f;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_End)) {
                    if (shiftHeld) {
                        if (selectionStart < 0) selectionStart = cursorPos;
                    } else {
                        selectionStart = -1;
                    }
                    cursorPos = len;
                    cursorBlink = 0.0f;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    isFocused = false;
                    selectionStart = -1;
                }
            }
            std::string lowerLabel = ToLower(label);
            float slideOffset = 3.0f * hoverAnim;
            ImVec2 textPos(itemMin.x + padding + slideOffset, itemMin.y + (itemHeight - g_TextFont.size) * 0.5f);
            ImU32 labelColor = (hoverAnim > 0.5f || isFocused) ? Colors::TextActive() : Colors::TextInactive();
            g_TextRenderer.RenderText(dl, textPos, lowerLabel, labelColor, g_TextFont);
            ImU32 inputBg = Colors::LerpColor(
                IM_COL32(25, 25, 25, (int)(255 * Colors::GlobalAlpha())),
                IM_COL32(30, 30, 30, (int)(255 * Colors::GlobalAlpha())),
                hoverAnim
            );
            ImU32 inputBorder = Colors::LerpColor(Colors::Border(), Colors::Accent(), focusAnim * 0.7f);

            dl->AddRectFilled(inputMin, inputMax, inputBg, 3.0f);
            dl->AddRect(inputMin, inputMax, inputBorder, 3.0f);
            dl->PushClipRect(ImVec2(inputMin.x + textPadding, inputMin.y), ImVec2(inputMax.x - textPadding, inputMax.y), true);
            if (isFocused && hasSelection()) {
                auto [start, end] = getSelectionRange();
                std::string beforeStart(buffer, start);
                std::string beforeEnd(buffer, end);
                ImVec2 startSize = g_TextRenderer.MeasureText(beforeStart, g_TextFont);
                ImVec2 endSize = g_TextRenderer.MeasureText(beforeEnd, g_TextFont);

                ImU32 selectionColor = IM_COL32(114, 137, 218, (int)(100 * Colors::GlobalAlpha()));
                dl->AddRectFilled(
                    ImVec2(textStart.x + startSize.x, inputMin.y + 2),
                    ImVec2(textStart.x + endSize.x, inputMax.y - 2),
                    selectionColor
                );
            }
            if (strlen(buffer) > 0) {
                g_TextRenderer.RenderText(dl, textStart, buffer, Colors::TextActive(), g_TextFont);
            } else if (!isFocused) {
                g_TextRenderer.RenderText(dl, textStart, "...", Colors::TextInactive(), g_TextFont);
            }
            if (isFocused && !hasSelection() && cursorBlink < 0.5f) {
                std::string beforeCursor(buffer, cursorPos);
                ImVec2 cursorTextSize = g_TextRenderer.MeasureText(beforeCursor, g_TextFont);
                float cursorX = textStart.x + cursorTextSize.x;
                dl->AddLine(
                    ImVec2(cursorX, inputMin.y + 3),
                    ImVec2(cursorX, inputMax.y - 3),
                    Colors::TextActive(), 1.0f
                );
            }

            dl->PopClipRect();

            return valueChanged;
        }

        bool Slider(const char* label, float* value, float minVal, float maxVal, const char* format) {
            State& s = State::Get();
            ImDrawList* dl = s.drawList;
            Style& style = Style::Get();

            const float itemHeight = style.toggleHeight;
            const float padding = 8.0f;
            const float sliderHeight = 4.0f;
            const float knobRadius = 5.0f;
            const float sliderWidth = 120.0f;

            ImVec2 itemMin(s.currentGroupX + padding, s_LayoutState.groupContentY);
            ImVec2 itemMax(s.currentGroupX + s_LayoutState.groupWidth - padding, itemMin.y + itemHeight);

            s_LayoutState.groupContentY = itemMax.y + 2.0f;

            bool isHovered = ImGui::IsMouseHoveringRect(itemMin, itemMax);
            bool valueChanged = false;
            static std::map<std::string, float> s_sliderHoverAnim;
            float targetAnim = isHovered ? 1.0f : 0.0f;
            float& hoverAnim = s_sliderHoverAnim[label];
            hoverAnim = ImLerp(hoverAnim, targetAnim, ImGui::GetIO().DeltaTime * 12.0f);
            if (isHovered) {
                g_Config.blockWindowDrag = true;
                if (ImGui::IsMouseDown(0)) {
                    g_Config.interactiveMouseDown = true;
                }
            }
            std::string lowerLabel = ToLower(label);
            float slideOffset = 3.0f * hoverAnim;
            ImVec2 textPos(itemMin.x + padding + slideOffset, itemMin.y + (itemHeight - g_TextFont.size) * 0.5f);
            ImU32 labelColor = hoverAnim > 0.5f ? Colors::TextActive() : Colors::TextInactive();
            g_TextRenderer.RenderText(dl, textPos, lowerLabel, labelColor, g_TextFont);
            float sliderX = itemMax.x - padding - sliderWidth;
            float sliderY = itemMin.y + (itemHeight - sliderHeight) * 0.5f;
            ImVec2 sliderMin(sliderX, sliderY);
            ImVec2 sliderMax(sliderX + sliderWidth, sliderY + sliderHeight);
            float normalizedValue = (*value - minVal) / (maxVal - minVal);
            normalizedValue = ImClamp(normalizedValue, 0.0f, 1.0f);
            float knobX = sliderMin.x + normalizedValue * sliderWidth;
            float knobY = sliderMin.y + sliderHeight * 0.5f;
            bool sliderHovered = ImGui::IsMouseHoveringRect(
                ImVec2(sliderMin.x - knobRadius, sliderMin.y - knobRadius),
                ImVec2(sliderMax.x + knobRadius, sliderMax.y + knobRadius)
            );
            static std::map<std::string, bool> s_sliderDragging;
            bool& isDragging = s_sliderDragging[label];
            static std::map<std::string, float> s_knobAnim;
            float targetKnobAnim = (sliderHovered || isDragging) ? 1.0f : 0.0f;
            float& knobAnim = s_knobAnim[label];
            knobAnim = ImLerp(knobAnim, targetKnobAnim, ImGui::GetIO().DeltaTime * 15.0f);
            ImU32 trackBg = Colors::ToggleOff();
            dl->AddRectFilled(sliderMin, sliderMax, trackBg, sliderHeight * 0.5f);
            static std::map<std::string, float> s_normalizedAnim;
            float& animatedNormalized = s_normalizedAnim[label];
            animatedNormalized = ImLerp(animatedNormalized, normalizedValue, ImGui::GetIO().DeltaTime * 20.0f);
            if (animatedNormalized < -0.01f || animatedNormalized > 1.01f) {
                animatedNormalized = normalizedValue;
            }
            float animatedKnobX = sliderMin.x + animatedNormalized * sliderWidth;
            if (animatedKnobX > sliderMin.x) {
                ImVec2 filledMax(animatedKnobX, sliderMax.y);
                ImU32 fillColor = Colors::LerpColor(Colors::Accent(), Colors::AccentHover(), knobAnim);
                dl->AddRectFilled(sliderMin, filledMax, fillColor, sliderHeight * 0.5f);
            }
            float currentKnobRadius = knobRadius + 1.0f * knobAnim;
            ImU32 knobColor = Colors::LerpColor(Colors::Accent(), Colors::AccentHover(), knobAnim);
            ImU32 knobBorder = IM_COL32(255, 255, 255, (int)((80 + 40 * knobAnim) * Colors::GlobalAlpha()));
            dl->AddCircleFilled(ImVec2(animatedKnobX, knobY + 1.0f), currentKnobRadius, IM_COL32(0, 0, 0, (int)(60 * Colors::GlobalAlpha())));
            dl->AddCircleFilled(ImVec2(animatedKnobX, knobY), currentKnobRadius, knobColor);
            dl->AddCircle(ImVec2(animatedKnobX, knobY), currentKnobRadius, knobBorder, 0, 1.0f);
            static std::map<std::string, bool> s_sliderEditing;
            static std::map<std::string, char[32]> s_sliderEditBuffer;
            static std::map<std::string, bool> s_sliderSelectAll;
            static std::map<std::string, float> s_sliderEditAnim;
            static std::map<std::string, float> s_sliderSelectAnim;
            static std::map<std::string, float> s_sliderHoverAnim2;
            static std::map<std::string, float> s_sliderTypeAnim;
            static std::map<std::string, int> s_sliderLastLen;
            static std::string s_activeSliderEdit = "";
            bool& isEditing = s_sliderEditing[label];
            bool& isSelectAll = s_sliderSelectAll[label];
            float& editAnim = s_sliderEditAnim[label];
            float& selectAnim = s_sliderSelectAnim[label];
            float& valueHoverAnim = s_sliderHoverAnim2[label];
            float& typeAnim = s_sliderTypeAnim[label];
            int& lastLen = s_sliderLastLen[label];
            char valueText[32];
            snprintf(valueText, sizeof(valueText), format, *value);
            std::string cleanValue = valueText;
            for (size_t i = 0; i < cleanValue.size(); i++) {
                if (cleanValue[i] == '%' || (unsigned char)cleanValue[i] > 127) {
                    cleanValue = cleanValue.substr(0, i);
                    break;
                }
            }

            ImVec2 valueSize = g_TextRenderer.MeasureText(cleanValue, g_TextFont);
            float valueBoxWidth = 50.0f;
            float valueBoxHeight = 18.0f;
            ImVec2 valueBoxMin(sliderMin.x - valueBoxWidth - 10.0f, itemMin.y + (itemHeight - valueBoxHeight) * 0.5f);
            ImVec2 valueBoxMax(valueBoxMin.x + valueBoxWidth, valueBoxMin.y + valueBoxHeight);

            bool valueHovered = ImGui::IsMouseHoveringRect(valueBoxMin, valueBoxMax);
            float targetEditAnim = (isEditing && s_activeSliderEdit == label) ? 1.0f : 0.0f;
            editAnim = ImLerp(editAnim, targetEditAnim, ImGui::GetIO().DeltaTime * 15.0f);
            float targetSelectAnim = isSelectAll ? 1.0f : 0.0f;
            selectAnim = ImLerp(selectAnim, targetSelectAnim, ImGui::GetIO().DeltaTime * 12.0f);
            float targetHoverAnim = valueHovered ? 1.0f : 0.0f;
            valueHoverAnim = ImLerp(valueHoverAnim, targetHoverAnim, ImGui::GetIO().DeltaTime * 12.0f);
            float boxExpand = 2.0f * editAnim;
            ImVec2 animBoxMin(valueBoxMin.x - boxExpand, valueBoxMin.y - boxExpand * 0.5f);
            ImVec2 animBoxMax(valueBoxMax.x + boxExpand, valueBoxMax.y + boxExpand * 0.5f);
            ImU32 boxBg = Colors::LerpColor(IM_COL32(0, 0, 0, 0), Colors::PanelHeader(), editAnim);
            ImU32 boxBorder = Colors::LerpColor(
                Colors::WithAlpha(Colors::Border(), valueHoverAnim * 0.5f),
                Colors::Accent(),
                editAnim
            );

            if (editAnim > 0.01f) {
                dl->AddRectFilled(animBoxMin, animBoxMax, boxBg, 2.0f + editAnim);
                dl->AddRect(animBoxMin, animBoxMax, boxBorder, 2.0f + editAnim);
            } else if (valueHoverAnim > 0.01f) {
                dl->AddRect(valueBoxMin, valueBoxMax, Colors::WithAlpha(Colors::Border(), valueHoverAnim * 0.3f), 2.0f);
            }

            if (isEditing && s_activeSliderEdit == label) {
                char* editBuffer = s_sliderEditBuffer[label];
                ImGuiIO& io = ImGui::GetIO();
                if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A)) {
                    isSelectAll = true;
                }
                bool textChanged = false;
                for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
                    ImWchar c = io.InputQueueCharacters[i];
                    if ((c >= '0' && c <= '9') || c == '.' || c == '-') {
                        if (isSelectAll) {
                            editBuffer[0] = (char)c;
                            editBuffer[1] = '\0';
                            isSelectAll = false;
                        } else {
                            size_t len = strlen(editBuffer);
                            if (len < 15) {
                                editBuffer[len] = (char)c;
                                editBuffer[len + 1] = '\0';
                            }
                        }
                        textChanged = true;
                    }
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
                    if (isSelectAll) {
                        editBuffer[0] = '\0';
                        isSelectAll = false;
                    } else {
                        size_t len = strlen(editBuffer);
                        if (len > 0) {
                            editBuffer[len - 1] = '\0';
                        }
                    }
                    textChanged = true;
                }
                int currentLen = (int)strlen(editBuffer);
                if (textChanged || currentLen != lastLen) {
                    typeAnim = 1.0f;
                    lastLen = currentLen;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
                    float newValue = (float)atof(editBuffer);
                    newValue = ImClamp(newValue, minVal, maxVal);
                    if (newValue != *value) {
                        *value = newValue;
                        valueChanged = true;
                    }
                    isEditing = false;
                    isSelectAll = false;
                    s_activeSliderEdit = "";
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    isEditing = false;
                    isSelectAll = false;
                    s_activeSliderEdit = "";
                }
                if (ImGui::IsMouseClicked(0) && !valueHovered) {
                    float newValue = (float)atof(editBuffer);
                    newValue = ImClamp(newValue, minVal, maxVal);
                    if (newValue != *value) {
                        *value = newValue;
                        valueChanged = true;
                    }
                    isEditing = false;
                    isSelectAll = false;
                    s_activeSliderEdit = "";
                }
                typeAnim = ImLerp(typeAnim, 0.0f, ImGui::GetIO().DeltaTime * 12.0f);
                std::string editStr = editBuffer;
                ImVec2 editSize = g_TextRenderer.MeasureText(editStr, g_TextFont);
                float typeBounce = sinf(typeAnim * 3.14159f) * 2.0f;

                ImVec2 editPos(animBoxMin.x + ((animBoxMax.x - animBoxMin.x) - editSize.x) * 0.5f,
                              animBoxMin.y + ((animBoxMax.y - animBoxMin.y) - g_TextFont.size) * 0.5f - typeBounce);
                if (selectAnim > 0.01f && strlen(editBuffer) > 0) {
                    float selectWidth = editSize.x * selectAnim;
                    float selectCenterX = editPos.x + editSize.x * 0.5f;
                    dl->AddRectFilled(
                        ImVec2(selectCenterX - selectWidth * 0.5f - 2, animBoxMin.y + 3),
                        ImVec2(selectCenterX + selectWidth * 0.5f + 2, animBoxMax.y - 3),
                        Colors::WithAlpha(Colors::Accent(), 0.4f * selectAnim), 2.0f
                    );
                }
                float typePulse = 1.0f + typeAnim * 0.15f;
                ImU32 textColor = Colors::LerpColor(
                    Colors::TextActive(),
                    Colors::Accent(),
                    typeAnim * 0.3f
                );
                g_TextRenderer.RenderText(dl, editPos, editStr, textColor, g_TextFont);
                g_Config.blockWindowDrag = true;
            } else {
                ImU32 textColor = Colors::LerpColor(Colors::TextDim(), Colors::TextActive(), valueHoverAnim);
                ImVec2 valuePos(valueBoxMin.x + (valueBoxWidth - valueSize.x) * 0.5f, valueBoxMin.y + (valueBoxHeight - g_TextFont.size) * 0.5f);
                g_TextRenderer.RenderText(dl, valuePos, cleanValue, textColor, g_TextFont);
                if (valueHovered && ImGui::IsMouseClicked(0) && !g_Config.IsPopupBlocking()) {
                    isEditing = true;
                    isSelectAll = true;
                    s_activeSliderEdit = label;
                    snprintf(s_sliderEditBuffer[label], 32, "%.2f", *value);
                }
            }
            if (!isEditing && sliderHovered && ImGui::IsMouseClicked(0) && !g_Config.IsPopupBlocking()) {
                isDragging = true;
            }

            if (isDragging) {
                if (ImGui::IsMouseDown(0)) {
                    float mouseX = ImGui::GetIO().MousePos.x;
                    float newNormalized = (mouseX - sliderMin.x) / sliderWidth;
                    newNormalized = ImClamp(newNormalized, 0.0f, 1.0f);
                    float newValue = minVal + newNormalized * (maxVal - minVal);
                    if (newValue != *value) {
                        *value = newValue;
                        valueChanged = true;
                    }
                } else {
                    isDragging = false;
                }
            }

            return valueChanged;
        }

        bool Dropdown(const char* label, const char* preview, bool* open) {
            State& s = State::Get();
            ImDrawList* dl = s.drawList;
            Style& style = Style::Get();

            const float itemHeight = style.toggleHeight;
            const float padding = 8.0f;

            ImVec2 itemMin(s.currentGroupX + padding, s_LayoutState.groupContentY);
            ImVec2 itemMax(s.currentGroupX + s_LayoutState.groupWidth - padding, itemMin.y + itemHeight);

            s_LayoutState.groupContentY = itemMax.y + 2.0f;

            bool isHovered = ImGui::IsMouseHoveringRect(itemMin, itemMax);
            static std::map<std::string, float> s_dropdownHoverAnim;
            float targetAnim = isHovered ? 1.0f : 0.0f;
            float& hoverAnim = s_dropdownHoverAnim[label];
            hoverAnim = ImLerp(hoverAnim, targetAnim, ImGui::GetIO().DeltaTime * 12.0f);
            if (isHovered) {
                g_Config.blockWindowDrag = true;
                if (ImGui::IsMouseDown(0)) {
                    g_Config.interactiveMouseDown = true;
                }
            }
            std::string lowerLabel = ToLower(label);
            float slideOffset = 3.0f * hoverAnim;
            ImVec2 textPos(itemMin.x + padding + slideOffset, itemMin.y + (itemHeight - g_TextFont.size) * 0.5f);
            ImU32 labelColor = hoverAnim > 0.5f ? Colors::TextActive() : Colors::TextInactive();
            g_TextRenderer.RenderText(dl, textPos, lowerLabel, labelColor, g_TextFont);
            std::string lowerPreview = ToLower(preview);
            ImVec2 previewSize = g_TextRenderer.MeasureText(lowerPreview, g_TextFont);
            ImVec2 previewPos(itemMax.x - padding - previewSize.x - 20.0f, itemMin.y + (itemHeight - g_TextFont.size) * 0.5f);
            g_TextRenderer.RenderText(dl, previewPos, lowerPreview, Colors::TextDim(), g_TextFont);
            ImVec2 arrowCenter(itemMax.x - padding - 8.0f, itemMin.y + itemHeight * 0.5f);
            float arrowSize = 4.0f;
            dl->AddTriangleFilled(
                ImVec2(arrowCenter.x - arrowSize, arrowCenter.y - arrowSize * 0.5f),
                ImVec2(arrowCenter.x + arrowSize, arrowCenter.y - arrowSize * 0.5f),
                ImVec2(arrowCenter.x, arrowCenter.y + arrowSize * 0.5f),
                Colors::TextInactive()
            );

            if (isHovered && ImGui::IsMouseClicked(0) && !g_Config.IsPopupBlocking()) {
                *open = !*open;
                return true;
            }

            return false;
        }

        bool MultiSelectDropdown(const char* label, int* flagsValue, bool* open) {
            State& s = State::Get();
            ImDrawList* dl = s.drawList;
            Style& style = Style::Get();

            const float itemHeight = style.toggleHeight;
            const float padding = 8.0f;
            struct FlagOption {
                const char* name;
                int bit;
            };
            static const FlagOption options[] = {
                { "armor",      1 << 0 },
                { "scoped",     1 << 1 },
                { "flashed",    1 << 2 },
                { "defusing",   1 << 3 },
                { "planting",   1 << 4 },
                { "reloading",  1 << 5 },
                { "money",      1 << 6 },
                { "distance",   1 << 7 }
            };
            static const int optionCount = sizeof(options) / sizeof(options[0]);
            std::string preview;
            int selectedCount = 0;
            for (int i = 0; i < optionCount; i++) {
                if (*flagsValue & options[i].bit) {
                    if (!preview.empty()) preview += ", ";
                    preview += options[i].name;
                    selectedCount++;
                }
            }
            if (preview.empty()) preview = "none";
            float maxPreviewWidth = s_LayoutState.groupWidth * 0.4f;
            ImVec2 previewSize = ImGui::CalcTextSize(preview.c_str());
            if (previewSize.x > maxPreviewWidth && selectedCount > 2) {
                char buf[64];
                snprintf(buf, sizeof(buf), "%d selected", selectedCount);
                preview = buf;
            }

            ImVec2 itemMin(s.currentGroupX + padding, s_LayoutState.groupContentY);
            ImVec2 itemMax(s.currentGroupX + s_LayoutState.groupWidth - padding, itemMin.y + itemHeight);

            s_LayoutState.groupContentY = itemMax.y + 2.0f;

            bool isHovered = ImGui::IsMouseHoveringRect(itemMin, itemMax);
            bool valueChanged = false;
            static std::map<std::string, float> s_multiSelectHoverAnim;
            float targetAnim = isHovered ? 1.0f : 0.0f;
            float& hoverAnim = s_multiSelectHoverAnim[label];
            hoverAnim = ImLerp(hoverAnim, targetAnim, ImGui::GetIO().DeltaTime * 12.0f);
            if (isHovered) {
                g_Config.blockWindowDrag = true;
                if (ImGui::IsMouseDown(0)) {
                    g_Config.interactiveMouseDown = true;
                }
            }
            std::string lowerLabel = ToLower(label);
            float slideOffset = 3.0f * hoverAnim;
            ImVec2 textPos(itemMin.x + padding + slideOffset, itemMin.y + (itemHeight - g_TextFont.size) * 0.5f);
            ImU32 labelColor = hoverAnim > 0.5f ? Colors::TextActive() : Colors::TextInactive();
            g_TextRenderer.RenderText(dl, textPos, lowerLabel, labelColor, g_TextFont);
            ImVec2 dwPreviewSize = g_TextRenderer.MeasureText(preview, g_TextFont);
            ImVec2 previewPos(itemMax.x - padding - dwPreviewSize.x - 20.0f, itemMin.y + (itemHeight - g_TextFont.size) * 0.5f);
            g_TextRenderer.RenderText(dl, previewPos, preview, Colors::TextDim(), g_TextFont);
            ImVec2 arrowCenter(itemMax.x - padding - 8.0f, itemMin.y + itemHeight * 0.5f);
            float arrowSize = 4.0f;
            if (*open) {
                dl->AddTriangleFilled(
                    ImVec2(arrowCenter.x - arrowSize, arrowCenter.y + arrowSize * 0.5f),
                    ImVec2(arrowCenter.x + arrowSize, arrowCenter.y + arrowSize * 0.5f),
                    ImVec2(arrowCenter.x, arrowCenter.y - arrowSize * 0.5f),
                    Colors::Accent()
                );
            } else {
                dl->AddTriangleFilled(
                    ImVec2(arrowCenter.x - arrowSize, arrowCenter.y - arrowSize * 0.5f),
                    ImVec2(arrowCenter.x + arrowSize, arrowCenter.y - arrowSize * 0.5f),
                    ImVec2(arrowCenter.x, arrowCenter.y + arrowSize * 0.5f),
                    Colors::TextInactive()
                );
            }
            if (isHovered && ImGui::IsMouseClicked(0) && !g_Config.IsPopupBlocking()) {
                *open = !*open;
            }
            static std::map<std::string, ImVec2> s_lockedDropdownOffset;
            static std::map<std::string, bool> s_wasOpen;
            static std::map<std::string, float> s_dropdownExpandAnim;
            float targetExpandAnim = *open ? 1.0f : 0.0f;
            float& expandAnim = s_dropdownExpandAnim[label];
            expandAnim = ImLerp(expandAnim, targetExpandAnim, ImGui::GetIO().DeltaTime * 12.0f);
            if (expandAnim > 0.01f) {
                g_Config.anyDropdownOpen = true;

                std::string posKey = label;
                bool wasOpenLastFrame = s_wasOpen[posKey];
                if (*open && !wasOpenLastFrame) {
                    s_lockedDropdownOffset[posKey] = ImVec2(
                        itemMin.x - s.windowPos.x,
                        s_LayoutState.groupContentY - s.windowPos.y
                    );
                }
                if (*open) s_wasOpen[posKey] = true;
                ImVec2 lockedOffset = s_lockedDropdownOffset[posKey];
                float dropdownX = s.windowPos.x + lockedOffset.x;
                float dropdownY = s.windowPos.y + lockedOffset.y;
                float optionHeight = 24.0f;
                float fullHeight = optionCount * optionHeight + 4.0f;
                float animatedHeight = fullHeight * expandAnim;
                float alpha = expandAnim;
                ImDrawList* fgDl = ImGui::GetForegroundDrawList();
                ImVec2 dropdownMin(dropdownX, dropdownY);
                ImVec2 dropdownMax(dropdownX + (itemMax.x - itemMin.x), dropdownY + animatedHeight);
                fgDl->AddRectFilled(dropdownMin, dropdownMax, Colors::WithAlpha(Colors::PanelHeader(), alpha), 3.0f);
                fgDl->AddRect(dropdownMin, dropdownMax, Colors::WithAlpha(Colors::Border(), alpha), 3.0f);
                for (int i = 0; i < optionCount; i++) {
                    ImVec2 optMin(dropdownMin.x + 2.0f, dropdownY + 2.0f + i * optionHeight);
                    ImVec2 optMax(dropdownMax.x - 2.0f, optMin.y + optionHeight);
                    float optionVisibility = 1.0f;
                    if (optMax.y > dropdownMax.y) {
                        float visiblePortion = (dropdownMax.y - optMin.y) / optionHeight;
                        optionVisibility = ImClamp(visiblePortion, 0.0f, 1.0f);
                    }
                    float optAlpha = alpha * optionVisibility;
                    if (optAlpha < 0.01f) continue;
                    bool optHovered = ImGui::IsMouseHoveringRect(optMin, optMax, false);
                    bool isSelected = (*flagsValue & options[i].bit) != 0;
                    if (optHovered && *open) {
                        fgDl->AddRectFilled(optMin, optMax, Colors::WithAlpha(Colors::Accent(), 0.15f * optAlpha), 2.0f);
                    }
                    float checkSize = 14.0f;
                    ImVec2 checkMin(optMin.x + 6.0f, optMin.y + (optionHeight - checkSize) * 0.5f);
                    ImVec2 checkMax(checkMin.x + checkSize, checkMin.y + checkSize);
                    static std::map<std::string, float> s_multiCheckAnim;
                    std::string animKey = std::string(label) + "_" + std::to_string(i);
                    float targetCheckAnim = isSelected ? 1.0f : 0.0f;
                    float& checkAnim = s_multiCheckAnim[animKey];
                    checkAnim = ImLerp(checkAnim, targetCheckAnim, ImGui::GetIO().DeltaTime * 15.0f);
                    ImU32 checkBg = Colors::WithAlpha(Colors::LerpColor(Colors::ToggleOff(), Colors::ToggleOn(), checkAnim), optAlpha);
                    fgDl->AddRectFilled(checkMin, checkMax, checkBg, 0.0f);
                    fgDl->AddRect(checkMin, checkMax, Colors::WithAlpha(Colors::Border(), optAlpha), 0.0f, 0, 1.0f);
                    if (checkAnim > 0.01f) {
                        ImVec2 checkCenter(checkMin.x + checkSize * 0.5f, checkMin.y + checkSize * 0.5f);
                        float cs = checkSize * 0.25f * checkAnim;
                        ImU32 checkColor = Colors::WithAlpha(Colors::TextActive(), checkAnim * optAlpha);
                        fgDl->AddLine(
                            ImVec2(checkCenter.x - cs, checkCenter.y),
                            ImVec2(checkCenter.x - cs * 0.2f, checkCenter.y + cs * 0.6f),
                            checkColor, 1.5f
                        );
                        fgDl->AddLine(
                            ImVec2(checkCenter.x - cs * 0.2f, checkCenter.y + cs * 0.6f),
                            ImVec2(checkCenter.x + cs, checkCenter.y - cs * 0.5f),
                            checkColor, 1.5f
                        );
                    }
                    ImVec2 labelPos(checkMax.x + 8.0f, optMin.y + (optionHeight - g_TextFont.size) * 0.5f);
                    ImU32 labelColor = Colors::WithAlpha(isSelected ? Colors::TextActive() : Colors::TextInactive(), optAlpha);
                    g_TextRenderer.RenderText(fgDl, labelPos, options[i].name, labelColor, g_TextFont);
                    if (optHovered && ImGui::IsMouseClicked(0) && !g_Config.IsPopupBlocking() && *open) {
                        *flagsValue ^= options[i].bit;
                        valueChanged = true;
                        g_Config.dropdownConsumedClick = true;
                    }
                }
                if (*open) {
                    s_LayoutState.groupContentY = dropdownY + fullHeight + 4.0f;
                }
                ImVec2 fullDropdownMax(dropdownMin.x + (itemMax.x - itemMin.x), dropdownY + fullHeight);
                bool isOverDropdown = ImGui::IsMouseHoveringRect(itemMin, fullDropdownMax, false);
                if (isOverDropdown && *open) {
                    g_Config.dropdownConsumedClick = true;
                    g_Config.blockWindowDrag = true;
                    if (ImGui::IsMouseDown(0)) {
                        g_Config.interactiveMouseDown = true;
                    }
                }
                if (ImGui::IsMouseClicked(0) && !isOverDropdown && *open) {
                    *open = false;
                }
            }
            if (!*open && expandAnim < 0.01f) {
                s_wasOpen[label] = false;
            }

            return valueChanged;
        }

        void ColorPickerPopup(float* colorPtr) {
            static float* s_lastColorPtr = nullptr;
            if (colorPtr) s_lastColorPtr = colorPtr;
            if (!g_Config.menuOpen) {
                g_Config.showColorPicker = false;
            }
            float targetAnim = g_Config.showColorPicker ? 1.0f : 0.0f;
            g_Config.colorPickerAnim = ImLerp(g_Config.colorPickerAnim, targetAnim, ImGui::GetIO().DeltaTime * 12.0f);
            float* usePtr = colorPtr ? colorPtr : s_lastColorPtr;
            if (!usePtr || g_Config.colorPickerAnim < 0.01f) return;
            colorPtr = usePtr;
            float slideOffset = (1.0f - g_Config.colorPickerAnim) * -10.0f;
            ImVec2 animatedPos(g_Config.colorPickerAnchor.x, g_Config.colorPickerAnchor.y + slideOffset);
            ImGui::SetNextWindowPos(animatedPos, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
            if (g_Config.colorPickerAnim > 0.01f) {
                ImGui::SetNextWindowFocus();
            }
            float alpha = g_Config.colorPickerAnim * g_Config.menuOpenAnim;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.16f, 0.16f, 0.16f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.447f, 0.537f, 0.855f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.527f, 0.617f, 0.935f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.85f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.447f, 0.537f, 0.855f, 0.4f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.447f, 0.537f, 0.855f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.447f, 0.537f, 0.855f, 0.8f));

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 14));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 3.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 8));
            ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 16.0f);

            bool isPickerHovered = false;
            bool isPickerItemActive = false;

            if (ImGui::Begin("##ColorPicker", &g_Config.showColorPicker,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::SetNextItemWidth(200.0f);
                ImGui::ColorPicker4("##picker", colorPtr,
                    ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview |
                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueBar |
                    ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs |
                    ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoOptions);
                isPickerHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
                isPickerItemActive = ImGui::IsAnyItemActive();
            }
            ImGui::End();

            ImGui::PopStyleVar(9);
            ImGui::PopStyleColor(16);
            bool justOpened = (ImGui::GetFrameCount() - g_Config.colorPickerOpenFrame) < 2;
            if (!justOpened && ImGui::IsMouseClicked(0) && !isPickerHovered && !isPickerItemActive) {
                g_Config.showColorPicker = false;
                g_Config.activeColorEdit = nullptr;
                g_Config.colorPickerOpenFrame = -1;
            }
        }

        void KeybindPopup() {
            static std::string s_lastLabel;
            if (!g_Config.activeKeybindLabel.empty()) s_lastLabel = g_Config.activeKeybindLabel;
            if (!g_Config.menuOpen) {
                g_Config.showKeybindPopup = false;
                g_Config.isListeningForKey = false;
            }
            float targetAnim = g_Config.showKeybindPopup ? 1.0f : 0.0f;
            g_Config.keybindPopupAnim = ImLerp(g_Config.keybindPopupAnim, targetAnim, ImGui::GetIO().DeltaTime * 12.0f);
            if (s_lastLabel.empty() || g_Config.keybindPopupAnim < 0.01f) return;
            float slideOffset = (1.0f - g_Config.keybindPopupAnim) * -15.0f;
            ImVec2 animatedPos(g_Config.keybindPopupAnchor.x, g_Config.keybindPopupAnchor.y + slideOffset);
            ImGui::SetNextWindowPos(animatedPos, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
            if (g_Config.keybindPopupAnim > 0.01f) {
                ImGui::SetNextWindowFocus();
            }

            static bool s_typeDropdownOpen = false;
            static int s_dropdownOpenFrame = -1;
            static ImVec2 s_dropdownButtonPos;
            std::string& useLabel = g_Config.activeKeybindLabel.empty() ? s_lastLabel : g_Config.activeKeybindLabel;
            Keybind& kb = g_Config.keybinds[useLabel];
            float alpha = g_Config.keybindPopupAnim * g_Config.menuOpenAnim;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::ToVec4(Colors::WithAlpha(Colors::PanelBg(), alpha)));
            ImGui::PushStyleColor(ImGuiCol_Border, Colors::ToVec4(Colors::WithAlpha(Colors::Border(), alpha)));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, Colors::ToVec4(Colors::WithAlpha(Colors::ToggleOff(), alpha)));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Colors::ToVec4(Colors::WithAlpha(Colors::PanelHeader(), alpha)));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Colors::ToVec4(Colors::WithAlpha(Colors::AccentDim(), alpha)));
            ImGui::PushStyleColor(ImGuiCol_Button, Colors::ToVec4(Colors::WithAlpha(Colors::ToggleOff(), alpha)));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::ToVec4(Colors::WithAlpha(Colors::PanelHeader(), alpha)));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Colors::ToVec4(Colors::WithAlpha(Colors::AccentDim(), alpha)));
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::ToVec4(Colors::WithAlpha(Colors::TextActive(), alpha)));
            ImGui::PushStyleColor(ImGuiCol_PopupBg, Colors::ToVec4(Colors::WithAlpha(Colors::PanelBg(), alpha)));
            ImGui::PushStyleColor(ImGuiCol_Header, Colors::ToVec4(Colors::WithAlpha(Colors::AccentDim(), 0.5f * alpha)));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Colors::ToVec4(Colors::WithAlpha(Colors::AccentDim(), 0.8f * alpha)));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, Colors::ToVec4(Colors::WithAlpha(Colors::Accent(), alpha)));

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 14));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 8));

            bool isPopupHovered = false;

            if (ImGui::Begin("##KeybindPopup", &g_Config.showKeybindPopup,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextColored(Colors::ToVec4(Colors::WithAlpha(Colors::TextInactive(), alpha)), "Key");
                ImGui::SameLine(100.0f);
                const char* keyText = g_Config.isListeningForKey ? "Press a key..." : kb.GetKeyName();
                ImGui::SetNextItemWidth(120.0f);
                if (g_Config.isListeningForKey) {
                    ImGui::PushStyleColor(ImGuiCol_Button, Colors::ToVec4(Colors::WithAlpha(Colors::AccentDim(), alpha)));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::ToVec4(Colors::WithAlpha(Colors::Accent(), alpha)));
                    ImGui::PushStyleColor(ImGuiCol_Text, Colors::ToVec4(Colors::WithAlpha(Colors::TextActive(), alpha)));
                }
                if (ImGui::Button(keyText, ImVec2(120.0f, 0))) {
                    g_Config.isListeningForKey = true;
                }
                if (g_Config.isListeningForKey) {
                    ImGui::PopStyleColor(3);
                }
                if (g_Config.isListeningForKey) {
                    for (int i = 0; i < 5; i++) {
                        int vk[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2 };
                        if (GetAsyncKeyState(vk[i]) & 0x8000) {
                            kb.key = vk[i];
                            g_Config.isListeningForKey = false;
                            break;
                        }
                    }
                    if (g_Config.isListeningForKey) {
                        for (int vk = 0x08; vk <= 0xFE; vk++) {
                            if (vk >= VK_LBUTTON && vk <= VK_XBUTTON2) continue;
                            if (vk == VK_LWIN || vk == VK_RWIN) continue;

                            if (GetAsyncKeyState(vk) & 0x8000) {
                                if (vk == VK_ESCAPE) {
                                    kb.key = 0;
                                } else {
                                    kb.key = vk;
                                }
                                g_Config.isListeningForKey = false;
                                break;
                            }
                        }
                    }
                }
                ImGui::TextColored(Colors::ToVec4(Colors::WithAlpha(Colors::TextInactive(), alpha)), "Type");
                ImGui::SameLine(100.0f);
                const float dropdownWidth = 120.0f;
                const float dropdownHeight = 24.0f;

                ImVec2 typeButtonPos = ImGui::GetCursorScreenPos();
                ImVec2 typeButtonMin = typeButtonPos;
                ImVec2 typeButtonMax(typeButtonMin.x + dropdownWidth, typeButtonMin.y + dropdownHeight);

                ImDrawList* dl = ImGui::GetWindowDrawList();
                bool typeButtonHovered = ImGui::IsMouseHoveringRect(typeButtonMin, typeButtonMax);
                ImU32 buttonBg = typeButtonHovered ? Colors::WithAlpha(Colors::PanelHeader(), alpha) : Colors::WithAlpha(Colors::ToggleOff(), alpha);
                ImU32 buttonBorder = typeButtonHovered ? Colors::WithAlpha(Colors::Accent(), alpha) : Colors::WithAlpha(Colors::Border(), alpha);
                dl->AddRectFilled(typeButtonMin, typeButtonMax, buttonBg, 3.0f);
                dl->AddRect(typeButtonMin, typeButtonMax, buttonBorder, 3.0f);
                const char* typeText = Keybind::GetTypeName(kb.type);
                ImVec2 textSize = ImGui::CalcTextSize(typeText);
                ImVec2 textPos(typeButtonMin.x + 8.0f, typeButtonMin.y + (dropdownHeight - textSize.y) * 0.5f);
                dl->AddText(textPos, Colors::WithAlpha(Colors::TextActive(), alpha), typeText);
                ImVec2 arrowCenter(typeButtonMax.x - 12.0f, typeButtonMin.y + dropdownHeight * 0.5f);
                float arrowSize = 4.0f;
                if (s_typeDropdownOpen) {
                    dl->AddTriangleFilled(
                        ImVec2(arrowCenter.x - arrowSize, arrowCenter.y + arrowSize * 0.5f),
                        ImVec2(arrowCenter.x + arrowSize, arrowCenter.y + arrowSize * 0.5f),
                        ImVec2(arrowCenter.x, arrowCenter.y - arrowSize * 0.5f),
                        Colors::WithAlpha(Colors::Accent(), alpha)
                    );
                } else {
                    dl->AddTriangleFilled(
                        ImVec2(arrowCenter.x - arrowSize, arrowCenter.y - arrowSize * 0.5f),
                        ImVec2(arrowCenter.x + arrowSize, arrowCenter.y - arrowSize * 0.5f),
                        ImVec2(arrowCenter.x, arrowCenter.y + arrowSize * 0.5f),
                        Colors::WithAlpha(Colors::TextInactive(), alpha)
                    );
                }
                if (typeButtonHovered && ImGui::IsMouseClicked(0)) {
                    s_typeDropdownOpen = !s_typeDropdownOpen;
                    if (s_typeDropdownOpen) {
                        s_dropdownOpenFrame = ImGui::GetFrameCount();
                    }
                }
                s_dropdownButtonPos = ImVec2(typeButtonMin.x, typeButtonMax.y + 2.0f);
                ImGui::Dummy(ImVec2(dropdownWidth, dropdownHeight));

                isPopupHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
                                                        ImGuiHoveredFlags_ChildWindows |
                                                        ImGuiHoveredFlags_AllowWhenBlockedByPopup);

            }
            ImGui::End();

            ImGui::PopStyleVar(7);
            ImGui::PopStyleColor(13);
            bool dropdownHovered = false;
            static float s_typeDropdownExpandAnim = 0.0f;
            float targetDropdownAnim = s_typeDropdownOpen ? 1.0f : 0.0f;
            s_typeDropdownExpandAnim = ImLerp(s_typeDropdownExpandAnim, targetDropdownAnim, ImGui::GetIO().DeltaTime * 12.0f);

            if (s_typeDropdownExpandAnim > 0.01f && alpha > 0.01f) {
                ImDrawList* fgDl = ImGui::GetForegroundDrawList();
                const float optionHeight = 24.0f;
                const float dropdownWidth = 120.0f;
                const int numOptions = 3;
                ImVec2 dropdownMin = s_dropdownButtonPos;
                float fullHeight = numOptions * optionHeight + 4.0f;
                float animatedHeight = fullHeight * s_typeDropdownExpandAnim;
                float dropdownAlpha = alpha * s_typeDropdownExpandAnim;
                ImVec2 dropdownMax(dropdownMin.x + dropdownWidth, dropdownMin.y + animatedHeight);
                ImU32 dropdownBg = Colors::WithAlpha(Colors::PanelBg(), dropdownAlpha);
                ImU32 dropdownBorder = Colors::WithAlpha(Colors::Border(), dropdownAlpha);
                fgDl->AddRectFilled(dropdownMin, dropdownMax, dropdownBg, 3.0f);
                fgDl->AddRect(dropdownMin, dropdownMax, dropdownBorder, 3.0f);

                const char* typeNames[] = { "Hold", "Toggle", "Always" };
                KeybindType types[] = { KeybindType::Hold, KeybindType::Toggle, KeybindType::Always };
                Keybind& kb = g_Config.keybinds[useLabel];

                for (int i = 0; i < numOptions; i++) {
                    ImVec2 optMin(dropdownMin.x + 2.0f, dropdownMin.y + 2.0f + i * optionHeight);
                    ImVec2 optMax(dropdownMax.x - 2.0f, optMin.y + optionHeight);
                    float optionVisibility = 1.0f;
                    if (optMax.y > dropdownMax.y) {
                        float visiblePortion = (dropdownMax.y - optMin.y) / optionHeight;
                        optionVisibility = ImClamp(visiblePortion, 0.0f, 1.0f);
                    }
                    float optAlpha = dropdownAlpha * optionVisibility;

                    if (optAlpha < 0.01f) continue;
                    bool optHovered = ImGui::IsMouseHoveringRect(optMin, optMax, false);
                    bool isSelected = kb.type == types[i];
                    static std::map<int, float> s_hoverAnim;
                    float targetAnim = optHovered ? 1.0f : 0.0f;
                    float& currentAnim = s_hoverAnim[i];
                    currentAnim = ImLerp(currentAnim, targetAnim, ImGui::GetIO().DeltaTime * 10.0f);

                    if (optHovered) dropdownHovered = true;
                    if (currentAnim > 0.01f) {
                        fgDl->AddRectFilled(optMin, optMax, Colors::WithAlpha(Colors::Accent(), 0.15f * currentAnim * optAlpha), 2.0f);
                    }
                    float slideOffset = 4.0f * currentAnim;
                    ImVec2 labelPos(optMin.x + 10.0f + slideOffset, optMin.y + (optionHeight - ImGui::GetFontSize()) * 0.5f);
                    ImU32 textColor = isSelected ? Colors::WithAlpha(Colors::Accent(), optAlpha) : Colors::WithAlpha(Colors::TextInactive(), optAlpha);
                    fgDl->AddText(labelPos, textColor, typeNames[i]);
                    if (optHovered && ImGui::IsMouseClicked(0) && s_typeDropdownOpen) {
                        kb.type = types[i];
                        s_typeDropdownOpen = false;
                        g_Config.dropdownConsumedClick = true;
                    }
                }
                if (dropdownHovered) {
                    g_Config.dropdownConsumedClick = true;
                    g_Config.blockWindowDrag = true;
                    if (ImGui::IsMouseDown(0)) {
                        g_Config.interactiveMouseDown = true;
                    }
                }
                bool dropdownJustOpened = (ImGui::GetFrameCount() - s_dropdownOpenFrame) < 2;
                if (!dropdownJustOpened && ImGui::IsMouseClicked(0) && !dropdownHovered) {
                    s_typeDropdownOpen = false;
                }
            }
            bool justOpened2 = (ImGui::GetFrameCount() - g_Config.keybindPopupOpenFrame) < 2;
            if (!justOpened2 && ImGui::IsMouseClicked(0) && !isPopupHovered && !g_Config.isListeningForKey && !dropdownHovered && !s_typeDropdownOpen) {
                g_Config.showKeybindPopup = false;
                g_Config.activeKeybindLabel.clear();
                g_Config.isListeningForKey = false;
                g_Config.keybindPopupOpenFrame = -1;
            }
        }

        void RenderFooter() {
        }

        void Render() {
            ImGuiIO& io = ImGui::GetIO();
            static bool insertKeyWasDown = false;
            bool insertKeyDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
            if (insertKeyDown && !insertKeyWasDown) {
                g_Config.menuOpen = !g_Config.menuOpen;
            }
            insertKeyWasDown = insertKeyDown;
            float targetAnim = g_Config.menuOpen ? 1.0f : 0.0f;
            g_Config.menuOpenAnim = ImLerp(g_Config.menuOpenAnim, targetAnim, io.DeltaTime * 12.0f);
            if (g_Config.menuOpenAnim < 0.01f && !g_Config.menuOpen) {
                Colors::GlobalAlpha() = 1.0f;
                return;
            }
            g_Config.anyDropdownOpenLastFrame = g_Config.anyDropdownOpen;
            g_Config.dropdownConsumedClick = false;
            g_Config.blockWindowDrag = false;
            g_Config.anyDropdownOpen = false;
            if (!ImGui::IsMouseDown(0)) {
                g_Config.interactiveMouseDown = false;
            }
            ProcessKeybinds();
            Style& style = Style::Get();
            const ImVec2 menuSize(960.0f, 700.0f);
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(menuSize);
            g_Config.menuOpenAnim = 1.0f;
            g_Config.menuOpen = true;
            Colors::GlobalAlpha() = 1.0f;

            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::ToVec4(Colors::BackgroundDark()));
            ImGui::PushStyleColor(ImGuiCol_Border, Colors::ToVec4(Colors::Border()));
            ImGui::Begin("##KrxSlaxyMenu", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoCollapse |
                ((g_Config.showColorPicker || g_Config.showKeybindPopup) ? ImGuiWindowFlags_NoBringToFrontOnFocus : 0) |
                ImGuiWindowFlags_NoMove);
            BeginFrame(ImGui::GetWindowPos(), ImGui::GetWindowSize());
            {
                State& bgState = State::Get();
                ImDrawList* bgDl = bgState.drawList;
                ImVec2 bgMin = bgState.windowPos;
                ImVec2 bgMax(bgMin.x + bgState.windowSize.x, bgMin.y + bgState.windowSize.y);

                bgDl->AddRectFilled(bgMin, bgMax, Colors::BackgroundDark(), 8.0f);
            }
            RenderHeader("krxslaxy");
            Tab("Aimbot", "", 0, &g_Config.activeMainTab);
            Tab("Visuals", "", 1, &g_Config.activeMainTab);
            Tab("Misc", "", 2, &g_Config.activeMainTab);
            Tab("Config", "", 3, &g_Config.activeMainTab);
            State& s = State::Get();
            float contentStartY = s.windowPos.y + style.headerHeight + style.panelPadding;
            float contentWidth = s.windowSize.x;
            float footerSpace = 0.0f;
            float contentHeight = s.windowSize.y - style.headerHeight - footerSpace - style.panelPadding * 2;
            g_Config.tabTransitionAnim = 1.0f;
            g_Config.subTabTransitionAnim = 1.0f;
            float tabSlideOffset = 0.0f;
            float tabAlpha = 1.0f;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tabAlpha);
            if (g_Config.activeMainTab == 0) {
                float totalPadding = style.panelPadding * 3;
                float availableWidth = s.windowSize.x - totalPadding;
                float columnWidth = availableWidth * 0.5f;
                float leftColumnX = s.windowPos.x + style.panelPadding + tabSlideOffset;
                float rightColumnX = leftColumnX + columnWidth + style.panelPadding;
                s.cursorPos = ImVec2(leftColumnX, contentStartY);
                BeginGroupFixed("aimbot", columnWidth, contentHeight);

                Toggle("enabled", &g_Config.aimbot.enabled, true, nullptr);
                Toggle("visibility check", &g_Config.aimbot.visibilityCheck, false, nullptr);
                Toggle("auto fire", &g_Config.aimbot.autoFire, false, nullptr);
                Toggle("auto scope", &g_Config.aimbot.autoScope, false, nullptr);
                Toggle("auto stop", &g_Config.aimbot.autoStop, false, nullptr);
                Toggle("silent aim", &g_Config.aimbot.silentAim, false, nullptr);
                Toggle("no spread", &g_Config.aimbot.noSpread, false, nullptr);
                Toggle("no recoil", &g_Config.aimbot.noRecoil, false, nullptr);
                Toggle("recoil control", &g_Config.aimbot.recoilControl, false, nullptr);
                Toggle("auto wall", &g_Config.aimbot.autoWall, true, nullptr);
                Toggle("360 fov", &g_Config.aimbot.fov360, false, nullptr);
                Toggle("multi point", &g_Config.aimbot.multiPoint, true, nullptr);
                Toggle("delay shot", &g_Config.aimbot.delayShot, false, nullptr);
                Slider("fov", &g_Config.aimbot.fov, 0.0f, 1000.0f, "%.0f");
                Slider("smooth", &g_Config.aimbot.smooth, 1.0f, 100.0f, "%.1f");
                Slider("hitchance", &g_Config.aimbot.hitchance, 0.0f, 100.0f, "%.0f%%");
                Slider("min damage", &g_Config.aimbot.minDamage, 0.0f, 100.0f, "%.0f");

                EndGroup();
                s.cursorPos = ImVec2(rightColumnX, contentStartY);
                BeginGroupFixed("anti aim", columnWidth, contentHeight);

                Toggle("enable anti-aim", &g_Config.antiAim.enableAntiAim, false, nullptr);
                Toggle("server anti-aim", &g_Config.antiAim.serverAntiAim, false, nullptr);
                Toggle("fast duck", &g_Config.antiAim.fastDuck, false, nullptr);
                Toggle("desync move", &g_Config.antiAim.desyncMove, false, nullptr);
                Toggle("anti aim control", &g_Config.antiAim.antiAimControl, false, nullptr);
                if (g_Config.antiAim.antiAimControl) {
                    Slider("pitch value", &g_Config.antiAim.pitchValue, -90.0f, 90.0f, "%.1f");
                    Slider("yaw value", &g_Config.antiAim.yawValue, -180.0f, 180.0f, "%.1f");
                }
                Slider("spin value", &g_Config.antiAim.spinValue, 0.0f, 140.0f, "%.1f");
                Slider("fake lag", &g_Config.antiAim.fakeLagTicks, 0.0f, 64.0f, "%.0f");
                Slider("jitter range", &g_Config.antiAim.jitterRange, 0.0f, 360.0f, "%.1f");
                Slider("desync range", &g_Config.antiAim.desyncRange, 0.0f, 360.0f, "%.1f");
                Toggle("jitter enabled", &g_Config.antiAim.jitterEnabled, false, nullptr);
                Toggle("jitter on back", &g_Config.antiAim.jitterOnBack, false, nullptr);
                Toggle("wall standing", &g_Config.antiAim.wallStanding, false, nullptr);
                Toggle("freestanding", &g_Config.antiAim.freestanding, false, nullptr);
                Toggle("center jitter", &g_Config.antiAim.centerJitter, false, nullptr);
                if (g_Config.antiAim.centerJitter) {
                    Slider("center speed", &g_Config.antiAim.centerSpeed, 0.1f, 3.0f, "%.2f");
                }
                Toggle("prediction resolver", &g_Config.antiAim.predictionResolver, false, nullptr);

                EndGroup();
            }
            else if (g_Config.activeMainTab == 1) {
                float totalPadding = style.panelPadding * 3;
                float availableWidth = s.windowSize.x - totalPadding;
                float columnWidth = availableWidth * 0.5f;
                float leftColumnX = s.windowPos.x + style.panelPadding + tabSlideOffset;
                float rightColumnX = leftColumnX + columnWidth + style.panelPadding;
                float availableHeight = contentHeight;
                s.cursorPos = ImVec2(leftColumnX, contentStartY);
                s_LayoutState.groupWidth = columnWidth;
                Config::ESPTarget& esp = g_Config.espTargets[0];
                BeginGroupFixed("esp features", columnWidth, availableHeight);

                Toggle("enabled", &esp.enabled, false, nullptr);
                Toggle("only visible", &esp.onlyVisible, false, nullptr);
                Toggle("only audible", &esp.onlyAudible, false, nullptr);
                Toggle("bounding box", &esp.boundingBox, false, esp.boundingBoxColor);
                Toggle("name", &esp.name, false, esp.nameColor);
                Toggle("avatar", &esp.avatar, false, nullptr);
                Toggle("health bar", &esp.healthBar, false, esp.healthBarColor);
                Toggle("ammo bar", &esp.ammoBar, false, esp.ammoBarColor);
                Toggle("weapon name", &esp.weaponName, false, esp.weaponNameColor);
                Toggle("weapon icon", &esp.weaponIcon, false, esp.weaponIconColor);
                static bool flagsOpen[3] = {false, false, false};
                MultiSelectDropdown("flags", &esp.flagsSelection, &flagsOpen[g_Config.activeSubTab]);

                Toggle("grenades", &esp.grenades, false, esp.grenadesColor);
                Toggle("skeleton", &esp.skeleton, false, esp.skeletonColor);
                Toggle("line of sight", &esp.lineOfSight, false, esp.lineOfSightColor);
                Toggle("sounds", &esp.sounds, false, esp.soundsColor);

                EndGroup();
                float chamsHeight = availableHeight * 0.55f;
                float otherHeight = availableHeight * 0.45f - style.panelPadding;

                s.cursorPos = ImVec2(rightColumnX, contentStartY);
                BeginGroupFixed("chams", columnWidth, chamsHeight);

                Toggle("visible chams", &g_Config.chams.visibleChams, false, g_Config.chams.visibleColor);
                Toggle("invisible chams", &g_Config.chams.invisibleChams, false, g_Config.chams.invisibleColor);
                Toggle("overlay chams", &g_Config.chams.overlayChams, false, g_Config.chams.overlayColor);
                Toggle("backtrack chams", &g_Config.chams.backtrackChams, false, g_Config.chams.backtrackColor);
                Toggle("disable occlusion", &g_Config.chams.disableOcclusion, false, nullptr);

                EndGroup();
                s.cursorPos = ImVec2(rightColumnX, contentStartY + chamsHeight + style.panelPadding);
                BeginGroupFixed("other", columnWidth, otherHeight);

                Toggle("glow", &g_Config.other.glow, false, nullptr);
                Toggle("offscreen arrows", &g_Config.other.offscreenArrows, false, g_Config.other.offscreenArrowsColor);

                EndGroup();
            }
            else if (g_Config.activeMainTab == 2) {
                float totalPadding = style.panelPadding * 3;
                float availableWidth = s.windowSize.x - totalPadding;
                float columnWidth = availableWidth * 0.5f;
                float leftColumnX = s.windowPos.x + style.panelPadding + tabSlideOffset;
                float rightColumnX = leftColumnX + columnWidth + style.panelPadding;

                float movementHeight = contentHeight * 0.65f;
                float otherHeight = contentHeight - movementHeight - style.panelPadding;

                s.cursorPos = ImVec2(leftColumnX, contentStartY);
                BeginGroupFixed("movement", columnWidth, movementHeight);

                Toggle("bunny hop", &g_Config.misc.bunnyHop, false, nullptr);
                Toggle("auto strafe", &g_Config.misc.autoStrafe, true, nullptr);
                Toggle("air strafe", &g_Config.misc.airStrafe, true, nullptr);
                Toggle("edge jump", &g_Config.misc.edgeJump, true, nullptr);
                Toggle("jump bug", &g_Config.misc.jumpBug, true, nullptr);
                Toggle("fast stop", &g_Config.misc.fastStop, true, nullptr);
                Toggle("quick stop", &g_Config.misc.quickStop, true, nullptr);
                Toggle("fake lag", &g_Config.misc.fakelag, false, nullptr);

                EndGroup();

                s.cursorPos = ImVec2(leftColumnX, contentStartY + movementHeight + style.panelPadding);
                BeginGroupFixed("other", columnWidth, otherHeight);

                Toggle("resolver", &g_Config.misc.resolver, false, nullptr);
                Toggle("lag compensation", &g_Config.misc.lagCompensation, false, nullptr);

                EndGroup();

                float changerHeight = 115.0f;
                float featuresHeight = contentHeight - changerHeight - style.panelPadding;

                s.cursorPos = ImVec2(rightColumnX, contentStartY);
                BeginGroupFixed("models / changer", columnWidth, changerHeight);

                Toggle("skin changer", &g_Config.changer.skinChanger, false, nullptr);
                Toggle("agent changer", &g_Config.changer.agentChanger, false, nullptr);

                EndGroup();

                s.cursorPos = ImVec2(rightColumnX, contentStartY + changerHeight + style.panelPadding);
                BeginGroupFixed("features", columnWidth, featuresHeight);

                Toggle("reveal ranks", &g_Config.misc.revealRanks, false, nullptr);
                Toggle("auto accept", &g_Config.misc.autoAccept, false, nullptr);
                Toggle("clantag", &g_Config.misc.clantag, true, nullptr);
                TextInput("clantag text", g_Config.misc.clantagText, sizeof(g_Config.misc.clantagText));
                Toggle("hit sound", &g_Config.misc.hitSound, true, nullptr);
                Toggle("kill sound", &g_Config.misc.killSound, true, nullptr);

                EndGroup();
            }
            else if (g_Config.activeMainTab == 3) {
                ImDrawList* dl = s.drawList;
                float totalPadding = style.panelPadding * 3;
                float availableWidth = s.windowSize.x - totalPadding;
                float columnWidth = availableWidth * 0.5f;
                float leftColumnX = s.windowPos.x + style.panelPadding + tabSlideOffset;
                float rightColumnX = leftColumnX + columnWidth + style.panelPadding;

                float managerHeight = 210.0f;
                float settingsHeight = contentHeight - managerHeight - style.panelPadding;

                s.cursorPos = ImVec2(leftColumnX, contentStartY);
                BeginGroupFixed("profile manager", columnWidth, managerHeight);

                TextInput("config name", g_Config.configTab.newProfileName, sizeof(g_Config.configTab.newProfileName));
                if (Button("create profile")) {
                    if (strlen(g_Config.configTab.newProfileName) > 0) {
                        g_Config.configTab.profiles.push_back(std::string(g_Config.configTab.newProfileName));
                        g_Config.configTab.selectedProfile = (int)g_Config.configTab.profiles.size() - 1;
                        g_Config.configTab.statusMessage = "Created: " + std::string(g_Config.configTab.newProfileName);
                        g_Config.configTab.newProfileName[0] = '\0';
                    }
                }
                if (Button("save active config")) {
                    if (g_Config.configTab.selectedProfile >= 0 && g_Config.configTab.selectedProfile < (int)g_Config.configTab.profiles.size()) {
                        g_Config.configTab.statusMessage = "Saved: " + g_Config.configTab.profiles[g_Config.configTab.selectedProfile];
                    }
                }
                if (Button("reset to defaults")) {
                    g_Config.configTab.statusMessage = "Settings reset to defaults";
                }

                EndGroup();

                s.cursorPos = ImVec2(leftColumnX, contentStartY + managerHeight + style.panelPadding);
                BeginGroupFixed("menu settings", columnWidth, settingsHeight);

                Toggle("watermark", &g_Config.configTab.watermark, false, nullptr);
                Toggle("stream proof", &g_Config.configTab.streamProof, false, nullptr);
                Toggle("particle effects", &g_Config.configTab.particleEffects, false, nullptr);
                Toggle("spectator list", &g_Config.configTab.spectatorList, false, nullptr);
                if (Button("eject menu")) {
                    ::PostQuitMessage(0);
                }

                EndGroup();

                s.cursorPos = ImVec2(rightColumnX, contentStartY);
                BeginGroupFixed("saved profiles", columnWidth, contentHeight);

                int profileToDelete = -1;
                for (int i = 0; i < (int)g_Config.configTab.profiles.size(); i++) {
                    const char* profName = g_Config.configTab.profiles[i].c_str();
                    bool isSelected = (g_Config.configTab.selectedProfile == i);

                    const float itemHeight = 52.0f;
                    const float cardPad = 8.0f;
                    ImVec2 itemMin(s.currentGroupX + cardPad, s_LayoutState.groupContentY);
                    ImVec2 itemMax(s.currentGroupX + s_LayoutState.groupWidth - cardPad, itemMin.y + itemHeight);
                    s_LayoutState.groupContentY = itemMax.y + 6.0f;
                    if (s_LayoutState.groupContentY <= s_LayoutState.groupStartPos.y + s_LayoutState.groupHeight - cardPad) {
                        bool isCardHovered = ImGui::IsMouseHoveringRect(itemMin, itemMax);
                        if (isCardHovered) {
                            g_Config.blockWindowDrag = true;
                            if (ImGui::IsMouseDown(0)) g_Config.interactiveMouseDown = true;
                        }

                        ImU32 cardBg = isSelected ? Colors::WithAlpha(Colors::AccentDim(), 0.30f) : (isCardHovered ? Colors::PanelHeader() : Colors::PanelBg());
                        ImU32 cardBorder = isSelected ? Colors::Accent() : (isCardHovered ? Colors::AccentHover() : Colors::Border());

                        dl->AddRectFilled(itemMin, itemMax, cardBg, 4.0f);
                        dl->AddRect(itemMin, itemMax, cardBorder, 4.0f, 0, isSelected ? 1.5f : 1.0f);

                        if (isSelected) {
                            dl->AddRectFilled(ImVec2(itemMin.x, itemMin.y + 8.0f), ImVec2(itemMin.x + 3.5f, itemMax.y - 8.0f), Colors::Accent(), 2.0f);
                        }

                        float badgeSize = 34.0f;
                        ImVec2 badgeMin(itemMin.x + (isSelected ? 14.0f : 10.0f), itemMin.y + (itemHeight - badgeSize) * 0.5f);
                        ImVec2 badgeMax(badgeMin.x + badgeSize, badgeMin.y + badgeSize);
                        dl->AddRectFilled(badgeMin, badgeMax, Colors::BackgroundDark(), 3.0f);
                        dl->AddRect(badgeMin, badgeMax, isSelected ? Colors::Accent() : Colors::Border(), 3.0f, 0, 1.0f);

                        TextFont badgeFont = g_TextFont;
                        badgeFont.size = 11;
                        badgeFont.weight = FW_BOLD;
                        ImVec2 cfgSize = g_TextRenderer.MeasureText("CFG", badgeFont);
                        ImVec2 cfgPos(badgeMin.x + (badgeSize - cfgSize.x) * 0.5f, badgeMin.y + (badgeSize - cfgSize.y) * 0.5f);
                        g_TextRenderer.RenderText(dl, cfgPos, "CFG", isSelected ? Colors::AccentHover() : Colors::TextDim(), badgeFont);

                        float textStartX = badgeMax.x + 10.0f;
                        TextFont titleFont = g_TextFont;
                        titleFont.weight = FW_BOLD;
                        titleFont.size = 13;
                        ImVec2 titlePos(textStartX, itemMin.y + 9.0f);
                        g_TextRenderer.RenderText(dl, titlePos, profName, isSelected ? Colors::TextActive() : Colors::TextInactive(), titleFont);

                        ImVec2 dotCenter(textStartX + 3.0f, itemMin.y + 34.0f);
                        dl->AddCircleFilled(dotCenter, 3.0f, isSelected ? Colors::Accent() : Colors::TextDim());

                        TextFont statusFont = g_TextFont;
                        statusFont.size = 11;
                        statusFont.weight = FW_NORMAL;
                        ImVec2 statusPos(textStartX + 10.0f, itemMin.y + 28.0f);
                        g_TextRenderer.RenderText(dl, statusPos, isSelected ? "Active Profile" : "Ready to load", isSelected ? Colors::AccentHover() : Colors::TextDim(), statusFont);

                        float btnY = itemMin.y + (itemHeight - 24.0f) * 0.5f;
                        float btnH = 24.0f;

                        float loadW = 48.0f;
                        ImVec2 loadMin(itemMax.x - loadW - 8.0f, btnY);
                        ImVec2 loadMax(loadMin.x + loadW, btnY + btnH);
                        bool loadHover = ImGui::IsMouseHoveringRect(loadMin, loadMax);

                        float saveW = 44.0f;
                        ImVec2 saveMin(loadMin.x - saveW - 6.0f, btnY);
                        ImVec2 saveMax(saveMin.x + saveW, btnY + btnH);
                        bool saveHover = ImGui::IsMouseHoveringRect(saveMin, saveMax);

                        float delW = 26.0f;
                        ImVec2 delMin(saveMin.x - delW - 6.0f, btnY);
                        ImVec2 delMax(delMin.x + delW, btnY + btnH);
                        bool delHover = ImGui::IsMouseHoveringRect(delMin, delMax);

                        TextFont btnFont = g_TextFont;
                        btnFont.size = 11;
                        btnFont.weight = FW_BOLD;

                        dl->AddRectFilled(delMin, delMax, delHover ? IM_COL32(239, 68, 68, 50) : Colors::BackgroundDark(), 3.0f);
                        dl->AddRect(delMin, delMax, delHover ? Colors::Red() : Colors::Border(), 3.0f, 0, 1.0f);
                        ImVec2 xSize = g_TextRenderer.MeasureText("X", btnFont);
                        g_TextRenderer.RenderText(dl, ImVec2(delMin.x + (delW - xSize.x) * 0.5f, delMin.y + (btnH - xSize.y) * 0.5f), "X", delHover ? Colors::Red() : Colors::TextDim(), btnFont);

                        dl->AddRectFilled(saveMin, saveMax, saveHover ? Colors::WithAlpha(Colors::AccentDim(), 0.4f) : Colors::BackgroundDark(), 3.0f);
                        dl->AddRect(saveMin, saveMax, saveHover ? Colors::Accent() : Colors::Border(), 3.0f, 0, 1.0f);
                        ImVec2 saveSize = g_TextRenderer.MeasureText("SAVE", btnFont);
                        g_TextRenderer.RenderText(dl, ImVec2(saveMin.x + (saveW - saveSize.x) * 0.5f, saveMin.y + (btnH - saveSize.y) * 0.5f), "SAVE", saveHover ? Colors::TextActive() : Colors::TextDim(), btnFont);

                        ImU32 loadBg = isSelected ? Colors::Accent() : (loadHover ? Colors::WithAlpha(Colors::AccentDim(), 0.5f) : Colors::BackgroundDark());
                        dl->AddRectFilled(loadMin, loadMax, loadBg, 3.0f);
                        dl->AddRect(loadMin, loadMax, isSelected ? Colors::AccentHover() : (loadHover ? Colors::Accent() : Colors::Border()), 3.0f, 0, 1.0f);
                        const char* loadLabel = isSelected ? "ACTIVE" : "LOAD";
                        ImVec2 loadSize = g_TextRenderer.MeasureText(loadLabel, btnFont);
                        g_TextRenderer.RenderText(dl, ImVec2(loadMin.x + (loadW - loadSize.x) * 0.5f, loadMin.y + (btnH - loadSize.y) * 0.5f), loadLabel, isSelected ? Colors::BackgroundDark() : (loadHover ? Colors::TextActive() : Colors::TextDim()), btnFont);

                        if (ImGui::IsMouseClicked(0) && !g_Config.dropdownConsumedClick && !g_Config.IsPopupBlocking()) {
                            if (delHover) {
                                profileToDelete = i;
                            } else if (saveHover) {
                                g_Config.configTab.statusMessage = "Saved: " + std::string(profName);
                            } else if (loadHover || isCardHovered) {
                                g_Config.configTab.selectedProfile = i;
                                g_Config.configTab.statusMessage = "Loaded: " + std::string(profName);
                            }
                        }
                    }
                }
                if (profileToDelete >= 0 && profileToDelete < (int)g_Config.configTab.profiles.size()) {
                    g_Config.configTab.profiles.erase(g_Config.configTab.profiles.begin() + profileToDelete);
                    if (g_Config.configTab.selectedProfile >= (int)g_Config.configTab.profiles.size()) {
                        g_Config.configTab.selectedProfile = (int)g_Config.configTab.profiles.size() - 1;
                    }
                }

                EndGroup();
            }
            ImGui::PopStyleVar();
            if (g_Config.activeMainTab == 1) {
                RenderFooter();
            }

            EndFrame();
            {
                State& dragState = State::Get();
                static bool s_dragging = false;
                static POINT s_dragOffset;
                ImVec2 headerMin = dragState.windowPos;
                ImVec2 headerMax(headerMin.x + dragState.windowSize.x, headerMin.y + Style::Get().headerHeight);
                bool hoveringHeader = ImGui::IsMouseHoveringRect(headerMin, headerMax);
                bool canStartDrag = hoveringHeader && !g_Config.blockWindowDrag;
                if (canStartDrag && ImGui::IsMouseClicked(0) && !g_Config.IsPopupBlocking()) {
                    s_dragging = true;
                    POINT pt;
                    ::GetCursorPos(&pt);
                    HWND targetHwnd = ::g_hWnd ? ::g_hWnd : ::GetActiveWindow();
                    RECT rect = {0};
                    if (targetHwnd) ::GetWindowRect(targetHwnd, &rect);
                    s_dragOffset.x = pt.x - rect.left;
                    s_dragOffset.y = pt.y - rect.top;
                }
                if (s_dragging) {
                    if (ImGui::IsMouseDown(0)) {
                        POINT pt;
                        ::GetCursorPos(&pt);
                        HWND targetHwnd = ::g_hWnd ? ::g_hWnd : ::GetActiveWindow();
                        if (targetHwnd) {
                            ::SetWindowPos(targetHwnd, nullptr, pt.x - s_dragOffset.x, pt.y - s_dragOffset.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        }
                    } else {
                        s_dragging = false;
                    }
                }
            }
            {
                State& st = State::Get();
                ImDrawList* dlBorder = st.drawList;
                dlBorder->AddRect(
                    st.windowPos,
                    ImVec2(st.windowPos.x + st.windowSize.x, st.windowPos.y + st.windowSize.y),
                    Colors::Border(), 8.0f, 0, 1.0f
                );
            }

            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(4);
            ColorPickerPopup(g_Config.activeColorEdit);
            KeybindPopup();
            RenderKeybindList();
            Colors::GlobalAlpha() = 1.0f;
        }

        void RenderWatermark(float fps, float cpuUsage, float gpuUsage) {
            (void)fps;
            (void)cpuUsage;
            (void)gpuUsage;
        }

        void RenderKeybindList() {
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            ImGuiIO& io = ImGui::GetIO();
            static float s_listAlpha = 0.0f;
            static std::map<std::string, float> s_itemAnims;
            static std::map<std::string, std::pair<std::string, std::string>> s_fadingItems;
            static std::map<std::string, int> s_activationOrder;
            static int s_orderCounter = 0;
            std::vector<std::pair<std::string, std::string>> activeBinds;
            std::set<std::string> currentActiveBinds;
            for (const auto& pair : g_Config.keybinds) {
                const Keybind& kb = pair.second;
                if (kb.key != 0 && kb.IsActive()) {
                    activeBinds.push_back({kb.GetKeyName(), pair.first});
                    currentActiveBinds.insert(pair.first);
                    if (s_activationOrder.find(pair.first) == s_activationOrder.end()) {
                        s_activationOrder[pair.first] = s_orderCounter++;
                    }
                }
            }
            std::sort(activeBinds.begin(), activeBinds.end(),
                [](const auto& a, const auto& b) {
                    return s_activationOrder[a.second] < s_activationOrder[b.second];
                });
            for (auto it = s_itemAnims.begin(); it != s_itemAnims.end(); ++it) {
                if (currentActiveBinds.find(it->first) == currentActiveBinds.end()) {
                    if (s_fadingItems.find(it->first) == s_fadingItems.end()) {
                        for (const auto& pair : g_Config.keybinds) {
                            if (pair.first == it->first) {
                                s_fadingItems[it->first] = {pair.second.GetKeyName(), it->first};
                                break;
                            }
                        }
                    }
                }
            }
            bool hasActiveBinds = !activeBinds.empty() || !s_fadingItems.empty();
            float targetAlpha = hasActiveBinds ? 1.0f : 0.0f;
            s_listAlpha = ImLerp(s_listAlpha, targetAlpha, io.DeltaTime * 10.0f);
            if (s_listAlpha < 0.01f) {
                s_itemAnims.clear();
                s_fadingItems.clear();
                return;
            }
            for (const auto& bind : activeBinds) {
                const std::string& key = bind.second;
                s_fadingItems.erase(key);
                if (s_itemAnims.find(key) == s_itemAnims.end()) {
                    s_itemAnims[key] = 0.0f;
                }
                s_itemAnims[key] = ImLerp(s_itemAnims[key], 1.0f, io.DeltaTime * 12.0f);
            }
            for (auto it = s_fadingItems.begin(); it != s_fadingItems.end();) {
                s_itemAnims[it->first] = ImLerp(s_itemAnims[it->first], 0.0f, io.DeltaTime * 12.0f);
                if (s_itemAnims[it->first] < 0.01f) {
                    s_itemAnims.erase(it->first);
                    s_activationOrder.erase(it->first);
                    it = s_fadingItems.erase(it);
                } else {
                    ++it;
                }
            }
            const float headerHeight = 26.0f;
            const float itemHeight = 24.0f;
            const float itemGap = 4.0f;
            const float panelWidth = 160.0f;
            const float cornerRadius = 4.0f;
            const float innerPadding = 10.0f;
            static ImVec2 s_keybindListPos(15.0f, 65.0f);
            static bool s_dragging = false;
            static ImVec2 s_dragOffset;
            ImVec2 headerMin = s_keybindListPos;
            ImVec2 headerMax(headerMin.x + panelWidth, headerMin.y + headerHeight);
            int alphaInt = (int)(220 * s_listAlpha);
            int borderAlphaInt = (int)(255 * s_listAlpha);
            int textAlphaInt = (int)(255 * s_listAlpha);

            ImU32 bgColor = Colors::WithAlpha(Colors::BackgroundDark(), s_listAlpha * 0.88f);
            ImU32 borderColor = Colors::WithAlpha(Colors::Border(), s_listAlpha);
            ImU32 accentColor = Colors::WithAlpha(Colors::Accent(), s_listAlpha);
            ImU32 whiteText = Colors::WithAlpha(Colors::TextActive(), s_listAlpha);
            ImU32 grayText = Colors::WithAlpha(Colors::TextInactive(), s_listAlpha);
            if (s_dragging) {
                if (ImGui::IsMouseDown(0)) {
                    s_keybindListPos = ImVec2(io.MousePos.x - s_dragOffset.x, io.MousePos.y - s_dragOffset.y);
                    s_keybindListPos.x = ImClamp(s_keybindListPos.x, 0.0f, io.DisplaySize.x - panelWidth);
                    s_keybindListPos.y = ImClamp(s_keybindListPos.y, 0.0f, io.DisplaySize.y - headerHeight);
                    headerMin = s_keybindListPos;
                    headerMax = ImVec2(headerMin.x + panelWidth, headerMin.y + headerHeight);
                } else {
                    s_dragging = false;
                }
            } else {
                bool headerHovered = io.MousePos.x >= headerMin.x && io.MousePos.x <= headerMax.x &&
                                     io.MousePos.y >= headerMin.y && io.MousePos.y <= headerMax.y;
                if (headerHovered && ImGui::IsMouseClicked(0)) {
                    s_dragging = true;
                    s_dragOffset = ImVec2(io.MousePos.x - s_keybindListPos.x, io.MousePos.y - s_keybindListPos.y);
                }
            }
            float itemsStartY = headerMax.y + 6.0f;
            dl->AddRectFilled(headerMin, headerMax, bgColor, 0.0f);
            dl->AddRect(headerMin, headerMax, borderColor, 0.0f);
            float gradientWidth = panelWidth * 0.4f;
            for (int i = 0; i < (int)panelWidth; i++) {
                ImU32 lineColor;
                if (i < (int)gradientWidth) {
                    float t = (float)i / gradientWidth;
                    lineColor = Colors::LerpColor(accentColor, borderColor, t);
                } else {
                    lineColor = borderColor;
                }
                dl->AddLine(
                    ImVec2(headerMin.x + i, headerMin.y),
                    ImVec2(headerMin.x + i + 1, headerMin.y),
                    lineColor, 1.0f
                );
            }
            float iconX = headerMin.x + innerPadding + 8.0f;
            float iconY = headerMin.y + headerHeight * 0.5f;
            float iconSize = 7.0f;
            dl->AddRect(
                ImVec2(iconX - iconSize, iconY - iconSize * 0.6f),
                ImVec2(iconX + iconSize, iconY + iconSize * 0.6f),
                accentColor, 2.0f, 0, 1.5f
            );
            dl->AddRectFilled(
                ImVec2(iconX - iconSize + 2, iconY - 2),
                ImVec2(iconX - iconSize + 5, iconY + 1),
                accentColor
            );
            dl->AddRectFilled(
                ImVec2(iconX - iconSize + 7, iconY - 2),
                ImVec2(iconX - iconSize + 10, iconY + 1),
                accentColor
            );
            TextFont boldFont = g_TextFont;
            boldFont.weight = FW_BOLD;
            float headerTextY = headerMin.y + (headerHeight - g_TextFont.size) * 0.5f;
            g_TextRenderer.RenderText(dl, ImVec2(iconX + iconSize + 8, headerTextY), "Hotkeys", whiteText, boldFont);
            std::vector<std::pair<std::string, std::string>> allItems = activeBinds;
            for (const auto& fadingPair : s_fadingItems) {
                bool found = false;
                for (const auto& active : activeBinds) {
                    if (active.second == fadingPair.first) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    allItems.push_back(fadingPair.second);
                }
            }
            std::sort(allItems.begin(), allItems.end(),
                [](const auto& a, const auto& b) {
                    int orderA = s_activationOrder.count(a.second) ? s_activationOrder[a.second] : 999999;
                    int orderB = s_activationOrder.count(b.second) ? s_activationOrder[b.second] : 999999;
                    return orderA < orderB;
                });
            float itemY = itemsStartY;
            for (const auto& bind : allItems) {
                float itemAnim = 1.0f;
                auto animIt = s_itemAnims.find(bind.second);
                if (animIt != s_itemAnims.end()) {
                    itemAnim = animIt->second;
                }
                if (itemAnim < 0.01f) continue;
                float slideOffset = (1.0f - itemAnim) * -20.0f;
                int itemAlphaInt = (int)(220 * s_listAlpha * itemAnim);
                int itemBorderAlphaInt = (int)(255 * s_listAlpha * itemAnim);
                int itemTextAlphaInt = (int)(255 * s_listAlpha * itemAnim);

                ImU32 itemBgColor = Colors::WithAlpha(Colors::PanelBg(), s_listAlpha * itemAnim);
                ImU32 itemBorderColor = Colors::WithAlpha(Colors::Border(), s_listAlpha * itemAnim);
                ImU32 itemAccentColor = Colors::WithAlpha(Colors::Accent(), s_listAlpha * itemAnim);
                ImU32 itemGrayText = Colors::WithAlpha(Colors::TextInactive(), s_listAlpha * itemAnim);

                ImVec2 itemMin(headerMin.x + slideOffset, itemY);
                ImVec2 itemMax(headerMin.x + panelWidth + slideOffset, itemY + itemHeight);
                dl->AddRectFilled(itemMin, itemMax, itemBgColor, 0.0f);
                dl->AddRect(itemMin, itemMax, itemBorderColor, 0.0f);
                float textY = itemY + (itemHeight - g_TextFont.size) * 0.5f;
                g_TextRenderer.RenderText(dl, ImVec2(itemMin.x + innerPadding, textY), bind.first, itemAccentColor, g_TextFont);
                ImVec2 keySize = g_TextRenderer.MeasureText(bind.first, g_TextFont);
                g_TextRenderer.RenderText(dl, ImVec2(itemMin.x + innerPadding + keySize.x + 10.0f, textY), bind.second, itemGrayText, g_TextFont);

                itemY += itemHeight + itemGap;
            }
        }

    }

}


