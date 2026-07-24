#ifndef MINICDI_CONFIG
#define MINICDI_CONFIG

namespace MiniCDI
{
	namespace Config
	{
		extern bool TestPlug; // enables service menu
		extern bool PAL;
		extern bool ShowFPS;
		extern bool ShowFPD;
		extern bool AnalogColors;
		extern size_t FrameSkip;
		extern int PointerAdvance;

		extern FILE* LogFile;
		extern std::string NvramFile;
	}
}

#endif