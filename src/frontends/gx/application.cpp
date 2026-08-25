/****************************************************************************
 * libgui Template
 * Tantric 2009
 *
 * demo.cpp
 * Basic template/demonstration of libgui capabilities. For a
 * full-featured app using many more extensions, check out Snes9x GX.
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "drivers/ogc/OgcPlatform.h"
#include "menu.h"
#include "filelist.h"
#include "application.h"
#include "libgui/Gui.h"

#include <gccore.h>
#include <ogcsys.h>
#include <wiiuse/wpad.h>

#include <filesystem>
#include "cdi/common.hpp"

struct SSettings Settings;
struct SEmulatorArguments EmulatorArguments;
int ExitRequested = 0;
int ShutdownRequested = 0;
static int MainMenuRequested = 0;

static void ShutdownCallback() { ShutdownRequested = 1; }
#ifdef HW_RVL
static void ShutdownCallbackWPAD(int32_t chan) { ShutdownRequested = 1; }
#endif

// Class responsible for handling OGC drivers (GX, ASND, PAD/WPAD/WiiDRC, etc)
static OgcPlatform ogcPlatformInstance;
Platform* platform = &ogcPlatformInstance;

static void DrawPhilipsCDIScreen(PhilipsCDI *philips_player)
{
	void *texture = platform->getVideo()->getImageRenderer()->createTexture((const uint8_t*)philips_player->get_display(), 768, 280);
	platform->getVideo()->getImageRenderer()->drawTexture(texture, platform->getVideo()->getScreenWidth()/2 - 384, 100, 768, 280, 0,
														  static_cast<float>(platform->getVideo()->getScreenWidth()) / 768.0f,
														  static_cast<float>(platform->getVideo()->getScreenHeight()) / 280.0f,
														  255);
	platform->getVideo()->getImageRenderer()->destroyTexture(texture);
}

static void RunCDIEmulator()
{
	MiniCDI::Config::FrameSkip = 1;

	const std::filesystem::path biosPath = EmulatorArguments.SystemROM;
	const std::filesystem::path discPath = EmulatorArguments.Disc;
	enum CDi::BoardType board = biosPath.stem().compare("cdi490a") == 0 ? CDi::MonoIV
							  : biosPath.stem().compare("cdi220c") == 0 ? CDi::MonoII
							  : CDi::MonoI;

	PhilipsCDI *philips_player = new PhilipsCDI();
	philips_player->init(biosPath.string(), board);
	if (access(discPath.string().c_str(), F_OK) == 0) philips_player->swap_disc(discPath.string());

	// uint8_t *GXtexture = (uint8_t*)memalign(32, GX_GetTexBufferSize(768, 280, GX_TF_RGBA8, GX_FALSE, 0));
	int frames_run = 0;

	while (!ExitRequested && !ShutdownRequested && !MainMenuRequested)
	{
		float deltaTime = 1.0f / 60.0f;
		platform->getInput()->update(deltaTime);
		if (userInput[0] != nullptr)
		{
			if (userInput[0]->isPressed(GUI_BTN_HOME))
			{
				MainMenuRequested = 1;
				break;
			}

			if (userInput[0]->getPadData().validPointer)
			{
				philips_player->pd.set_coord(userInput[0]->getPadData().cursor_x, userInput[0]->getPadData().cursor_y, platform->getVideo()->getScreenWidth(), 480);
				philips_player->pd.set_button(PointingDevice::Button1, userInput[0]->isHeld(GUI_BTN_A));
				philips_player->pd.set_button(PointingDevice::Button2, userInput[0]->isHeld(GUI_BTN_B));
			}
			else
			{
				philips_player->pd.set_button(PointingDevice::Button1, userInput[0]->isHeld(GUI_BTN_A | GUI_BTN_1));
				philips_player->pd.set_button(PointingDevice::Button2, userInput[0]->isHeld(GUI_BTN_B | GUI_BTN_2));
				philips_player->pd.set_button(PointingDevice::Up, userInput[0]->isHeld(GUI_BTN_UP));
				philips_player->pd.set_button(PointingDevice::Down, userInput[0]->isHeld(GUI_BTN_DOWN));
				philips_player->pd.set_button(PointingDevice::Left, userInput[0]->isHeld(GUI_BTN_LEFT));
				philips_player->pd.set_button(PointingDevice::Right, userInput[0]->isHeld(GUI_BTN_RIGHT));
			}
		}

		// Run a frame
		if (frames_run == 0) {
			philips_player->run(false);
			frames_run += MiniCDI::Config::FrameSkip;
			DrawPhilipsCDIScreen(philips_player);
		} else {
			philips_player->run(true);
			frames_run--;
			continue;
		}

		platform->getVideo()->render();
	}

	// fade out
	for (size_t i = 0; i <= 255; i += (ShutdownRequested ? 15 : 30))
	{
		DrawPhilipsCDIScreen(philips_player);
		platform->getVideo()->getImageRenderer()->drawRectangle(0,0,platform->getVideo()->getScreenWidth(),platform->getVideo()->getScreenHeight(),(PixelColor){0, 0, 0, (uint8_t)i},1);
		platform->getVideo()->render();
	}

	// if (GXtexture)
		// free(GXtexture);
}

int main(int argc, char *argv[])
{
	(void)argc; (void)argv; // unused

	// Initiate the platform drivers
	platform->init();

	// Initiate shutdown callbacks
	SYS_SetPowerCallback(ShutdownCallback);
	#ifdef HW_RVL
	WPAD_SetPowerButtonCallback(ShutdownCallbackWPAD);
	#endif

	InitGUIThreads(); // Initialize GUI

	// Load default settings
	Settings.Language = UI_LANG_JA;
	Settings.LoadMethod = METHOD_AUTO;
	Settings.SaveMethod = METHOD_AUTO;
	sprintf (Settings.Folder1,"apps/miniCDi/rom");
	sprintf (Settings.Folder2,"apps/miniCDi/discs");
	sprintf (Settings.Folder3,"libgui/third folder");
	Settings.AutoLoad = 1;
	Settings.AutoSave = 1;
	sprintf (EmulatorArguments.SystemROM, "%s/cdi200.rom", Settings.Folder1);

	// Load font and language
	textTranslator = new GuiTextTranslator();
	switch (Settings.Language)
	{
		default:
			fontSystem = new GuiTextRenderer(font2_ttf, font2_ttf_size, platform->getVideo()->getGlyphRenderer());
			textTranslator->loadLanguage(en_lang, en_lang_size);
			break;

		case UI_LANG_JA:
			fontSystem = new GuiTextRenderer(jp_ttf, jp_ttf_size, platform->getVideo()->getGlyphRenderer());
			textTranslator->loadLanguage(ja_lang, ja_lang_size);
			break;
	}

	CallMainMenu:
	MainMenuRequested = 0;
	MainMenu(MENU_MAIN);
	RunCDIEmulator();
	if (MainMenuRequested)
		goto CallMainMenu;
	else
		platform->shutdown();

	return 0;
}
