// ZyphFx V1.1

#include <windows.h>

#include "gui.h"
#include <algorithm>
#include <dwmapi.h>
#include "styling.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx9.h"


#pragma comment(lib, "dwmapi.lib")

const float dragBarHeight = 25.0f;

// Define these before using them
struct ACCENT_POLICY {
    int nAccentState;
    int nFlags;
    int nColor;
    int nAnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA {
    int Attrib;
    void* pvData;
    size_t cbData;
};

enum AccentState {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4
};

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND window,
    UINT message,
    WPARAM wideParameter,
    LPARAM longParameter
);


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND window,
    UINT message,
    WPARAM wideParameter,
    LPARAM longParameter
);

long __stdcall WindowProcess(HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter)
{
    if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
        return true;

    switch (message)
    {
    case WM_SIZE:
        if (gui::device && wideParameter != SIZE_MINIMIZED)
        {
            gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
            gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
            gui::ResetDevice();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wideParameter & 0xfff0) == SC_KEYMENU) // disable ALT application menu
            return 0;
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_LBUTTONDOWN:
        ReleaseCapture();
        SendMessage(window, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return 0;




    case WM_LBUTTONUP:
        return 0;

    case WM_MOUSEMOVE:
        if (wideParameter == MK_LBUTTON)
        {
            // Optional: Handle window dragging if needed
        }
        return 0;
    }

    return DefWindowProcW(window, message, wideParameter, longParameter);
}




// _________________-- GUI Functions --________________

void EnableBlurBehind(HWND hwnd, int width, int height)
{

    // Enable blur behind inside the rounded region
    DWM_BLURBEHIND bb = {};
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.fEnable = TRUE;
    DwmEnableBlurBehindWindow(hwnd, &bb);

    // Optional: Windows 10+ blur without tint (remove black box effect)
    HMODULE hUser = LoadLibraryA("user32.dll");
    if (hUser)
    {
        using pSetWindowCompositionAttribute = BOOL(WINAPI*)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);
        auto SetWindowCompositionAttribute = reinterpret_cast<pSetWindowCompositionAttribute>(
            GetProcAddress(hUser, "SetWindowCompositionAttribute"));

        if (SetWindowCompositionAttribute)
        {
            ACCENT_POLICY policy = {};
            policy.nAccentState = ACCENT_ENABLE_BLURBEHIND; // No tint acrylic blur
            policy.nFlags = 0;
            policy.nColor = 0x00000000; // ARGB: transparent tint (no black)

            WINDOWCOMPOSITIONATTRIBDATA data = {};
            data.Attrib = 19; // WCA_ACCENT_POLICY
            data.pvData = &policy;
            data.cbData = sizeof(policy);

            SetWindowCompositionAttribute(hwnd, &data);
        }
        FreeLibrary(hUser);
    }
}


void gui::CreateHWindow(
    const char* windowName,
    const char* className) noexcept
{
    windowClass.cbSize = sizeof(WNDCLASSEXA);
    windowClass.style = CS_CLASSDC;
    windowClass.lpfnWndProc = WindowProcess;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = GetModuleHandleA(0);
    windowClass.hIcon = 0;
    windowClass.hCursor = 0;
    windowClass.hbrBackground = 0;
    windowClass.lpszMenuName = 0;
    windowClass.lpszClassName = className;
    windowClass.hIconSm = 0;

    RegisterClassExA(&windowClass);



    window = CreateWindowExA(
        WS_EX_LAYERED,                      // Extended style (enable per-pixel alpha)
        className,
        windowName,
        WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU,
        700, 200,
        WIDTH, HEIGHT,
        nullptr,
        nullptr,
        windowClass.hInstance,
        nullptr
    );

    SetLayeredWindowAttributes(window, RGB(0, 0, 0), 255, LWA_ALPHA);





    // Only works on Windows 11
    enum DWM_WINDOW_CORNER_PREFERENCE {
        DWMWCP_DEFAULT = 0,
        DWMWCP_DONOTROUND = 1,
        DWMWCP_ROUND = 2,
        DWMWCP_ROUNDSMALL = 3
    };

    DWM_WINDOW_CORNER_PREFERENCE cornerPreference = DWMWCP_ROUND;
    DwmSetWindowAttribute(
        window,                         // your HWND
        33,                             // DWMWA_WINDOW_CORNER_PREFERENCE = 33
        &cornerPreference,
        sizeof(cornerPreference)
    );

    BOOL disableShadow = TRUE;

    // This disables the system drop shadow (which is the white/gray outline you're seeing)
    DwmSetWindowAttribute(
        window,
        2, // DWMWA_NCRENDERING_POLICY
        &disableShadow,
        sizeof(disableShadow)
    );

    // This ensures DWM doesn't draw any border shadow either
    DwmSetWindowAttribute(
        window,
        3, // DWMWA_NCRENDERING_ENABLED
        &disableShadow,
        sizeof(disableShadow)
    );

    // ✅ This is required to enable blur on the window
    EnableBlurBehind(window, 1, 1);

    ShowWindow(window, SW_SHOWDEFAULT);
    UpdateWindow(window);
}


void gui::DestroyHWindow() noexcept
{
    DestroyWindow(window);
    UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept
{
    d3d = Direct3DCreate9(D3D_SDK_VERSION);

    if (!d3d)
        return false;

    ZeroMemory(&presentParameters, sizeof(presentParameters));

    presentParameters.Windowed = TRUE;
    presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    presentParameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    presentParameters.EnableAutoDepthStencil = TRUE;
    presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
    presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    if (d3d->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        window,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &presentParameters,
        &device) < 0)
        return false;

    return true;

}

void gui::ResetDevice() noexcept
{
    ImGui_ImplDX9_InvalidateDeviceObjects();

    const auto result = device->Reset(&presentParameters);

    if (result == D3DERR_INVALIDCALL)
        IM_ASSERT(0);

    ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
    if (device)
    {
        device->Release();
        device = nullptr;
    }

    if (d3d)
    {
        d3d->Release();
        device = nullptr;
    }
}

void gui::CreateImGui() noexcept
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ::ImGui::GetIO();

    io.IniFilename = NULL;

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX9_Init(device);

    ImGui::GetStyle().Alpha = 1.0f;

}

void gui::DestroyImGui() noexcept
{
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0u, 0u, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}




void gui::EndRender() noexcept
{
    ImGui::EndFrame();



    if (device->BeginScene() >= 0)
    {
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        device->EndScene();


    }

    const auto result = device->Present(nullptr, nullptr, nullptr, nullptr);

    // handle lost device
    if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
        gui::ResetDevice();
}

void gui::Render() noexcept
{
    BeginRender();

    {

        device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
            D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);


        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(200, HEIGHT), ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.1f)); // Transparent background
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));    // No border

        ImGui::Begin("ZyphFx", nullptr,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings);

        // You can add widgets here...

        ImGui::End();

        // 👇 Pop after End()
        ImGui::PopStyleColor(2);
    }

    EndRender();
}

