#ifndef MINICDI_LOG
#define MINICDI_LOG

#include <stdarg.h>

namespace MiniCDI
{
	static void Log(const char* txt, ...)
	{
		#ifdef MINICDI_DEBUG
		#ifndef MINICDI_DEBUG_CPU
		va_list arg1, arg2;

		va_start(arg1, txt);

		// Search the total length
		va_copy(arg2, arg1);
		va_end(arg2);

		char *szBuff = new char[1024];

		// Format the string
		vsnprintf(szBuff, 1024, txt, arg1);

		va_end(arg1);
		
		#ifdef __WIIU__
		WHBLogPrintf(szBuff);
		#else
		printf("%s\n", szBuff);
		#endif

		delete[] szBuff;
		#endif
		#endif
	}
}

#endif