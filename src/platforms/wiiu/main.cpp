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

static SDL_Window* SDL_window = nullptr;
static SDL_Renderer* SDL_renderer = nullptr;

static void* SDL_fontData = nullptr;
static uint32_t SDL_fontSize = 0;
static std::map<int, FC_Font*> SDL_fontMap;

static bool SDL_init()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
		WHBLogPrintf("[miniCDi] Failed to init SDL, exiting");
		return false;
	}

	// SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	SDL_window = SDL_CreateWindow("miniCDi", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, 0);
	if (!SDL_window) {
		WHBLogPrintf("[miniCDi] Failed to init SDL window, exiting");
		return false;
	}

	SDL_renderer = SDL_CreateRenderer(SDL_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	if (!SDL_renderer) {
		SDL_DestroyWindow(SDL_window);
		SDL_window = nullptr;
		WHBLogPrintf("[miniCDi] Failed to init SDL renderer, exiting");
		return false;
	}

    if (!OSGetSharedData(OS_SHAREDDATATYPE_FONT_STANDARD, 0, &SDL_fontData, &SDL_fontSize)) {
        WHBLogPrintf("[miniCDi] OSGetSharedData failed\n");
        return false;
    }

	SDL_SetRenderDrawBlendMode(SDL_renderer, SDL_BLENDMODE_BLEND);
	return true;
}

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
	MiniCDI::Config::PAL = true;
	MiniCDI::Config::ShowLCD = false;
	MiniCDI::Config::FrameSkip = 1;
	// MiniCDI::Config::LogFile = fopen((devicePrefix + "wiiu/apps/miniCDi/log.txt").c_str(), "wt");

	MonoI cdi;
	cdi.board = CDi::MonoI;
	cdi.init((devicePrefix + "wiiu/apps/miniCDi/rom/" + biosName).c_str());
	cdi.disc.open((devicePrefix + "wiiu/apps/miniCDi/discs/" + discName).c_str());
	// cdi.run(true);

	EmuDisplay screen;
	bool paused = false;
	bool touchDown = false;

	while (WHBProcIsRunning()) {
		VPADStatus status{};
		VPADRead(VPAD_CHAN_0, &status, 1, nullptr);

		/*if (status.tpNormal.touched && !touchDown) { touchDown = true; paused = !paused; }
		if (!status.tpNormal.touched && touchDown) { touchDown = false; }*/
		if (status.trigger && VPAD_BUTTON_ZR) break; // exit

		// Clear screen
		SDL_SetRenderDrawColor(SDL_renderer, 0, 0, 0, 255);
		SDL_RenderClear(SDL_renderer);
		screen.draw();
		/*if (paused) {
			SDL_Rect rect{0, 0, 1920, 1080};
			SDL_SetRenderDrawColor(SDL_renderer, 0,0,0,192);
			SDL_RenderFillRect(SDL_renderer, &rect);
			SDL_print(1920/2,1080/2,48,{255,255,255,255},"Paused, touch to resume");
			SDL_RenderPresent(SDL_renderer);
			continue;
		}*/
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

	cdi.shutdown();
}

#include <filesystem>
static std::string RUN_MENU()
{
	if (!std::filesystem::is_directory(devicePrefix + "wiiu/apps/miniCDi/discs/")) {
		return "";
	}
	// Look for ROMs in directory
	std::vector<std::string> discs;
    for (const auto & disc : std::filesystem::directory_iterator(devicePrefix + "wiiu/apps/miniCDi/discs/")) {
        if (!disc.path().extension().compare(".bin") || !disc.path().extension().compare(".BIN"))
			discs.push_back(disc.path().filename());
	}
	if (discs.size() == 0) {
		return "";
	}

	size_t selected = 0;

	while (WHBProcIsRunning()) {
		VPADStatus status{};
		VPADRead(VPAD_CHAN_0, &status, 1, nullptr);

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
		if (status.trigger & VPAD_BUTTON_B) {
			break;
		}

		SDL_SetRenderDrawColor(SDL_renderer, 0, 0, 0, 255);
		SDL_RenderClear(SDL_renderer);

		SDL_print(1920/2,70,34,{255,255,0,255},"miniCDi");

		SDL_print(1920/2,150,37,{255,255,255,255},"Select a disc or press \ue001 to boot without disc"); // en
		// SDL_print(1920/2,150,34,{255,255,255,255},"ディスクを選んで\ue000を押してください。\n\ue001を押すとディスクなしで起動します。"); // ja
		// SDL_print(1920/2,150,34,{255,255,255,255},"Choisissez un fichier de disque.\nPour démarrer le système sans disque, appuyez sur \ue001."); // fr
		// SDL_print(1920/2,150,37,{255,255,255,255},"Elige una imagen de disco u oprime \ue001 para comenzar sin disco"); // es-LA
		// SDL_print(1920/2,150,34,{255,255,255,255},"Selecciona una imagen de disco.\nPara arrancar la consola sin disco, pulsa \ue001"); // es-ES
		// SDL_print(1920/2,150,34,{255,255,255,255},"Selecione uma imagen de disco.\nPara ligar a consola sem disco, prima \ue001"); // pt-PT
		// SDL_print(1920/2,150,34,{255,255,255,255},"Bitte wählen Sie eine CD-Datei aus.\nDrücke \ue001, um das System ohne CD zu hochfahren."); // de
		// SDL_print(1920/2,150,37,{255,255,255,255},"Välj en skiva eller tryck på \ue001 för att starta utan en cd-skiva."); // sv
		// SDL_print(1920/2,150,34,{255,255,255,255},"Velg en CD-fil.\nFor å starte uten en CD, trykk på \ue001."); // no
		// SDL_print(1920/2,150,34,{255,255,255,255},"Bir disk seçin.\nDisksiz başlatmak için \ue001 Butonuna basın"); // tr
		// SDL_print(1920/2,150,34,{255,255,255,255},"Trieu una imatge de disc.\nPer arrencar sense disc, pitgeu \ue001"); // ca

		for (size_t i = 0; i < discs.size(); i++) {
			SDL_print(1920/2,250+(i*40),24,{255,255,i == selected ? 0 : 255,255},discs[i]);
		}
		SDL_RenderPresent(SDL_renderer);
	}

	return "";
}

int main(int argc, char **argv) {
	// Init CafeOS and logging
	WHBProcInit();
	WHBLogCafeInit();
	WHBLogUdpInit();
	WHBLogPrintf("[miniCDi] logging initialized");

    VPADInit();
	bool mounted = WHBMountSdCard();
	if (mounted) { WHBLogPrintf("[miniCDi] mounted SD card"); }
	devicePrefix = mounted ? "fs:/vol/external01/" : "/vol/external01/";

	if (!SDL_init()) goto exit;
	atexit(SDL_Quit);

	RUN_CDI("cdi220b.rom", RUN_MENU());

	// Deinit SDL
	if (SDL_renderer) SDL_DestroyRenderer(SDL_renderer);
	if (SDL_window) SDL_DestroyWindow(SDL_window);

	exit:
	if (mounted) { WHBUnmountSdCard(); }
	SDL_Quit();
	// WHBProcShutdown();
	WHBLogPrintf("[miniCDi] The End");
	WHBLogCafeDeinit();
	WHBLogUdpDeinit();
	return 1;
}