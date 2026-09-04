/****************************************************************************
 * libgui Template
 * Tantric 2009
 *
 * demo.h
 ***************************************************************************/

#ifndef _MINICDI_OGC_H_
#define _MINICDI_OGC_H_

enum {
	METHOD_AUTO,
	METHOD_SD,
	METHOD_USB,
	METHOD_DVD,
	METHOD_SMB,
	METHOD_MC_SLOTA,
	METHOD_MC_SLOTB,
	METHOD_SD_SLOTA,
	METHOD_SD_SLOTB
};

enum {
	UI_LANG_EN = 0,
	UI_LANG_FR,
	UI_LANG_JA,
	UI_LANG_ES,
	UI_LANG_COUNT
};

struct SSettings {
    int		AutoLoad;
    int		AutoSave;
    int		LoadMethod;
	int		SaveMethod;
	char	Folder1[256]; // Path to files
	char	Folder2[256]; // Path to files
	char	Folder3[256]; // Path to files
	int		Language;
};
struct SEmulatorArguments {
	char	SystemROM[1280];
	char	Disc[1280];
};
extern struct SSettings Settings;
extern struct SEmulatorArguments EmulatorArguments;

void ExitApp();
extern int ExitRequested;
extern int ShutdownRequested;

#endif
