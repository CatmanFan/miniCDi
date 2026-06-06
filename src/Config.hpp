#ifndef MINICDI_CONFIG
#define MINICDI_CONFIG

namespace MiniCDI
{
	namespace Config
	{
		extern bool TestPlug; // enables service menu
		extern bool PAL;
		extern bool ShowLCD;
		extern bool HasDisc;
		extern size_t FrameSkip;
		extern FILE* LogFile;
	}
}

#endif