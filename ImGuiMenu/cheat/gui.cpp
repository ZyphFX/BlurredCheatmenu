// ZyphFx V1.1


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

long __stdcall WindowProcess(
    HWND window,
    UINT message,
    WPARAM wideParameter,
    LPARAM longParameter)
{
    if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
        return true;

    switch (message)
    {
    case WM_SIZE: {
        if (gui::device && wideParameter != SIZE_MINIMIZED)
        {
            gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
            gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
            gui::ResetDevice();
        }
    } return 0;

    case WM_SYSCOMMAND: {
        if ((wideParameter & 0xfff0) == SC_KEYMENU) // disable ALT application menu
            return 0;
    } break;

    case WM_DESTROY: {
        PostQuitMessage(0);
    } return 0;

    case WM_LBUTTONDOWN: {
        gui::position = MAKEPOINTS(longParameter); // set click points
    } return 0;

    case WM_MOUSEMOVE: {
        if (wideParameter == MK_LBUTTON)
        {
            const auto points = MAKEPOINTS(longParameter);
            auto rect = ::RECT{};

            GetWindowRect(gui::window, &rect);

            rect.left += points.x - gui::position.x;
            rect.top += points.y - gui::position.y;

            if (gui::position.x >= 0 &&
                gui::position.x <= gui::WIDTH &&
                gui::position.y >= 0 && gui::position.y <= 19)
            {
                SetWindowPos(
                    gui::window,
                    HWND_TOPMOST,
                    rect.left,
                    rect.top,
                    0, 0,
                    SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
                );
            }
        }
        return 0;
    }
    }

    return DefWindowProcW(window, message, wideParameter, longParameter);
}

// _________________-- GUI Functions --________________

void EnableBlurBehind(HWND hwnd, int width, int height)
{
    // Create a rounded region for both blur and clipping
    HRGN hRgn = CreateRoundRectRgn(0, 0, width + 1, height + 1, 32, 32);

    // Enable blur behind inside the rounded region
    DWM_BLURBEHIND bb = {};
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.fEnable = TRUE;
    bb.hRgnBlur = hRgn;
    DwmEnableBlurBehindWindow(hwnd, &bb);

    // Clip the actual window to the rounded region
    SetWindowRgn(hwnd, hRgn, TRUE);

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


void ApplyWindowRounding(HWND hwnd, int radius = 16) {
    RECT rect;
    GetWindowRect(hwnd, &rect);

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    HRGN region = CreateRoundRectRgn(0, 0, width, height, radius, radius);
    SetWindowRgn(hwnd, region, TRUE);
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




    window = CreateWindowA(
        className,
        windowName,
        WS_POPUP,
        100,
        100,
        WIDTH,
        HEIGHT,
        0,
        0,
        windowClass.hInstance,
        0
    );




    // Use WIDTH and HEIGHT (you already have those defined)
    HRGN hrgn = CreateRoundRectRgn(0, 0, WIDTH + 1, HEIGHT + 1, 32, 32);


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
    EnableBlurBehind(window, WIDTH, HEIGHT);
    ApplyWindowRounding(gui::window, 32);


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
    presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
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
}

void gui::DestroyImGui() noexcept
{
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
    MSG message;
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    // start imgui menu frame
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}



void gui::EndRender() noexcept
{
    ImGui::EndFrame();

    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

    if (device->BeginScene() >= 0)
    {
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
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(WIDTH, HEIGHT));

    // Transparent window + no border + no child bg + no table bg
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));

    ImGui::Begin("Aimbot Config", &exit,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar
    );

    // --- Dragging Logic ---
    {
        static bool dragging = false;
        static POINT dragStart = {};
        static POINT windowStart = {};

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
            !ImGui::IsAnyItemHovered() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            dragging = true;
            GetCursorPos(&dragStart);

            RECT rect;
            GetWindowRect(gui::window, &rect);
            windowStart.x = rect.left;
            windowStart.y = rect.top;
        }

        if (dragging)
        {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                dragging = false;
            }
            else
            {
                POINT current;
                GetCursorPos(&current);

                int dx = current.x - dragStart.x;
                int dy = current.y - dragStart.y;

                SetWindowPos(gui::window, HWND_TOPMOST,
                    windowStart.x + dx,
                    windowStart.y + dy,
                    0, 0,
                    SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
            }
        }
    }


    // -- Actual layout --
    {

    }


    ImGui::PopStyleColor(3); // 5 colors pushed above

    ImGui::End();
}
