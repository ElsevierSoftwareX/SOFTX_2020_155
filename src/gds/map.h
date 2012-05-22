/* Version: $Id$ */
#include "hardware.h"
#if (RMEM_LAYOUT == 0)
#error
#include "map_v1.h"
#elif (RMEM_LAYOUT == 1)
#error
#include "map_v2.h"
#elif (RMEM_LAYOUT == 2)
#include "map_v3.h"
#else
#error Bad reflective memory layout specified
#endif

