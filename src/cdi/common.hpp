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
#include <fstream>
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
#include "cdi/boards/common.hpp"
#include "cdi/boards/MonoI.hpp"

#endif