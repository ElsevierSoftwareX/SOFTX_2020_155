#ifndef MAP_H_INCLUDED
#define MAP_H_INCLUDED

#include "gsc16ai64.h"
#include "gsc16ao16.h"
#include "gsc18ao8.h"
#include "gsc20ao8.h"

#ifdef OVERSAMPLE
#ifdef SERVO2K
#define OVERSAMPLE_TIMES 32
#define FE_OVERSAMPLE_COEFF feCoeff32x
#elif defined(SERVO4K)
#define OVERSAMPLE_TIMES 16
#define FE_OVERSAMPLE_COEFF feCoeff16x
#elif defined(SERVO16K)
#define OVERSAMPLE_TIMES 4
#define FE_OVERSAMPLE_COEFF feCoeff4x
#elif defined(SERVO32K)
#define OVERSAMPLE_TIMES 2
#define FE_OVERSAMPLE_COEFF feCoeff2x
#elif defined(SERVO256K)
#define OVERSAMPLE_TIMES 1
#else
#error Unsupported system rate when in oversampling mode: only 2K, 16K and 32K are supported
#endif
#endif

#endif
