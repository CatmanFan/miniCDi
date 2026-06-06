#ifndef MINICDI_LOG
#define MINICDI_LOG

#include <stdarg.h>

namespace MiniCDI
{
	static void Log(const char* txt, ...)
	{
		#if defined(MINICDI_DEBUG) || defined(MINICDI_LOGFILE)

		// Copy arguments to string and allocate buffer
		va_list arg1, arg2;
		va_start(arg1, txt);
		va_copy(arg2, arg1);
		va_end(arg2);
		char *szBuff = new char[1024];
		vsnprintf(szBuff, 1024, txt, arg1);
		va_end(arg1);

		#ifdef __WIIU__
		WHBLogPrintf(szBuff);
		#else
			#if defined(MINICDI_DEBUG) || defined(__3DS__)
			printf("%s\n", szBuff);
			#endif
		#endif

		if (MiniCDI::Config::LogFile)
			fprintf(MiniCDI::Config::LogFile, "%s\n", szBuff);

		delete[] szBuff;

		#endif
	}
}

#endif