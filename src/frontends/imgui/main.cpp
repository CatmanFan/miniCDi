#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <filesystem>
#include <SDL2/SDL.h>

// CD-i emulator library
#include "cdi/common.hpp"

#ifdef __WIIU__
#include <whb/log_cafe.h>
#include <whb/log_udp.h>
#include <whb/log.h>
#include <whb/proc.h>
#include <whb/sdcard.h>
#include <coreinit/memory.h>
#include <sysapp/title.h>
#endif

// Dear ImGUI
// The code is partially copied from https://github.com/dkosmari/imgui-test-wiiu.
// The backend which is listed on ImGUI's official wiki (by GaryOderNichts) uses GX2, and just linking the SDL2 renderer by itself is still missing GamePad input.
#include "imgui.h"
#include "imgui_memory_editor.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"

static ImGuiIO io;
static bool has_quit = false;
#ifdef __WIIU__
static std::string wiiu_sd_prefix;
static bool wiiu_sd_mounted = false;
#endif

static std::vector<std::filesystem::path> discs;
#ifdef __WIIU__
const std::string discs_directory = (wiiu_sd_prefix + "wiiu/apps/miniCDi/discs/");
const std::string roms_directory = (wiiu_sd_prefix + "wiiu/apps/miniCDi/rom/");
#else
#error Platform not supported, requires static path for games.
#endif

#ifdef __WIIU__
const std::string config_path = (wiiu_sd_prefix + "wiiu/apps/miniCDi/config.ini");
const std::string log_path = (wiiu_sd_prefix + "wiiu/apps/miniCDi/log.txt");
#define USE_CONFIG
#endif

#ifdef USE_CONFIG
#include "../common/mINI.hpp"
#endif

static PhilipsCDI* philips_player = NULL;
static MemoryEditor mem_editor;
static bool emulation_window_open = true;

static void InitConfig(const std::filesystem::path &biosPath)
{
	#ifdef USE_CONFIG
		mINI::INIFile file(config_path.c_str());
		mINI::INIStructure ini;
		bool recreateIni = true;
		if (access(config_path.c_str(), F_OK) == 0) {
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
		MiniCDI::Config::LogFile = fopen(log_path.c_str(), "wt");
		#else
		MiniCDI::Config::LogFile = ini["MiniCDI"]["Logging"].compare("1") == 0 ? fopen(log_path.c_str(), "wt") : NULL;
		#endif
		MiniCDI::Config::ShowFPS = ini["MiniCDI"]["FPS"].compare("1") == 0;
		MiniCDI::Config::ShowFTD = true;
		#ifdef __WIIU__
			MiniCDI::Config::NvramFile = ini["CDI"]["AutosaveNVRAM"].compare("1") == 0 ? (wiiu_sd_prefix + "wiiu/apps/miniCDi/rom/" + biosPath.stem().string() + ".nvram") : "";
		#endif
	#endif
}

static void ReloadConfig()
{
	#ifdef USE_CONFIG
		mINI::INIFile file(config_path.c_str());
		mINI::INIStructure ini;
		bool recreateIni = true;
		if (access(config_path.c_str(), F_OK) == 0) {
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
		// MiniCDI::Config::PAL = ini["CDI"]["PAL"].compare("1") == 0;
		MiniCDI::Config::AnalogColors = ini["CDI"]["AnalogColors"].compare("1") == 0;
		MiniCDI::Config::FrameSkip = std::stoi(ini["MiniCDI"]["FrameSkip"]);
		MiniCDI::Config::PointerAdvance = std::stoi(ini["MiniCDI"]["PointerAdvance"]) + 1;
		// MiniCDI::Config::LogFile = fopen(log_path.c_str(), "wt");
		MiniCDI::Config::ShowFPS = ini["MiniCDI"]["FPS"].compare("1") == 0;
		MiniCDI::Config::ShowFTD = true;
		// MiniCDI::Config::NvramFile = biosPath.stem().string() + ".nvram");
	#endif
}

static void ShutdownCDI()
{
	if (philips_player != NULL)
	{
		delete philips_player;
		philips_player = NULL;
	}
}

static bool CreateCDI(const char* rom, const char* disc)
{
	ShutdownCDI();

	// ------------------------
	// Set up paths and board type
	// ------------------------
	#ifdef __WIIU__
		const std::filesystem::path biosPath = (wiiu_sd_prefix + roms_directory + rom);
		const std::filesystem::path discPath = (wiiu_sd_prefix + discs_directory + disc);
	#else
		const std::filesystem::path biosPath = (roms_directory + rom);
		const std::filesystem::path discPath = (discs_directory + disc);
	#endif
	enum CDi::BoardType board = biosPath.stem().compare("cdi490a") == 0 ? CDi::MonoIV
							  : biosPath.stem().compare("cdi220c") == 0 ? CDi::MonoII
							  : CDi::MonoI;

	if (access(biosPath.string().c_str(), F_OK) != 0) return false;

	// ------------------------
	// Load config.ini
	// ------------------------
	InitConfig(biosPath);

	// ------------------------
	// Actually create the player
	// ------------------------
	philips_player = new PhilipsCDI();
	philips_player->init(biosPath.string(), board);
	if (access(discPath.string().c_str(), F_OK) == 0) philips_player->swap_disc(discPath.string());

	return true;
}

static void ScanDiscs()
{
	discs.clear();
	if (std::filesystem::is_directory(discs_directory)) {
		for (const auto & disc : std::filesystem::directory_iterator(discs_directory)) {
			if (!disc.path().extension().compare(".bin") || !disc.path().extension().compare(".BIN"))
				discs.push_back(disc.path());
		}
	}
}

static void CreateFileDialog()
{
	ImGui::SetNextWindowSize(ImVec2(400, -FLT_MIN));
	ImGui::Begin("miniCDi", NULL, ImGuiWindowFlags_NoResize);

	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
	{
		ImGui::Text("cdi220b.rom not found in '/miniCDi/rom/'");
		ImGui::Separator();

		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}

	// Look for ROMs in directory
	static int item_selected_idx = -1; // Here we store our selected data as an index.

	ImGui::Separator();
	ImGui::Text("Select a disc image (.bin):");
	if (ImGui::BeginListBox("##Games", ImVec2(-FLT_MIN, 6 * ImGui::GetTextLineHeightWithSpacing())))
	{
		for (size_t n = 0; n < discs.size(); n++)
		{
			std::string label;
			if (access(discs[n].string().c_str(), F_OK) == 0)
			{
				std::ifstream disc(discs[n].string(), std::ios::in | std::ios::binary);
				if (disc.is_open())
				{
					disc.seekg(0x9340, std::ios::beg); // 00'02'16 LBA, address of title
					for (int i = 0; i < 32; i++) {
						char c;
						disc.get(c);
						if (c)
							label += c;
						else
							break;
					}
					disc.close();
				}
			}
			if (label.empty()) label = discs[n].filename().string();

			const bool is_selected = (item_selected_idx == n);
			if (ImGui::Selectable(label.c_str(), is_selected))
				item_selected_idx = n;
		}
		ImGui::EndListBox();
	}

	// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
	ImGui::Separator();
	if (ImGui::Button("Go", ImVec2(60, 0)) && !CreateCDI("cdi220b.rom", item_selected_idx >= 0 ? discs[item_selected_idx].filename().string().c_str() : ""))
		ImGui::OpenPopup("Error");
		// ImGui::SetItemDefaultFocus();

	ImGui::SameLine();
	if (ImGui::Button("Exit", ImVec2(60, 0)))
		has_quit = true;

	ImGui::End();
}

static void CreateMainDialog()
{
	ImGui::Begin("miniCDi", &emulation_window_open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNavFocus);

	if (ImGui::Button("Shutdown")) ShutdownCDI();
	ImGui::SameLine();
	if (ImGui::Button("Reset")) { if (philips_player != NULL) philips_player->reset(); }

	ImGui::End();

	if (philips_player != NULL && philips_player->get_memory() != NULL)
		mem_editor.DrawWindow("Memory Editor", philips_player->get_memory(), 8*1024*1024);
}

int main(int argc, char** argv)
{
	#ifdef __WIIU__
		SYSCheckTitleExists(0);

		// Init logging
		WHBLogCafeInit();
		WHBLogUdpInit();
		WHBLogPrintf("[miniCDi] WHB logging initialized");

		// Init mount
		wiiu_sd_mounted = WHBMountSdCard();
		wiiu_sd_prefix = wiiu_sd_mounted ? "fs:/vol/external01/" : "/vol/external01/";
		if (wiiu_sd_mounted) WHBLogPrintf("[miniCDi] mounted SD card");
	#endif

	// TO-DO: use a native UI message box to notify the user if this fails.
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
	#ifdef __WIIU__
        SDL_WiiUSetSWKBDKeyboardMode(SDL_WIIU_SWKBD_KEYBOARD_MODE_RESTRICTED);
        SDL_WiiUSetSWKBDHighlightInitialText(SDL_TRUE);
		SDL_Window* window = SDL_CreateWindow("miniCDi", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 996, 560, 0);
	#else
		SDL_Window* window = SDL_CreateWindow("miniCDi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 768, 560, SDL_WINDOW_RESIZABLE);
	#endif
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
	SDL_Texture* framebuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 768, 280);

	static int screen_width, screen_height;
	SDL_GetWindowSize(window, &screen_width, &screen_height);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	#ifdef __WIIU__
		ImGui::LoadIniSettingsFromDisk("/vol/content/imgui.ini");
	#endif

	ImGui::StyleColorsClassic();
	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer2_Init(renderer);

	ScanDiscs();

	static int frames_run = 0;
	while (!has_quit)
	{
		SDL_Event e;
		while(SDL_PollEvent(&e)) 
		{
			if (philips_player == NULL)
				ImGui_ImplSDL2_ProcessEvent(&e);

			// Capture mouse data.
			int w, h;
			SDL_GetWindowSize(window, &w, &h);
			bool mouse_active = e.motion.x >= 0 && e.motion.x < w && e.motion.y >= 0 && e.motion.y < h;

			switch (e.type)
			{
				case SDL_QUIT:
					has_quit = true;
					break;
				/*case SDL_MOUSEMOTION:
					if (philips_player != NULL && mouse_active) {
						philips_player->pd.set_coord(e.motion.x, e.motion.y, w, h);
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					if (philips_player != NULL && mouse_active) {
						philips_player->pd.set_button(PointingDevice::Button1, e.button.button == 1 && e.button.state == SDL_PRESSED);
						philips_player->pd.set_button(PointingDevice::Button2, e.button.button == 3 && e.button.state == SDL_PRESSED);
					}
					break;*/

                case SDL_CONTROLLERDEVICEADDED:
					SDL_GameControllerOpen(e.cdevice.which);
                    break;

                case SDL_CONTROLLERDEVICEREMOVED:
                {
                    auto ctr = SDL_GameControllerFromInstanceID(e.cdevice.which);
                    if (ctr) SDL_GameControllerClose(ctr);
                    break;
                }

				case SDL_CONTROLLERBUTTONDOWN:
				case SDL_CONTROLLERBUTTONUP:
					if (philips_player != NULL) {
						philips_player->pd.set_button(PointingDevice::Button1, e.cbutton.state == SDL_PRESSED && e.cbutton.button == SDL_CONTROLLER_BUTTON_A);
						philips_player->pd.set_button(PointingDevice::Button2, e.cbutton.state == SDL_PRESSED && e.cbutton.button == SDL_CONTROLLER_BUTTON_B);
						philips_player->pd.set_button(PointingDevice::Left, e.cbutton.state == SDL_PRESSED && e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT);
						philips_player->pd.set_button(PointingDevice::Right, e.cbutton.state == SDL_PRESSED && e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
						philips_player->pd.set_button(PointingDevice::Up, e.cbutton.state == SDL_PRESSED && e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP);
						philips_player->pd.set_button(PointingDevice::Down, e.cbutton.state == SDL_PRESSED && e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN);

						if (e.cbutton.state == SDL_PRESSED && e.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) {
							ShutdownCDI();
						}
						if (e.cbutton.state == SDL_PRESSED && e.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) {
							philips_player->reset();
						}
						if (e.cbutton.state == SDL_PRESSED && e.cbutton.button == SDL_CONTROLLER_BUTTON_START) {
							philips_player->play_disc();
						}
					}
                    break;

				case SDL_CONTROLLERAXISMOTION:
					if (philips_player != NULL) {
						if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
							philips_player->pd.set_button(PointingDevice::Left, e.caxis.value < -20000);
							philips_player->pd.set_button(PointingDevice::Right, e.caxis.value > 20000);
						}
						else if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
							philips_player->pd.set_button(PointingDevice::Up, e.caxis.value < -20000);
							philips_player->pd.set_button(PointingDevice::Down, e.caxis.value > 20000);
						}
						else {
							philips_player->pd.set_button(PointingDevice::Left, false);
							philips_player->pd.set_button(PointingDevice::Right, false);
							philips_player->pd.set_button(PointingDevice::Up, false);
							philips_player->pd.set_button(PointingDevice::Down, false);
						}
					}
					break;
			}
		}

		if (philips_player == NULL)
		{
			// Start the Dear ImGui frame
			ImGui_ImplSDLRenderer2_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();

			// Create the ImGui interface
			CreateFileDialog();

			ImGui::EndFrame();
			ImGui::Render();
		}
		else
		{
			if (frames_run == 0) {
				ReloadConfig();
				philips_player->run(false);
				SDL_UpdateTexture(framebuffer, NULL, philips_player->get_display(), philips_player->get_display_width()*sizeof(uint32_t));
				frames_run += MiniCDI::Config::FrameSkip;
			} else {
				philips_player->run(true);
				frames_run--;
				continue;
			}
		}

		SDL_SetRenderDrawColor(renderer, 0,0,0,255);
		SDL_RenderClear(renderer);
		if (philips_player != NULL) 
		{
			SDL_Rect display = {114,0,768,560};
			SDL_RenderCopy(renderer, framebuffer, NULL, &display);
		}
		else
			ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());

	#ifdef __WIIU__
		// WORKAROUND: SDL does not update clipping until the next draw call.
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderDrawPoint(renderer, 1, 1);
	#endif

		SDL_RenderPresent(renderer);
	}

	// Deinit in reverse order
	#ifdef __WIIU__
		if (wiiu_sd_mounted) WHBUnmountSdCard();
	#endif

	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyTexture(framebuffer);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}