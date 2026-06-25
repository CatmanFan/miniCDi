#ifndef MINICDI_LOG
#define MINICDI_LOG

#include <stdarg.h>
#include "cdi/Musashi/m68k.h"

#ifdef __WIIU__
#include <whb/log_cafe.h>
#include <whb/log_udp.h>
#include <whb/log.h>
#endif

namespace MiniCDI
{
	inline static void Log(const char* fmt, ...)
	{
	#if defined(MINICDI_DEBUG) || defined(MINICDI_LOGFILE)
		// Copy arguments to string and allocate buffer
		char szBuff[512];
		// va_list args, args2;
		va_list args;
		va_start(args, fmt);
		// va_copy(args2, args);
		// va_end(args2);
		vsnprintf(szBuff, sizeof(szBuff), fmt, args);
		va_end(args);

		#ifdef MINICDI_DEBUG_MODULE
		MiniCDI::OS9::Module *module = MiniCDI::OS9::get_module(m68k_get_reg(NULL, M68K_REG_PC));
		#endif

		// Print to logfile if available
		if (MiniCDI::Config::LogFile) {
			#ifdef MINICDI_DEBUG_MODULE
			if (module != nullptr)
				fprintf(MiniCDI::Config::LogFile, "[@%08X(%s)]%s\n", m68k_get_reg(NULL, M68K_REG_PC), module->name.c_str(), szBuff);
			else
				fprintf(MiniCDI::Config::LogFile, "[@%08X]%s\n", m68k_get_reg(NULL, M68K_REG_PC), szBuff);
			#else
			fprintf(MiniCDI::Config::LogFile, "%s\n", szBuff);
			#endif
		}

		// Print to screen or other debug output
		#ifdef __WIIU__
			#ifdef MINICDI_DEBUG_MODULE
			if (module != nullptr)
				WHBLogPrintf("[@%08X(%s)]%s", m68k_get_reg(NULL, M68K_REG_PC), module->name.c_str(), szBuff);
			else
				WHBLogPrintf("[@%08X]%s", m68k_get_reg(NULL, M68K_REG_PC), szBuff);
			#else
			WHBLogPrintf("%s", szBuff);
			#endif
		#else
		#ifdef MINICDI_DEBUG
			#ifdef MINICDI_DEBUG_MODULE
			if (module != nullptr)
				printf("[@%08X(%s)]%s\n", m68k_get_reg(NULL, M68K_REG_PC), module->name.c_str(), szBuff);
			else
				printf("[@%08X]%s\n", m68k_get_reg(NULL, M68K_REG_PC), szBuff);
			#else
			printf("%s\n", szBuff);
			#endif
		#endif
		#endif

	#endif
	}
}

#endif