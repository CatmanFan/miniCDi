#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#include <SDL2/SDL.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>
#include <string>
#include <fstream>
#include <dirent.h>

#include "CDI220.hpp"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

class SDL
{
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr;

public:
	void update(void* display_output, size_t width)
	{
		if (display_output) {
			// Clear screen
			SDL_SetRenderDrawColor(this->renderer, 15, 15, 15, 255);
			SDL_RenderClear(this->renderer);

			// Draw screen
			// SDL_Rect dest = {640 / 2 - (384 / 2), 480 / 2 - (280 / 2), 384, 280};
			SDL_UpdateTexture(this->texture, NULL, display_output, width);
			SDL_RenderCopy(this->renderer, this->texture, NULL, /*&dest*/ NULL);
			SDL_RenderPresent(this->renderer);
		}
	}

	SDL()
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == 0) {
			atexit(SDL_Quit);
			SDL_ShowCursor(SDL_DISABLE);

			this->window = SDL_CreateWindow("", 0, 0, 640, 480, SDL_WINDOW_SHOWN);
			this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED);
			this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);
		}
	}

	~SDL()
	{
		SDL_DestroyTexture(this->texture);
		SDL_DestroyRenderer(this->renderer);
		SDL_DestroyWindow(this->window);
		SDL_Quit();
	}
};

static std::string devicePrefix;
static bool FAT_Init() {
	if (!fatInitDefault()) {
		return false;
	}

	if (fatMountSimple("sd", &__io_wiisd))
		devicePrefix = "sd:/";
	else if (fatMountSimple("usb", &__io_usbstorage))
		devicePrefix = "usb:/";
	else {
		return false;
	}

	DIR *pdir = opendir(devicePrefix.c_str());
	if (!pdir) {
		return false;
	}
	closedir(pdir);

	return true;
}

static void RUN_CDI()
{
	MiniCDIConfig config;
	config.pal = true;

	CDI220 cdi = CDI220((devicePrefix + "apps/CDIEmu/cdi220b.rom").c_str(), &config);
	// SDL screen;

	while (SYS_MainLoop()) {
		WPAD_ScanPads();
		uint32_t down = WPAD_ButtonsDown(0);
		// uint32_t held = WPAD_ButtonsHeld(0);

		if (down & WPAD_BUTTON_HOME || down & WPAD_CLASSIC_BUTTON_HOME)
			break;

		if (cdi.step()) {
			VIDEO_WaitVSync();
			// screen.update(cdi.get_display(), cdi.get_display_width());
		}
	}
}

int main(int argc, char **argv) {

	// Init controllers
	WPAD_Init();

	{
		// Init console gfx
		VIDEO_Init();
		rmode = VIDEO_GetPreferredMode(NULL);
		xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
		console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
		VIDEO_Configure(rmode);
		VIDEO_SetNextFramebuffer(xfb);
		VIDEO_SetBlack(false);
		VIDEO_Flush();
		VIDEO_WaitVSync();
		if (rmode->viTVMode & VI_NON_INTERLACE) { VIDEO_WaitVSync(); }
	}

	if (!FAT_Init()) {
		printf("failed to init FAT, exiting");
		sleep(5);
		exit(0);
	}

	if (access((devicePrefix + "apps/CDIEmu/cdi220b.rom").c_str(), F_OK) != 0) {
		printf("BIOS not found, exiting");
		sleep(5);
		exit(0);
	}

	printf("miniCDi - Philips CD-i 220/20 F2 experimental emulator\n");

	RUN_CDI();

	VIDEO_SetBlack(true);
	return 0;
}