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

			this->window = SDL_CreateWindow("miniCDi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
			if (this->window == NULL) goto failed;
			this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
			if (this->renderer == NULL) goto failed;
			this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 768, 280);
			if (this->texture == NULL) goto failed;
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
#define BIOS_PATH "apps/miniCDi/rom/cdi220b.rom"

static void RUN_CDI()
{
	MiniCDIConfig config = {
		true	/** PAL mode **/,
		true	/** show LCD **/
	};

	MonoI cdi;
	cdi.Init((devicePrefix + BIOS_PATH).c_str(), &config);
	SDL screen;

	while (WHBProcIsRunning()) {
		do { cdi.step(); } while (!cdi.frame_ready());
		screen.update(cdi.get_display(), cdi.get_display_width(), cdi.get_lcd());
	}
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
	if (access((devicePrefix + BIOS_PATH).c_str(), F_OK) == 0) {
		PrintToScreen(0,1, "Loading", true);
		RUN_CDI();

		if (mounted) { WHBUnmountSdCard(); }
		goto exit;
	}

	WHBLogPrintf("[miniCDi] error: BIOS not found in required path");
	PrintToScreen(0,1, "BIOS not found, exiting", true);
	OSSleepTicks(OSSecondsToTicks(5));

	exit:
	if (tvBuffer) free(tvBuffer);
	if (drcBuffer) free(drcBuffer);

	OSScreenShutdown();
	WHBProcShutdown();

	WHBLogPrintf("[miniCDi] The End");
	WHBLogCafeDeinit();
	WHBLogUdpDeinit();
	return 1;
}