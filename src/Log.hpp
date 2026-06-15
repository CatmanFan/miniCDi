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
	static void Log(const char* txt, ...)
	{
		#if defined(MINICDI_DEBUG) || defined(MINICDI_LOGFILE) || defined(__3DS__)

		// Copy arguments to string and allocate buffer
		va_list arg1, arg2;
		va_start(arg1, txt);
		va_copy(arg2, arg1);
		va_end(arg2);
		char *szBuff = new char[1024];
		vsnprintf(szBuff, 1024, txt, arg1);
		va_end(arg1);

		MiniCDI::OS9::Module *module = MiniCDI::OS9::get_module(m68k_get_reg(NULL, M68K_REG_PC));

		#ifdef __WIIU__
		if (module)
			WHBLogPrintf("@%08X(%s) %s\n", m68k_get_reg(NULL, M68K_REG_PC), module->name.c_str(), szBuff);
		else
			WHBLogPrintf("@%08X %s\n", m68k_get_reg(NULL, M68K_REG_PC), szBuff);
		#else
			#if defined(MINICDI_DEBUG) || defined(__3DS__)
			// printf("%s\n", szBuff);
			// printf("@%08X %s\n", m68k_get_reg(NULL, M68K_REG_PC), szBuff);
			if (module)
				printf("@%08X(%s) %s\n", m68k_get_reg(NULL, M68K_REG_PC), module->name.c_str(), szBuff);
			else
				printf("@%08X %s\n", m68k_get_reg(NULL, M68K_REG_PC), szBuff);
			#endif
		#endif

		if (MiniCDI::Config::LogFile) {
			if (module)
				fprintf(MiniCDI::Config::LogFile, "@%08X(%s) %s\n", m68k_get_reg(NULL, M68K_REG_PC), module->name.c_str(), szBuff);
			else
				fprintf(MiniCDI::Config::LogFile, "@%08X %s\n", m68k_get_reg(NULL, M68K_REG_PC), szBuff);
		}

		delete[] szBuff;

		#endif
	}
}

#endif