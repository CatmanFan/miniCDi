#ifndef MINICDI_COMMON
#define MINICDI_COMMON

// Macros
#define READ8(x, l)			(uint8_t)(x[l])
#define READ16(x, l)		(uint16_t)((x[l] << 8) | x[l+1])
#define READ32(x, l)		(uint32_t)((x[l] << 24) | (x[l+1] << 16) | (x[l+2] << 8) | x[l+3])
#define WRITE8(x, l, v)		x[l] = (uint8_t)(v) & 0x00FF;
#define WRITE16(x, l, v)	x[l] = ((uint16_t)(v) >> 8) & 0xFF; \
							x[l+1] = (uint16_t)(v) & 0xFF;
#define WRITE32(x, l, v)	x[l] = ((uint32_t)(v) >> 24) & 0xFF; \
							x[l+1] = ((uint32_t)(v) >> 16) & 0xFF; \
							x[l+2] = ((uint32_t)(v) >> 8) & 0xFF; \
							x[l+3] = (uint32_t)(v) & 0xFF;

#if defined(HW_RVL) || defined(HW_DOL)
	#include <ogc/lwp_watchdog.h>
	#define MY_GETTIME (ticks_to_nanosecs(gettime()))
#endif

#if defined(__3DS__)
	#include <3ds.h>
	#define MY_GETTIME (osGetTime() * 1000000)
#endif

// Libraries
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <fstream>
#include <dirent.h>
#include <malloc.h>
#include <vector>

#include "cdi/Config.hpp"
#include "cdi/Musashi/m68k.h"

// Cores
#include "cdi/VDSC_HLE.hpp"
#include "cdi/SCC68070.hpp"
#include "cdi/OS9.hpp"
#include "cdi/MCD212.hpp"
#include "cdi/MC6805_SLAVE.hpp"
#include "cdi/LCD.hpp"

#include "cdi/Players.hpp"

#endif