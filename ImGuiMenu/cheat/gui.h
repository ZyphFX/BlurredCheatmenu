#pragma once
#include <d3d9.h>

namespace gui
{
	// constant window size
	constexpr int WIDTH = 900;
	constexpr int HEIGHT = 500;

	inline bool exit = true;

	// winapi window vars
	inline HWND window = nullptr;
	inline WNDCLASSEXA windowClass = { };

	// points for window movement
	inline POINTS position = { };

		
	//direct x state vars
	inline PDIRECT3D9 d3d = nullptr;
	inline LPDIRECT3DDEVICE9 device = nullptr;
	inline D3DPRESENT_PARAMETERS presentParameters = { };

	// handle window creation and destruction
	void CreateHWindow(
		const char* windowname,
		const char* classname) noexcept;
	void DestroyHWindow() noexcept;

	// handle device creation and destruction
	bool CreateDevice() noexcept;
	void ResetDevice() noexcept;
	void DestroyDevice() noexcept;

	// handle ImGui creation And destruction
	void CreateImGui() noexcept;
	void DestroyImGui() noexcept;

	void BeginRender() noexcept;
	void EndRender() noexcept;
	void Render() noexcept;

}