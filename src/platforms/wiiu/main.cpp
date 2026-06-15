#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdi/common.hpp"

// Console-specific libraries
#include <SDL2/SDL.h>
#include <whb/log_cafe.h>
#include <whb/log_udp.h>
#include <whb/log.h>
#include <whb/proc.h>
#include <whb/sdcard.h>
#include <vpad/input.h>

class SDL
{
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr;
	SDL_Texture* lcd = nullptr;

public:
	bool valid;

	void update(void* display_output, size_t width, void* lcd_output)
	{
		if (display_output) {
			// Clear screen
			SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
			SDL_RenderClear(this->renderer);

			// Draw screen
			SDL_UpdateTexture(this->texture, NULL, display_output, width*sizeof(uint32_t));
			SDL_Rect dest = {96, -90, 1728, 1260};
			SDL_RenderCopy(this->renderer, this->texture, NULL, &dest);

			// Draw LCD if available
			if (lcd_output)
			{
				SDL_UpdateTexture(this->lcd, NULL, lcd_output, (20*7)*sizeof(uint32_t));
				dest = {1920-(20*7), 0, (20*7), 22};
				SDL_RenderCopy(this->renderer, this->lcd, NULL, &dest);
			}

			SDL_RenderPresent(this->renderer);
		}
	}

	SDL()
	{
		valid = false;
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			return;
		}

		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
		this->window = SDL_CreateWindow("miniCDi", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, 0);
		if (!this->window) {
			return;
		}

		this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		if (!renderer) {
			SDL_DestroyWindow(this->window);
			this->window = nullptr;
			return;
		}

		this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 384, 280);
		this->lcd = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, (20*7), 22);
		valid = true;
	}

	~SDL()
	{
		if (this->texture) SDL_DestroyTexture(this->texture);
		if (this->lcd) SDL_DestroyTexture(this->lcd);
		if (this->renderer) SDL_DestroyRenderer(this->renderer);
		if (this->window) SDL_DestroyWindow(this->window);

		SDL_Quit();
		valid = false;
	}
};

static std::string devicePrefix;

static void RUN_CDI(const std::string &biosName, const std::string &discName)
{
	if (access((devicePrefix + "wiiu/apps/miniCDi/rom/" + biosName).c_str(), F_OK) != 0) {
		WHBLogPrintf("[miniCDi] error: BIOS not found in required path");
		// OSSleepTicks(OSSecondsToTicks(5));
		return;
	}

	MiniCDI::Config::FrameSkip = 0;
	MiniCDI::Config::PAL = false;
	MiniCDI::Config::ShowLCD = true;

	MonoI cdi;
	// MonoIII cdi;
	// MonoIV cdi;
	// Robocon cdi;

	cdi.init((devicePrefix + "wiiu/apps/miniCDi/rom/" + biosName).c_str());
	cdi.disc.open((devicePrefix + "wiiu/apps/miniCDi/discs/" + discName).c_str());

	SDL screen;

	if (!screen.valid) {
		WHBLogPrintf("[miniCDi] error: SDL init failed");
		return;
	}

	while (WHBProcIsRunning()) {
		VPADStatus status{};
		VPADRead(VPAD_CHAN_0, &status, 1, nullptr);
		cdi.pd.set_button(PointingDevice::Button1, status.hold & VPAD_BUTTON_A);
		cdi.pd.set_button(PointingDevice::Button2, status.hold & VPAD_BUTTON_B);
		cdi.pd.set_button(PointingDevice::Left, status.hold & (VPAD_BUTTON_LEFT | VPAD_STICK_L_EMULATION_LEFT | VPAD_STICK_R_EMULATION_LEFT));
		cdi.pd.set_button(PointingDevice::Right, status.hold & (VPAD_BUTTON_RIGHT | VPAD_STICK_L_EMULATION_RIGHT | VPAD_STICK_R_EMULATION_RIGHT));
		cdi.pd.set_button(PointingDevice::Down, status.hold & (VPAD_BUTTON_DOWN | VPAD_STICK_L_EMULATION_DOWN | VPAD_STICK_R_EMULATION_DOWN));
		cdi.pd.set_button(PointingDevice::Up, status.hold & (VPAD_BUTTON_UP | VPAD_STICK_L_EMULATION_UP | VPAD_STICK_R_EMULATION_UP));

		// static FPS fps;

		if (MiniCDI::Config::FrameSkip > 0) {
			for (size_t i = 0; i < MiniCDI::Config::FrameSkip; i++) { cdi.run(true); }
			cdi.run();
			// fps.update(MiniCDI::Config::FrameSkip+1);
		} else {
			cdi.run();
			// fps.update();
		}

		screen.update(cdi.get_display(), cdi.get_display_width(), MiniCDI::Config::ShowLCD ? cdi.get_lcd() : nullptr);
	}
}

int main(int argc, char **argv) {
	// Init CafeOS and logging
	WHBProcInit();
	WHBLogCafeInit();
	WHBLogUdpInit();
	WHBLogPrintf("[miniCDi] logging initialized");

    // call AXInit to stop already playing sounds
    // AXInit();

    VPADInit();

	bool mounted = WHBMountSdCard();
	if (mounted) { WHBLogPrintf("[miniCDi] mounted SD card"); }
	devicePrefix = mounted ? "fs:/vol/external01/" : "/vol/external01/";
	RUN_CDI("cdi220b.rom", "FROG.BIN");
	if (mounted) { WHBUnmountSdCard(); }

    // AXQuit();

	WHBProcShutdown();
	WHBLogPrintf("[miniCDi] The End");
	WHBLogCafeDeinit();
	WHBLogUdpDeinit();
	return 1;
}