#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "menu_framework.h"
#include "loader_framework.h"
#include "imgui_text_renderer.h"
#include "badcache.h"
#include "verdana.h"
#include "verdanabold.h"
#include "smalle.h"
#include "menubackground.h"
#include "inter_fonts.h"
#include "fontawesome/RawAwesome6.hpp"
#include <d3d11.h>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <tchar.h>
HWND                            g_hWnd = nullptr;
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static constexpr float kWatermarkCpuStub = 52.0f;
static constexpr float kWatermarkGpuStub = 90.0f;
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"KrxSlaxyWindow", nullptr };
    ::RegisterClassExW(&wc);
    const int loader_width = (int)(340 * main_scale);
    const int loader_height = (int)(280 * main_scale);
    const int window_x = (::GetSystemMetrics(SM_CXSCREEN) - loader_width) / 2;
    const int window_y = (::GetSystemMetrics(SM_CYSCREEN) - loader_height) / 2;
    HWND hwnd = ::CreateWindowExW(0, wc.lpszClassName, L"KRX SLAXY", WS_POPUP, window_x, window_y, loader_width, loader_height, nullptr, nullptr, wc.hInstance, nullptr);
    g_hWnd = hwnd;

    #ifndef DWMWA_WINDOW_CORNER_PREFERENCE
    #define DWMWA_WINDOW_CORNER_PREFERENCE 33
    #endif
    #ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
    #define DWMWA_USE_IMMERSIVE_DARK_MODE 20
    #endif
    BOOL darkMode = TRUE;
    ::DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    DWORD cornerPref = 2;
    ::DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    ::ShowWindow(hwnd, nCmdShow);
    ::UpdateWindow(hwnd);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;
    fontConfig.OversampleH = 3;
    fontConfig.OversampleV = 2;
    fontConfig.PixelSnapH = true;

    static const ImWchar turkishRanges[] =
    {
        0x0020, 0x00FF,
        0x0100, 0x017F,
        0x0400, 0x052F,
        0x2000, 0x206F,
        0x20AC, 0x20BA,
        0
    };

    KrxSlaxy::g_FontRegular = io.Fonts->AddFontFromMemoryTTF(
        (void*)inter_medium.data(), (int)inter_medium.size(), 14.0f * main_scale, &fontConfig, turkishRanges
    );

    KrxSlaxy::g_FontBold = io.Fonts->AddFontFromMemoryTTF(
        (void*)inter_semibold.data(), (int)inter_semibold.size(), 14.0f * main_scale, &fontConfig, turkishRanges
    );

    KrxSlaxy::g_FontTitle = io.Fonts->AddFontFromMemoryTTF(
        (void*)inter_semibold.data(), (int)inter_semibold.size(), 16.0f * main_scale, &fontConfig, turkishRanges
    );

    KrxSlaxy::g_FontBadge = io.Fonts->AddFontFromMemoryTTF(
        (void*)inter_semibold.data(), (int)inter_semibold.size(), 11.0f * main_scale, &fontConfig, turkishRanges
    );

    static const ImWchar iconRanges[] = { 0x41, 0x49, 0 };
    fontConfig.FontDataOwnedByAtlas = false;
    fontConfig.GlyphMinAdvanceX = 28.0f;
    KrxSlaxy::g_IconFont = io.Fonts->AddFontFromMemoryTTF(
        (void*)badcache, sizeof(badcache), 28.0f * main_scale, &fontConfig, iconRanges
    );
    static const ImWchar faRanges[] = { 0xf000, 0xf8ff, 0 };
    fontConfig.FontDataOwnedByAtlas = false;
    fontConfig.GlyphMinAdvanceX = 14.0f;
    KrxSlaxy::g_FontAwesome = io.Fonts->AddFontFromMemoryCompressedTTF(
        FontAwesome6Solid_compressed_data, FontAwesome6Solid_compressed_size,
        14.0f * main_scale, &fontConfig, faRanges
    );
    fontConfig.FontDataOwnedByAtlas = false;
    fontConfig.GlyphMinAdvanceX = 0.0f;
    KrxSlaxy::g_PixelFont = io.Fonts->AddFontFromMemoryTTF(
        (void*)smalle, sizeof(smalle), 8.0f * main_scale, &fontConfig
    );
    io.Fonts->Build();
    g_TextRenderer.Init(g_pd3dDevice, g_pd3dDeviceContext);
    g_TextFont.fontFamily = "Inter";
    g_TextFont.size = (int)(14.0f * main_scale);
    g_TextFont.weight = FW_NORMAL;
    g_TextFont.antialiased = true;
    {
        int width, height, channels;
        unsigned char* imageData = stbi_load_from_memory(
            menuBackground, sizeof(menuBackground), &width, &height, &channels, 4
        );

        if (imageData) {
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = width;
            desc.Height = height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA subResource = {};
            subResource.pSysMem = imageData;
            subResource.SysMemPitch = width * 4;

            ID3D11Texture2D* pTexture = nullptr;
            g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

            if (pTexture) {
                D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = 1;

                ID3D11ShaderResourceView* pSRV = nullptr;
                g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &pSRV);

                KrxSlaxy::g_BackgroundTexture = (void*)pSRV;
                KrxSlaxy::g_BackgroundWidth = width;
                KrxSlaxy::g_BackgroundHeight = height;

                pTexture->Release();
            }

            stbi_image_free(imageData);
        }
    }
    bool show_demo_window = false;
    enum class AppState {
        Loader,
        Menu
    };
    AppState appState = AppState::Loader;
    KrxSlaxy::GUI::Initialize();
    KrxSlaxy::Loader::GUI::Initialize();
    KrxSlaxy::Colors::GlobalAlpha() = 1.0f;
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (appState == AppState::Loader) {
            bool loaderComplete = KrxSlaxy::Loader::GUI::Render();
            if (loaderComplete) {
                appState = AppState::Menu;
                const int menu_w = (int)(960 * main_scale);
                const int menu_h = (int)(700 * main_scale);
                const int menu_x = (::GetSystemMetrics(SM_CXSCREEN) - menu_w) / 2;
                const int menu_y = (::GetSystemMetrics(SM_CYSCREEN) - menu_h) / 2;
                ::SetWindowPos(hwnd, nullptr, menu_x, menu_y, menu_w, menu_h, SWP_NOZORDER | SWP_FRAMECHANGED);
            }
        } else {
            KrxSlaxy::Colors::GlobalAlpha() = 1.0f;
            KrxSlaxy::GUI::Render();
        }

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 8.0f / 255.0f, 11.0f / 255.0f, 18.0f / 255.0f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }
    g_TextRenderer.Shutdown();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}


