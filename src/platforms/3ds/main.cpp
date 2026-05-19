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

			float x = 0, y = 0;
			float pixelSize = 0.75f;
			for(int imgY = 0; imgY < this->height; imgY++) {
				for(int imgX = 0; imgX < this->width; imgX+=2) {
					int display_index = (imgY*this->width)+imgX;
					C2D_DrawRectSolid(x, y, 0, pixelSize, pixelSize,
									  C2D_Color32((display_output[display_index] & 0xFF000000) >> 24,
												  (display_output[display_index] & 0x00FF0000) >> 16,
												  (display_output[display_index] & 0x0000FF00) >> 8,
												  0xFF));
					x += pixelSize;
				}
				x = 0;
				y += pixelSize;
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
		if ((kDown & KEY_A) && cdi.get_display()) {
			std::ofstream bmp_file("miniCDi_output.bmp", std::ios::binary);

			const char* SIGNATURE = "BM";
			bmp_file.write(SIGNATURE, 2);

			constexpr static int DISPLAY_WIDTH = 384;
			constexpr static int DISPLAY_HEIGHT = 280;
			constexpr static int DATA_SIZE = (DISPLAY_WIDTH * DISPLAY_HEIGHT * 2);
			uint32_t file_size = DATA_SIZE + 0x36;
			bmp_file.write((char*)&file_size, 4);

			uint32_t reserved = 0;
			bmp_file.write((char*)&reserved, 4);

			uint32_t data_offs = 0x36;
			bmp_file.write((char*)&data_offs, 4);

			uint32_t info_size = 0x28;
			bmp_file.write((char*)&info_size, 4);

			bmp_file.write((char*)&DISPLAY_WIDTH, 4);
			bmp_file.write((char*)&DISPLAY_HEIGHT, 4);

			uint16_t planes = 1;
			bmp_file.write((char*)&planes, 2);

			uint16_t bpp = 16;
			bmp_file.write((char*)&bpp, 2);

			uint32_t compression = 0;
			bmp_file.write((char*)&compression, 4);
			bmp_file.write((char*)&compression, 4);
			bmp_file.write((char*)&compression, 4);
			bmp_file.write((char*)&compression, 4);
			bmp_file.write((char*)&compression, 4);
			bmp_file.write((char*)&compression, 4);

			for (int y = 0; y < DISPLAY_HEIGHT; y++)
			{
				int flipped_y = DISPLAY_HEIGHT - y - 1;
				bmp_file.write((char*)&cdi.get_display()[flipped_y*DISPLAY_WIDTH], DISPLAY_WIDTH * 2);
			}

			printf("Captured screenshot\n");
		}

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