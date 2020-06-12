///	\file timing.c
///	\brief File contains some timing diagnostics previously imbedded
///<		into the controller.c code.
//***********************************************************************
/// \brief Get current kernel time (in GPS)
///	@return Current time in form of GPS Seconds.
//***********************************************************************
inline unsigned long
current_time_fe( void )
{
    struct timespec        t;
    extern struct timespec current_kernel_time( void );
    unsigned long gpsoffset;
    extern long ligo_get_gps_driver_offset( void );

    t = current_kernel_time( );
    gpsoffset = ligo_get_gps_driver_offset( );
    if(gpsoffset == 0) 
    { 
        // IRIG-B installed so no offset reported
        // Up to date with leap second for Jan 1, 2017
        // Next leap second expected June 30, 2020
        t.tv_sec += 37; 
    } else {
        t.tv_sec += gpsoffset;
    }
    // t.tv_sec -= 315964819;
    return t.tv_sec;
}

//***********************************************************************
/// \brief Calculate ADC/DAC duotone offset for diagnostics. \n
///< Code should only run on IOP
//***********************************************************************
#ifdef IOP_MODEL
inline float
duotime( int count, float meanVal, float data[] )
{
    float x, y, sumX, sumY, sumXX, sumXY, msumX;
    int   ii;
    float xInc;
    float offset, slope, answer;
    float den;

    x = 0;
    sumX = 0;
    sumY = 0;
    sumXX = 0;
    sumXY = 0;
    xInc = 1000000 / IOP_IO_RATE;

    for ( ii = 0; ii < count; ii++ )
    {
        y = data[ ii ];
        sumX += x;
        sumY += y;
        sumXX += x * x;
        sumXY += x * y;
        x += xInc;
    }
    msumX = sumX * -1;
    den = ( count * sumXX - sumX * sumX );
    if ( den == 0.0 )
    {
        return ( -1000 );
    }
    offset = ( msumX * sumXY + sumXX * sumY ) / den;
    slope = ( msumX * sumY + count * sumXY ) / den;
    if ( slope == 0.0 )
    {
        return ( -1000 );
    }
    meanVal -= offset;
    answer = meanVal / slope - 91.552;
    return ( answer );
}
inline void
initializeDuotoneDiags( duotone_diag_t* dt_diag )
{
    int ii;
    for ( ii = 0; ii < IOP_IO_RATE; ii++ )
    {
        dt_diag->adc[ ii ] = 0;
        dt_diag->dac[ ii ] = 0;
    }
    dt_diag->totalAdc = 0.0;
    dt_diag->totalDac = 0.0;
    dt_diag->meanAdc = 0.0;
    dt_diag->meanDac = 0.0;
    dt_diag->dacDuoEnable = 0.0;
}
#endif

inline void
initializeTimingDiags( timing_diag_t* timeinfo )
{
    timeinfo->cpuTimeEverMax = 0;
    timeinfo->cpuTimeEverMaxWhen = 0;
    timeinfo->startGpsTime = 0;
    timeinfo->usrHoldTime = 0;
    timeinfo->timeHold = 0;
    timeinfo->timeHoldHold = 0;
    timeinfo->timeHoldWhen = 0;
    timeinfo->timeHoldWhenHold = 0;
    timeinfo->usrTime = 0;
    timeinfo->cycleTime = 0;
}
inline void
sendTimingDiags2Epics( volatile CDS_EPICS* pLocalEpics,
                       timing_diag_t*      timeinfo,
                       adcInfo_t*          adcinfo )
{
    pLocalEpics->epicsOutput.cpuMeter = timeinfo->timeHold;
    pLocalEpics->epicsOutput.cpuMeterMax = timeinfo->timeHoldMax;
    pLocalEpics->epicsOutput.userTime = timeinfo->usrHoldTime;
    timeinfo->timeHoldHold = timeinfo->timeHold;
    timeinfo->timeHold = 0;
    timeinfo->timeHoldWhenHold = timeinfo->timeHoldWhen;
    timeinfo->usrHoldTime = 0;

    pLocalEpics->epicsOutput.adcWaitTime =
        adcinfo->adcHoldTimeAvg / ( CYCLE_PER_SECOND / UNDERSAMPLE );
    pLocalEpics->epicsOutput.adcWaitMin = adcinfo->adcHoldTimeMin;
    pLocalEpics->epicsOutput.adcWaitMax = adcinfo->adcHoldTimeMax;

    adcinfo->adcHoldTimeAvgPerSec = adcinfo->adcHoldTimeAvg / CYCLE_PER_SECOND;
    adcinfo->adcHoldTimeMax = 0;
    adcinfo->adcHoldTimeMin = 0xffff;
    adcinfo->adcHoldTimeAvg = 0;
}
inline void
captureEocTiming( int            cycle,
                  unsigned int   cycle_gps,
                  timing_diag_t* timeinfo,
                  adcInfo_t*     adcinfo )
{

    // Hold the max cycle time over the last 1 second
    if ( timeinfo->cycleTime > timeinfo->timeHold )
    {
        timeinfo->timeHold = timeinfo->cycleTime;
        timeinfo->timeHoldWhen = cycle;
    }
    // Hold the max cycle time since last diag reset
    if ( timeinfo->cycleTime > timeinfo->timeHoldMax )
        timeinfo->timeHoldMax = timeinfo->cycleTime;
    // Avoid calculating the max hold time for the first few seconds
    // if (cycle != 0 && (timeinfo->startGpsTime+3) < cycle_gps) {
    if ( ( timeinfo->startGpsTime + 3 ) < cycle_gps )
    {
        if ( adcinfo->adcHoldTime > adcinfo->adcHoldTimeMax )
            adcinfo->adcHoldTimeMax = adcinfo->adcHoldTime;
        if ( adcinfo->adcHoldTime < adcinfo->adcHoldTimeMin )
            adcinfo->adcHoldTimeMin = adcinfo->adcHoldTime;
        adcinfo->adcHoldTimeAvg += adcinfo->adcHoldTime;
        if ( adcinfo->adcHoldTimeMax > adcinfo->adcHoldTimeEverMax )
        {
            adcinfo->adcHoldTimeEverMax = adcinfo->adcHoldTimeMax;
            adcinfo->adcHoldTimeEverMaxWhen = cycle_gps;
        }
        if ( timeinfo->timeHoldMax > timeinfo->cpuTimeEverMax )
        {
            timeinfo->cpuTimeEverMax = timeinfo->timeHoldMax;
            timeinfo->cpuTimeEverMaxWhen = cycle_gps;
        }
    }
}
