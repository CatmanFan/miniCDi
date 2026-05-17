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
	int width, height;

public:
	/*!
	 * @brief Creates a renderer with the specified width and height dimensions.
	 */
	EmulatorWindow(int width, int height)
	{
		this->width = width;
		this->height = height;

		/*int memW = 1, memH = 1;
		while (memW < width) memW *= 2;
		while (memH < height) memH *= 2;

		if (C3D_TexInitVRAM(&this->tex, memW, memH, GPU_RGBA8)) {
			C3D_TexSetFilter(&this->tex, GPU_LINEAR, GPU_NEAREST);
			this->subtex = (Tex3DS_SubTexture){ .width = this->width, .height = this->height,
												.left = 0, .top = 1, .right = 1, .bottom = 0 };
			this->img = (C2D_Image){&this->tex, &this->subtex};
		}*/
	}

	~EmulatorWindow()
	{
		// C3D_TexDelete(&this->tex);
		// this->subtex = (Tex3DS_SubTexture){0};
	}

	void Draw(uint32_t* display_output)
	{
		if (display_output) {
			// C3D_TexUpload(&this->tex, display_output);
			// C2D_DrawImageAt(this->img, 0, 0, 0.5f, NULL, /*this->width / 320.0f, this->height / 280.0f*/ 1,1);

			int display_index = 0;
			for(int y = 0; y < this->height; y++) {
				for(int x = 0; x < this->width; x++) {
					display_index = (x + (y * this->height));
					if(display_output[display_index]) {
						C2D_DrawRectSolid(x, y, 0, 1, 1, C2D_Color32((display_output[display_index] & 0xFF000000) >> 24,
																	 (display_output[display_index] & 0x00FF0000) >> 16,
																	 (display_output[display_index] & 0x0000FF00) >> 8,
																	 0xFF));
					}
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

	MiniCDIConfig config = {
		true	/** PAL mode **/,
		false	/** show LCD **/
	};

	// config.log = fopen("miniCDi_log.txt", "wt");

	EmulatorWindow cdiScreen(768,280);
	MonoIPlayer cdi;
	cdi.Init("romfs:/cdi220b.rom", &config);

    bool has_quit = false;
    while (aptMainLoop() && !has_quit)
	{
		// Your code goes here
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_ZR)
			break; // break in order to return to hbmenu

		cdi.step();
		if (cdi.frame_ready())
		{
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(top, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
			C2D_SceneBegin(top);

			cdiScreen.Draw(cdi.get_display());

			C3D_FrameEnd(0);
		}
	}

	// if (config.log)
		// fclose(config.log);

	C2D_Fini();
	C3D_Fini();
	gfxExit();
	romfsExit();
	return 0;
}