#include "loader_framework.h"
#include "menu_framework.h"
#include "imgui_text_renderer.h"
#include <Windows.h>

extern HWND g_hWnd;
#include <map>
#include <cstring>

namespace KrxSlaxy {
namespace Loader {
    LoaderConfig g_LoaderConfig;

    namespace GUI {
        
        void Initialize() {
            g_LoaderConfig = LoaderConfig();
            g_LoaderConfig.windowAlpha = 0.0f;
            g_LoaderConfig.windowScale = 0.95f;
        }
        
        bool InputField(const char* label, const char* placeholder, char* buffer, size_t bufferSize, bool isPassword, bool* focused) {
            ImGuiIO& io = ImGui::GetIO();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            
            const float fieldWidth = 280.0f;
            const float fieldHeight = 36.0f;
            const float padding = 12.0f;
            const float rounding = 4.0f;
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 fieldMin = pos;
            ImVec2 fieldMax(pos.x + fieldWidth, pos.y + fieldHeight);
            ImGui::InvisibleButton(label, ImVec2(fieldWidth, fieldHeight));
            bool hovered = ImGui::IsItemHovered();
            static std::map<std::string, int> s_cursorPos;
            static std::map<std::string, int> s_selectionStart;
            static std::map<std::string, bool> s_isDragging;
            
            int& cursorPos = s_cursorPos[label];
            int& selectionStart = s_selectionStart[label];
            bool& isDragging = s_isDragging[label];
            
            int len = (int)strlen(buffer);
            if (cursorPos > len) cursorPos = len;
            if (cursorPos < 0) cursorPos = 0;
            ImVec2 textPos(fieldMin.x + padding, fieldMin.y + (fieldHeight - g_TextFont.size) * 0.5f);
            auto getCursorPosFromMouse = [&](float mouseX) -> int {
                float relX = mouseX - textPos.x;
                if (relX <= 0) return 0;
                for (int i = 1; i <= len; i++) {
                    std::string sub;
                    if (isPassword) {
                        sub = std::string(i, '*');
                    } else {
                        sub = std::string(buffer, i);
                    }
                    ImVec2 size = g_TextRenderer.MeasureText(sub, g_TextFont);
                    if (relX < size.x) {
                        std::string prevSub;
                        if (isPassword) {
                            prevSub = std::string(i - 1, '*');
                        } else {
                            prevSub = std::string(buffer, i - 1);
                        }
                        ImVec2 prevSize = g_TextRenderer.MeasureText(prevSub, g_TextFont);
                        return (relX - prevSize.x < size.x - relX) ? i - 1 : i;
                    }
                }
                return len;
            };
            if (ImGui::IsMouseClicked(0)) {
                if (hovered) {
                    *focused = true;
                    cursorPos = getCursorPosFromMouse(io.MousePos.x);
                    selectionStart = cursorPos;
                    isDragging = true;
                } else {
                    *focused = false;
                    selectionStart = -1;
                    isDragging = false;
                }
            }
            if (isDragging && ImGui::IsMouseDown(0) && *focused) {
                cursorPos = getCursorPosFromMouse(io.MousePos.x);
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
                for (int i = start; i <= len - (end - start); i++) {
                    buffer[i] = buffer[i + (end - start)];
                }
                cursorPos = start;
                selectionStart = -1;
            };
            static std::map<std::string, float> s_focusAnim;
            float targetAnim = *focused ? 1.0f : (hovered ? 0.5f : 0.0f);
            float& anim = s_focusAnim[label];
            anim = ImLerp(anim, targetAnim, io.DeltaTime * 12.0f);
            ImU32 bgColor = Colors::PanelBg();
            ImU32 borderColor = Colors::LerpColor(Colors::Border(), Colors::Accent(), anim);
            ImU32 textColor = Colors::TextActive();
            ImU32 placeholderColor = Colors::TextDim();
            dl->AddRectFilled(fieldMin, fieldMax, bgColor, rounding);
            dl->AddRect(fieldMin, fieldMax, borderColor, rounding, 0, 1.0f + anim);
            bool valueChanged = false;
            static std::map<std::string, float> s_cursorBlink;
            float& cursorBlink = s_cursorBlink[label];
            
            if (*focused) {
                cursorBlink += io.DeltaTime;
                if (cursorBlink > 1.0f) cursorBlink = 0.0f;
                
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
                        valueChanged = true;
                    } else if (cursorPos > 0) {
                        len = (int)strlen(buffer);
                        for (int j = cursorPos - 1; j < len; j++) {
                            buffer[j] = buffer[j + 1];
                        }
                        cursorPos--;
                        valueChanged = true;
                    }
                    cursorBlink = 0.0f;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                    if (hasSelection()) {
                        deleteSelection();
                        valueChanged = true;
                    } else {
                        len = (int)strlen(buffer);
                        if (cursorPos < len) {
                            for (int j = cursorPos; j < len; j++) {
                                buffer[j] = buffer[j + 1];
                            }
                            valueChanged = true;
                        }
                    }
                    cursorBlink = 0.0f;
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
                if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
                }
            }
            dl->PushClipRect(ImVec2(fieldMin.x + padding - 2, fieldMin.y), 
                            ImVec2(fieldMax.x - padding + 2, fieldMax.y), true);
            if (*focused && hasSelection()) {
                auto [start, end] = getSelectionRange();
                std::string beforeStart, beforeEnd;
                if (isPassword) {
                    beforeStart = std::string(start, '*');
                    beforeEnd = std::string(end, '*');
                } else {
                    beforeStart = std::string(buffer, start);
                    beforeEnd = std::string(buffer, end);
                }
                ImVec2 startSize = g_TextRenderer.MeasureText(beforeStart, g_TextFont);
                ImVec2 endSize = g_TextRenderer.MeasureText(beforeEnd, g_TextFont);
                
                dl->AddRectFilled(
                    ImVec2(textPos.x + startSize.x, fieldMin.y + 6),
                    ImVec2(textPos.x + endSize.x, fieldMax.y - 6),
                    Colors::WithAlpha(Colors::AccentDim(), 0.5f)
                );
            }
            if (strlen(buffer) > 0) {
                std::string displayText;
                if (isPassword) {
                    displayText = std::string(strlen(buffer), '*');
                } else {
                    displayText = buffer;
                }
                g_TextRenderer.RenderText(dl, textPos, displayText, textColor, g_TextFont);
            } else {
                g_TextRenderer.RenderText(dl, textPos, placeholder, placeholderColor, g_TextFont);
            }
            if (*focused && !hasSelection() && cursorBlink < 0.5f) {
                std::string textBeforeCursor;
                if (isPassword) {
                    textBeforeCursor = std::string(cursorPos, '*');
                } else {
                    textBeforeCursor = std::string(buffer, cursorPos);
                }
                ImVec2 textSize = g_TextRenderer.MeasureText(textBeforeCursor, g_TextFont);
                float cursorX = textPos.x + textSize.x;
                dl->AddLine(
                    ImVec2(cursorX, fieldMin.y + 8),
                    ImVec2(cursorX, fieldMax.y - 8),
                    textColor, 1.0f
                );
            }
            
            dl->PopClipRect();
            
            return valueChanged;
        }
        
        bool Checkbox(const char* label, bool* value) {
            ImGuiIO& io = ImGui::GetIO();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            
            const float boxSize = 16.0f;
            const float spacing = 8.0f;
            std::string lowerLabel = KrxSlaxy::GUI::ToLower(label);
            ImVec2 labelSize = g_TextRenderer.MeasureText(lowerLabel, g_TextFont);
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 boxMin = pos;
            ImVec2 boxMax(pos.x + boxSize, pos.y + boxSize);
            ImVec2 totalSize(boxSize + spacing + labelSize.x, boxSize);
            ImGui::Dummy(totalSize);
            ImVec2 interactMin = pos;
            ImVec2 interactMax(pos.x + totalSize.x, pos.y + totalSize.y);
            bool hovered = ImGui::IsMouseHoveringRect(interactMin, interactMax);
            bool clicked = hovered && ImGui::IsMouseClicked(0);
            
            if (clicked) {
                *value = !*value;
            }
            static std::map<std::string, float> s_checkAnim;
            static std::map<std::string, float> s_hoverAnim;
            float& checkAnim = s_checkAnim[label];
            float& hoverAnim = s_hoverAnim[label];
            checkAnim = ImLerp(checkAnim, *value ? 1.0f : 0.0f, io.DeltaTime * 15.0f);
            hoverAnim = ImLerp(hoverAnim, hovered ? 1.0f : 0.0f, io.DeltaTime * 12.0f);
            ImU32 boxBg = Colors::LerpColor(Colors::ToggleOff(), Colors::ToggleOn(), checkAnim);
            ImU32 labelColor = Colors::LerpColor(Colors::TextInactive(), Colors::TextActive(), hoverAnim);
            ImVec2 alignedMin(floorf(boxMin.x), floorf(boxMin.y));
            ImVec2 alignedMax(floorf(boxMax.x), floorf(boxMax.y));
            dl->AddRectFilled(alignedMin, alignedMax, boxBg);
            dl->AddRect(alignedMin, alignedMax, Colors::Border(), 0.0f, 0, 1.0f);
            if (checkAnim > 0.01f) {
                ImVec2 center(boxMin.x + boxSize * 0.5f, boxMin.y + boxSize * 0.5f);
                float cs = boxSize * 0.25f * checkAnim;
                ImU32 checkColor = Colors::WithAlpha(Colors::TextActive(), checkAnim);
                
                ImVec2 points[3] = {
                    ImVec2(center.x - cs * 0.9f, center.y),
                    ImVec2(center.x - cs * 0.2f, center.y + cs * 0.7f),
                    ImVec2(center.x + cs, center.y - cs * 0.6f)
                };
                dl->AddPolyline(points, 3, checkColor, ImDrawFlags_None, 1.2f);
            }
            ImVec2 labelPos(boxMax.x + spacing, pos.y + (boxSize - g_TextFont.size) * 0.5f);
            g_TextRenderer.RenderText(dl, labelPos, lowerLabel, labelColor, g_TextFont);
            
            return clicked;
        }
        
        bool Button(const char* label, float width) {
            ImGuiIO& io = ImGui::GetIO();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            
            const float height = 40.0f;
            const float rounding = 4.0f;
            std::string lowerLabel = KrxSlaxy::GUI::ToLower(label);
            ImVec2 labelSize = g_TextRenderer.MeasureText(lowerLabel, g_TextFont);
            
            if (width <= 0.0f) {
                width = labelSize.x + 60.0f;
            }
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 btnMin = pos;
            ImVec2 btnMax(pos.x + width, pos.y + height);
            ImGui::Dummy(ImVec2(width, height));
            bool hovered = ImGui::IsMouseHoveringRect(btnMin, btnMax);
            bool held = hovered && ImGui::IsMouseDown(0);
            bool clicked = hovered && ImGui::IsMouseClicked(0);
            static std::map<std::string, float> s_hoverAnim;
            static std::map<std::string, float> s_pressAnim;
            float& hoverAnim = s_hoverAnim[label];
            float& pressAnim = s_pressAnim[label];
            hoverAnim = ImLerp(hoverAnim, hovered ? 1.0f : 0.0f, io.DeltaTime * 12.0f);
            pressAnim = ImLerp(pressAnim, held ? 1.0f : 0.0f, io.DeltaTime * 20.0f);
            ImU32 bgColor = Colors::LerpColor(
                Colors::LerpColor(Colors::Accent(), Colors::AccentHover(), hoverAnim),
                Colors::AccentDim(),
                pressAnim
            );
            float pressOffset = pressAnim * 2.0f;
            ImVec2 drawMin(btnMin.x, btnMin.y + pressOffset);
            ImVec2 drawMax(btnMax.x, btnMax.y + pressOffset);
            if (pressAnim < 0.9f) {
                float shadowAlpha = (1.0f - pressAnim) * 0.3f;
                dl->AddRectFilled(
                    ImVec2(btnMin.x + 2, btnMin.y + 4),
                    ImVec2(btnMax.x + 2, btnMax.y + 4),
                    IM_COL32(0, 0, 0, (int)(255 * shadowAlpha)),
                    rounding
                );
            }
            dl->AddRectFilled(drawMin, drawMax, bgColor, rounding);
            if (hoverAnim > 0.01f) {
                ImU32 highlightColor = Colors::WithAlpha(IM_COL32(255, 255, 255, 50), hoverAnim);
                dl->AddRect(drawMin, drawMax, highlightColor, rounding);
            }
            ImVec2 labelPos(
                drawMin.x + (width - labelSize.x) * 0.5f,
                drawMin.y + (height - g_TextFont.size) * 0.5f
            );
            g_TextRenderer.RenderText(dl, labelPos, lowerLabel, Colors::TextActive(), g_TextFont);
            
            return clicked;
        }
        
        void RenderLoginPage() {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 windowSize = ImGui::GetWindowSize();
            
            float emblemY = windowPos.y + 26.0f;
            ImVec2 emblemCenter(windowPos.x + windowSize.x * 0.5f - 48.0f, emblemY);
            KrxSlaxy::GUI::DrawKrxSlaxyEmblem(dl, emblemCenter, 13.0f, Colors::Accent(), Colors::AccentHover());
            TextFont loaderTitleFont;
            loaderTitleFont.size = 15;
            loaderTitleFont.weight = FW_BOLD;
            ImVec2 krxSize = g_TextRenderer.MeasureText("KRX ", loaderTitleFont);
            ImVec2 titlePos(emblemCenter.x + 20.0f, emblemY - krxSize.y * 0.5f);
            g_TextRenderer.RenderText(dl, titlePos, "KRX ", Colors::TextActive(), loaderTitleFont, FLAG_DROPSHADOW);
            g_TextRenderer.RenderText(dl, ImVec2(titlePos.x + krxSize.x, titlePos.y), "SLAXY", Colors::Accent(), loaderTitleFont, FLAG_DROPSHADOW);
            
            const float contentWidth = 280.0f;
            float offsetX = (windowSize.x - contentWidth) * 0.5f;
            float offsetY = 58.0f;
            ImGui::SetCursorPos(ImVec2(offsetX, offsetY));
            InputField("##username_input", "username", g_LoaderConfig.username, sizeof(g_LoaderConfig.username), false, &g_LoaderConfig.usernameFocused);
            ImGui::SetCursorPos(ImVec2(offsetX, ImGui::GetCursorPos().y + 12.0f));
            InputField("##password_input", "password", g_LoaderConfig.password, sizeof(g_LoaderConfig.password), true, &g_LoaderConfig.passwordFocused);
            ImGui::SetCursorPos(ImVec2(offsetX, ImGui::GetCursorPos().y + 16.0f));
            Checkbox("remember me", &g_LoaderConfig.rememberMe);
            ImGui::SetCursorPos(ImVec2(offsetX, ImGui::GetCursorPos().y + 24.0f));
            if (Button("login", contentWidth)) {
                if (strlen(g_LoaderConfig.username) > 0 && strlen(g_LoaderConfig.password) > 0) {
                    g_LoaderConfig.isTransitioning = true;
                    g_LoaderConfig.previousState = State::Login;
                    g_LoaderConfig.pageTransition = 1.0f;
                } else {
                    g_LoaderConfig.loginFailed = true;
                    g_LoaderConfig.errorMessage = "please enter username and password";
                }
            }
            if (g_LoaderConfig.loginFailed && !g_LoaderConfig.errorMessage.empty()) {
                ImGui::SetCursorPos(ImVec2(offsetX, ImGui::GetCursorPos().y + 12.0f));
                ImVec2 errorPos = ImGui::GetCursorScreenPos();
                ImVec2 errorSize = g_TextRenderer.MeasureText(g_LoaderConfig.errorMessage, g_TextFont);
                ImVec2 centeredPos(errorPos.x + (contentWidth - errorSize.x) * 0.5f, errorPos.y);
                g_TextRenderer.RenderText(dl, centeredPos, g_LoaderConfig.errorMessage, IM_COL32(239, 68, 68, 255), g_TextFont);
                ImGui::Dummy(ImVec2(contentWidth, g_TextFont.size));
            }
            if ((g_LoaderConfig.usernameFocused || g_LoaderConfig.passwordFocused) && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                if (strlen(g_LoaderConfig.username) > 0 && strlen(g_LoaderConfig.password) > 0) {
                    g_LoaderConfig.isTransitioning = true;
                    g_LoaderConfig.previousState = State::Login;
                    g_LoaderConfig.pageTransition = 1.0f;
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
                bool shiftHeld = ImGui::GetIO().KeyShift;
                
                if (g_LoaderConfig.usernameFocused && !shiftHeld) {
                    g_LoaderConfig.usernameFocused = false;
                    g_LoaderConfig.passwordFocused = true;
                } else if (g_LoaderConfig.passwordFocused && shiftHeld) {
                    g_LoaderConfig.passwordFocused = false;
                    g_LoaderConfig.usernameFocused = true;
                } else if (g_LoaderConfig.passwordFocused && !shiftHeld) {
                    g_LoaderConfig.passwordFocused = false;
                    g_LoaderConfig.usernameFocused = true;
                } else if (!g_LoaderConfig.usernameFocused && !g_LoaderConfig.passwordFocused) {
                    g_LoaderConfig.usernameFocused = true;
                }
            }
        }
        
        void RenderLoadingPage(float alpha, float scale) {
            ImGuiIO& io = ImGui::GetIO();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 windowSize = ImGui::GetWindowSize();
            const float baseSpinnerRadius = 25.0f;
            const float spinnerRadius = baseSpinnerRadius * scale;
            const float thickness = 4.0f * scale;
            const int numSegments = 30;
            ImVec2 center(
                windowPos.x + windowSize.x * 0.5f,
                windowPos.y + windowSize.y * 0.5f
            );
            float animTime = g_LoaderConfig.spinnerAnimTime;
            ImU32 spinnerColor = Colors::WithAlpha(Colors::Accent(), alpha);
            ImU32 trackColor = Colors::WithAlpha(Colors::Border(), 0.3f * alpha);
            dl->PathClear();
            for (int i = 0; i <= numSegments; i++) {
                float angle = ((float)i / (float)numSegments) * IM_PI * 2.0f - IM_PI * 0.5f;
                dl->PathLineTo(ImVec2(
                    center.x + cosf(angle) * spinnerRadius,
                    center.y + sinf(angle) * spinnerRadius
                ));
            }
            dl->PathStroke(trackColor, ImDrawFlags_None, thickness);
            float rotationCycle = fmodf(animTime, 1.6f) / 1.6f;
            float baseRotation;
            float scaleY;
            if (rotationCycle < 0.5f) {
                baseRotation = rotationCycle * 2.0f * 135.0f * (IM_PI / 180.0f);
                scaleY = 1.0f;
            } else {
                baseRotation = (rotationCycle - 0.5f) * 2.0f * -135.0f * (IM_PI / 180.0f);
                scaleY = -1.0f;
            }
            float arcCycle = fmodf(animTime, 1.6f) / 0.8f;
            float arcProgress;
            if (fmodf(arcCycle, 2.0f) < 1.0f) {
                arcProgress = fmodf(arcCycle, 1.0f);
            } else {
                arcProgress = 1.0f - fmodf(arcCycle, 1.0f);
            }
            float arcStart = baseRotation - IM_PI * 0.5f;
            float arcLength = 0.1f + arcProgress * 0.75f;
            float arcEnd = arcStart + arcLength * IM_PI * 2.0f;
            if (scaleY < 0) {
                float temp = arcStart;
                arcStart = -arcEnd + IM_PI;
                arcEnd = -temp + IM_PI;
            }
            dl->PathClear();
            int arcSegments = (int)(numSegments * arcLength) + 5;
            for (int i = 0; i <= arcSegments; i++) {
                float t = (float)i / (float)arcSegments;
                float angle = arcStart + t * (arcEnd - arcStart);
                dl->PathLineTo(ImVec2(
                    center.x + cosf(angle) * spinnerRadius,
                    center.y + sinf(angle) * spinnerRadius
                ));
            }
            dl->PathStroke(spinnerColor, ImDrawFlags_None, thickness);
        }
        
        void RenderTransition() {
            ImGuiIO& io = ImGui::GetIO();
            g_LoaderConfig.transitionProgress += io.DeltaTime * 2.0f;
            
            if (g_LoaderConfig.transitionProgress >= 1.0f) {
                g_LoaderConfig.transitionProgress = 1.0f;
                g_LoaderConfig.currentState = State::Complete;
            }
            g_LoaderConfig.windowAlpha = 1.0f - g_LoaderConfig.transitionProgress;
            g_LoaderConfig.windowScale = 1.0f - (g_LoaderConfig.transitionProgress * 0.05f);
        }
        
        bool Render() {
            ImGuiIO& io = ImGui::GetIO();
            if (g_LoaderConfig.currentState == State::Complete) {
                return true;
            }
            float targetAlpha = (g_LoaderConfig.currentState == State::Transition) ? 0.0f : 1.0f;
            float targetScale = (g_LoaderConfig.currentState == State::Transition) ? 0.95f : 1.0f;
            g_LoaderConfig.windowAlpha = ImLerp(g_LoaderConfig.windowAlpha, targetAlpha, io.DeltaTime * 8.0f);
            g_LoaderConfig.windowScale = ImLerp(g_LoaderConfig.windowScale, targetScale, io.DeltaTime * 8.0f);
            if (g_LoaderConfig.windowAlpha < 0.01f && g_LoaderConfig.currentState == State::Transition) {
                g_LoaderConfig.currentState = State::Complete;
                return true;
            }
            const float windowWidth = 340.0f;
            const float windowHeight = 280.0f;
            ImVec2 windowPos(0, 0);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_LoaderConfig.windowAlpha);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::ToVec4(Colors::BackgroundDark()));
            ImGui::PushStyleColor(ImGuiCol_Border, Colors::ToVec4(Colors::Border()));
            
            ImGui::SetNextWindowPos(windowPos);
            ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
            
            ImGui::Begin("##KrxSlaxyLoader", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 bgMin = ImVec2(0, 0);
            ImVec2 bgMax = ImVec2(windowWidth, windowHeight);
            dl->AddRectFilled(bgMin, bgMax, Colors::BackgroundDark(), 8.0f);

            {
                static bool s_loaderDragging = false;
                static POINT s_loaderDragOffset;
                bool hoveringTop = ImGui::IsMouseHoveringRect(ImVec2(0, 0), ImVec2(windowWidth - 32.0f, 48.0f));
                if (hoveringTop && ImGui::IsMouseClicked(0)) {
                    s_loaderDragging = true;
                    POINT pt;
                    ::GetCursorPos(&pt);
                    HWND targetHwnd = ::g_hWnd ? ::g_hWnd : ::GetActiveWindow();
                    RECT rect = {0};
                    if (targetHwnd) ::GetWindowRect(targetHwnd, &rect);
                    s_loaderDragOffset.x = pt.x - rect.left;
                    s_loaderDragOffset.y = pt.y - rect.top;
                }
                if (s_loaderDragging) {
                    if (ImGui::IsMouseDown(0)) {
                        POINT pt;
                        ::GetCursorPos(&pt);
                        HWND targetHwnd = ::g_hWnd ? ::g_hWnd : ::GetActiveWindow();
                        if (targetHwnd) {
                            ::SetWindowPos(targetHwnd, nullptr, pt.x - s_loaderDragOffset.x, pt.y - s_loaderDragOffset.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        }
                    } else {
                        s_loaderDragging = false;
                    }
                }
            }
            float transitionSpeed = 6.0f;
            if (g_LoaderConfig.isTransitioning) {
                g_LoaderConfig.pageTransition -= io.DeltaTime * transitionSpeed;
                
                if (g_LoaderConfig.pageTransition <= 0.0f) {
                    g_LoaderConfig.pageTransition = 0.0f;
                    g_LoaderConfig.isTransitioning = false;
                    if (g_LoaderConfig.previousState == State::Login) {
                        g_LoaderConfig.currentState = State::Loading;
                        g_LoaderConfig.loadingSpinnerAlpha = 0.0f;
                        g_LoaderConfig.loadingTime = 0.0f;
                        g_LoaderConfig.spinnerAnimTime = 0.0f;
                    } else if (g_LoaderConfig.previousState == State::Loading) {
                        g_LoaderConfig.currentState = State::Transition;
                    }
                }
            } else if (g_LoaderConfig.pageTransition < 1.0f) {
                g_LoaderConfig.pageTransition += io.DeltaTime * transitionSpeed;
                if (g_LoaderConfig.pageTransition > 1.0f) {
                    g_LoaderConfig.pageTransition = 1.0f;
                }
            }
            if (g_LoaderConfig.currentState == State::Loading) {
                g_LoaderConfig.spinnerAnimTime += io.DeltaTime;
                
                if (!g_LoaderConfig.isTransitioning) {
                    g_LoaderConfig.loadingSpinnerAlpha = ImLerp(g_LoaderConfig.loadingSpinnerAlpha, 1.0f, io.DeltaTime * 8.0f);
                    g_LoaderConfig.loadingTime += io.DeltaTime;
                    if (g_LoaderConfig.loadingTime > 2.0f) {
                        g_LoaderConfig.isTransitioning = true;
                        g_LoaderConfig.previousState = State::Loading;
                        g_LoaderConfig.pageTransition = 1.0f;
                    }
                }
            }
            float pageAlpha = g_LoaderConfig.pageTransition;
            float pageScale = 0.9f + 0.1f * g_LoaderConfig.pageTransition;
            switch (g_LoaderConfig.currentState) {
                case State::Login:
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, pageAlpha * g_LoaderConfig.windowAlpha);
                    RenderLoginPage();
                    ImGui::PopStyleVar();
                    break;
                case State::Loading: {
                    float spinnerAlpha = g_LoaderConfig.isTransitioning ? pageAlpha : g_LoaderConfig.loadingSpinnerAlpha;
                    float spinnerScale = g_LoaderConfig.isTransitioning ? pageScale : (0.8f + 0.2f * g_LoaderConfig.loadingSpinnerAlpha);
                    RenderLoadingPage(spinnerAlpha, spinnerScale);
                    break;
                }
                case State::Transition:
                    RenderTransition();
                    break;
                default:
                    break;
            }
            dl->AddRect(bgMin, bgMax, Colors::Border(), 8.0f, 0, 1.0f);
            
            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(4);
            
            return false;
        }

    }

}
}



