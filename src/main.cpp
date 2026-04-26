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

/*
#include "json.hpp"
#include "m68k.hpp"

static void m68k_test()
{
	nlohmann::json test;
	FILE *testf;
	if (access((devicePrefix + "apps/CDIEmu/tests/NOP.json").c_str(), F_OK) == 0) {
		printf("detected json\n");
		testf = fopen((devicePrefix + "apps/CDIEmu/tests/NOP.json").c_str(), "rt");
		if (testf) {
			printf("opened json\n");
			test = nlohmann::json::parse(testf, nullptr, false, true);
			fclose(testf);

			printf("initing cpu\n\n");
			M68000 m68k;
			std::vector<uint32_t> memLocations;
			m68k.memory = memory;
			for (int i = 0; i < 6; i++) {
				memLocations.push_back(test[0]["initial"]["ram"][i][0].get_ref<const nlohmann::json::number_unsigned_t&>());
				m68k.memory[memLocations[i]]
						  = test[0]["initial"]["ram"][i][1].get_ref<const nlohmann::json::number_unsigned_t&>();
			}

			m68k.r.d[0] = test[0]["initial"]["d0"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.d[1] = test[0]["initial"]["d1"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.d[2] = test[0]["initial"]["d2"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.d[3] = test[0]["initial"]["d3"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.d[4] = test[0]["initial"]["d4"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.d[5] = test[0]["initial"]["d5"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.d[6] = test[0]["initial"]["d6"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.d[7] = test[0]["initial"]["d7"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.a[0] = test[0]["initial"]["a0"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.a[1] = test[0]["initial"]["a1"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.a[2] = test[0]["initial"]["a2"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.a[3] = test[0]["initial"]["a3"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.a[4] = test[0]["initial"]["a4"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.a[5] = test[0]["initial"]["a5"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.a[6] = test[0]["initial"]["a6"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.usp = test[0]["initial"]["usp"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.ssp = test[0]["initial"]["ssp"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.sr = test[0]["initial"]["sr"].get_ref<const nlohmann::json::number_unsigned_t&>();
			m68k.r.pc = test[0]["initial"]["pc"].get_ref<const nlohmann::json::number_unsigned_t&>();

			for (int i = 0; i < 8; i++) {
				printf("d%d: %8x   a%d: %8x\n", i, m68k.r.d[i], i, i == 7 ? m68k.r.usp : m68k.r.a[i]);
			}
			printf("\n");

			for (size_t i = 0; i < memLocations.size(); i++) {
				printf("mem %d, addr %8x = %8x\n", i, memLocations[i], memory[memLocations[i]]);
			}
		}
	} else {
		printf("failed to access directory");
	}
}*/

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
			SDL_UpdateTexture(this->texture, NULL, display_output, width);
			SDL_RenderCopy(this->renderer, this->texture, NULL, NULL);
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

	printf("miniCDi - Philips CD-i 220/20 F2 experimental emulator\n");

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

	MiniCDIConfig config;
	config.pal = false;

	CDI220 cdi = CDI220((devicePrefix + "apps/CDIEmu/cdi220b.rom").c_str(), &config);
	SDL screen;

	while(SYS_MainLoop()) {
		WPAD_ScanPads();
		uint32_t down = WPAD_ButtonsDown(0);
		// uint32_t held = WPAD_ButtonsHeld(0);

		if (down & WPAD_BUTTON_HOME || down & WPAD_CLASSIC_BUTTON_HOME) { exit(0); }

		// if (down & WPAD_BUTTON_A) {
			cdi.step();
			// VIDEO_WaitVSync();
			screen.update(cdi.get_display(), 384);
		// }
	}

	VIDEO_SetBlack(true);
	return 0;
}