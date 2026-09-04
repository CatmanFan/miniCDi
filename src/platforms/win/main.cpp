#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <filesystem>
#include "cdi/common.hpp"
#include "../common/mINI.hpp"

class SDL
{
public:
	struct
	{
		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;
		SDL_Texture* texture = nullptr;
	} Video;

	struct
	{
		SDL_Window* window = nullptr;
		SDL_Renderer* renderer = nullptr;
		SDL_Texture* texture = nullptr;
	} FTD;

	void update_video(void* buffer, int width)
	{
		// Clear screen
		SDL_SetRenderDrawColor(this->Video.renderer, 128, 128, 128, 255);
		SDL_RenderClear(this->Video.renderer);

		if (buffer) {
			// Draw screen
			SDL_UpdateTexture(this->Video.texture, NULL, buffer, width*sizeof(uint32_t));
			SDL_RenderCopy(this->Video.renderer, this->Video.texture, NULL, NULL);
		}

		SDL_RenderPresent(this->Video.renderer);
	}

	void update_ftd(void* buffer, int width)
	{
		// Clear screen
		SDL_SetRenderDrawColor(this->FTD.renderer, 128, 128, 128, 255);
		SDL_RenderClear(this->FTD.renderer);

		if (buffer) {
			// Draw screen
			SDL_UpdateTexture(this->FTD.texture, NULL, buffer, width*sizeof(uint8_t));
			SDL_RenderCopy(this->FTD.renderer, this->FTD.texture, NULL, NULL);
		}

		SDL_RenderPresent(this->FTD.renderer);
	}

	void add_ftd(int width, int height)
	{
		this->FTD.window = SDL_CreateWindow("FTD", 32, 32, width*3, height*3, SDL_WINDOW_SKIP_TASKBAR | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_HIDDEN);
		this->FTD.renderer = SDL_CreateRenderer(this->FTD.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
		this->FTD.texture = SDL_CreateTexture(this->FTD.renderer, SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_STREAMING, width, height);
		SDL_SetTextureScaleMode(this->FTD.texture, SDL_ScaleModeNearest);
	}

	SDL()
	{
		this->Video.window = SDL_CreateWindow("miniCDi v0.1(beta)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 768, 560, SDL_WINDOW_RESIZABLE);
		this->Video.renderer = SDL_CreateRenderer(this->Video.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
		this->Video.texture = SDL_CreateTexture(this->Video.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 768, 280);
		this->FTD.window = nullptr;
		// SDL_SetRenderDrawBlendMode(this->Video.renderer, SDL_BLENDMODE_BLEND);
	}

	~SDL()
	{
		if (this->FTD.texture) SDL_DestroyTexture(this->FTD.texture);
		if (this->FTD.renderer) SDL_DestroyRenderer(this->FTD.renderer);
		if (this->FTD.window) SDL_DestroyWindow(this->FTD.window);

		if (this->Video.texture) SDL_DestroyTexture(this->Video.texture);
		if (this->Video.renderer) SDL_DestroyRenderer(this->Video.renderer);
		if (this->Video.window) SDL_DestroyWindow(this->Video.window);
	}
};

static void SwapDisc(PhilipsCDI *cdi, enum CDi::BoardType board, const char *path)
{
	if (access(path, F_OK) != 0) return;

	switch (board)
	{
		default: cdi->swap_disc(path); break;

		case CDi::MonoII:
			printf("[miniCDi] Warning: no full emulation of DRVDSP, discs will not play.\n");
			cdi->swap_disc(path);
			break;

		case CDi::MonoIII:
		case CDi::MonoIV:
			printf("[miniCDi] Warning: no full emulation of CIAP, discs will not play.\n");
			cdi->swap_disc(path);
			break;
	}
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("usage: miniCDi <boot.rom> [disc.bin]\n");
        return 1;
    }

	if (access(argv[1], F_OK) != 0)
    {
        printf("unable to access ROM file, exiting\n");
        return 1;
    }

    std::ifstream romcheck(argv[1], std::ifstream::ate | std::ifstream::binary);
	if (romcheck.tellg() != 512*1024)
    {
        printf("ROM file is not 512kB, exiting\n");
        return 1;
    }

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("unable to init SDL, exiting\n");
        return 1;
    }
	atexit(SDL_Quit);

	const std::filesystem::path biosPath = argv[1];
	const std::string iniPath = "miniCDi.ini";
	const std::string logPath = "log.txt";
	const std::string nvramPath = (biosPath.stem().string() + ".nvram");

	// load whatever settings we have
	static mINI::INIStructure ini;
	mINI::INIFile file(iniPath.c_str());
	bool recreateIni = true;
	if (access(iniPath.c_str(), F_OK) == 0) {
		file.read(ini);
		recreateIni = !(ini.has("CDI") && ini.has("MiniCDI") && ini["CDI"].size() == 4 && ini["MiniCDI"].size() == 4);
	}
	if (recreateIni) {
		ini["CDI"].set({
			{"AutosaveNVRAM", "0"},
			{"TestPlug", "0"},
			{"PAL", "1"},
			{"AnalogColors", "0"}
		});
		ini["MiniCDI"].set({
			{"HideCursor", "0"},
			{"FrameSkip", "0"},
			{"PointerAdvance", "0"},
			{"Logging", "0"},
		});
		file.generate(ini);
	}

	MiniCDI::Config.TestPlug = ini["CDI"]["TestPlug"].compare("1") == 0;
	MiniCDI::Config.PAL = ini["CDI"]["PAL"].compare("1") == 0;
	MiniCDI::Config.AnalogColors = ini["CDI"]["AnalogColors"].compare("1") == 0;
	MiniCDI::Config.FrameSkip = std::stoi(ini["MiniCDI"]["FrameSkip"]);
	MiniCDI::Config.PointerAdvance = std::stoi(ini["MiniCDI"]["PointerAdvance"]) + 1;
	#ifdef MINICDI_FORCE_LOGFILE
	MiniCDI::Config.LogFile = fopen(logPath.c_str(), "wt");
	#else
	MiniCDI::Config.LogFile = ini["CDI"]["Logging"].compare("1") == 0 ? fopen(logPath.c_str(), "wt") : NULL;
	#endif
	MiniCDI::Config.ShowFPS = false;
	MiniCDI::Config.ShowFTD = true;
	MiniCDI::Config.NvramFile = ini["CDI"]["AutosaveNVRAM"].compare("1") == 0 ? nvramPath : "";

	enum CDi::BoardType board = biosPath.stem().compare("cdi490a") == 0 ? CDi::MonoIV
							  : biosPath.stem().compare("cdi220c") == 0 ? CDi::MonoII
							  : CDi::MonoI;
	PhilipsCDI cdi;
	cdi.init(biosPath.string(), board);
	if (argc >= 3) SwapDisc(&cdi, board, argv[3]);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	if (ini["MiniCDI"]["HideCursor"].compare("1") == 0) { SDL_ShowCursor(SDL_DISABLE); }
	SDL screen;

	int frames_run = 0;
	bool has_quit = false;
	while (!has_quit)
	{
        SDL_Event e;
        if(SDL_PollEvent(&e)) 
        {
			int w, h;
			SDL_GetWindowSize(screen.Video.window, &w, &h);
			bool mouse_active = e.motion.x >= 0 && e.motion.x < w && e.motion.y >= 0 && e.motion.y < h;

            switch (e.type)
            {
				case SDL_KEYDOWN:
				case SDL_KEYUP:
					if (!mouse_active)
					{
						cdi.pd.set_button(PointingDevice::Button1, e.key.keysym.sym == SDLK_RETURN && e.key.state == SDL_PRESSED);
						cdi.pd.set_button(PointingDevice::Button2, e.key.keysym.sym == SDLK_SPACE && e.key.state == SDL_PRESSED);
						cdi.pd.set_button(PointingDevice::Down, e.key.keysym.sym == SDLK_DOWN && e.key.state == SDL_PRESSED);
						cdi.pd.set_button(PointingDevice::Up, e.key.keysym.sym == SDLK_UP && e.key.state == SDL_PRESSED);
						cdi.pd.set_button(PointingDevice::Left, e.key.keysym.sym == SDLK_LEFT && e.key.state == SDL_PRESSED);
						cdi.pd.set_button(PointingDevice::Right, e.key.keysym.sym == SDLK_RIGHT && e.key.state == SDL_PRESSED);
					}

					if (e.key.keysym.sym == SDLK_r && e.type == SDL_KEYDOWN) cdi.reset();
					if (e.key.keysym.sym == SDLK_e && e.type == SDL_KEYDOWN) cdi.play_disc();
					if (e.key.keysym.sym == SDLK_f && e.type == SDL_KEYDOWN) MiniCDI::Config.ShowFTD = !MiniCDI::Config.ShowFTD;
					if (e.key.keysym.sym == SDLK_t && e.type == SDL_KEYDOWN) MiniCDI::Config.NoFrameLimit = !MiniCDI::Config.NoFrameLimit;
					if (e.key.keysym.sym == SDLK_v && e.type == SDL_KEYDOWN) {
						SDL_SetWindowSize(screen.Video.window, w == 768 ? 384 : 768, h == 560 ? 280 : 560);
					}
					break;

				case SDL_MOUSEMOTION:
					if (mouse_active) {
						cdi.pd.set_coord(e.motion.x, e.motion.y, w, h);
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					if (mouse_active) {
						cdi.pd.set_button(PointingDevice::Button1, e.button.button == 1 && e.button.state == SDL_PRESSED);
						cdi.pd.set_button(PointingDevice::Button2, e.button.button == 3 && e.button.state == SDL_PRESSED);
					}
					break;

				case SDL_WINDOWEVENT:
					if (e.window.event == SDL_WINDOWEVENT_CLOSE)
					{
						if (e.window.windowID == SDL_GetWindowID(screen.FTD.window))
							MiniCDI::Config.ShowFTD = false;
						else if (e.window.windowID == SDL_GetWindowID(screen.Video.window))
							has_quit = true;
					}
					break;

				case SDL_QUIT:
					has_quit = true;
					break;

				case SDL_DROPFILE:
					SwapDisc(&cdi, board, e.drop.file);
					if (e.drop.file) free(e.drop.file);
					break;
            }
        }

		if (frames_run == 0) {
			cdi.run(false);
			frames_run += MiniCDI::Config.FrameSkip;
		} else {
			cdi.run(true);
			frames_run--;
			continue;
		}

		screen.update_video(cdi.get_display(), cdi.get_display_width());

		// FTD handling
		if (MiniCDI::Config.ShowFTD && cdi.get_ftd())
		{
			if (screen.FTD.window == nullptr)
				screen.add_ftd(cdi.get_ftd_width(), cdi.get_ftd_height());
			else
				SDL_ShowWindow(screen.FTD.window);
		}
		if (!MiniCDI::Config.ShowFTD && screen.FTD.window) SDL_HideWindow(screen.FTD.window);
		if (MiniCDI::Config.ShowFTD && cdi.get_ftd()) screen.update_ftd(cdi.get_ftd(), cdi.get_ftd_width());
	}
	
	return 0;
}