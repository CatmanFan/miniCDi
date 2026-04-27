#ifndef MINICDI_COMMON
#define MINICDI_COMMON

// Macros
#define READ16(x, l)		((*(x+l) << 8) | *((x+l)+1))
#define READ32(x, l)		((*(x+l) << 24) | (*((x+l)+1) << 16) | (*((x+l)+2) << 8) | *((x+l)+3))
#define WRITE16(x, l, v)	*(x+l) = ((v) >> 8) & 0xFF; \
							*((x+l)+1) = ((v) >> 0) & 0xFF;
#define WRITE32(x, l, v)	*(x+l) = ((v) >> 24) & 0xFF; \
							*((x+l)+1) = ((v) >> 16) & 0xFF; \
							*((x+l)+2) = ((v) >> 8) & 0xFF; \
							*((x+l)+3) = ((v) >> 0) & 0xFF;

#if defined(HW_RVL) || defined(HW_DOL)
	#include <ogc/lwp_watchdog.h>
	#define MY_GETTIME ticks_to_nanosecs(gettime())
#endif

// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <dirent.h>
#include <malloc.h>
#include <vector>

// Cores
#include "cdi/config.hpp"
#include "cdi/video.hpp"
#include "cdi/M68000.hpp"
#include "cdi/MC68HC.hpp"
#include "cdi/MCD212.hpp"
#include "cdi/OS9.hpp"

#endif