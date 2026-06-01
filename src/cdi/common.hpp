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

// Main namespace defs
#include "../Config.hpp"
#include "../Log.hpp"

// 68K processor
#include "cdi/Musashi/m68k.h"
#include "cdi/chips/SCC68070.hpp"
#include "os9/OS9.hpp"

// Other cores
#include "cdi/VDSC.hpp"
#include "cdi/chips/MCD212.hpp"
#include "cdi/chips/MC6805_SLAVE.hpp"
#include "cdi/chips/MC6805_IKAT.hpp"
#include "cdi/PointingDevice.hpp"
#include "cdi/CDiDisc.hpp"
#include "cdi/chips/IMS66490_CDIC.hpp"
#include "cdi/chips/MCD221_CIAP.hpp"

// Other namespace defs
#include "../LCD.hpp"

// Boards
#include "cdi/boards/common.hpp"
#include "cdi/boards/MonoI.hpp"
#include "cdi/boards/MonoIV.hpp"

#endif