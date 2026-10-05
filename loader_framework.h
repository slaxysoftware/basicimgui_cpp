#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include <string>

namespace KrxSlaxy {
namespace Loader {
    enum class State {
        Login,
        Loading,
        Transition,
        Complete
    };
    struct LoaderConfig {
        State currentState = State::Login;
        State previousState = State::Login;
        char username[64] = "";
        char password[64] = "";
        bool rememberMe = false;
        bool loginFailed = false;
        std::string errorMessage = "";
        float windowAlpha = 0.0f;
        float windowScale = 0.95f;
        float transitionProgress = 0.0f;
        float pageTransition = 1.0f;
        float loadingSpinnerAlpha = 0.0f;
        bool isTransitioning = false;
        bool usernameFocused = false;
        bool passwordFocused = false;
        bool loginButtonHovered = false;
        float loginButtonAnim = 0.0f;
        float loadingTime = 0.0f;
        float spinnerAnimTime = 0.0f;
    };
    extern LoaderConfig g_LoaderConfig;
    namespace GUI {
        void Initialize();
        bool Render();
        void RenderLoginPage();
        void RenderLoadingPage();
        void RenderTransition();
        bool InputField(const char* label, const char* placeholder, char* buffer, size_t bufferSize, bool isPassword, bool* focused);
        bool Checkbox(const char* label, bool* value);
        bool Button(const char* label, float width = 0.0f);
    }

}
}

