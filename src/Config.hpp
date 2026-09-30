#ifndef MINICDI_CONFIG
#define MINICDI_CONFIG

namespace MiniCDI
{
	struct _config
	{
		bool TestPlug = false; // enables service menu
		bool PCB_LLTest = false;
		bool PAL = true;
		bool ShowFPS = false;
		bool AnalogColors = false;
		size_t FrameSkip = 0;
		int PointerAdvance = 1;

		FILE* LogFile = nullptr;
		std::string NvramFile = "";
	};
	inline struct _config Config;
}

#endif