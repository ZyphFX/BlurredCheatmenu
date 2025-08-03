#include "gui.h"
#include <algorithm>
#include <dwmapi.h>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx9.h"

#pragma comment(lib, "dwmapi.lib")

// ✅ Define these before using them
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

void EnableBlurBehind(HWND hwnd)
{
    // Windows 7 Aero Glass
    DWM_BLURBEHIND bb = { 0 };
    bb.dwFlags = DWM_BB_ENABLE;
    bb.fEnable = TRUE;
    bb.hRgnBlur = NULL;
    DwmEnableBlurBehindWindow(hwnd, &bb);

    // Windows 10+ Acrylic Blur
    HMODULE hUser = LoadLibraryA("user32.dll");
    if (hUser)
    {
        typedef HRESULT(WINAPI* pSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);
        auto SetWindowCompositionAttribute =
            (pSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

        if (SetWindowCompositionAttribute)
        {
            ACCENT_POLICY policy = { ACCENT_ENABLE_BLURBEHIND, 0, 0, 0 };
            WINDOWCOMPOSITIONATTRIBDATA data = { 19, &policy, sizeof(policy) };
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

    // ✅ This is required to enable blur on the window
    EnableBlurBehind(window);

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

    ImGui::StyleColorsDark();

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

    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(15, 15, 15, 255), 1.0f, 0);

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
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(WIDTH, HEIGHT));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));



    ImGui::Begin("Aimbot Config", &exit,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar
    );

    // === TOP STRIP DRAGGABLE ZONE ===
    const float dragBarHeight = 20.0f;
    ImVec2 dragZone = ImVec2(ImGui::GetWindowWidth(), dragBarHeight);
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::InvisibleButton("##drag_zone", dragZone, ImGuiButtonFlags_MouseButtonLeft);

    static bool dragging = false;
    static POINT dragStart = {};
    static POINT windowStart = {};

    if (ImGui::IsItemActive() && !dragging)
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

    ImGui::SetCursorPosY(dragBarHeight + 5); // Move below drag zone

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();

    ImGui::Columns(2, nullptr, false);  // Two-column layout

    // --- Left Column ---
    {
        ImGui::Text("Aimbot Settings");
        static int aimbot_mode = 0;
        const char* modes[] = { "Disable", "Enable", "Rage", "Legit" };
        ImGui::Combo("Aimbot mode", &aimbot_mode, modes, IM_ARRAYSIZE(modes));

        static bool auto_fire = false;
        ImGui::Checkbox("Auto fire", &auto_fire);

        static float fire_rating = 25.0f;
        ImGui::SliderFloat("Fire rating", &fire_rating, 0.0f, 50.0f, "(%.1f / 50)");

        static bool auto_resolver = false;
        ImGui::Checkbox("Auto resolver", &auto_resolver);

        static float fov = 100.0f;
        ImGui::SliderFloat("Field of view", &fov, 0.0f, 180.0f, "(%.0f)");

        static bool silent = false;
        ImGui::Checkbox("Silent aim", &silent);

        static bool perfect_silent = false;
        ImGui::Checkbox("Perfect silent", &perfect_silent);

        static bool auto_scope = false;
        ImGui::Checkbox("Auto scope", &auto_scope);
    }

    ImGui::NextColumn();

    // --- Right Column ---
    {
        ImGui::Text("Body Aimbot Settings");
        static int body_aimbot_mode = 0;
        const char* body_modes[] = { "Disable", "Enable" };
        ImGui::Combo("Auto body-aimbot", &body_aimbot_mode, body_modes, IM_ARRAYSIZE(body_modes));

        static float body_before = 25.0f;
        ImGui::SliderFloat("Body before (X HP)", &body_before, 0.0f, 50.0f, "(%.0f / 50)");

        static float body_after = 25.0f;
        ImGui::SliderFloat("Body after (X HP)", &body_after, 0.0f, 50.0f, "(%.0f / 50)");

        static bool velocity_comp = false;
        ImGui::Checkbox("Velocity compensation", &velocity_comp);

        static float spinbot_speed = 100.0f;
        ImGui::SliderFloat("Spinbot speed", &spinbot_speed, 0.0f, 100.0f);

        static float lowerbody_delta = 100.0f;
        ImGui::SliderFloat("Lowerbody delta", &lowerbody_delta, 0.0f, 100.0f);

        static bool anti_aim_target = false;
        ImGui::Checkbox("Anti aim at target", &anti_aim_target);
    }

    ImGui::Columns(1);
    ImGui::End();
}



