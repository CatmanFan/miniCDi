#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdi/common.hpp"

// Console-specific libraries
#include <3ds.h>
#include <citro2d.h>

/*class FPS
{
	int8_t aggregate;
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
			printf("FPS: %2d\n", aggregate);
		}
	}
};*/

class EmulatorWindow
{
	C2D_Image img;
	C3D_Tex tex;
	Tex3DS_SubTexture subtex;
	uint16_t width, height;
	uint16_t tWidth, tHeight;

public:
	/*!
	 * @brief Creates a renderer with the specified width and height dimensions.
	 */
	EmulatorWindow(int width, int height)
	{
		this->width = width;
		this->height = height;

		// Image data
		this->img.tex = &this->tex;

		// Texture dimensions must be square powers of two between 64x64 and 1024x1024
		this->tex.width = this->tex.height = 64;
		while (this->tex.width < width && this->tex.width < 1024)
			this->tex.width *= 2;
		while (this->tex.height < height && this->tex.height < 1024)
			this->tex.height *= 2;
		if (this->tex.width > 1024) this->tex.width = 1024;
		if (this->tex.height > 1024) this->tex.height = 1024;

		// Subtexture
		this->img.subtex = &this->subtex;
		this->subtex.width = this->width;
		this->subtex.height = this->height;

		// (U, V) coordinates
		this->subtex.left = 0.0f;
		this->subtex.top = 1.0f;
		this->subtex.right = (float)this->width / (float)this->tex.width;
		this->subtex.bottom = 1.0 - ((float)this->height / (float)this->tex.height);

		// Initialize the texture with a specific format
		C3D_TexInit(&this->tex, this->tex.width, this->tex.height, GPU_RGBA8);
		C3D_TexSetFilter(&this->tex, GPU_LINEAR, GPU_NEAREST);
	}

	~EmulatorWindow()
	{
		if (this->tex.data != NULL) {
			C3D_TexDelete(&this->tex);
		}
	}

	void Draw()
	{
		C2D_DrawImageAt(this->img, 8, -20, 0.5f, NULL, 0.5f, 1.0f);
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

#include <filesystem>
static std::string RUN_MENU()
{
	if (!std::filesystem::is_directory("sdmc:/3ds/miniCDi/discs/")) {
		return "";
	}
	// Look for ROMs in directory
	std::vector<std::string> discs;
    for (const auto & disc : std::filesystem::directory_iterator("sdmc:/3ds/miniCDi/discs/")) {
        if (!disc.path().extension().compare(".bin") || !disc.path().extension().compare(".BIN"))
			discs.push_back(disc.path().filename());
	}
	if (discs.size() == 0) {
		return "";
	}

	size_t selected = 0;
	bool render = true;

	while (aptMainLoop()) {
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_ZR) exit(0);

		if (kDown & KEY_DOWN) {
			render = true;
			selected = (selected + 1) % discs.size();
		}
		if (kDown & KEY_UP) {
			render = true;
			if (selected == 0) { selected = discs.size() - 1; }
			else selected--;
		}
		if (kDown & KEY_A) {
			return discs[selected];
		}
		if (kDown & KEY_B) {
			break;
		}

		if (render) {
			render = false;
			printf("\033[2J\033[H"); // Clear screen
			printf("miniCDi\n");
			printf("select a disc\n");

			for (size_t i = 0; i < discs.size(); i++) {
				if (i == selected)
					printf((">" + discs[i] + "\n").c_str());
				else
					printf((" " + discs[i] + "\n").c_str());
			}
		}
	}

	return "";
}

static C3D_RenderTarget* top = NULL;

static void RUN_CDI(const std::string &biosName, const std::string &discName)
{
	// Check for BIOS
	if (access(("sdmc:/3ds/miniCDi/rom/" + biosName + ".rom").c_str(), F_OK) != 0) {
		printf("BIOS not found, exiting");
		sleep(5);
		exit(0);
		return;
	}

	// Make use of N3DS clock speed
	osSetSpeedupEnable(true);

	printf("\033[2J\033[H"); // Clear screen
	printf("miniCDi\n");
	printf("Loading CDi 220 bios\n");

	MiniCDI::Config::TestPlug = false;
	MiniCDI::Config::PAL = true;
	MiniCDI::Config::ShowLCD = false;
	MiniCDI::Config::FrameSkip = 3;
	// MiniCDI::Config::LogFile = fopen("sdmc:/3ds/miniCDi/log.txt", "wt");

	MonoI cdi;
	cdi.init("sdmc:/3ds/miniCDi/rom/" + biosName + ".rom", CDi::MonoI);
	cdi.disc.open("sdmc:/3ds/miniCDi/discs/" + discName);

	EmulatorWindow TOPSCREEN(768,280);

	while (aptMainLoop())
	{
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kDown & KEY_ZR) break; // break in order to return to hbmenu

		cdi.pd.set_button(PointingDevice::Left, kHeld & KEY_LEFT);
		cdi.pd.set_button(PointingDevice::Right, kHeld & KEY_RIGHT);
		cdi.pd.set_button(PointingDevice::Down, kHeld & KEY_DOWN);
		cdi.pd.set_button(PointingDevice::Up, kHeld & KEY_UP);
		cdi.pd.set_button(PointingDevice::Button1, kHeld & KEY_A);
		cdi.pd.set_button(PointingDevice::Button2, kHeld & KEY_B);

		// static FPS fps;

		// Ensure that drawing is done at 30fps
		if (MiniCDI::Config::FrameSkip > 0) {
			for (size_t i = 0; i < MiniCDI::Config::FrameSkip; i++) { cdi.run(true); }
			cdi.run();
			// fps.update(MiniCDI::Config::FrameSkip+1);
		} else {
			cdi.run();
			// fps.update();
		}

		TOPSCREEN.Update(cdi.get_display(), cdi.get_display_width());
		C3D_FrameBegin(C3D_FRAME_NONBLOCK);
		C2D_TargetClear(top, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
		C2D_SceneBegin(top);
		TOPSCREEN.Draw();
		C3D_FrameSync();
		C3D_FrameEnd(0);
	}
}

int main(int argc, char* argv[])
{
	// Init libs
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	consoleInit(GFX_BOTTOM, NULL);

	// Create screens
	top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

	// Main loop
	printf("miniCDi\n");
	RUN_CDI("cdi220b", RUN_MENU());

	// Deinit libs
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	return 0;
}