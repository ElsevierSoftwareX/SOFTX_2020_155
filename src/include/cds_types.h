typedef struct adcInfo_t
{
    int adcWait;
    int adcTime;
    int chanHop;
    int adcHoldTime; ///< Stores time between code cycles
    int adcHoldTimeMax; ///< Stores time between code cycles
    int adcHoldTimeEverMax; ///< Maximum cycle time recorded
    int adcHoldTimeEverMaxWhen;
    int adcHoldTimeMin;
    int adcHoldTimeAvg;
    int adcHoldTimeAvgPerSec;
    int adcRdTime[ MAX_ADC_MODULES ];
    int adcRdTimeMax[ MAX_ADC_MODULES ];
    int adcRdTimeErr[ MAX_ADC_MODULES ];
    int adcChanErr[ MAX_ADC_MODULES ];
    int adcOF[ MAX_ADC_MODULES ];
    int adcData[ MAX_ADC_MODULES ][ MAX_ADC_CHN_PER_MOD ];
    int overflowAdc[ MAX_ADC_MODULES ][ MAX_ADC_CHN_PER_MOD ];
} adcInfo_t;

typedef struct dacInfo_t
{
    int overflowDac[ MAX_DAC_MODULES ]
                   [ MAX_DAC_CHN_PER_MOD ]; // DAC overflow diagnostics
    int dacOutBufSize[ MAX_DAC_MODULES ];
    int dacOutEpics[ MAX_DAC_MODULES ][ MAX_DAC_CHN_PER_MOD ];
} dacInfo_t;

typedef struct duotone_diag_t
{
    float adc[ IOP_IO_RATE ]; // Duotone timing diagnostic variables
    float dac[ IOP_IO_RATE ];
    float timeDac;
    float timeAdc;
    float totalAdc;
    float meanAdc;
    float totalDac;
    float meanDac;
    int   dacDuoEnable;
} duotone_diag_t;

typedef struct timing_diag_t
{
    int cpuTimeEverMax; ///< Maximum code cycle time recorded
    int cpuTimeEverMaxWhen;
    int startGpsTime;
    int usrTime; ///< Time spent in user app code
    int usrHoldTime; ///< Max time spent in user app code
    int cycleTime; ///< Current cycle time
    int timeHold; ///< Max code cycle time within 1 sec period
    int timeHoldHold; ///< Max code cycle time within 1 sec period; hold for
                      ///< another sec
    int timeHoldWhen; ///< Cycle number within last second when maximum reached;
                      ///< running
    int timeHoldWhenHold; ///< Cycle number within last second when maximum
                          ///< reached
    int timeHoldMax;

} timing_diag_t;
