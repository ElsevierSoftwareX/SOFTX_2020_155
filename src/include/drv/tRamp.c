/************************************************/
/* Time ramp function */
/* Takes in a ramping state */

inline double
RampParamGetVal( RampParamState* state )
{
    return state->val;
}

inline int
RampParamGetIsRamping( RampParamState* state )
{
    return state->isRamping;
}

inline void
RampParamInit( RampParamState* state, double xInit, const int fe_rate )
{
    state->isRamping = 0;
    state->val = xInit;
    state->dxPrev = 0.0;

    state->req = xInit;
    state->dxMax = 0.0;
    state->ddxMax = 0.0;

    state->softRatio = 4.0;
    state->softVel = 1.36; // how to compute this from softRatio?
    state->minAccCycles = 0.05 * fe_rate;
}

inline void
RampParamLoad( RampParamState* state,
               double          req,
               double          tRamp,
               const int       fe_rate )
{
    double inv_nRamp; // 1 / number of ramp cycles

    if ( tRamp <= 0.0 )
    {
        // no ramp
        state->isRamping = 0;
        state->val = req;
        state->dxPrev = 0.0;

        state->req = req;
        state->dxMax = 0.0;
        state->ddxMax = 0.0;
    }
    else if ( state->isRamping || req != state->val )
    {
        // setup ramp parameters
        inv_nRamp = 1.0 / ( fe_rate * tRamp );
        state->req = req;
        state->dxMax = state->softVel * lfabs( req - state->val ) * inv_nRamp;
        state->ddxMax = state->softRatio * state->dxMax * inv_nRamp;

        // if currently ramping, allow high accelleration
        if ( state->isRamping )
            state->ddxMax =
                lfabs( state->dxMax + state->dxPrev ) / state->minAccCycles;

        // start the ramp
        state->isRamping = 1;
    }
}

inline double
RampParamUpdate( RampParamState* state )
{
    double dxReq; // distance to requested value
    double dxNow; // current step size
    double dxNowAbs; // lfabs(dxNow)
    double ddxNow; // current change in step size
    double nStop; // steps required to stop (may be non-integer)
    double dxStop; // shortest distance required to stop
    double dxLand; // current step size to achieve "soft landing"

    // if not ramping, just return
    if ( !state->isRamping )
        return state->val;

    // requested change
    dxReq = state->req - state->val;

    // apply slew limit
    if ( dxReq > state->dxMax )
        dxNow = state->dxMax;
    else if ( dxReq < -state->dxMax )
        dxNow = -state->dxMax;
    else
        dxNow = dxReq;

    // apply accelleration limit
    ddxNow = dxNow - state->dxPrev;
    if ( ddxNow > state->ddxMax )
        dxNow = state->dxPrev + state->ddxMax;
    else if ( ddxNow < -state->ddxMax )
        dxNow = state->dxPrev - state->ddxMax;

    // enforce "soft landing"
    dxNowAbs = lfabs( dxNow );
    if ( dxNowAbs > state->ddxMax && dxNow * dxReq > 0.0 )
    {
        // number of decellerations required to stop
        nStop = dxNowAbs / state->ddxMax;

        // distance traveled before stopping
        // assuming max decelleration in all subsequent steps
        dxStop = dxNow * ( 1.5 + 0.5 * nStop );

        // dxNow required to make stopping distance = dxReq
        // (approximation for dxReq ~ dxStop)
        dxLand = dxNow + ( dxReq - dxStop ) / ( 1.5 + nStop );

        // if dxLand is slower than dxNow, use it
        if ( lfabs( dxLand ) < dxNowAbs )
            dxNow = dxLand;
    }


    //approx. minimum fraction of a double precision value that when added, changes that value
    const double minprecision = 1.1103e-16;

    //minimum change that will affect the current value (this is approximate, and can be bigger than the minimum change).
    double minchange = lfabs(state->val)*minprecision;

    if(dxNow != 0.0)
    {
        //handle the case where dxReq is smaller than our approx. minimum
        if(minchange > lfabs(dxReq))
        {
            dxNow = dxReq;
        }
        else
        {
            //otherwise, use the approx. minimum as a lower limit for our change.
            if(minchange > lfabs(dxNow))
            {
                dxNow = dxNow > 0 ? minchange : -minchange;
            }
        }
    }


    // update state
    state->isRamping = !( dxNow == 0.0 && state->dxPrev == 0.0 );

    //if close enough, just jump to the end
    if ( lfabs(dxNow) > lfabs(dxReq))
    {
        state->val = state->req;
        dxNow = dxReq;
    }
    else
    {
        state->val += dxNow;
    }

    state->dxPrev = dxNow;

    return state->val;
}
