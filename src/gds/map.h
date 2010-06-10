#include "hardware.h"
#if defined(_ADVANCED_LIGO)
#include "map_v3.h"
#elif (RMEM_LAYOUT == 0)
#include "map_v1.h"
#elif (RMEM_LAYOUT == 1)
#include "map_v2.h"
#else
#error Bad reflective memory layout specified
#endif

