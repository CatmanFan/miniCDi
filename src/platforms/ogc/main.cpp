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
#include <ogc/lwp_watchdog.h>

#ifdef HW_RVL
#include <sdcard/wiisd_io.h>
#include <wiiuse/wpad.h>
#endif

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

class FPS
{
	int8_t aggregate;
	int8_t incremented;
	clock_t lastTime;
	clock_t currentTime;

public:
	FPS() : aggregate(0)
		  , incremented(0)
		  , lastTime(ticks_to_millisecs(gettime()))
	{ }

	void update(int frames = 1)
	{
		incremented += frames;
		currentTime = ticks_to_millisecs(gettime());

		if(currentTime - lastTime > 1000)
		{
			lastTime = currentTime;
			aggregate = incremented;
			incremented = 0;
			#ifdef MINICDI_DEBUG
			// printf("[FPS] %d\n", aggregate);
			#endif
		}
	}
};

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
			SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
			SDL_RenderClear(this->renderer);

			// Draw screen
			SDL_UpdateTexture(this->texture, NULL, display_output, width*sizeof(uint32_t));
			#ifdef MINICDI_NATIVERES
			SDL_Rect dest = {width/3, 280/3, width, 280};
			SDL_RenderCopy(this->renderer, this->texture, NULL, &dest);
			#else
			SDL_RenderCopy(this->renderer, this->texture, NULL, NULL);
			#endif

			// Draw LCD if available
			if (lcd_output != NULL)
			{
				SDL_UpdateTexture(this->lcd, NULL, lcd_output, (20*7)*sizeof(uint32_t));
				#ifdef MINICDI_NATIVERES
				dest = {
				#else
				SDL_Rect dest = {
				#endif
					640-(20*7), 480-22, (20*7), 22
				};
				SDL_RenderCopy(this->renderer, this->lcd, NULL, &dest);
			}

			SDL_RenderPresent(this->renderer);
		}
	}

	SDL()
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == 0) {
			// atexit(SDL_Quit);
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
			SDL_ShowCursor(SDL_DISABLE);

			this->window = SDL_CreateWindow("miniCDi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
			this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 384, 280);
			this->lcd = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, (20*7), 22);
		}
	}

	~SDL()
	{
		if (this->lcd) SDL_DestroyTexture(this->lcd);
		if (this->texture) SDL_DestroyTexture(this->texture);
		if (this->renderer) SDL_DestroyRenderer(this->renderer);
		if (this->window) SDL_DestroyWindow(this->window);

		SDL_Quit();
		VIDEO_SetBlack(true);
	}
};

static std::string appPath;
static bool FAT_Init() {
	if (!fatInitDefault()) {
		return false;
	}

	#ifdef HW_RVL
	if (fatMountSimple("sd", &__io_wiisd))
		appPath = "sd:/apps/miniCDi/";
	else if (fatMountSimple("usb", &__io_usbstorage))
		appPath = "usb:/apps/miniCDi/";
	else {
		return false;
	}
	#else // HW_DOL
	appPath = "/miniCDi/";
	#endif

	DIR *pdir = opendir(appPath.c_str());
	if (!pdir) {
		printf("error: miniCDi path does not exist in %s\n", appPath.c_str());
		return false;
	}
	closedir(pdir);

	return true;
}

static void RUN_CDI(const std::string &biosName, const std::string &discName)
{
	if (access((appPath + "rom/" + biosName).c_str(), F_OK) != 0) {
		printf("BIOS not found at %s, exiting", (appPath + "rom/" + biosName).c_str());
		sleep(5);
		exit(0);
	}

	MiniCDI::Config::TestPlug = false;
	MiniCDI::Config::PAL = VIDEO_GetCurrentTvMode() == VI_PAL;
	MiniCDI::Config::ShowLCD = true;
	MiniCDI::Config::FrameSkip = 1;

	MonoI cdi;
	// MonoIII cdi;
	// MonoIV cdi;
	// Robocon cdi;

	cdi.init(appPath + "rom/" + biosName);
	cdi.disc.open(appPath + "discs/" + discName);
	if (MiniCDI::Config::FrameSkip == 0)
		cdi.run(true); // Skip a frame anyway, to prevent crashing.

	#ifndef MINICDI_DEBUG
	SDL screen;
	#endif

	while (SYS_MainLoop()) {
		#ifdef HW_RVL
			WPAD_ScanPads();
			WPADData* data = WPAD_Data(0);
			uint32_t down = WPAD_ButtonsDown(0);

			if (data->exp.type == WPAD_EXP_CLASSIC) {
				if (down & WPAD_CLASSIC_BUTTON_HOME) break;

				cdi.pd.set_button(PointingDevice::Button1, WPAD_ButtonsHeld(0) & WPAD_CLASSIC_BUTTON_A);
				cdi.pd.set_button(PointingDevice::Button2, WPAD_ButtonsHeld(0) & WPAD_CLASSIC_BUTTON_B);
				cdi.pd.set_button(PointingDevice::Left, WPAD_ButtonsHeld(0) & WPAD_CLASSIC_BUTTON_LEFT);
				cdi.pd.set_button(PointingDevice::Right, WPAD_ButtonsHeld(0) & WPAD_CLASSIC_BUTTON_RIGHT);
				cdi.pd.set_button(PointingDevice::Down, WPAD_ButtonsHeld(0) & WPAD_CLASSIC_BUTTON_DOWN);
				cdi.pd.set_button(PointingDevice::Up, WPAD_ButtonsHeld(0) & WPAD_CLASSIC_BUTTON_UP);
			}
			if (data->ir.valid && data->exp.type != WPAD_EXP_CLASSIC) {
				if (down & WPAD_BUTTON_HOME) break;

				cdi.pd.set_button(PointingDevice::Button1, WPAD_ButtonsHeld(0) & WPAD_BUTTON_A);
				cdi.pd.set_button(PointingDevice::Button2, WPAD_ButtonsHeld(0) & WPAD_BUTTON_B);
				cdi.pd.set_coord(data->ir.x / 640.0f, data->ir.y / 480.0f);
			} else {
				if (down & WPAD_BUTTON_HOME) break;

				cdi.pd.set_button(PointingDevice::Button1, WPAD_ButtonsHeld(0) & WPAD_BUTTON_1);
				cdi.pd.set_button(PointingDevice::Button2, WPAD_ButtonsHeld(0) & WPAD_BUTTON_2);
				cdi.pd.set_button(PointingDevice::Left, WPAD_ButtonsHeld(0) & WPAD_BUTTON_UP);
				cdi.pd.set_button(PointingDevice::Right, WPAD_ButtonsHeld(0) & WPAD_BUTTON_DOWN);
				cdi.pd.set_button(PointingDevice::Down, WPAD_ButtonsHeld(0) & WPAD_BUTTON_LEFT);
				cdi.pd.set_button(PointingDevice::Up, WPAD_ButtonsHeld(0) & WPAD_BUTTON_RIGHT);
			}
		#else // HW_DOL
			PAD_ScanPads();
			uint32_t down = PAD_ButtonsDown(0);

			if (down & PAD_BUTTON_Z) break;
			cdi.pd.set_button(PointingDevice::Button1, PAD_ButtonsHeld(0) & PAD_BUTTON_A);
			cdi.pd.set_button(PointingDevice::Button2, PAD_ButtonsHeld(0) & PAD_BUTTON_B);
			cdi.pd.set_button(PointingDevice::Left, PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT);
			cdi.pd.set_button(PointingDevice::Right, PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT);
			cdi.pd.set_button(PointingDevice::Down, PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN);
			cdi.pd.set_button(PointingDevice::Up, PAD_ButtonsHeld(0) & PAD_BUTTON_UP);
		#endif

		static FPS fps;

		// Ensure that drawing is done at 30fps or 25fps (native Wii 60fps mode). Slightly slower on 50fps mode (likely because emulated machine is configured to use 60Hz?).
		// TO-DO: Address crashing if doing do_frame(true) solo
		if (MiniCDI::Config::FrameSkip > 0) {
			for (size_t i = 0; i < MiniCDI::Config::FrameSkip; i++) { cdi.run(true); }
			cdi.run();
			fps.update(MiniCDI::Config::FrameSkip+1);
		} else {
			cdi.run();
			fps.update();
		}

		#ifdef MINICDI_DEBUG
		VIDEO_WaitVSync();
		#else
		screen.update(cdi.get_display(), cdi.get_display_width(), MiniCDI::Config::ShowLCD ? cdi.get_lcd() : nullptr);
		#endif
	}
}

static std::string selectedDisc;
static int boardType;
#include <filesystem>

static bool MINICDI_CLI_MENU() {
	if (!std::filesystem::is_directory(appPath + "discs")) {
		selectedDisc = "";
		return true;
	}
	// Look for ROMs in directory
	std::vector<std::string> discs;
    for (const auto & disc : std::filesystem::directory_iterator(appPath + "discs")) {
        if (!disc.path().extension().compare(".bin") || !disc.path().extension().compare(".BIN"))
			discs.push_back(disc.path().filename());
	}
	if (discs.size() == 0) {
		selectedDisc = "";
		return true;
	}

	size_t selected = 0;
	bool render = true;
	while (SYS_MainLoop())
	{
		#ifdef HW_RVL
		WPAD_ScanPads();
		PAD_ScanPads();
		#else // HW_DOL
		PAD_ScanPads();
		#endif

		#ifdef HW_RVL
		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME || WPAD_ButtonsDown(0) & WPAD_CLASSIC_BUTTON_HOME || PAD_ButtonsDown(0) & PAD_TRIGGER_Z) {
		#else // HW_DOL
		if (PAD_ButtonsDown(0) & PAD_TRIGGER_Z) {
		#endif
			break;
		}

		#ifdef HW_RVL
		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_UP || WPAD_ButtonsDown(0) & WPAD_CLASSIC_BUTTON_UP || PAD_ButtonsDown(0) & PAD_BUTTON_UP) {
		#else // HW_DOL
		if (PAD_ButtonsDown(0) & PAD_BUTTON_UP) {
		#endif
			selected = selected <= 0 ? discs.size() - 1 : selected - 1;
			render = true;
		}

		#ifdef HW_RVL
		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_DOWN || WPAD_ButtonsDown(0) & WPAD_CLASSIC_BUTTON_DOWN || PAD_ButtonsDown(0) & PAD_BUTTON_DOWN) {
		#else // HW_DOL
		if (PAD_ButtonsDown(0) & PAD_BUTTON_DOWN) {
		#endif
			selected = (selected + 1) % discs.size();
			render = true;
		}

		#ifdef HW_RVL
		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_A || WPAD_ButtonsDown(0) & WPAD_CLASSIC_BUTTON_A || PAD_ButtonsDown(0) & PAD_BUTTON_A) {
		#else // HW_DOL
		if (PAD_ButtonsDown(0) & PAD_BUTTON_A) {
		#endif
			selectedDisc = discs[selected];
			boardType = 0;
			return true;
		}

		#ifdef HW_RVL
		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_B || WPAD_ButtonsDown(0) & WPAD_CLASSIC_BUTTON_B || PAD_ButtonsDown(0) & PAD_BUTTON_B) {
		#else // HW_DOL
		if (PAD_ButtonsDown(0) & PAD_BUTTON_B) {
		#endif
			selectedDisc = discs[selected];
			boardType = 1;
			return true;
		}

		if (render) {
			printf("\033[2J\033[H"); // Clear screen
			#ifdef HW_RVL
			printf("miniCDi - Philips CD-i emulator (EXPERIMENTAL)                  Wii version\n");
			#else // HW_DOL
			printf("miniCDi - Philips CD-i emulator (EXPERIMENTAL)             GameCube version\n");
			#endif
			printf("___________________________________________________________________________\n\n");

			#ifdef HW_RVL
			printf("Up/Down to navigate, A to select, HOME (Wiimote) or Z (GC) to exit\n\n");
			#else // HW_DOL
			printf("Up/Down to navigate, A to select, Z to exit\n\n");
			#endif

			for (size_t i = 0; i < discs.size(); i++) {
				if (i == selected)	{ printf("> "); }
				else					{ printf("  "); }

				printf("%s\n", discs[i].c_str());
			}
			render = false;
		}
	}

	return false;
}

int main(int argc, char **argv) {
	// Init controllers
	#ifdef HW_RVL
	WPAD_Init();
	#else // HW_DOL
	PAD_Init();
	#endif

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

	printf("miniCDi - Philips CD-i emulator\n");

	if (!FAT_Init()) {
		printf("failed to init FAT, exiting");
		sleep(5);
		exit(0);
	}

	if (MINICDI_CLI_MENU()) {
		printf("\033[2J\033[H"); // Clear screen
		printf("miniCDi - Philips CD-i emulator\nLoading\n");

		RUN_CDI("cdi220b.rom", selectedDisc);
		// RUN_CDI("cdi490a.rom", selectedDisc);
	}

	VIDEO_SetBlack(true);
	return 0;
}