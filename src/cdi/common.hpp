#ifndef MINICDI_COMMON
#define MINICDI_COMMON

// Libraries
#include <cstdio>
#include <cstdlib>
#ifdef _WIN32
#include <cstdint>
#endif
#include <string>
#include <cstring>
#include <dirent.h>
#ifdef __APPLE__
#include <unistd.h>
#else
#include <malloc.h>
#endif
#include <vector>
#include <cassert>
#include <cmath>
#include <algorithm>

// Platform macros
#ifdef _WIN32
	#define MINICDI_MEMALIGN(TYPE, PTR, SIZE) PTR = (TYPE *)_aligned_malloc((SIZE)*sizeof(TYPE), 32);
	#define MINICDI_MEMFREE(PTR) _aligned_free(PTR);
#elif defined(__APPLE__)
	#define MINICDI_MEMALIGN(TYPE, PTR, SIZE) posix_memalign((void**)&PTR, 32, (SIZE)*sizeof(TYPE));
	#define MINICDI_MEMFREE(PTR) free(PTR);
#else
	#define MINICDI_MEMALIGN(TYPE, PTR, SIZE) PTR = (TYPE *)memalign(32, (SIZE)*sizeof(TYPE));
	#define MINICDI_MEMFREE(PTR) free(PTR);
#endif

// Global defs
#include "cdi/m68k/m68k.h"
#include "cdi/os9/OS9.hpp"
#include "../Config.hpp"
#include "../Log.hpp"
#include "cdi/CDiDisc.hpp"
#include "cdi/AdpcmDecoder.hpp"
#include "FTD.hpp"

// Hardware
#include "cdi/chips/SCC68070.hpp"
#include "cdi/chips/MCD212.hpp"
#include "cdi/chips/MC6805_SLAVE.hpp"
#include "cdi/chips/MC6805_IKAT.hpp"
#include "cdi/PointingDevice.hpp"
#include "cdi/chips/IMS66490_CDIC.hpp"
#include "cdi/chips/DSP56001_DRVDSP.hpp"
#include "cdi/chips/MCD221_CIAP.hpp"

// Boards
#include "cdi/players/common.hpp"
#include "cdi/players/Philips.hpp"

#endif