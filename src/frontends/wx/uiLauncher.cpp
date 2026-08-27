#include <SDL2/SDL.h>
#include "uiLauncher.hpp"

#ifdef _WIN32
#pragma message "note: WIN32 macro is enabled (wxWidgets frontend)"
#endif

using namespace miniCDi;

wxIMPLEMENT_APP(uiLauncher);

uiLauncher::uiLauncher()
{
}

uiLauncher::~uiLauncher()
{
	SDL_Quit();
}

int uiLauncher::OnRun()
{
	// initialize SDL
	if (SDL_Init(SDL_INIT_AUDIO) != 0) {
		std::cerr << "unable to init SDL: " << SDL_GetError() << '\n';
		return -1;
	}

	return wxApp::OnRun();
}

bool uiLauncher::OnInit()
{
	// create the mainFrame
	frame = new mainFrame(NULL);
	frame->Show();

	// Our mainFrame is the Top Window
	SetTopWindow(frame);

	// initialization should always succeed
	return wxApp::OnInit();
}