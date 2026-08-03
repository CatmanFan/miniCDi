#ifndef MINICDI_CONFIG
#define MINICDI_CONFIG

namespace MiniCDI
{
	namespace Config
	{
		extern bool TestPlug; // enables service menu
		extern bool PAL;
		extern bool ShowFPS;
		extern bool ShowFTD;
		extern bool AnalogColors;
		extern size_t FrameSkip;
		extern bool NoFrameLimit;
		extern int PointerAdvance;

		extern FILE* LogFile;
		extern std::string NvramFile;
	}
}

#endif