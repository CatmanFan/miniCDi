#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdi/common.hpp"

// Console-specific libraries
#include <SDL2/SDL.h>
#include <coreinit/screen.h>
#include <coreinit/thread.h>
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
	void update(void* display_output, size_t width, void* lcd_output)
	{
		if (display_output) {
			// Clear screen
			SDL_SetRenderDrawColor(this->renderer, 64, 64, 64, 255);
			SDL_RenderClear(this->renderer);

			// Draw screen
			SDL_UpdateTexture(this->texture, NULL, display_output, width*sizeof(uint32_t));
			SDL_RenderCopy(this->renderer, this->texture, NULL, NULL);

			// Draw LCD if available
			if (lcd_output)
			{
				SDL_UpdateTexture(this->lcd, NULL, lcd_output, (20*7)*sizeof(uint32_t));
				SDL_Rect dest = {0, 0, (20*7), 22};
				SDL_RenderCopy(this->renderer, this->lcd, NULL, &dest);
			}

			SDL_RenderPresent(this->renderer);
		}
	}

	SDL()
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == 0) {
			atexit(SDL_Quit);
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
			SDL_ShowCursor(SDL_DISABLE);

			this->window = SDL_CreateWindow("miniCDi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
			this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 384, 280);
			this->lcd = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, (20*7), 22);
			return;
		}

		failed:
		OSScreenPutFontEx(SCREEN_TV, 0, 3, "SDL init failed");
		OSScreenPutFontEx(SCREEN_DRC, 0, 3, "SDL init failed");
	}

	~SDL()
	{
		if (this->texture) SDL_DestroyTexture(this->texture);
		if (this->lcd) SDL_DestroyTexture(this->lcd);
		if (this->renderer) SDL_DestroyRenderer(this->renderer);
		if (this->window) SDL_DestroyWindow(this->window);

		SDL_Quit();
	}
};

static size_t tvBufferSize;
static size_t drcBufferSize;
static void* tvBuffer;
static void* drcBuffer;
static void PrintToScreen(int r, int c, const char* txt, bool flip = false)
{
	OSScreenPutFontEx(SCREEN_TV, r, c, txt);
	OSScreenPutFontEx(SCREEN_DRC, r, c, txt);

	if (flip) {
		DCFlushRange(tvBuffer, tvBufferSize);
		DCFlushRange(drcBuffer, drcBufferSize);
		OSScreenFlipBuffersEx(SCREEN_TV);
		OSScreenFlipBuffersEx(SCREEN_DRC);
	}
}

static std::string devicePrefix;

static void RUN_CDI(const std::string &biosName, const std::string &discName)
{
	if (access((devicePrefix + "wiiu/apps/miniCDi/rom/" + biosName).c_str(), F_OK) != 0) {
		WHBLogPrintf("[miniCDi] error: BIOS not found in required path");
		PrintToScreen(0,1, "BIOS not found, exiting", true);
		OSSleepTicks(OSSecondsToTicks(5));
		return;
	}
	PrintToScreen(0,1, "Loading", true);
	OSScreenShutdown();

	MiniCDI::Config::FrameSkip = 0;
	MiniCDI::Config::PAL = true;
	MiniCDI::Config::ShowLCD = true;

	MonoI cdi;
	// MonoIII cdi;
	// MonoIV cdi;
	// Robocon cdi;

	cdi.init((devicePrefix + "wiiu/apps/miniCDi/rom/" + biosName).c_str());
	cdi.disc.open((devicePrefix + "wiiu/apps/miniCDi/discs/" + discName).c_str());

	VPADStatus status;
	VPADReadError error;
	bool vpad_fatal = false;
	SDL screen;

	while (WHBProcIsRunning()) {
		VPADRead(VPAD_CHAN_0, &status, 1, &error);
		switch (error) {
			case VPAD_READ_SUCCESS:
			case VPAD_READ_NO_SAMPLES:
				break;

			case VPAD_READ_INVALID_CONTROLLER:
				WHBLogPrint("Gamepad disconnected!");
				vpad_fatal = true;
				break;

			default:
				WHBLogPrintf("Unknown VPAD error! %08X", error);
				vpad_fatal = true;
				break;
		}
		if (vpad_fatal) break;

		VPADRead(VPAD_CHAN_0, &status, 1, &error);
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
		screen.update(cdi.get_display(), cdi.get_display_width(), MiniCDI::Config::ShowLCD ? cdi.get_lcd() : nullptr);
	}

	OSScreenInit();
}

int main(int argc, char **argv) {
	// Init logging
	WHBProcInit();
	WHBLogCafeInit();
	WHBLogUdpInit();
	WHBLogPrintf("[miniCDi] logging initialized");

	// Init on-screen console
	OSScreenInit();
	tvBufferSize = OSScreenGetBufferSizeEx(SCREEN_TV);
	drcBufferSize = OSScreenGetBufferSizeEx(SCREEN_DRC);
	tvBuffer = memalign(0x100, tvBufferSize);
	drcBuffer = memalign(0x100, drcBufferSize);
	OSScreenSetBufferEx(SCREEN_TV, tvBuffer);
	OSScreenSetBufferEx(SCREEN_DRC, drcBuffer);
	OSScreenEnableEx(SCREEN_TV, true);
	OSScreenEnableEx(SCREEN_DRC, true);
	OSScreenClearBufferEx(SCREEN_TV, 0);
	OSScreenClearBufferEx(SCREEN_DRC, 0);
	WHBLogPrintf("[miniCDi] console screen initialized");

	PrintToScreen(0,0, "miniCDi for Wii U");

	bool mounted = WHBMountSdCard();
	if (mounted) { WHBLogPrintf("[miniCDi] mounted SD card"); }
	devicePrefix = mounted ? "fs:/vol/external01/" : "/vol/external01/";
	RUN_CDI("cdi220b.rom", "FROG.BIN");
	if (mounted) { WHBUnmountSdCard(); }

	if (tvBuffer) free(tvBuffer);
	if (drcBuffer) free(drcBuffer);

	WHBProcShutdown();

	WHBLogPrintf("[miniCDi] The End");
	WHBLogCafeDeinit();
	WHBLogUdpDeinit();
	return 1;
}