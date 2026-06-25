#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdi/common.hpp"

// Console-specific libraries
#include <SDL2/SDL.h>
#include "SDL_FontCache.h"
#include <map>
#include <coreinit/memory.h>

#include <whb/log_cafe.h>
#include <whb/log_udp.h>
#include <whb/log.h>
#include <whb/proc.h>
#include <whb/sdcard.h>
#include <vpad/input.h>

#include <proc_ui/procui.h>
#include <sysapp/launch.h>

static SDL_Window* SDL_window = nullptr;
static SDL_Renderer* SDL_renderer = nullptr;
static void* SDL_fontData = nullptr;
static uint32_t SDL_fontSize = 0;
static std::map<int, FC_Font*> SDL_fontMap;

static FC_Font* GetFontForSize(int size)
{
    if (SDL_fontMap.count(size)) {
        return SDL_fontMap[size];
    }

    FC_Font* font = FC_CreateFont();
    if (!font) {
        return font;
    }

    if (!FC_LoadFont_RW(font, SDL_renderer, SDL_RWFromMem(SDL_fontData, SDL_fontSize), 1, size, {255,255,255,255}, TTF_STYLE_NORMAL)) {
        FC_FreeFont(font);
        return nullptr;
    }

    SDL_fontMap.insert({size, font});
    return font;
}

static void SDL_print(int x, int y, int size, SDL_Color color, std::string text)
{
    FC_Font* font = GetFontForSize(size);
    if (!font) { return; }

    FC_Effect effect;
    effect.color = color;
    effect.scale = FC_MakeScale(1,1);
    effect.alignment = FC_ALIGN_LEFT;

	x -= FC_GetWidth(font, "%s", text.c_str()) / 2;
	y -= FC_GetHeight(font, "%s", text.c_str()) / 2;

    FC_DrawEffect(font, SDL_renderer, x, y, effect, "%s", text.c_str());
}

namespace MiniCDI_WiiU
{
	static bool Mounted = false;
	static bool SDL = false;
	static bool Close = false;
	enum Language
	{
		ENGLISH = 0,
		JAPANESE,
		FRENCH,
		GERMAN,
		SPANISH_US,
		SPANISH_EU,
		PORTUGUESE_EU,
		PORTUGUESE_US,
		SWEDISH,
		NORWEGIAN,
		TURKISH,
		CATALAN
	};
	enum Language UILanguage = MiniCDI_WiiU::FRENCH;

	static bool Running()
	{
		if (MiniCDI_WiiU::Close) return false;

        SDL_Event event;
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                WHBLogPrintf("[miniCDi] got SDL_QUIT");
				MiniCDI_WiiU::Close = true;
                return false;
            }
        }
		return true;
	}

	static void Exit()
	{
		// Deinit in reverse order
		if (MiniCDI_WiiU::Mounted) { WHBUnmountSdCard(); }

		WHBLogPrintf("[miniCDi] The End");
		WHBLogUdpDeinit();
		WHBLogCafeDeinit();

		if (MiniCDI_WiiU::SDL) {
			if (SDL_renderer) SDL_DestroyRenderer(SDL_renderer);
			if (SDL_window) SDL_DestroyWindow(SDL_window);
			SDL_fontMap.clear();
			SDL_Quit(); // This already calls WHBProcShutdown!!!
		} else {
			WHBProcShutdown();
		}
	}

	static void InitSDL()
	{
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			WHBLogPrintf("[miniCDi] Failed to init SDL, exiting");
			MiniCDI_WiiU::SDL = false;
			return;
		}

		// SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

		SDL_window = SDL_CreateWindow("miniCDi", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, 0);
		if (!SDL_window) {
			WHBLogPrintf("[miniCDi] Failed to init SDL window, exiting");
			MiniCDI_WiiU::SDL = false;
			return;
		}

		SDL_renderer = SDL_CreateRenderer(SDL_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
		if (!SDL_renderer) {
			SDL_DestroyWindow(SDL_window);
			SDL_window = nullptr;
			WHBLogPrintf("[miniCDi] Failed to init SDL renderer, exiting");
			MiniCDI_WiiU::SDL = false;
			return;
		}

		if (!OSGetSharedData(OS_SHAREDDATATYPE_FONT_STANDARD, 0, &SDL_fontData, &SDL_fontSize)) {
			WHBLogPrintf("[miniCDi] OSGetSharedData failed\n");
			MiniCDI_WiiU::SDL = false;
			return;
		}

		SDL_SetRenderDrawBlendMode(SDL_renderer, SDL_BLENDMODE_BLEND);
		MiniCDI_WiiU::SDL = true;
	}
}

class EmuDisplay
{
	SDL_Texture* texture = nullptr;
	SDL_Texture* lcd = nullptr;

public:
	void update(void* display_output, size_t width, void* lcd_output)
	{
		if (display_output) {
			// Draw screen
			SDL_UpdateTexture(this->texture, NULL, display_output, width*sizeof(uint32_t));

			// Draw LCD if available
			if (lcd_output)
			{
				SDL_UpdateTexture(this->lcd, NULL, lcd_output, (20*7)*sizeof(uint32_t));
			}
		}
	}

	void draw()
	{
		SDL_Rect dest =
		{
			MiniCDI::Config::PAL ? 219 : 96,
			MiniCDI::Config::PAL ? 0 : -90,
			MiniCDI::Config::PAL ? 1481 : 1728,
			MiniCDI::Config::PAL ? 1080 : 1260
		};
		SDL_RenderCopy(SDL_renderer, this->texture, NULL, &dest);

		if (MiniCDI::Config::ShowLCD)
		{
			dest = {1920-(20*7), 0, (20*7), 22};
			SDL_RenderCopy(SDL_renderer, this->lcd, NULL, &dest);
		}
	}

	EmuDisplay()
	{
		this->texture = SDL_CreateTexture(SDL_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 768, 280);
		this->lcd = SDL_CreateTexture(SDL_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, (20*7), 22);
	}

	~EmuDisplay()
	{
		if (this->texture) SDL_DestroyTexture(this->texture);
		if (this->lcd) SDL_DestroyTexture(this->lcd);
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

	MiniCDI::Config::TestPlug = false;
	MiniCDI::Config::PAL = false;
	MiniCDI::Config::ShowLCD = true;
	MiniCDI::Config::FrameSkip = 0;
	MiniCDI::Config::LogFile = fopen((devicePrefix + "wiiu/apps/miniCDi/log.txt").c_str(), "wt");

	MonoI cdi;
	cdi.board = CDi::MonoI;
	cdi.init((devicePrefix + "wiiu/apps/miniCDi/rom/" + biosName).c_str());
	cdi.disc.open((devicePrefix + "wiiu/apps/miniCDi/discs/" + discName).c_str());
	cdi.run(true);

	EmuDisplay screen;
	bool paused = false;
	bool touchDown = false;

	while (MiniCDI_WiiU::Running()) {
		VPADStatus status{};
		VPADRead(VPAD_CHAN_0, &status, 1, nullptr);

		if (status.tpNormal.touched && !touchDown) { touchDown = true; paused = !paused; }
		if (!status.tpNormal.touched && touchDown) { touchDown = false; }
		if (status.trigger & (VPAD_BUTTON_ZR)) break; // exit

		// Clear screen
		SDL_SetRenderDrawColor(SDL_renderer, 0, 0, 0, 255);
		SDL_RenderClear(SDL_renderer);
		screen.draw();
		if (paused) {
			SDL_Rect rect{0, 0, 1920, 1080};
			SDL_SetRenderDrawColor(SDL_renderer, 0,0,0,192);
			SDL_RenderFillRect(SDL_renderer, &rect);
			SDL_print(1920/2,1080/2,48,{255,255,255,255},"Paused, touch to resume");
			SDL_RenderPresent(SDL_renderer);
			continue;
		}
		SDL_RenderPresent(SDL_renderer);

		cdi.pd.set_button(PointingDevice::Button1, status.hold & VPAD_BUTTON_A);
		cdi.pd.set_button(PointingDevice::Button2, status.hold & VPAD_BUTTON_B);
		cdi.pd.set_button(PointingDevice::Left, status.hold & (VPAD_BUTTON_LEFT | VPAD_STICK_L_EMULATION_LEFT | VPAD_STICK_R_EMULATION_LEFT));
		cdi.pd.set_button(PointingDevice::Right, status.hold & (VPAD_BUTTON_RIGHT | VPAD_STICK_L_EMULATION_RIGHT | VPAD_STICK_R_EMULATION_RIGHT));
		cdi.pd.set_button(PointingDevice::Down, status.hold & (VPAD_BUTTON_DOWN | VPAD_STICK_L_EMULATION_DOWN | VPAD_STICK_R_EMULATION_DOWN));
		cdi.pd.set_button(PointingDevice::Up, status.hold & (VPAD_BUTTON_UP | VPAD_STICK_L_EMULATION_UP | VPAD_STICK_R_EMULATION_UP));

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

#include <filesystem>
static std::string RUN_MENU()
{
	bool noDiscs = false;
	if (!std::filesystem::is_directory(devicePrefix + "wiiu/apps/miniCDi/discs/")) {
		noDiscs = true;
	}

	std::vector<std::string> discs;
	if (!noDiscs) {
		// Look for ROMs in directory
		for (const auto & disc : std::filesystem::directory_iterator(devicePrefix + "wiiu/apps/miniCDi/discs/")) {
			if (!disc.path().extension().compare(".bin") || !disc.path().extension().compare(".BIN"))
				discs.push_back(disc.path().filename());
		}

		if (discs.size() == 0) {
			noDiscs = true;
		}
	}

	size_t selected = 0;

	while (MiniCDI_WiiU::Running()) {
		VPADStatus status{};
		VPADRead(VPAD_CHAN_0, &status, 1, nullptr);

		if (!noDiscs) {
			if (status.trigger & (VPAD_BUTTON_DOWN | VPAD_STICK_L_EMULATION_DOWN | VPAD_STICK_R_EMULATION_DOWN)) {
				selected = (selected + 1) % discs.size();
			}
			if (status.trigger & (VPAD_BUTTON_UP | VPAD_STICK_L_EMULATION_UP | VPAD_STICK_R_EMULATION_UP)) {
				if (selected == 0) { selected = discs.size() - 1; }
				else selected--;
			}
			if (status.trigger & VPAD_BUTTON_A) {
				return discs[selected];
			}
		}
		if (status.trigger & VPAD_BUTTON_B) {
			break;
		}

		SDL_SetRenderDrawColor(SDL_renderer, 0, 0, 0, 255);
		SDL_RenderClear(SDL_renderer);

		SDL_print(1920/2,70,34,{255,255,0,255},"miniCDi");

		std::string menu = "Select a disc or press \ue001 to boot without disc";
		std::string empty = "Warning: Directory is empty";
		switch (MiniCDI_WiiU::UILanguage)
		{
			default:
				break;
			case MiniCDI_WiiU::SWEDISH:
				menu = "Välj en skiva eller tryck på \ue001 för att starta utan en cd-skiva.";
				empty = "Varning! Katalogen är tom";
				break;
			case MiniCDI_WiiU::JAPANESE:
				menu = "ディスクを選んで\ue000を押してください。\n\ue001を押すとディスクなしで起動します。";
				empty = "ディスクファイルがありません";
				break;
			case MiniCDI_WiiU::FRENCH:
				menu = "Choisissez un fichier de disque.\nPour démarrer le système sans disque, appuyez sur \ue001.";
				empty = "Le dossier est actuellement vide.";
				break;
			case MiniCDI_WiiU::SPANISH_US:
				menu = "Elige una imagen de disco u oprime \ue001 para comenzar sin disco";
				empty = "No hay ninguna imagen de disco.";
				break;
			case MiniCDI_WiiU::SPANISH_EU:
				menu = "Selecciona una imagen de disco.\nPara arrancar la consola sin disco, pulsa \ue001";
				empty = "No hay ninguna imagen de disco.";
				break;
			case MiniCDI_WiiU::PORTUGUESE_EU:
				menu = "Selecione uma imagen de disco.\nPara ligar a consola sem disco, prima \ue001";
				break;
			case MiniCDI_WiiU::GERMAN:
				menu = "Bitte wählen Sie eine CD-Datei aus.\nDrücke \ue001, um das System ohne CD zu hochfahren.";
				break;
			case MiniCDI_WiiU::NORWEGIAN:
				menu = "Velg en CD-fil.\nFor å starte uten en CD, trykk på \ue001.";
				empty = "Advarsel: katalogen er tom";
				break;
			case MiniCDI_WiiU::TURKISH:
				menu = "Bir disk seçin.\nDisksiz başlatmak için \ue001 Butonuna basın";
				break;
			case MiniCDI_WiiU::CATALAN:
				menu = "Trieu una imatge de disc.\nPer arrencar sense disc, pitgeu \ue001";
				empty = "No hi ha cap fitxer.";
				break;
		}

		SDL_print(1920/2,150,34,{255,255,255,255},menu);
		if (!noDiscs) {
			for (size_t i = 0; i < discs.size(); i++) {
				SDL_print(1920/2,250+(i*40),24,{255,255,(uint8_t)(i == selected ? 0 : 255),255},discs[i]);
			}
		} else {
			SDL_print(1920/2,250,28,{255,255,0,255},empty);
		}
		SDL_RenderPresent(SDL_renderer);
	}

	return "";
}

int main(int argc, char **argv) {
	// WHBProcInit is not necessary for SDL (SDL_QUIT event is called on exit)
	atexit(MiniCDI_WiiU::Exit);
	MiniCDI_WiiU::InitSDL();
	if (!MiniCDI_WiiU::SDL) {
		return 1;
	}

	// Init logging
	WHBLogCafeInit();
	WHBLogUdpInit();
	WHBLogPrintf("[miniCDi] logging initialized");

	// Init mount
	MiniCDI_WiiU::Mounted = WHBMountSdCard();
	devicePrefix = MiniCDI_WiiU::Mounted ? "fs:/vol/external01/" : "/vol/external01/";
	if (MiniCDI_WiiU::Mounted)
		WHBLogPrintf("[miniCDi] mounted SD card");

	// Init controller
    VPADInit();

	while (MiniCDI_WiiU::Running()) {
		std::string disc = RUN_MENU();
		if (MiniCDI_WiiU::Running()) RUN_CDI("cdi220b.rom", disc);
	}

	return 0;
}