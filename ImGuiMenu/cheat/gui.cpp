// ZyphFx V1.1

#include <windows.h>
#include <windowsx.h>

#include "gui.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <dwmapi.h>
#include <string>
#include "styling.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx9.h"


#pragma comment(lib, "dwmapi.lib")

const float dragBarHeight = 25.0f;

namespace
{
    constexpr ImVec4 Glass = ImVec4(0.030f, 0.028f, 0.045f, 0.30f);
    constexpr ImVec4 Background = ImVec4(0.035f, 0.031f, 0.039f, 0.00f);
    constexpr ImVec4 Panel = ImVec4(0.070f, 0.070f, 0.110f, 0.50f);
    constexpr ImVec4 SidebarPanel = ImVec4(0.060f, 0.060f, 0.095f, 0.38f);
    constexpr ImVec4 PanelSoft = ImVec4(0.110f, 0.105f, 0.145f, 0.34f);
    constexpr ImVec4 Stroke = ImVec4(1.000f, 1.000f, 1.000f, 0.075f);
    constexpr ImVec4 Text = ImVec4(0.890f, 0.865f, 0.920f, 1.00f);
    constexpr ImVec4 TextMuted = ImVec4(0.560f, 0.520f, 0.610f, 1.00f);
    ImVec4 Accent = ImVec4(0.610f, 0.355f, 0.930f, 1.00f);
    ImVec4 AccentHot = ImVec4(0.800f, 0.530f, 1.000f, 1.00f);
    ImVec4 Support = ImVec4(0.080f, 0.560f, 1.000f, 1.00f);
    ImVec4 Success = ImVec4(0.470f, 0.860f, 0.060f, 1.00f);

    void ApplyShowcaseStyle();

    ImVec4 Mix(const ImVec4& a, const ImVec4& b, float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);
        return ImVec4(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t
        );
    }

    ImU32 Color(const ImVec4& value)
    {
        return ImGui::ColorConvertFloat4ToU32(value);
    }

    void SetThemeAccent(const ImVec4& color)
    {
        Accent = ImVec4(color.x, color.y, color.z, 1.0f);
        AccentHot = Mix(Accent, ImVec4(1.0f, 0.86f, 1.0f, 1.0f), 0.34f);
        Support = Mix(Accent, ImVec4(0.050f, 0.560f, 1.000f, 1.0f), 0.50f);
        Success = Mix(Accent, ImVec4(0.250f, 0.880f, 0.250f, 1.0f), 0.62f);
        ApplyShowcaseStyle();
    }

    void HexColor(const ImVec4& color, char* buffer, size_t bufferSize)
    {
        const int r = static_cast<int>(std::clamp(color.x, 0.0f, 1.0f) * 255.0f + 0.5f);
        const int g = static_cast<int>(std::clamp(color.y, 0.0f, 1.0f) * 255.0f + 0.5f);
        const int b = static_cast<int>(std::clamp(color.z, 0.0f, 1.0f) * 255.0f + 0.5f);
        snprintf(buffer, bufferSize, "%02X%02X%02X", r, g, b);
    }

    float AnimateFloat(ImGuiID id, float target, float speed = 12.0f)
    {
        ImGuiStorage* storage = ImGui::GetStateStorage();
        const float current = storage->GetFloat(id, target);
        const float step = std::clamp(ImGui::GetIO().DeltaTime * speed, 0.0f, 1.0f);
        const float next = current + (target - current) * step;
        storage->SetFloat(id, next);
        return next;
    }

    void ApplyShowcaseStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(0.0f, 0.0f);
        style.FramePadding = ImVec2(11.0f, 8.0f);
        style.ItemSpacing = ImVec2(8.0f, 10.0f);
        style.WindowRounding = 16.0f;
        style.ChildRounding = 12.0f;
        style.FrameRounding = 8.0f;
        style.PopupRounding = 8.0f;
        style.ScrollbarRounding = 8.0f;
        style.GrabRounding = 8.0f;
        style.WindowBorderSize = 0.0f;
        style.ChildBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text] = Text;
        colors[ImGuiCol_TextDisabled] = TextMuted;
        colors[ImGuiCol_WindowBg] = Background;
        colors[ImGuiCol_ChildBg] = Panel;
        colors[ImGuiCol_Border] = Stroke;
        colors[ImGuiCol_FrameBg] = ImVec4(0.115f, 0.112f, 0.155f, 0.56f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.160f, 0.135f, 0.205f, 0.68f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.200f, 0.150f, 0.260f, 0.76f);
        colors[ImGuiCol_Button] = ImVec4(0.115f, 0.097f, 0.145f, 0.55f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.205f, 0.145f, 0.285f, 0.72f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.300f, 0.190f, 0.430f, 0.82f);
        colors[ImGuiCol_SliderGrab] = Accent;
        colors[ImGuiCol_SliderGrabActive] = AccentHot;
        colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    void DrawFilledText(ImDrawList* draw, const ImVec2& pos, ImU32 color, const char* text)
    {
        draw->AddText(pos, color, text);
    }

    void DrawIslandBottomFade(const ImVec2& min, const ImVec2& max)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const float fadeHeight = 54.0f;
        const float fadeTop = (max.y - fadeHeight > min.y + 1.0f) ? max.y - fadeHeight : min.y + 1.0f;
        const ImVec2 fadeMin(min.x + 1.0f, fadeTop);
        const ImVec2 fadeMax(max.x - 1.0f, max.y - 1.0f);
        const ImU32 transparent = Color(ImVec4(0.070f, 0.050f, 0.120f, 0.00f));
        const ImU32 bottom = Color(ImVec4(0.110f, 0.075f, 0.220f, 0.26f));

        draw->PushClipRect(min, max, true);
        draw->AddRectFilledMultiColor(fadeMin, fadeMax, transparent, transparent, bottom, bottom);
        draw->AddRectFilled(
            ImVec2(min.x + 1.0f, max.y - 18.0f),
            ImVec2(max.x - 1.0f, max.y - 1.0f),
            Color(ImVec4(0.050f, 0.042f, 0.095f, 0.12f)),
            12.0f,
            ImDrawFlags_RoundCornersBottom
        );
        draw->AddLine(
            ImVec2(min.x + 20.0f, max.y - 1.5f),
            ImVec2(max.x - 20.0f, max.y - 1.5f),
            Color(ImVec4(Accent.x, Accent.y, Accent.z, 0.18f)),
            1.0f
        );
        draw->PopClipRect();
    }

    void TextMutedLine(const char* text)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, TextMuted);
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
    }

    void UpdateSmoothScroll()
    {
        const float maxScroll = ImGui::GetScrollMaxY();
        if (maxScroll <= 0.0f)
            return;

        ImGuiStorage* storage = ImGui::GetStateStorage();
        const ImGuiID targetId = ImGui::GetID("smooth_scroll_target");
        const ImGuiID initializedId = ImGui::GetID("smooth_scroll_initialized");
        const float current = ImGui::GetScrollY();

        if (storage->GetInt(initializedId, 0) == 0)
        {
            storage->SetFloat(targetId, current);
            storage->SetInt(initializedId, 1);
        }

        float target = storage->GetFloat(targetId, current);
        const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        const float wheel = ImGui::GetIO().MouseWheel;

        if (hovered && wheel != 0.0f)
        {
            target = std::clamp(target - wheel * 82.0f, 0.0f, maxScroll);
            storage->SetFloat(targetId, target);
        }
        else if (std::abs(current - target) < 0.35f)
        {
            target = current;
            storage->SetFloat(targetId, target);
        }

        const float speed = 13.0f;
        const float step = std::clamp(ImGui::GetIO().DeltaTime * speed, 0.0f, 1.0f);
        const float next = current + (target - current) * step;
        ImGui::SetScrollY(next);
    }

    bool IconButtonSmall(const char* id, const char* text)
    {
        ImGui::PushID(id);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 5.0f));
        const bool pressed = ImGui::Button(text, ImVec2(30.0f, 28.0f));
        ImGui::PopStyleVar(2);
        ImGui::PopID();
        return pressed;
    }

    bool ToggleSwitch(const char* id, bool* value)
    {
        ImGui::PushID(id);
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 size(42.0f, 22.0f);
        ImGui::InvisibleButton("toggle", size);
        const bool clicked = ImGui::IsItemClicked();
        if (clicked)
            *value = !*value;

        const float anim = AnimateFloat(ImGui::GetID("toggle_anim"), *value ? 1.0f : 0.0f, 14.0f);
        const bool hovered = ImGui::IsItemHovered();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImU32 bg = Color(Mix(ImVec4(0.155f, 0.140f, 0.180f, 1.0f), hovered ? AccentHot : Accent, anim));
        draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bg, 11.0f);
        draw->AddCircleFilled(ImVec2(pos.x + 11.0f + anim * 20.0f, pos.y + 11.0f), 7.0f, Color(ImVec4(0.965f, 0.940f, 0.985f, 1.0f)));
        ImGui::PopID();
        return clicked;
    }

    bool AnimatedCheckbox(const char* label, bool* value)
    {
        ImGui::PushID(label);
        const ImVec2 start = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        const ImVec2 size(width, 24.0f);
        ImGui::InvisibleButton("checkbox_row", size);
        const bool pressed = ImGui::IsItemClicked();
        if (pressed)
            *value = !*value;

        const bool hovered = ImGui::IsItemHovered();
        const float anim = AnimateFloat(ImGui::GetID("check_anim"), *value ? 1.0f : 0.0f, 15.0f);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 boxMin(start.x, start.y + 3.0f);
        const ImVec2 boxMax(start.x + 18.0f, start.y + 21.0f);
        const ImVec4 off = hovered ? ImVec4(0.165f, 0.145f, 0.195f, 1.0f) : ImVec4(0.120f, 0.108f, 0.140f, 1.0f);
        draw->AddRectFilled(boxMin, boxMax, Color(Mix(off, Accent, anim)), 4.0f);
        draw->AddRect(boxMin, boxMax, Color(Mix(Stroke, AccentHot, anim)), 4.0f, 0, 1.0f);
        if (anim > 0.02f)
        {
            const ImU32 check = Color(ImVec4(1.0f, 1.0f, 1.0f, anim));
            draw->AddLine(ImVec2(start.x + 5.0f, start.y + 12.0f), ImVec2(start.x + 8.0f, start.y + 15.0f), check, 2.0f);
            draw->AddLine(ImVec2(start.x + 8.0f, start.y + 15.0f), ImVec2(start.x + 14.0f, start.y + 8.0f), check, 2.0f);
        }

        DrawFilledText(draw, ImVec2(start.x + 28.0f, start.y + 3.0f), Color(hovered ? Text : ImVec4(0.760f, 0.720f, 0.820f, 1.0f)), label);
        ImGui::PopID();
        return pressed;
    }

    bool SmoothSliderFloat(const char* label, float* value, float minValue, float maxValue, const char* suffix = "")
    {
        ImGui::PushID(label);
        ImGui::TextUnformatted(label);
        const ImVec2 start = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        const ImVec2 size(width, 24.0f);
        ImGui::InvisibleButton("slider", size);

        bool changed = false;
        if (ImGui::IsItemActive())
        {
            const float mouse = ImGui::GetIO().MousePos.x;
            const float t = std::clamp((mouse - start.x) / width, 0.0f, 1.0f);
            *value = minValue + (maxValue - minValue) * t;
            changed = true;
        }

        const float target = (*value - minValue) / (maxValue - minValue);
        const float anim = AnimateFloat(ImGui::GetID("slider_anim"), target, 18.0f);
        const float knobX = start.x + anim * width;
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(ImVec2(start.x, start.y + 10.0f), ImVec2(start.x + width, start.y + 14.0f), Color(ImVec4(0.115f, 0.105f, 0.135f, 1.0f)), 4.0f);
        draw->AddRectFilled(ImVec2(start.x, start.y + 10.0f), ImVec2(knobX, start.y + 14.0f), Color(Accent), 4.0f);
        draw->AddCircleFilled(ImVec2(knobX, start.y + 12.0f), 7.0f, Color(ImVec4(0.880f, 0.760f, 1.0f, 1.0f)));
        draw->AddCircle(ImVec2(knobX, start.y + 12.0f), 7.0f, Color(AccentHot), 0, 1.0f);

        char buffer[32] = {};
        snprintf(buffer, sizeof(buffer), "%.0f%s", *value, suffix);
        const ImVec2 textSize = ImGui::CalcTextSize(buffer);
        DrawFilledText(draw, ImVec2(start.x + width - textSize.x, start.y - 22.0f), Color(TextMuted), buffer);
        ImGui::Dummy(ImVec2(width, 3.0f));
        ImGui::PopID();
        return changed;
    }

    bool NavItem(const char* label, const char* symbol, bool selected)
    {
        ImGui::PushID(label);
        const ImVec2 start = ImGui::GetCursorScreenPos();
        const ImVec2 size(ImGui::GetContentRegionAvail().x, 38.0f);
        ImGui::InvisibleButton("nav", size);
        const bool pressed = ImGui::IsItemClicked();
        const bool hovered = ImGui::IsItemHovered();
        const float anim = AnimateFloat(ImGui::GetID("nav_anim"), selected ? 1.0f : hovered ? 0.45f : 0.0f, 11.0f);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(start, ImVec2(start.x + size.x, start.y + size.y), Color(Mix(ImVec4(0, 0, 0, 0), ImVec4(Accent.x, Accent.y, Accent.z, 0.55f), anim)), 7.0f);
        if (selected)
            draw->AddRectFilled(ImVec2(start.x, start.y + 8.0f), ImVec2(start.x + 3.0f, start.y + size.y - 8.0f), Color(AccentHot), 2.0f);

        DrawFilledText(draw, ImVec2(start.x + 14.0f, start.y + 10.0f), Color(selected ? AccentHot : TextMuted), symbol);
        DrawFilledText(draw, ImVec2(start.x + 42.0f, start.y + 10.0f), Color(selected ? Text : TextMuted), label);
        ImGui::PopID();
        return pressed;
    }

    void BeginCard(const char* title, const char* subtitle, float height, bool* enabled = nullptr)
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Panel);
        ImGui::PushStyleColor(ImGuiCol_Border, Stroke);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 16.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 12.0f));
        ImGui::BeginChild(title, ImVec2(0.0f, height), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PushStyleColor(ImGuiCol_Text, Text);
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
        if (enabled)
        {
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 42.0f);
            ToggleSwitch("card_toggle", enabled);
        }
        TextMutedLine(subtitle);
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
    }

    void EndCard()
    {
        UpdateSmoothScroll();
        ImGui::EndChild();
        DrawIslandBottomFade(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    bool ThemeColorSelector(const char* label)
    {
        bool changed = false;
        char hex[16] = {};
        HexColor(Accent, hex, sizeof(hex));

        ImGui::TextUnformatted(label);
        const ImVec2 rowPos = ImGui::GetCursorScreenPos();
        const float rowWidth = ImGui::GetContentRegionAvail().x;
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(rowPos, ImVec2(rowPos.x + rowWidth, rowPos.y + 38.0f), Color(PanelSoft), 8.0f);
        draw->AddCircleFilled(ImVec2(rowPos.x + 22.0f, rowPos.y + 19.0f), 8.0f, Color(Accent));
        DrawFilledText(draw, ImVec2(rowPos.x + 44.0f, rowPos.y + 11.0f), Color(Text), "Menu accent");
        DrawFilledText(draw, ImVec2(rowPos.x + rowWidth - 86.0f, rowPos.y + 11.0f), Color(TextMuted), hex);

        ImGui::SetCursorScreenPos(rowPos);
        ImGui::InvisibleButton("accent_row", ImVec2(rowWidth, 38.0f));
        if (ImGui::IsItemClicked())
            ImGui::OpenPopup("accent_picker");

        ImGui::SetNextWindowSize(ImVec2(280.0f, 322.0f), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.060f, 0.055f, 0.085f, 0.96f));
        ImGui::PushStyleColor(ImGuiCol_Border, Stroke);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 14.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 12.0f);
        if (ImGui::BeginPopup("accent_picker"))
        {
            ImGui::TextUnformatted("Menu accent");
            TextMutedLine("Drag inside the square or hue bar");
            ImGui::Dummy(ImVec2(0.0f, 4.0f));

            ImVec4 edited = Accent;
            const ImGuiColorEditFlags flags =
                ImGuiColorEditFlags_NoSidePreview |
                ImGuiColorEditFlags_NoSmallPreview |
                ImGuiColorEditFlags_NoAlpha |
                ImGuiColorEditFlags_DisplayHex |
                ImGuiColorEditFlags_PickerHueBar;

            ImGui::SetNextItemWidth(252.0f);
            if (ImGui::ColorPicker4("##accent_picker_square", &edited.x, flags))
            {
                SetThemeAccent(edited);
                changed = true;
            }

            HexColor(Accent, hex, sizeof(hex));
            ImGui::Dummy(ImVec2(0.0f, 4.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, TextMuted);
            ImGui::TextUnformatted(hex);
            ImGui::PopStyleColor();
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 28.0f);
            ImGui::ColorButton("##accent_preview", Accent, ImGuiColorEditFlags_NoTooltip, ImVec2(24.0f, 24.0f));
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        return changed;
    }

    void ColorRow(const char* label, const ImVec4& color)
    {
        char hex[16] = {};
        HexColor(color, hex, sizeof(hex));
        ImGui::TextUnformatted(label);
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + 28.0f), Color(PanelSoft), 5.0f);
        draw->AddCircleFilled(ImVec2(pos.x + 18.0f, pos.y + 14.0f), 7.0f, Color(color));
        DrawFilledText(draw, ImVec2(pos.x + 38.0f, pos.y + 7.0f), Color(TextMuted), hex);
        DrawFilledText(draw, ImVec2(pos.x + width - 22.0f, pos.y + 6.0f), Color(TextMuted), "o");
        ImGui::Dummy(ImVec2(width, 32.0f));
    }
}

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


LRESULT CALLBACK WindowProcess(HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter)
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
        gui::exit = false;
        PostQuitMessage(0);
        return 0;

    case WM_NCHITTEST:
    {
        POINT cursor = { GET_X_LPARAM(longParameter), GET_Y_LPARAM(longParameter) };
        ScreenToClient(window, &cursor);

        const bool insideWindow =
            cursor.x >= 0 && cursor.x < gui::WIDTH &&
            cursor.y >= 0 && cursor.y < gui::HEIGHT;
        const bool insideTitleBar = cursor.y >= 0 && cursor.y < 55;
        const bool overWindowButtons = cursor.x >= gui::WIDTH - 102 && cursor.y < 48;

        if (insideWindow && insideTitleBar && !overWindowButtons)
        {
            return HTCAPTION;
        }
        break;
    }
    }

    return DefWindowProcA(window, message, wideParameter, longParameter);
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
            policy.nAccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
            policy.nFlags = 0;
            policy.nColor = 0x55000000; // ARGB tint: subtle dark acrylic, not an opaque cover.

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

    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(window, &margins);


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
    UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
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
    io.Fonts->AddFontDefault();
    if (io.Fonts->AddFontFromFileTTF("ImGuiMenu\\assets\\fonts\\KamishirasawaSans.ttf", 15.0f) != nullptr)
        io.FontDefault = io.Fonts->Fonts.back();

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX9_Init(device);

    ApplyShowcaseStyle();
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

        static int selectedTab = 0;
        static bool moduleA = true;
        static bool moduleB = true;
        static bool moduleC = false;
        static bool moduleD = true;
        static bool checkA = true;
        static bool checkB = true;
        static bool checkC = false;
        static bool checkD = true;
        static bool checkE = false;
        static bool checkF = true;
        static bool checkG = false;
        static float range = 208.0f;
        static float intensity = 64.0f;
        static float spacing = 42.0f;
        static float opacity = 86.0f;
        static int mode = 0;
        static int palette = 1;

        const std::array<const char*, 4> modes = { "Soft", "Balanced", "Sharp", "Custom" };
        const std::array<const char*, 4> palettes = { "Violet", "Coral", "Ocean", "Lime" };
        const std::array<ImVec4, 4> paletteColors = {
            ImVec4(0.610f, 0.355f, 0.930f, 1.0f),
            ImVec4(1.000f, 0.365f, 0.365f, 1.0f),
            ImVec4(0.170f, 0.510f, 1.000f, 1.0f),
            ImVec4(0.470f, 0.860f, 0.060f, 1.0f)
        };

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(WIDTH), static_cast<float>(HEIGHT)), ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, Background);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

        ImGui::Begin("Showcase", nullptr,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 winMin = ImGui::GetWindowPos();
        const ImVec2 winMax(winMin.x + WIDTH, winMin.y + HEIGHT);
        constexpr float outerPad = 24.0f;
        constexpr float topBarHeight = 64.0f;
        constexpr float sectionGap = 16.0f;
        constexpr float sidebarWidth = 200.0f;
        constexpr float contentTop = outerPad + topBarHeight;
        constexpr float contentHeight = HEIGHT - contentTop - outerPad;
        constexpr float contentX = outerPad + sidebarWidth + sectionGap;
        constexpr float contentWidth = WIDTH - contentX - outerPad;
        constexpr float columnWidth = (contentWidth - sectionGap) * 0.5f;
        constexpr float halfCardHeight = (contentHeight - sectionGap) * 0.5f;
        draw->AddRectFilled(winMin, winMax, Color(Glass), 16.0f);
        draw->AddRect(winMin, winMax, Color(Stroke), 16.0f, 0, 1.0f);

        draw->AddRectFilled(winMin, ImVec2(winMax.x, winMin.y + topBarHeight), Color(ImVec4(0.050f, 0.047f, 0.080f, 0.34f)), 16.0f, ImDrawFlags_RoundCornersTop);
        ImGui::SetCursorPos(ImVec2(outerPad, 18.0f));
        draw->AddRectFilled(ImVec2(winMin.x + contentX, winMin.y + 18.0f), ImVec2(winMin.x + contentX + 32.0f, winMin.y + 46.0f), Color(Accent), 8.0f);
        DrawFilledText(draw, ImVec2(winMin.x + contentX + 10.0f, winMin.y + 24.0f), Color(Text), "S");
        ImGui::SetCursorPos(ImVec2(contentX + 48.0f, 17.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, Text);
        ImGui::TextUnformatted("SHOWCASE UI");
        ImGui::PopStyleColor();
        ImGui::SetCursorPos(ImVec2(contentX + 48.0f, 36.0f));
        TextMutedLine("Animated controls and layout starter");

        ImGui::SetCursorPos(ImVec2(WIDTH - outerPad - 68.0f, 18.0f));
        if (IconButtonSmall("minimize", "-"))
            ShowWindow(window, SW_MINIMIZE);
        ImGui::SameLine(0.0f, 8.0f);
        if (IconButtonSmall("close", "X"))
            PostMessage(window, WM_CLOSE, 0, 0);

        ImGui::SetCursorPos(ImVec2(outerPad, 18.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, TextMuted);
        ImGui::TextUnformatted("Workspace");
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2(outerPad, contentTop));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, SidebarPanel);
        ImGui::PushStyleColor(ImGuiCol_Border, Stroke);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 18.0f));
        ImGui::BeginChild("sidebar", ImVec2(sidebarWidth, contentHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PushStyleColor(ImGuiCol_Text, TextMuted);
        ImGui::TextUnformatted("Navigation");
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        if (NavItem("Overview", "#", selectedTab == 0)) selectedTab = 0;
        if (NavItem("Display", "@", selectedTab == 1)) selectedTab = 1;
        if (NavItem("Controls", "%", selectedTab == 2)) selectedTab = 2;
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, TextMuted);
        ImGui::TextUnformatted("Configuration");
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        if (NavItem("Profiles", "&", selectedTab == 3)) selectedTab = 3;
        if (NavItem("Theme", "+", selectedTab == 4)) selectedTab = 4;
        if (NavItem("About", "?", selectedTab == 5)) selectedTab = 5;
        ImGui::EndChild();
        DrawIslandBottomFade(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        ImGui::SetCursorPos(ImVec2(contentX, contentTop));
        ImGui::BeginGroup();

        ImGui::BeginGroup();
        ImGui::BeginChild("left_column", ImVec2(columnWidth, contentHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        BeginCard("Primary module", "Use this panel as a starter block", halfCardHeight, &moduleA);
        AnimatedCheckbox("Enable polished mode", &checkA);
        AnimatedCheckbox("Show live indicators", &checkB);
        SmoothSliderFloat("Range", &range, 0.0f, 300.0f);
        ImGui::TextUnformatted("Mode");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::Combo("##mode_combo", &mode, modes.data(), static_cast<int>(modes.size()));
        AnimatedCheckbox("Remember selection", &checkC);
        EndCard();

        ImGui::Dummy(ImVec2(0.0f, sectionGap));
        BeginCard("Theme controls", "Pick the live color style", halfCardHeight, &moduleB);
        ThemeColorSelector("Accent selector");
        SmoothSliderFloat("Background opacity", &opacity, 20.0f, 100.0f, "%");
        ColorRow("Accent color", Accent);
        ColorRow("Support color", Support);
        ColorRow("Success color", Success);
        EndCard();
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::SameLine(0.0f, sectionGap);

        ImGui::BeginGroup();
        ImGui::BeginChild("right_column", ImVec2(columnWidth, contentHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        BeginCard("Preview module", "A larger area for dense option groups", 365.0f, &moduleC);
        ImGui::TextUnformatted("Palette");
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::Combo("##palette_combo", &palette, palettes.data(), static_cast<int>(palettes.size())))
            SetThemeAccent(paletteColors[palette]);
        AnimatedCheckbox("Animate transitions", &checkD);
        AnimatedCheckbox("Use compact spacing", &checkE);
        AnimatedCheckbox("Draw subtle borders", &checkF);
        AnimatedCheckbox("Show advanced rows", &checkG);
        SmoothSliderFloat("Intensity", &intensity, 0.0f, 100.0f, "%");
        SmoothSliderFloat("Panel spacing", &spacing, 20.0f, 80.0f);
        EndCard();

        ImGui::Dummy(ImVec2(0.0f, sectionGap));
        BeginCard("Status panel", "A simple feature-style content block", contentHeight - 365.0f - sectionGap, &moduleD);
        const ImVec2 statusPos = ImGui::GetCursorScreenPos();
        const float statusWidth = ImGui::GetContentRegionAvail().x;
        draw->AddRectFilled(statusPos, ImVec2(statusPos.x + statusWidth, statusPos.y + 78.0f), Color(PanelSoft), 7.0f);
        draw->AddCircleFilled(ImVec2(statusPos.x + 20.0f, statusPos.y + 22.0f), 6.0f, Color(Success));
        DrawFilledText(draw, ImVec2(statusPos.x + 38.0f, statusPos.y + 13.0f), Color(Text), "Ready for your own layout");
        DrawFilledText(draw, ImVec2(statusPos.x + 38.0f, statusPos.y + 38.0f), Color(TextMuted), "Replace labels, split cards, and wire your own state.");
        draw->AddRectFilled(ImVec2(statusPos.x + 18.0f, statusPos.y + 60.0f), ImVec2(statusPos.x + statusWidth - 18.0f, statusPos.y + 64.0f), Color(ImVec4(0.120f, 0.108f, 0.140f, 1.0f)), 4.0f);
        draw->AddRectFilled(ImVec2(statusPos.x + 18.0f, statusPos.y + 60.0f), ImVec2(statusPos.x + 18.0f + (statusWidth - 36.0f) * 0.72f, statusPos.y + 64.0f), Color(Accent), 4.0f);
        ImGui::Dummy(ImVec2(statusWidth, 83.0f));
        EndCard();
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::EndGroup();

        DrawIslandBottomFade(winMin, winMax);
        ImGui::End();

        ImGui::PopStyleColor(2);
    }

    EndRender();
}


