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

#include <swkbd/rpl_interface.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/userconfig.h>
#include <coreinit/time.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
static ImGuiIO io;

#include "../common/mINI.hpp"

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

class EmuDisplay
{
	SDL_Texture* texture = nullptr;
	SDL_Texture* texture_ftd = nullptr;

public:
	void draw(void* display_output, size_t width)
	{
		if (display_output)
		{
			SDL_UpdateTexture(this->texture, NULL, display_output, width*sizeof(uint32_t));

			#ifdef MINICDI_NATIVERES
			SDL_Rect dest = {1920/2-384,1080/2-280, 384*2,280*2};
			#else
			SDL_Rect dest =
			{
				MiniCDI::Config::PAL ? 219 : 96,
				MiniCDI::Config::PAL ? 0 : -90,
				MiniCDI::Config::PAL ? 1481 : 1728,
				MiniCDI::Config::PAL ? 1080 : 1260
			};
			#endif
			SDL_RenderCopy(SDL_renderer, this->texture, NULL, &dest);
		}
	}

	void draw_ftd(void* display_output, int width, int height)
	{
		if (display_output)
		{
			SDL_UpdateTexture(this->texture_ftd, NULL, display_output, width*sizeof(uint8_t));
			SDL_Rect dest = {
				1920-width*2, 1080-height*2, width*2, height*2
			};
			SDL_RenderCopy(SDL_renderer, this->texture_ftd, NULL, &dest);
		}
	}

	void add_ftd(int width, int height)
	{
		this->texture_ftd = SDL_CreateTexture(SDL_renderer, SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_STREAMING, width, height);
		SDL_SetTextureScaleMode(this->texture_ftd, SDL_ScaleModeNearest);
	}

	EmuDisplay()
	{
		this->texture = SDL_CreateTexture(SDL_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 768, 280);
		SDL_SetTextureScaleMode(this->texture, SDL_ScaleModeLinear);
	}

	~EmuDisplay()
	{
		if (this->texture_ftd) SDL_DestroyTexture(this->texture_ftd);
		if (this->texture) SDL_DestroyTexture(this->texture);
	}
};

static std::string devicePrefix;

static std::string menu, empty, warning;
static void DISPLAY_BIOS_ERROR();

static void RUN_CDI(const std::string &discName)
{
	std::string biosName = "";
	if (access((devicePrefix + "wiiu/apps/miniCDi/rom/cdi220b.rom").c_str(), F_OK) == 0) biosName = "cdi220b";
	else if (access((devicePrefix + "wiiu/apps/miniCDi/rom/cdi200.rom").c_str(), F_OK) == 0) biosName = "cdi200";
	else {
		WHBLogPrintf("[miniCDi] error: BIOS not found in required path");
		DISPLAY_BIOS_ERROR();
		return;
	}

	mINI::INIFile file((devicePrefix + "wiiu/apps/miniCDi/config.ini").c_str());
	mINI::INIStructure ini;
	bool recreateIni = true;
	if (access((devicePrefix + "wiiu/apps/miniCDi/config.ini").c_str(), F_OK) == 0) {
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
			{"FrameSkip", "0"},
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
	MiniCDI::Config::LogFile = fopen((devicePrefix + "wiiu/apps/miniCDi/log.txt").c_str(), "wt");
	#else
	MiniCDI::Config::LogFile = ini["MiniCDI"]["Logging"].compare("1") == 0 ? fopen((devicePrefix + "wiiu/apps/miniCDi/log.txt").c_str(), "wt") : NULL;
	#endif
	MiniCDI::Config::ShowFPS = ini["MiniCDI"]["FPS"].compare("1") == 0;
	MiniCDI::Config::ShowFTD = true;
	MiniCDI::Config::NvramFile = ini["CDI"]["AutosaveNVRAM"].compare("1") == 0 ? devicePrefix + "wiiu/apps/miniCDi/rom/" + biosName + ".nvram" : "";

	MonoI cdi;
	if (!cdi.init(devicePrefix + "wiiu/apps/miniCDi/rom/" + biosName + ".rom", biosName.compare("cdi490a") == 0 ? CDi::MonoIV : CDi::MonoI)) {
		WHBLogPrintf("[miniCDi] error: failed to create CD-i player");
		// OSSleepTicks(OSSecondsToTicks(5));
		return;
	}
	cdi.swap_disc(devicePrefix + "wiiu/apps/miniCDi/discs/" + discName);

	FPS fps;
	EmuDisplay screen;
	screen.add_ftd(cdi.get_ftd_width(), cdi.get_ftd_height());
	/*bool paused = false;
	bool touchDown = false;*/

	while (MiniCDI_WiiU::Running()) {
		VPADStatus status{};
		VPADRead(VPAD_CHAN_0, &status, 1, nullptr);

		/*if (status.tpNormal.touched && !touchDown) { touchDown = true; paused = !paused; }
		if (!status.tpNormal.touched && touchDown) { touchDown = false; }*/
		if (status.trigger & (VPAD_BUTTON_ZR)) break; // exit
		if (status.trigger & (VPAD_BUTTON_PLUS)) cdi.play_disc();
		if (status.trigger & (VPAD_BUTTON_MINUS)) cdi.reset();

		cdi.pd.set_button(PointingDevice::Button1, status.hold & VPAD_BUTTON_A);
		cdi.pd.set_button(PointingDevice::Button2, status.hold & VPAD_BUTTON_B);
		cdi.pd.set_button(PointingDevice::Left, status.hold & (VPAD_BUTTON_LEFT | VPAD_STICK_L_EMULATION_LEFT | VPAD_STICK_R_EMULATION_LEFT));
		cdi.pd.set_button(PointingDevice::Right, status.hold & (VPAD_BUTTON_RIGHT | VPAD_STICK_L_EMULATION_RIGHT | VPAD_STICK_R_EMULATION_RIGHT));
		cdi.pd.set_button(PointingDevice::Down, status.hold & (VPAD_BUTTON_DOWN | VPAD_STICK_L_EMULATION_DOWN | VPAD_STICK_R_EMULATION_DOWN));
		cdi.pd.set_button(PointingDevice::Up, status.hold & (VPAD_BUTTON_UP | VPAD_STICK_L_EMULATION_UP | VPAD_STICK_R_EMULATION_UP));

		if (MiniCDI::Config::FrameSkip != 0) {
			cdi.run(MiniCDI::Config::FrameSkip+1);
			fps.update(MiniCDI::Config::FrameSkip+1);
		} else {
			cdi.run(1);
			fps.update(1);
		}

		SDL_SetRenderDrawColor(SDL_renderer, 0, 0, 0, 255);
		SDL_RenderClear(SDL_renderer);

		screen.draw(cdi.get_display(), cdi.get_display_width());
		if (MiniCDI::Config::ShowFTD && cdi.get_ftd())
			screen.draw_ftd(cdi.get_ftd(), cdi.get_ftd_width(), cdi.get_ftd_height());

		/*if (paused) {
			SDL_Rect rect{0, 0, 1920, 1080};
			SDL_SetRenderDrawColor(SDL_renderer, 0,0,0,192);
			SDL_RenderFillRect(SDL_renderer, &rect);
			SDL_print(1920/2,1080/2,48,{255,255,255,255},"Paused, touch to resume");
			SDL_RenderPresent(SDL_renderer);
			continue;
		}*/
		if (MiniCDI::Config::ShowFPS) {
			SDL_print(2,2,25,{255,255,255,255},"FPS:",true);
			SDL_print(72,2,25,{255,255,0,255},std::to_string(fps.get()),true);
		}

		SDL_RenderPresent(SDL_renderer);
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
		} else {
			std::stable_sort(discs.begin(), discs.end());
		}
	}

	size_t selected = 0;

	while (MiniCDI_WiiU::Running()) {
		VPADStatus status{};
		VPADRead(VPAD_CHAN_0, &status, 1, nullptr);

		// Start the Dear ImGui frame
		ImGui_ImplSDLRenderer2_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		ImGui::ShowDemoWindow();

		// Rendering
		ImGui::Render();
		SDL_RenderSetScale(SDL_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
		SDL_SetRenderDrawColor(SDL_renderer, 0,0,0,255);
		SDL_RenderClear(SDL_renderer);
		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
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

	// Init language strings
	MiniCDI_WiiU::GetSystemLanguage();
	switch (MiniCDI_WiiU::UILanguage)
	{
		default:
			menu = "Select a disc, press \ue001 to boot without disc, or exit with \ue055";
			empty = "No files found";
			warning = "This is an experimental build of miniCDi.\n\n"
					 "Emulation may not function properly.\n"
					 "If the CD-i machine does not boot, press \ue055 to cancel\n"
					 "emulation and try again (this may take several tries).\n\n"
					 "For more information, consult the GitHub repo webpage:\n"
					 "https://github.com/CatmanFan/miniCDi.\n\n"
					 "Press \ue000 or \ue045 to proceed to the main menu";
			break;

		case MiniCDI_WiiU::JAPANESE:
			menu = "ディスクを選んで\ue000を押してください\n\ue001を押すとディスクなしで起動します。\ue055を押すと終了します。";
			empty = "ディスクファイルはありません";
			warning = "こちらはminiCDiの実験版です。\n\n"
					 "エミュレーションは、適切に機能しない場合があります。\n"
					 "起動できない場合は、\ue055を押してエミュレーションを終了してから\n"
					 "やり直してください。詳しくはhttps://github.com/CatmanFan/miniCDiを\n"
					 "ご覧ください。\n\n"
					 "\ue000または\ue045を押しください";
			break;

		case MiniCDI_WiiU::FRENCH_EU:
		case MiniCDI_WiiU::FRENCH_US:
			menu = "Choisissez un fichier de disque, appuyez sur \ue001 pour démarrer le système\nsans disque ou appuyez sur \ue055 pour quitter";
			empty = "Aucun fichier disponible";
			warning = "Attention : vous utilisez une version préliminaire de miniCDi.\n\n"
					 "Il est possible que l'émulation ne puisse pas fonctionner complètement.\n"
					 "Si le système ne semble pas se démarrer, appuyez sur \ue055 afin de\n"
					 "terminer l'émulation, puis réessayez.\n\n"
					 "Pour plus d'informations, veuillez vous référer au site du dépôt\n"
					 "GitHub de miniCDi : https://github.com/CatmanFan/miniCDi.\n\n"
					 "Appuyez sur \ue000 ou \ue045 pour accéder au menu principal";
			break;

		case MiniCDI_WiiU::SPANISH_EU:
			menu = "Selecciona una imagen de disco, pulsa \ue001 para arrancar la consola\nsin disco o pulsa \ue055 para salir";
			empty = "No hay ninguna imagen de disco.";
			warning = "Estás utilizando una versión preliminar de miniCDi.\n\n"
					 "Es posible que la emulación no pueda funcionar correctamente.\n"
					 "Si el sistema no puede arrancar, pulsa \ue055 para terminar\n"
					 "la emulación y inténtalo de nuevo.\n\n"
					 "Para más información, consulta la página GitHub de miniCDi:\n"
					 "https://github.com/CatmanFan/miniCDi.\n\n"
					 "Pulsa \ue000 o \ue045 para acceder al menú principal";

		case MiniCDI_WiiU::SPANISH_US:
			menu = "Elige una imagen de disco, oprime \ue001 para iniciar la consola sin\ndisco u oprime \ue055 para salir";
			empty = "No hay ninguna imagen de disco.";
			warning = "Estás utilizando una versión preliminar de miniCDi.\n\n"
					 "Es posible que la emulación no pueda funcionar correctamente.\n"
					 "Si el sistema no puede iniciar, oprime \ue055 para terminar\n"
					 "la emulación y inténtalo de nuevo.\n\n"
					 "Para más información, consulta la página GitHub de miniCDi:\n"
					 "https://github.com/CatmanFan/miniCDi.\n\n"
					 "Oprime \ue000 o \ue045 para acceder al menú principal";
			break;
	}

	if (MiniCDI_WiiU::Running()) DISPLAY_WARNING();
	while (MiniCDI_WiiU::Running()) {
		std::string disc = RUN_MENU();
		if (MiniCDI_WiiU::Running()) RUN_CDI(disc);
	}

	return 0;
}