#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <filesystem>
#include "cdi/common.hpp"

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
	} FPD;

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

	void update_fpd(void* buffer, int width)
	{
		// Clear screen
		SDL_SetRenderDrawColor(this->FPD.renderer, 128, 128, 128, 255);
		SDL_RenderClear(this->FPD.renderer);

		if (buffer) {
			// Draw screen
			SDL_UpdateTexture(this->FPD.texture, NULL, buffer, width*sizeof(uint8_t));
			SDL_RenderCopy(this->FPD.renderer, this->FPD.texture, NULL, NULL);
		}

		SDL_RenderPresent(this->FPD.renderer);
	}

	void add_fpd(int width, int height)
	{
		this->FPD.window = SDL_CreateWindow("FPD", 32, 32, width*4, height*4, SDL_WINDOW_SKIP_TASKBAR | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_HIDDEN);
		this->FPD.renderer = SDL_CreateRenderer(this->FPD.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
		this->FPD.texture = SDL_CreateTexture(this->FPD.renderer, SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_STREAMING, width, height);
	}

	SDL()
	{
		this->Video.window = SDL_CreateWindow("miniCDi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 768, 560, SDL_WINDOW_RESIZABLE);
		this->Video.renderer = SDL_CreateRenderer(this->Video.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
		this->Video.texture = SDL_CreateTexture(this->Video.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 768, 280);
		this->FPD.window = nullptr;
		// SDL_SetRenderDrawBlendMode(this->Video.renderer, SDL_BLENDMODE_BLEND);
	}

	~SDL()
	{
		if (this->FPD.texture) SDL_DestroyTexture(this->FPD.texture);
		if (this->FPD.renderer) SDL_DestroyRenderer(this->FPD.renderer);
		if (this->FPD.window) SDL_DestroyWindow(this->FPD.window);

		if (this->Video.texture) SDL_DestroyTexture(this->Video.texture);
		if (this->Video.renderer) SDL_DestroyRenderer(this->Video.renderer);
		if (this->Video.window) SDL_DestroyWindow(this->Video.window);
	}
};

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("usage: miniCDi <boot.rom> [disc.bin]\n");
        return 1;
    }

	if (access(argv[1], F_OK) != 0)
    {
        printf("unable to access bootrom, exiting\n");
        return 1;
    }
	
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("unable to init SDL, exiting\n");
        return 1;
    }
	atexit(SDL_Quit);

	MiniCDI::Config::TestPlug = false;
	MiniCDI::Config::PAL = true;
	MiniCDI::Config::AnalogColors = false;
	MiniCDI::Config::FrameSkip = 0;
	MiniCDI::Config::PointerAdvance = 2;
	#ifdef MINICDI_FORCE_LOGFILE
	// MiniCDI::Config::LogFile = fopen("sdmc:/3ds/miniCDi/log.txt", "wt");
	MiniCDI::Config::LogFile = NULL;
	#else
	MiniCDI::Config::LogFile = NULL;
	#endif
	MiniCDI::Config::ShowFPS = false;
	MiniCDI::Config::ShowLCD = true;
	MiniCDI::Config::NvramFile = "";

	const std::filesystem::path biosPath = argv[1];
	enum CDi::BoardType board = biosPath.stem().compare("cdi490a") == 0 ? CDi::MonoIV
							  : biosPath.stem().compare("cdi220c") == 0 ? CDi::MonoII
							  : CDi::MonoI;
	MonoI cdi;
	cdi.init(biosPath.string(), board);
	if (argc >= 3 && access(argv[3], F_OK) != 0)
    {
		switch (board)
		{
			default: cdi.swap_disc(argv[3]); break;
			case CDi::MonoII: printf("[miniCDi] Warning: DRVDSP not supported, cannot run discs.\n"); break;
			case CDi::MonoIII:
			case CDi::MonoIV: printf("[miniCDi] Warning: CIAP not supported, cannot run discs.\n"); break;
		}
    }

	// SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	SDL screen;

	bool has_quit = false;
	while (!has_quit)
	{
        SDL_Event e;
        if(SDL_PollEvent(&e)) 
        {
            switch (e.type)
            {
				case SDL_KEYDOWN:
				case SDL_KEYUP:
					cdi.pd.set_button(PointingDevice::Button1, e.key.keysym.sym == SDLK_RETURN && e.type == SDL_KEYDOWN);
					cdi.pd.set_button(PointingDevice::Button2, e.key.keysym.sym == SDLK_ESCAPE && e.type == SDL_KEYDOWN);
					cdi.pd.set_button(PointingDevice::Down, e.key.keysym.sym == SDLK_DOWN && e.type == SDL_KEYDOWN);
					cdi.pd.set_button(PointingDevice::Up, e.key.keysym.sym == SDLK_UP && e.type == SDL_KEYDOWN);
					cdi.pd.set_button(PointingDevice::Left, e.key.keysym.sym == SDLK_LEFT && e.type == SDL_KEYDOWN);
					cdi.pd.set_button(PointingDevice::Right, e.key.keysym.sym == SDLK_RIGHT && e.type == SDL_KEYDOWN);

					if (e.key.keysym.sym == SDLK_r && e.type == SDL_KEYDOWN) cdi.reset();
					if (e.key.keysym.sym == SDLK_e && e.type == SDL_KEYDOWN) cdi.play_disc();
					if (e.key.keysym.sym == SDLK_f && e.type == SDL_KEYDOWN) MiniCDI::Config::ShowLCD = !MiniCDI::Config::ShowLCD;
					break;

				case SDL_WINDOWEVENT:
					if (e.window.event == SDL_WINDOWEVENT_CLOSE)
					{
						if (e.window.windowID == SDL_GetWindowID(screen.FPD.window))
							MiniCDI::Config::ShowLCD = false;
						else if (e.window.windowID == SDL_GetWindowID(screen.Video.window))
							has_quit = true;
					}
					break;

				case SDL_QUIT:
					has_quit = true;
					break;

				case SDL_DROPFILE:
					cdi.swap_disc(e.drop.file);
					if (e.drop.file) free(e.drop.file);
					switch (board)
					{
						default: break;
						case CDi::MonoII: printf("[miniCDi] Warning: DRVDSP not properly supported, cannot run discs.\n"); break;
						case CDi::MonoIII:
						case CDi::MonoIV: printf("[miniCDi] Warning: CIAP not properly supported, cannot run discs.\n"); break;
					}
					break;
            }
        }

		if (MiniCDI::Config::FrameSkip != 0) {
			cdi.run(MiniCDI::Config::FrameSkip+1);
			// fps.update(MiniCDI::Config::FrameSkip+1);
		} else {
			cdi.run(1);
			// fps.update(1);
		}

		screen.update_video(cdi.get_display(), cdi.get_display_width());

		// FPD handling
		if (MiniCDI::Config::ShowLCD && cdi.get_fpd())
		{
			if (screen.FPD.window == nullptr)
				screen.add_fpd(cdi.get_fpd_width(), cdi.get_fpd_height());
			else
				SDL_ShowWindow(screen.FPD.window);
		}
		if (!MiniCDI::Config::ShowLCD && screen.FPD.window) SDL_HideWindow(screen.FPD.window);
		if (MiniCDI::Config::ShowLCD && cdi.get_fpd()) screen.update_fpd(cdi.get_fpd(), cdi.get_fpd_width());
	}
	
	return 0;
}