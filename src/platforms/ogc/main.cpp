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

#include "../common/mINI.hpp"

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
	void update(void* display_output, int width, void* lcd_output, bool cd_read_status)
	{
		if (display_output) {
			// Clear screen
			SDL_SetRenderDrawColor(this->renderer, 12, 12, 12, 255);
			SDL_RenderClear(this->renderer);

			// Draw screen
			SDL_UpdateTexture(this->texture, NULL, display_output, width*sizeof(uint32_t));
			#ifdef MINICDI_NATIVERES
			SDL_Rect dest = {320-((width/2)/2), 100, width/2, 280};
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

			#ifdef MINICDI_CDINDICATOR
			SDL_Rect cd_led = {0,0,24,24};
			if (cd_read_status)
				SDL_SetRenderDrawColor(this->renderer, 0,255,0,128);
			else
				SDL_SetRenderDrawColor(this->renderer, 0,0,0,128);
			SDL_RenderFillRect(this->renderer, &cd_led);
			#endif

			SDL_RenderPresent(this->renderer);
		}
	}

	SDL()
	{
		if (SDL_Init(SDL_INIT_VIDEO) == 0) {
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
			SDL_ShowCursor(SDL_DISABLE);

			this->window = SDL_CreateWindow("miniCDi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
			this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
			this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 768, 280);
			this->lcd = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, (20*7), 22);

			SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND);
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

static void FAT_Exit()
{
	#ifdef HW_RVL
	if (!appPath.compare("sd:/apps/miniCDi/")) {
		fatUnmount("sd:/");
		__io_wiisd.shutdown();
	}
	else if (!appPath.compare("usb:/apps/miniCDi/")) {
		fatUnmount("usb:/");
		__io_usbstorage.shutdown();
	}
	#else // HW_DOL
	#endif
}

static void RUN_CDI(const std::string &discName)
{
	#ifdef MINICDI_FORCE_MONOIV
	const std::string biosName = "cdi490a";
	if (access((appPath + "rom/" + biosName + ".rom").c_str(), F_OK) != 0) {
		printf("BIOS not found at %s", (appPath + "rom/" + biosName + ".rom").c_str());
	#else
	std::string biosName = "";
	if (access((appPath + "rom/cdi220b.rom").c_str(), F_OK) == 0) biosName = "cdi220b";
	else if (access((appPath + "rom/cdi200.rom").c_str(), F_OK) == 0) biosName = "cdi200";
	else {
		printf("BIOS not found at %s.\nPlease supply a system ROM (either CD-i 220/20 or 200/00)", (appPath + "rom/").c_str());
	#endif
		sleep(5);
		return;
	}

	// load whatever settings we have
	mINI::INIFile file((appPath + "config.ini").c_str());
	mINI::INIStructure ini;
	bool recreateIni = true;
	if (access((appPath + "config.ini").c_str(), F_OK) == 0) {
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
			{"FPS", "0"},
			{"FrameSkip", "1"},
			{"PointerAdvance", "0"},
			{"Logging", "0"}
		});
		file.generate(ini);
	}
	MiniCDI::Config::TestPlug = ini["CDI"]["TestPlug"].compare("1") == 0;
	MiniCDI::Config::PAL = ini["CDI"]["PAL"].compare("1") == 0;
	MiniCDI::Config::AnalogColors = ini["CDI"]["AnalogColors"].compare("1") == 0;
	MiniCDI::Config::FrameSkip = std::stoi(ini["MiniCDI"]["FrameSkip"]);
	MiniCDI::Config::PointerAdvance = std::stoi(ini["MiniCDI"]["PointerAdvance"]) + 1;
	#ifdef MINICDI_FORCE_LOGFILE
	MiniCDI::Config::LogFile = fopen((appPath + "log.txt").c_str(), "wt");
	#else
	MiniCDI::Config::LogFile = ini["MiniCDI"]["Logging"].compare("1") == 0 ? fopen((appPath + "log.txt").c_str(), "wt") : NULL;
	#endif
	MiniCDI::Config::ShowFPS = false;
	MiniCDI::Config::ShowLCD = false;
	MiniCDI::Config::HasDisc = false;
	MiniCDI::Config::NvramFile = ini["CDI"]["AutosaveNVRAM"].compare("1") == 0 ? appPath + "rom/" + biosName + ".nvram" : "";

	// Declare the CD-i machine
	MonoI cdi;
	if (!cdi.init(appPath + "rom/" + biosName + ".rom", biosName.compare("cdi490a") == 0 ? CDi::MonoIV : CDi::MonoI)) {
		printf("Failed to init virtual machine");
		sleep(5);
		return;
	}
	cdi.disc.open(appPath + "discs/" + discName);

	#ifndef MINICDI_DEBUG
	SDL screen;
	#endif

	#ifdef HW_RVL
	bool pointer = true;
	#endif

	while (SYS_MainLoop()) {
		#ifdef HW_RVL
			WPAD_ScanPads();
			WPADData* data = WPAD_Data(0);
			uint32_t down = WPAD_ButtonsDown(0);
			uint32_t held = WPAD_ButtonsHeld(0);

			if (data->exp.type & WPAD_EXP_CLASSIC) {
				if (down & WPAD_CLASSIC_BUTTON_HOME) break;
				if (down & WPAD_CLASSIC_BUTTON_MINUS) cdi.reset();

				cdi.pd.set_button(PointingDevice::Button1, held & WPAD_CLASSIC_BUTTON_A);
				cdi.pd.set_button(PointingDevice::Button2, held & WPAD_CLASSIC_BUTTON_B);
				cdi.pd.set_button(PointingDevice::Left, held & WPAD_CLASSIC_BUTTON_LEFT);
				cdi.pd.set_button(PointingDevice::Right, held & WPAD_CLASSIC_BUTTON_RIGHT);
				cdi.pd.set_button(PointingDevice::Down, held & WPAD_CLASSIC_BUTTON_DOWN);
				cdi.pd.set_button(PointingDevice::Up, held & WPAD_CLASSIC_BUTTON_UP);
			} else  {
				if (down & WPAD_BUTTON_HOME) break;
				if (down & WPAD_BUTTON_MINUS) cdi.reset();
				if (down & WPAD_BUTTON_PLUS) pointer = !pointer;

				if (pointer && data->ir.valid) {
					cdi.pd.set_button(PointingDevice::Button1, held & WPAD_BUTTON_A);
					cdi.pd.set_button(PointingDevice::Button2, held & WPAD_BUTTON_B);
					cdi.pd.set_coord(data->ir.x / 640.0f, data->ir.y / 480.0f);
				} else {
					cdi.pd.set_button(PointingDevice::Button1, held & WPAD_BUTTON_1);
					cdi.pd.set_button(PointingDevice::Button2, held & WPAD_BUTTON_2);
					cdi.pd.set_button(PointingDevice::Left, held & WPAD_BUTTON_UP);
					cdi.pd.set_button(PointingDevice::Right, held & WPAD_BUTTON_DOWN);
					cdi.pd.set_button(PointingDevice::Down, held & WPAD_BUTTON_LEFT);
					cdi.pd.set_button(PointingDevice::Up, held & WPAD_BUTTON_RIGHT);
				}
			}
		#else // HW_DOL
			PAD_ScanPads();
			uint32_t down = PAD_ButtonsDown(0);
			uint32_t held = PAD_ButtonsHeld(0);

			if (down & PAD_TRIGGER_Z) break;
			cdi.pd.set_button(PointingDevice::Button1, held & PAD_BUTTON_A);
			cdi.pd.set_button(PointingDevice::Button2, held & PAD_BUTTON_B);
			cdi.pd.set_button(PointingDevice::Left, held & PAD_BUTTON_LEFT);
			cdi.pd.set_button(PointingDevice::Right, held & PAD_BUTTON_RIGHT);
			cdi.pd.set_button(PointingDevice::Down, held & PAD_BUTTON_DOWN);
			cdi.pd.set_button(PointingDevice::Up, held & PAD_BUTTON_UP);
		#endif

		// static FPS fps;
		if (MiniCDI::Config::FrameSkip != 0) {
			cdi.run(MiniCDI::Config::FrameSkip+1);
			// fps.update(MiniCDI::Config::FrameSkip+1);
		} else {
			cdi.run(1);
			// fps.update(1);
		}

		#ifdef MINICDI_DEBUG
		VIDEO_WaitVSync();
		#else
		screen.update(cdi.get_display(), cdi.get_display_width(), MiniCDI::Config::ShowLCD ? cdi.get_lcd() : nullptr, cdi.get_cd_read_status());
		#endif
	}
}

static std::string selectedDisc;
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
			exit(0);
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
			return true;
		}

		#ifdef HW_RVL
		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_B || WPAD_ButtonsDown(0) & WPAD_CLASSIC_BUTTON_B || PAD_ButtonsDown(0) & PAD_BUTTON_B) {
		#else // HW_DOL
		if (PAD_ButtonsDown(0) & PAD_BUTTON_B) {
		#endif
			selectedDisc = "";
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
			printf("Up/Down to navigate, A to select, B to boot without disc, HOME to exit\n\n");
			#else // HW_DOL
			printf("Up/Down to navigate, A to select, B to boot without disc, Z to exit\n\n");
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

	do {
		if (MINICDI_CLI_MENU()) {
			printf("\033[2J\033[H"); // Clear screen
			#ifdef HW_RVL
			printf("miniCDi - Philips CD-i emulator (EXPERIMENTAL)                  Wii version\n");
			#else // HW_DOL
			printf("miniCDi - Philips CD-i emulator (EXPERIMENTAL)             GameCube version\n");
			#endif
			printf("___________________________________________________________________________\n\nLoading");
			RUN_CDI(selectedDisc);

			// Reinit console gfx
			console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
			VIDEO_Configure(rmode);
			VIDEO_SetNextFramebuffer(xfb);
			VIDEO_SetBlack(false);
			VIDEO_Flush();
			VIDEO_WaitVSync();
			if (rmode->viTVMode & VI_NON_INTERLACE) { VIDEO_WaitVSync(); }
		}
	} while (SYS_MainLoop());

	FAT_Exit();
	VIDEO_SetBlack(true);
	return 0;
}