#include <cstdlib>
#include <cstdio>
#include <ctime>

#include <3ds.h>
#include <citro3d.h>
#include "imgui.h"
#include "backends/imgui_ctru.h"
#include "backends/imgui_citro3d.h"

// CD-i emulator library
#include "cdi/common.hpp"
#include <filesystem>

constexpr auto FB_SCALE = 2.0f;
constexpr auto FB_WIDTH = 400.0f * FB_SCALE;
constexpr auto FB_HEIGHT = 480.0f * FB_SCALE;
constexpr auto DISPLAY_TRANSFER_FLAGS =
    GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) |
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_XY);

static C3D_RenderTarget *s_top = nullptr;
static C3D_RenderTarget *s_bottom = nullptr;
static PhilipsCDI* cdi = nullptr;
static int currentPage = 0;

class FPS
{
public:
	int8_t aggregate;

private:
	int8_t incremented;
	clock_t lastTime;
	clock_t currentTime;

public:
	FPS() : aggregate(0)
		  , incremented(0)
		  , lastTime(osGetTime())
	{ }

	void update(int frames = 1)
	{
		incremented += frames;
		currentTime = osGetTime();

		if(currentTime - lastTime > 1000)
		{
			lastTime = currentTime;
			aggregate = incremented;
			incremented = 0;
		}
	}
};

static FPS fps;

/****************************************************************************
 * drawErrorScreen
 *
 * Draws error screen using console
 ***************************************************************************/

static void drawErrorScreen()
{
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);

	printf("No system ROM found!\n\nAdd system ROMs to sdmc:/3ds/miniCDi/rom\n(e.g. cdi200.rom, cdi220b.rom).\n\nPress START to exit");

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxSwapBuffers();
		hidScanInput();

		// Your code goes here
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu
	}

	gfxExit();
}

/****************************************************************************
 * drawFileBrowser
 ***************************************************************************/
static std::vector<std::filesystem::path> discs;
static bool no_disc_folder = true;
static void drawFileBrowser()
{
	static int currentItem = 0;
	bool selected = false;

	ImGui::SetNextWindowPos( ImVec2(40, 240) );
	ImGui::SetNextWindowSize( ImVec2(320, 240) );
	ImGui::Begin("Select disc image", nullptr, /*ImGuiWindowFlags_NoTitleBar |*/ ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar );

	// Look for ROMs in directory
	static int item_selected_idx = -1; // Here we store our selected data as an index.
	ImGui::SetNextItemWidth(-1);
	if (ImGui::BeginListBox("##Games", ImVec2(-FLT_MIN, 11 * ImGui::GetTextLineHeightWithSpacing())))
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

			const bool is_selected = (item_selected_idx == static_cast<int>(n));
			if (ImGui::Selectable(label.c_str(), is_selected))
				item_selected_idx = n;
		}
		ImGui::EndListBox();
	}

	int buttonWidth = (ImGui::GetContentRegionAvail().x - 6) / 2;
	if (ImGui::Button("Back", ImVec2(buttonWidth, 28))) currentPage = 0;
	ImGui::SameLine();
	if (ImGui::Button("OK", ImVec2(buttonWidth, 28))) selected = true;

	ImGui::End();

	if (selected)
	{
		currentPage = 0;
		if (currentItem >= 0) cdi->swap_disc(discs[item_selected_idx].string());
	}
}

/****************************************************************************
 * drawOptionsPage
 ***************************************************************************/
static void drawOptionsPage()
{
	ImGui::SetNextWindowPos( ImVec2(40, 240) );
	ImGui::SetNextWindowSize( ImVec2(320, 240) );
	ImGui::Begin("Options", nullptr, /*ImGuiWindowFlags_NoTitleBar |*/ ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar );
	ImGui::Text("FPS - %.1f (3DS native), %d (CD-i)", ImGui::GetIO().Framerate, fps.aggregate);
	ImGui::Separator();

	uint8_t fs_min = 0, fs_max = 10, pa_min = 0, pa_max = 10;
	ImGui::DragScalar("Frameskip", ImGuiDataType_U8, &MiniCDI::Config.FrameSkip, 0.2f, &fs_min, &fs_max, "%d", 1);
	ImGui::DragScalar("Pointer advance", ImGuiDataType_U8, &MiniCDI::Config.PointerAdvance, 0.2f, &pa_min, &pa_max, "%d", 1);

	ImGui::Checkbox("Connect test plug", &MiniCDI::Config.TestPlug);
	ImGui::Checkbox("Analog colors", &MiniCDI::Config.AnalogColors);

	ImGui::Separator();
	if (ImGui::Button("Back", ImVec2(ImGui::GetContentRegionAvail().x, 24)))
	{
		currentPage = 0;
	}

	ImGui::End();
}

/****************************************************************************
 * drawMainPage
 ***************************************************************************/

static void drawMainPage()
{
	ImGui::SetNextWindowPos( ImVec2(40, 240) );
	ImGui::SetNextWindowSize( ImVec2(320, 240) );
	ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
	// ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Text("FPS - %.1f (3DS), %d (CD-i)", ImGui::GetIO().Framerate, fps.aggregate);
	ImGui::Separator();

	// if( ImGui::Button("Save State", ImVec2(ImGui::GetContentRegionAvailWidth(), 60)) ) currentPage = 1;
	// if( ImGui::Button("Load State", ImVec2(ImGui::GetContentRegionAvailWidth(), 60)) ) currentPage = 2;

	int buttonWidth = (ImGui::GetContentRegionAvail().x - 6) / 2;
	static int promptIndex = 0;

	if (ImGui::Button("Reset", ImVec2(buttonWidth, 40))) { promptIndex = 1; ImGui::OpenPopup("Are you sure?"); }
	ImGui::SameLine();
	if (no_disc_folder) ImGui::BeginDisabled();
	if (ImGui::Button("Change Disc", ImVec2(buttonWidth, 40))) { currentPage = 1; }
	if (no_disc_folder) ImGui::EndDisabled();

	if (ImGui::Button("Options", ImVec2(buttonWidth, 40))) { currentPage = 2; }
	ImGui::SameLine();
	if (ImGui::Button("Exit", ImVec2(buttonWidth, 40))) { promptIndex = 2; ImGui::OpenPopup("Are you sure?"); }

	ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, -1.0f));
	if (ImGui::BeginPopupModal("Are you sure?", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
	{
		ImGui::Text("Any unsaved progress will be lost\n\n");

		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			switch (promptIndex)
			{
				case 1: cdi->reset(); break;
				case 2: currentPage = -1; break;
			}
			promptIndex = 0;
		}

		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			promptIndex = 0;
		}

		ImGui::EndPopup();
	}

	ImGui::End();
}

class EmuTexture
{
	uint16_t width, height;

public:
	int x, y;
	float scaleX, scaleY;
	C3D_Tex tex;

	/*!
	 * @brief Creates a renderer with the specified width and height dimensions.
	 */
	EmuTexture(int width, int height)
	{
		this->width = width;
		this->height = height;

		// Texture dimensions must be square powers of two between 64x64 and 1024x1024
		this->tex.width = this->tex.height = 64;
		while (this->tex.width < width && this->tex.width < 1024)
			this->tex.width *= 2;
		while (this->tex.height < height && this->tex.height < 1024)
			this->tex.height *= 2;
		if (this->tex.width > 1024) this->tex.width = 1024;
		if (this->tex.height > 1024) this->tex.height = 1024;

		// Initialize the texture with a specific format
		C3D_TexInit(&this->tex, this->tex.width, this->tex.height, GPU_RGBA8);
		C3D_TexSetFilter(&this->tex, GPU_NEAREST, GPU_NEAREST);
	}

	~EmuTexture()
	{
		if (this->tex.data != NULL) {
			C3D_TexDelete(&this->tex);
		}
	}

	void Draw()
	{
		// C2D_DrawImageAt(this->img, this->x, this->y, 0.5f, NULL, this->scaleX, this->scaleY);
	}

	void Update(uint32_t* display_output, int width)
	{
		if (display_output) {
			this->width = width;
			// Clear texture data
			memset(this->tex.data, 0, this->tex.width * this->tex.height * sizeof(u32));

			// Process the pixel data to convert it to the correct format and swizzle it
			for (int i = 0; i < this->width; i++) {
				for (int j = 0; j < this->height; j++) {
					// Swizzle magic to convert into a t3x format
					u32 dst_ptr_offset = ((((j >> 3) * (this->tex.width >> 3) + (i >> 3)) << 6) +
						((i & 1) | ((j & 1) << 1) | ((i & 2) << 1) |
							((j & 2) << 2) | ((i & 4) << 2) | ((j & 4) << 3)));

					// Store the swizzled pixel in the texture
					((u32*)this->tex.data)[dst_ptr_offset] = display_output[j * this->width + i];
				}
			}
		}
	}
};

/****************************************************************************
 * main
 ***************************************************************************/
int main(int argc, char *argv[])
{
	osSetSpeedupEnable(true);
	romfsInit();

	/// **************************************************************************
	// Check for BIOS
	bool available_220b = access("sdmc:/3ds/miniCDi/rom/cdi220b.rom", F_OK) == 0;
	bool available_200 = access("sdmc:/3ds/miniCDi/rom/cdi200.rom", F_OK) == 0;

	if (!available_220b && !available_200)
	{
		romfsExit();
		drawErrorScreen();
		return 1;
	}

	std::filesystem::path biosPath = available_220b ? "sdmc:/3ds/miniCDi/rom/cdi220b.rom"
													: "sdmc:/3ds/miniCDi/rom/cdi200.rom";

	if (std::filesystem::exists("sdmc:/3ds/miniCDi/discs/") && std::filesystem::is_directory("sdmc:/3ds/miniCDi/discs/")) {
		no_disc_folder = false;
		for (const auto & disc : std::filesystem::directory_iterator("sdmc:/3ds/miniCDi/discs/")) {
			if (!disc.path().extension().compare(".bin") || !disc.path().extension().compare(".BIN"))
				discs.push_back(disc.path());
		}
	}

	/// **************************************************************************

	/// **************************************************************************
	// Init graphics and ImGUI
	gfxInitDefault();

#ifndef NDEBUG
	consoleDebugInit(debugDevice_SVC);
	std::setvbuf(stderr, nullptr, _IOLBF, 0);
#endif

	C3D_Init(2 * C3D_DEFAULT_CMDBUF_SIZE);
	s_top = C3D_RenderTargetCreate(FB_HEIGHT * 0.5f, FB_WIDTH, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(s_top, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
	s_bottom = C3D_RenderTargetCreate(FB_HEIGHT * 0.5f, FB_WIDTH * 0.8f, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(s_bottom, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	if (!imgui::ctru::init()) return false;
	imgui::citro3d::init();

	auto &io = ImGui::GetIO();
	io.IniFilename = nullptr; // disable imgui.ini file
	ImGui::StyleColorsDark();
	auto &style = ImGui::GetStyle();
	style.Colors[ImGuiCol_WindowBg].w = 0.8f;
	style.ScaleAllSizes(0.5f);
	io.DisplaySize = ImVec2(400.0f, 480.0f);
	io.DisplayFramebufferScale = ImVec2(FB_SCALE, FB_SCALE);
	io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
	/// **************************************************************************

	// Actually create the CD-i Player
	MiniCDI::Config.PAL = true;
	int frames_run = 0;
	enum CDi::BoardType board = biosPath.stem().compare("cdi490a") == 0 ? CDi::MonoIV
							  : biosPath.stem().compare("cdi220c") == 0 ? CDi::MonoII
							  : CDi::MonoI;
	cdi = new PhilipsCDI();
	cdi->init(biosPath.string(), board);

	EmuTexture texture(768, 280);

	while (aptMainLoop())
	{
		hidScanInput();
		auto const kHeld = hidKeysHeld();

		cdi->pd.set_button(PointingDevice::Left, kHeld & KEY_LEFT);
		cdi->pd.set_button(PointingDevice::Right, kHeld & KEY_RIGHT);
		cdi->pd.set_button(PointingDevice::Down, kHeld & KEY_DOWN);
		cdi->pd.set_button(PointingDevice::Up, kHeld & KEY_UP);
		cdi->pd.set_button(PointingDevice::Button1, kHeld & KEY_A);
		cdi->pd.set_button(PointingDevice::Button2, kHeld & KEY_B);

		if (frames_run == 0) {
			cdi->run(false);
			frames_run += MiniCDI::Config.FrameSkip;

			texture.Update(cdi->get_display(), cdi->get_display_width());
			fps.update(MiniCDI::Config.FrameSkip+1);
		} else {
			cdi->run(true);
			frames_run--;
		}

		// Start the ImGui frame
		imgui::ctru::newFrame();
		ImGui::NewFrame();

		// calculate uv coords
		auto const uv1 = ImVec2(0, 1);
		auto const uv2 = ImVec2(0.75f, 0.453125f);

		// draw to top screen
		auto const drawList = ImGui::GetForegroundDrawList();
        drawList->AddRectFilled(ImVec2(0, 0), ImVec2(400, 240), 0xFF000000);
		drawList->AddImage(&texture.tex, ImVec2(40, 0), ImVec2(360, 240), uv1, uv2);

		switch (currentPage) {
			default: currentPage = 0; drawMainPage(); break;
			case 0: drawMainPage(); break;
			case 1: drawFileBrowser(); break;
			case 2: drawOptionsPage(); break;
		}
		if (currentPage == -1) break;

		// Generate the ImGui graphics
		ImGui::Render();

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

		// clear frame/depth buffers
		C3D_RenderTargetClear(s_top, C3D_CLEAR_ALL, 0x000000FF, 0);
		C3D_RenderTargetClear(s_bottom, C3D_CLEAR_ALL, 0x000000FF, 0);
		imgui::citro3d::render(s_top, s_bottom);

		C3D_FrameEnd(0);
	}

	delete cdi;

	imgui::citro3d::exit();
	ImGui::DestroyContext();

	C3D_RenderTargetDelete(s_bottom);
	C3D_RenderTargetDelete(s_top);
	C3D_Fini();

	romfsExit();

	return 0;
}