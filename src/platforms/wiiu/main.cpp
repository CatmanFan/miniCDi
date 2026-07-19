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

#ifdef MINICDI_USE_IMGUI
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
static ImGuiIO io;
#endif

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

static void SDL_print(int x, int y, int size, SDL_Color color, std::string text, bool absolute = false, bool center = false)
{
	FC_Font* font = GetFontForSize(size);
	if (!font) { return; }

	FC_Effect effect;
	effect.color = color;
	effect.scale = FC_MakeScale(1,1);
	effect.alignment = center ? FC_ALIGN_CENTER : FC_ALIGN_LEFT;

	if (!absolute && !center) {
		x -= FC_GetWidth(font, "%s", text.c_str()) / 2;
	}
	if (!absolute) {
		y -= FC_GetHeight(font, "%s", text.c_str()) / 2;
	}

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
		FRENCH_EU,
		FRENCH_US,
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
	enum Language UILanguage = MiniCDI_WiiU::ENGLISH;
	static void GetSystemLanguage()
	{
		UCError err;
		nn::swkbd::LanguageType language = nn::swkbd::LanguageType::English;
		nn::swkbd::RegionType region = nn::swkbd::RegionType::Europe;

		UCHandle handle = UCOpen();
		if(handle < 1)
			UILanguage = MiniCDI_WiiU::ENGLISH;

		UCSysConfig *settings = (UCSysConfig*)MEMAllocFromDefaultHeapEx(sizeof(UCSysConfig), 0x40);
		if(!settings)
		{
			UCClose(handle);
			UILanguage = MiniCDI_WiiU::ENGLISH;
			return;
		}

		strcpy(settings->name, "cafe.language");
		settings->access = 0;
		settings->dataType = UC_DATATYPE_UNSIGNED_INT;
		settings->error = UC_ERROR_OK;
		settings->dataSize = sizeof(nn::swkbd::LanguageType);
		settings->data = &language;

		err = UCReadSysConfig(handle, 1, settings);

		if(err == UC_ERROR_OK)
		{
			strcpy(settings->name, "cafe.region");
			settings->dataSize = sizeof(nn::swkbd::LanguageType);
			settings->data = &region;
			err = UCReadSysConfig(handle, 1, settings);
		}

		UCClose(handle);
		MEMFreeToDefaultHeap(settings);

		if(err != UC_ERROR_OK)
		{
			UILanguage = MiniCDI_WiiU::ENGLISH;
			return;
		}

		switch(language)
		{
			case nn::swkbd::LanguageType::Japanese:
				UILanguage = MiniCDI_WiiU::JAPANESE;
				break;

			default:
			case nn::swkbd::LanguageType::English:
				UILanguage = MiniCDI_WiiU::ENGLISH;
				break;

			case nn::swkbd::LanguageType::French:
				UILanguage = region == nn::swkbd::RegionType::USA ? MiniCDI_WiiU::FRENCH_US : MiniCDI_WiiU::FRENCH_EU;
				break;

			case nn::swkbd::LanguageType::Spanish:
				UILanguage = region == nn::swkbd::RegionType::USA ? MiniCDI_WiiU::SPANISH_US : MiniCDI_WiiU::SPANISH_EU;
				break;

			case nn::swkbd::LanguageType::Portuguese:
				UILanguage = region == nn::swkbd::RegionType::USA ? MiniCDI_WiiU::PORTUGUESE_US : MiniCDI_WiiU::PORTUGUESE_EU;
				break;
		}
	}

	static bool Running()
	{
		if (MiniCDI_WiiU::Close) return false;

		SDL_Event event;
		#ifdef MINICDI_USE_IMGUI
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			switch (event.type) {
				case SDL_QUIT:
					WHBLogPrintf("[miniCDi] got SDL_QUIT");
					MiniCDI_WiiU::Close = true;
					return false;
				default:
					break;
			}
		}
		#else
		if (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				WHBLogPrintf("[miniCDi] got SDL_QUIT");
				MiniCDI_WiiU::Close = true;
				return false;
			}
		}
		#endif

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
			#ifdef MINICDI_USE_IMGUI
			ImGui_ImplSDLRenderer2_Shutdown();
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();
			#endif

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
		#ifdef MINICDI_USE_IMGUI
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
		#else
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		#endif
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

		#ifdef MINICDI_USE_IMGUI
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		// Setup Platform/Renderer backends
		ImGui_ImplSDL2_InitForSDLRenderer(SDL_window, SDL_renderer);
		ImGui_ImplSDLRenderer2_Init(SDL_renderer);
		#endif

		SDL_SetRenderDrawBlendMode(SDL_renderer, SDL_BLENDMODE_BLEND);
		MiniCDI_WiiU::SDL = true;
	}
}

class FPS
{
	int aggregate;
	int incremented;
	clock_t lastTime;
	clock_t currentTime;

public:
	FPS() : aggregate(0)
		  , incremented(0)
		  , lastTime(OSTicksToMilliseconds(OSGetTick()))
	{ }

	int get() { return aggregate; }

	void update(int frames = 1)
	{
		incremented += frames;
		currentTime = OSTicksToMilliseconds(OSGetTick());

		if(currentTime - lastTime >= 1000)
		{
			lastTime = currentTime;
			aggregate = incremented;
			incremented = 0;
		}
	}
};

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
			/*if (lcd_output)
			{
				SDL_UpdateTexture(this->lcd, NULL, lcd_output, (20*7)*sizeof(uint32_t));
			}*/
		}
	}

	void draw()
	{
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
	MiniCDI::Config::LogFile = fopen((devicePrefix + "wiiu/apps/miniCDi/log.txt").c_str(), "wt");
	#else
	MiniCDI::Config::LogFile = ini["MiniCDI"]["Logging"].compare("1") == 0 ? fopen((devicePrefix + "wiiu/apps/miniCDi/log.txt").c_str(), "wt") : NULL;
	#endif
	MiniCDI::Config::ShowFPS = ini["MiniCDI"]["FPS"].compare("1") == 0;
	MiniCDI::Config::ShowLCD = false;
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
		screen.update(cdi.get_display(), cdi.get_display_width(), MiniCDI::Config::ShowLCD ? cdi.get_lcd() : nullptr);

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

		#ifdef MINICDI_USE_IMGUI

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

		#else

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

		SDL_print(1920/2,150,34,{255,255,255,255},menu,false,true);
		if (!noDiscs) {
			for (size_t i = 0; i < discs.size(); i++) {
				SDL_print(1920/2,250+(i*38),28,{255,255,(uint8_t)(i == selected ? 0 : 255),255},discs[i]);
			}
		} else {
			SDL_print(1920/2,250,32,{255,255,0,255},empty);
		}
		SDL_RenderPresent(SDL_renderer);

		#endif
	}

	return "";
}

static void DISPLAY_WARNING()
{
	#ifndef MINICDI_USE_IMGUI
	while (MiniCDI_WiiU::Running()) {
		VPADStatus status{};
		VPADRead(VPAD_CHAN_0, &status, 1, nullptr);
		if (status.trigger & (VPAD_BUTTON_A | VPAD_BUTTON_PLUS))
			break;

		SDL_SetRenderDrawColor(SDL_renderer, 0, 0, 0, 255);
		SDL_RenderClear(SDL_renderer);
		SDL_print(1920/2,70,34,{255,255,0,255},"miniCDi");
		SDL_print(1920/2,1080/2,40,{255,255,255,255},warning,false,true);
		SDL_RenderPresent(SDL_renderer);
	}
	#endif
}

static void DISPLAY_BIOS_ERROR()
{
	#ifndef MINICDI_USE_IMGUI
	while (MiniCDI_WiiU::Running()) {
		VPADStatus status{};
		VPADRead(VPAD_CHAN_0, &status, 1, nullptr);
		if (status.trigger & VPAD_BUTTON_A)
			break;

		SDL_SetRenderDrawColor(SDL_renderer, 0, 0, 0, 255);
		SDL_RenderClear(SDL_renderer);
		SDL_print(1920/2,70,34,{255,255,0,255},"miniCDi");
		SDL_print(1920/2,1080/2,40,{255,255,255,255},"Error: BIOS not found in required path\n\nPress \ue000 to return to menu",false,true);
		SDL_RenderPresent(SDL_renderer);
	}
	#endif
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
			menu = "Select a disc or press \ue001 to boot without disc";
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
			menu = "ディスクを選んで\ue000を押してください。\n \ue001を押すとディスクなしで起動します。";
			empty = "ディスクファイルはありません";
			warning = "こちらはminiCDiの実験版です。\n\n"
					 "エミュレーションは、適切に機能しない場合があります。\n"
					 "起動できない場合は、\ue055を押してエミュレーションを終了してから\n"
					 "やり直してください。詳しくはhttps://github.com/CatmanFan/miniCDiを\n"
					 "ご覧ください。\n\n"
					 "\ue000または\ue045を押しください";
			break;

		case MiniCDI_WiiU::FRENCH_EU:
			menu = "Choisissez un fichier de disque.\nPour démarrer le système sans disque, appuyez sur \ue001.";
			empty = "Aucun fichier disponible";
			warning = "Attention : vous utilisez une version préliminaire de miniCDi.\n\n"
					 "Il est possible que l'émulation ne puisse pas fonctionner complètement.\n"
					 "Si le système ne semble pas se démarrer, appuyez sur \ue055 afin de\n"
					 "terminer l'émulation, puis réessayez.\n\n"
					 "Pour plus d'informations, veuillez vous référer au site du dépôt\n"
					 "GitHub de miniCDi : https://github.com/CatmanFan/miniCDi.\n\n"
					 "Appuyez sur \ue000 ou \ue045 pour accéder au menu principal";
			break;

		case MiniCDI_WiiU::FRENCH_US:
			menu = "Choisissez un fichier de disque ou appuyez sur \ue001 pour démarrer la console sans disque";
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
			menu = "Selecciona una imagen de disco.\nPara arrancar la consola sin disco, pulsa \ue001";
			empty = "No hay ninguna imagen de disco.";
			warning = "Estás utilizando una versión preliminar de miniCDi.\n\n"
					 "Es posible que la emulación no pueda funcionar correctamente.\n"
					 "Si el sistema no puede arrancar, pulsa \ue055 para terminar\n"
					 "la emulación y inténtalo de nuevo.\n\n"
					 "Para más información, consulta la página GitHub de miniCDi:\n"
					 "https://github.com/CatmanFan/miniCDi.\n\n"
					 "Pulsa \ue000 o \ue045 para acceder al menú principal";

		case MiniCDI_WiiU::SPANISH_US:
			menu = "Elige una imagen de disco u oprime \ue001 para iniciar la consola sin disco";
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