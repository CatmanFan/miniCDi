#include <iostream>
#include <wx/wx.h>
#include <SDL2/SDL.h>
#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
#else // _WIN32
int main(int argc, char** argv)
#endif
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
		#ifdef _WIN32
		MessageBox(
			NULL,
			(LPCWSTR)L"Could not initialize SDL.",
			(LPCWSTR)L"Error",
			MB_ICONWARNING | MB_OK
		);
		#else // _WIN32
        printf("Could not initialize SDL.\n");
		#endif
        return 1;
    }

	#ifdef _WIN32
    return wxEntry(hInst, hInstPrev, cmdline, cmdshow);
	#else // _WIN32
    return wxEntry(argc, argv);
	#endif
}