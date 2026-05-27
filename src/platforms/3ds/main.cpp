#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdi/common.hpp"

// Console-specific libraries
#include <3ds.h>
#include <citro2d.h>

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
		C3D_TexDelete(&this->tex);
	}

	void Draw()
	{
		C2D_DrawImageAt(this->img, 40, 0, 0.5f, NULL, 320.0f / this->width, 240.0f / this->height);
	}

	void Update(uint32_t* display_output, int width)
	{
		if (display_output) {
			this->width = width;
			// Clear texture data
			memset(this->tex.data, 0, this->tex.width * this->tex.height * sizeof(u32));

			// Process the pixel data to convert it to the correct format and swizzle it
			for (int i = 0; i < this->height; i++) {
				for (int j = 0; j < this->width; j++) {
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

int main(int argc, char* argv[])
{
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	romfsInit();

	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	consoleInit(GFX_BOTTOM, NULL);
	printf("miniCDi\n");
	printf("Loading CDi 220 bios\n");

	// Check for BIOS
	if (access("romfs:/cdi220b.rom", F_OK) != 0) {
		printf("BIOS not found, exiting");
		sleep(5);
		exit(0);
	}

	MiniCDI::Config::PAL = true;
	MiniCDI::Config::ShowLCD = false;

	// config.log = fopen("miniCDi_log.txt", "wt");

	EmulatorWindow cdiScreen(384,280);
	MonoI cdi;
	cdi.Init("romfs:/cdi220b.rom");
	// MonoIV cdi;

    bool has_quit = false;
    while (aptMainLoop() && !has_quit)
	{
		// Your code goes here
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kDown & KEY_ZR)
			break; // break in order to return to hbmenu

		cdi.pd.set_button(PointingDevice::Left, kHeld & KEY_LEFT);
		cdi.pd.set_button(PointingDevice::Right, kHeld & KEY_RIGHT);
		cdi.pd.set_button(PointingDevice::Down, kHeld & KEY_DOWN);
		cdi.pd.set_button(PointingDevice::Up, kHeld & KEY_UP);
		cdi.pd.set_button(PointingDevice::Button1, kHeld & KEY_A);
		cdi.pd.set_button(PointingDevice::Button2, kHeld & KEY_B);

		// Ensure that drawing is done at 30fps
		cdi.do_frame(false);
		cdi.do_frame(false);
		cdi.do_frame(false);
		cdi.do_frame(true);
		cdiScreen.Update(cdi.get_display(), cdi.get_display_width());

		C3D_FrameBegin(C3D_FRAME_NONBLOCK);
		C2D_TargetClear(top, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
		C2D_SceneBegin(top);

		// Use the top screen
		// C3D_FrameDrawOn(top);

		cdiScreen.Draw();

		C3D_FrameEnd(0);
	}

	// if (config.log)
		// fclose(config.log);

	C2D_Fini();
	C3D_Fini();
	gfxExit();
	romfsExit();
	return 0;
}