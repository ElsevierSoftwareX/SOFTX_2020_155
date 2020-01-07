#ifndef TRAMP_H
#define TRAMP_H

/*******************************************************/
/* Header file for the Ramping over Time functions (tRamp) */

typedef struct RampParamState
{
    // running variables (updated each cycle while ramping)
    int    isRamping;
    double val; // current value
    double dxPrev; // previous change

    // ramp parameters (updated on load)
    double req; // requested value
    double dxMax; // maximum change in x in one step
    double ddxMax; // maximum change in dx in one step

    // ramp construction parameters (user set, or fixed)
    double softRatio; // ratio of time at constant velocity to accelleration
    double softVel; // maximum velocity relative to linear ramp
    double minAccCycles; // accelleration cycles (when ramping is interrupted)

} RampParamState;

#include <drv/tRamp.c>

#endif
