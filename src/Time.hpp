#ifndef MINICDI_TIME
#define MINICDI_TIME

#if defined(HW_RVL) || defined(HW_DOL)
	#include <ogc/lwp_watchdog.h>
#endif
#if defined(__3DS__)
	#include <3ds.h>
#endif

namespace MiniCDI
{
	namespace Time
	{
		inline static uint64_t Get()
		{
			#if defined(HW_RVL) || defined(HW_DOL)
			return ticks_to_nanosecs(gettime());
			#else
				#if defined(__3DS__)
				return osGetTime() * 1000000;
				#else
					return 0;
				#endif
			#endif
		}
	}
}

#endif