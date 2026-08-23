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
	Settings.Language = UI_LANG_EN;
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
			textTranslator->loadLanguage(en_lang, en_lang_size);
			break;
	}

	CallMainMenu:
	MainMenuRequested = 0;
	MainMenu(MENU_MAIN);
	while (!ExitRequested && !ShutdownRequested && !MainMenuRequested)
	{
		float deltaTime = 1.0f / 60.0f;
		platform->getInput()->update(deltaTime);
		if (userInput[0] != nullptr && userInput[0]->isPressed(GUI_BTN_HOME))
		{
			MainMenuRequested = 1;
			break;
		}

		platform->getVideo()->getImageRenderer()->drawRectangle(0,0,platform->getVideo()->getScreenWidth(),platform->getVideo()->getScreenHeight(),(PixelColor){255, 0, 0, 255},1);
		platform->getVideo()->render();
	}
	if (MainMenuRequested)
		goto CallMainMenu;
	else
		platform->shutdown();

	return 0;
}
