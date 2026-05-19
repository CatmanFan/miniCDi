#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdi/common.hpp"

#include <fstream>
#include <dirent.h>

// Console-specific libraries
#include <SDL2/SDL.h>
#include <fat.h>
#include <gccore.h>

#ifdef HW_RVL
#include <sdcard/wiisd_io.h>
#include <wiiuse/wpad.h>
#endif

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

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
				SDL_Rect dest = {640-(20*7), 480-22, (20*7), 22};
				SDL_RenderCopy(this->renderer, this->lcd, NULL, &dest);
			}

			SDL_RenderPresent(this->renderer);
		}
	}

	SDL()
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == 0) {
			atexit(SDL_Quit);
			SDL_ShowCursor(SDL_DISABLE);

			this->window = SDL_CreateWindow("miniCDi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
			this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 384, 280);

			this->lcd = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, (20*7), 22);
		}
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

static void RUN_CDI(const std::string &biosName)
{
	if (access((devicePrefix + "apps/miniCDi/rom/" + biosName + ".rom").c_str(), F_OK) != 0) {
		printf("BIOS not found at %s, exiting", (devicePrefix + "apps/miniCDi/rom/" + biosName + ".rom").c_str());
		sleep(5);
		exit(0);
	}

	MiniCDIConfig config = {
		true	/** PAL mode **/,
		false	/** show LCD **/
	};
	bool running = true;

	// config.log = fopen((devicePrefix + "apps/miniCDi/log.txt").c_str(), "wt");

	MonoIV cdi;
	cdi.Init((devicePrefix + "apps/miniCDi/rom/" + biosName + ".rom").c_str(), &config);
	#ifndef MINICDI_DEBUG
		SDL screen;
	#endif

	while (SYS_MainLoop()) {
	#ifdef HW_RVL
		WPAD_ScanPads();
		uint32_t down = WPAD_ButtonsDown(0);
		// uint32_t held = WPAD_ButtonsHeld(0);

		if (down & WPAD_BUTTON_HOME || down & WPAD_CLASSIC_BUTTON_HOME)
			break;

		if (down & WPAD_BUTTON_B) {
	#else
		PAD_ScanPads();
		uint32_t down = PAD_ButtonsDown(0);
		// uint32_t held = PAD_ButtonsHeld(0);

		if (down & PAD_BUTTON_Z)
			break;

		if (down & PAD_BUTTON_B) {
	#endif
			running = !running;
			printf("\x1b[%d;%dH", 2, 0);
			if (!running)
				printf("Paused\n");
			if (running)
				printf("      \n");
		}

		if (down & WPAD_BUTTON_1)
			config.lines++;
		if (down & WPAD_BUTTON_2)
			config.lines--;

		if (running)
		{
			do { cdi.step(); } while (!cdi.frame_ready());

			#ifdef MINICDI_DEBUG
			VIDEO_WaitVSync();
			#else
			screen.update(cdi.get_display(), cdi.get_display_width(), config.lcd ? cdi.get_lcd() : nullptr);
			#endif
		}
	}

	// if (config.log)
		// fclose(config.log);
}

int main(int argc, char **argv) {

	// Init controllers
	#ifdef HW_RVL
	WPAD_Init();
	#else
	PAD_Init();
	#endif

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

	printf("miniCDi - Philips CD-i 220/20 F2 experimental emulator\n");

	#ifdef HW_RVL
		if (!FAT_Init()) {
			printf("failed to init FAT, exiting");
			sleep(5);
			exit(0);
		}
	#else
		printf("GameCube support not fully implemented, exiting");
		sleep(5);
		exit(0);
	#endif

	RUN_CDI("cdi490a");

	VIDEO_SetBlack(true);
	return 0;
}