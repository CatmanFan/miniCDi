#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <filesystem>
#include "cdi/common.hpp"

class SDL
{
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr;
	SDL_Texture* lcd = nullptr;

public:
	void update(void* display_output, int width, void* lcd_output, bool cd_read_status)
	{
		// Clear screen
		SDL_SetRenderDrawColor(this->renderer, 128, 128, 128, 255);
		SDL_RenderClear(this->renderer);

		if (display_output) {
			// Draw screen
			SDL_UpdateTexture(this->texture, NULL, display_output, width*sizeof(uint32_t));
			SDL_RenderCopy(this->renderer, this->texture, NULL, NULL);
		}

		SDL_RenderPresent(this->renderer);
	}

	SDL()
	{
		if (SDL_Init(SDL_INIT_VIDEO) == 0) {
			// SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

			this->window = SDL_CreateWindow("miniCDi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 768, 560, SDL_WINDOW_RESIZABLE);
			this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
			this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 768, 280);
			this->lcd = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, (20*7), 22);

			// SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
		}
	}

	~SDL()
	{
		if (this->lcd) SDL_DestroyTexture(this->lcd);
		if (this->texture) SDL_DestroyTexture(this->texture);
		if (this->renderer) SDL_DestroyRenderer(this->renderer);
		if (this->window) SDL_DestroyWindow(this->window);

		SDL_Quit();
	}
};

static void RUN_CDI(const std::filesystem::path &biosPath, const std::filesystem::path &discPath)
{
	MiniCDI::Config::TestPlug = false;
	MiniCDI::Config::PAL = true;
	MiniCDI::Config::AnalogColors = false;
	MiniCDI::Config::FrameSkip = 0;
	MiniCDI::Config::PointerAdvance = 1;
	#ifdef MINICDI_FORCE_LOGFILE
	// MiniCDI::Config::LogFile = fopen("sdmc:/3ds/miniCDi/log.txt", "wt");
	MiniCDI::Config::LogFile = NULL;
	#else
	MiniCDI::Config::LogFile = NULL;
	#endif
	MiniCDI::Config::ShowFPS = false;
	MiniCDI::Config::ShowLCD = false;
	MiniCDI::Config::HasDisc = false;
	MiniCDI::Config::NvramFile = "";

	MonoI cdi;
	cdi.init(biosPath.string(), biosPath.stem().compare("cdi490a") == 0 ? CDi::MonoIV : CDi::MonoI);
	if (biosPath.stem().compare("cdi490a") == 0) {
		printf("[miniCDi] Warning: Mono-IV driver does not support discs\n");
	} else {
		cdi.disc.open(discPath.string());
	}
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
					if (e.key.keysym.sym == SDLK_n && e.type == SDL_KEYDOWN) cdi.reset();
					break;
				case SDL_QUIT:
					has_quit = true;
					break;
            }
        }

		// Ensure that drawing is done at 30fps
		if (MiniCDI::Config::FrameSkip != 0) {
			cdi.run(MiniCDI::Config::FrameSkip+1);
			// fps.update(MiniCDI::Config::FrameSkip+1);
		} else {
			cdi.run(1);
			// fps.update(1);
		}

		screen.update(cdi.get_display(), cdi.get_display_width(), MiniCDI::Config::ShowLCD ? cdi.get_lcd() : nullptr, cdi.get_cd_read_status());
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
        printf("unable to access bootrom, exiting\n");
        return 1;
    }

	if (argc >= 3 && access(argv[2], F_OK) == 0)
		RUN_CDI(argv[1], argv[2]);
	else
		RUN_CDI(argv[1], "");
	
	return 0;
}