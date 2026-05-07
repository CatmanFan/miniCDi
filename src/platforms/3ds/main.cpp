#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdi/common.hpp"

// Console-specific libraries
#include <3ds.h>
#include <citro2d.h>

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
		true	/** PAL mode **/
	};

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

		printf("\x1b[%d;%dH", 4, 0);
		if (cdi.step())
		{
			C3D_Tex fb;
			C3D_TexInit(&fb, cdi.get_display_width(), 280, GPU_RGBA8);
			C3D_TexUpload(&fb, &cdi.get_display()[0]);
			C3D_TexSetFilter(&fb, GPU_LINEAR, GPU_NEAREST);
			C3D_TexBind(0, &fb);

			Tex3DS_SubTexture fb_subtex = (Tex3DS_SubTexture)
											{ .width = fb.width, .height = fb.height, .left = 0, .top = 1, .right = 1, .bottom = 0 };
			C2D_Image fb_img = (C2D_Image){&fb, &fb_subtex};


			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(top, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
			C2D_SceneBegin(top);

			C2D_DrawImageAt(fb_img, 0, 0, 0.5f, NULL, fb.width * fb.width / 320.0f, 240.0f/280.0f);

			C3D_FrameEnd(0);
			C3D_TexDelete(&fb);
		}
	}

	C2D_Fini();
	C3D_Fini();
	gfxExit();
	romfsExit();
	return 0;
}