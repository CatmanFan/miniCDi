#ifndef MINICDI_COMMON
#define MINICDI_COMMON

// Libraries
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <fstream>
#include <dirent.h>
#include <malloc.h>
#include <vector>
#include <cassert>
#include <cmath>
#include <algorithm>

// Time
#ifndef MINICDI_GET_TIME
#if defined(__WIIU__)
	#include <coreinit/time.h>
	#define MINICDI_GET_TIME (double)(OSTicksToNanoseconds(OSGetTick()))
#endif
#if defined(HW_RVL) || defined(HW_DOL)
	#include <ogc/system.h>
	#include <ogc/lwp_watchdog.h>
	#define MINICDI_GET_TIME ticks_to_nanosecs((uint64_t)SYS_Time())
#endif
#if defined(__3DS__)
	#include <3ds.h>
	#define MINICDI_GET_TIME (double)(osGetTime() * 1000000)
#endif
#endif

#ifndef MINICDI_GET_TIME
	#define MINICDI_GET_TIME 0
#endif

// Global defs
#include "cdi/Musashi/m68k.h"
#include "os9/OS9.hpp"
#include "../Config.hpp"
#include "../Log.hpp"
#include "cdi/CDiDisc.hpp"

// Cores
#include "cdi/chips/SCC68070.hpp"
#include "cdi/chips/MCD212.hpp"
#include "cdi/chips/MC6805_SLAVE.hpp"
#include "cdi/chips/MC6805_IKAT.hpp"
#include "cdi/PointingDevice.hpp"
#include "cdi/chips/IMS66490_CDIC.hpp"
#include "cdi/chips/MCD221_CIAP.hpp"

// Other namespace defs
#include "../LCD.hpp"

// Boards
#include "cdi/boards/common.hpp"
#include "cdi/boards/MonoI.hpp"
#include "cdi/boards/MonoIV.hpp"

#endif