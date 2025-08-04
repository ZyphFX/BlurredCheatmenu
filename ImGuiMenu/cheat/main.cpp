#include "gui.h"

#include <thread>

int __stdcall wWinMain(
	HINSTANCE instance,
	HINSTANCE previousInstance,
	PWSTR arguments,
	int commandShow)
{
	// Create a gui
	gui::CreateHWindow("Cheat Menu", "cheat Menu Class");
	gui::CreateDevice();
	gui::CreateImGui();


	while (gui::exit)
	{
		gui::Render();  // ? Handles the whole frame
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}


	// destroy the gui
	gui::DestroyImGui();
	gui::DestroyDevice();
	gui::DestroyHWindow();


	return EXIT_SUCCESS;
}