#ifndef MAP_H_INCLUDED
#define MAP_H_INCLUDED

int mapPciModules(CDS_HARDWARE *);/* Init routine to map adc/dac cards*/
int gsaAdcTrigger(int,int[]);	/* Starts ADC acquisition.	*/
int gsaAdcStop(void);		/* Stop acquistion		*/
int adcDmaDone(int,int *);	/* Checks if ADC DMA complete.	*/
int dacDmaDone(int);
int checkAdcRdy(int,int);	/* Checks if ADC has samples avail */
int gsaAdcDma1(int,int,int);	/* Setup ADC DMA registers	*/
void gsaAdcDma2(int);		/* Send GO bit to ADC DMA registers*/
int gsaDacDma1(int,int);		/* Setup DAC DMA registers.	*/
void gsaDacDma2(int,int);	/* Send GO bit to DAC DMA registers.*/
unsigned int readDio(CDS_HARDWARE *,int);
/*int mapRfm(CDS_HARDWARE *pHardware, struct pci_dev *rfmdev);*/
void writeDio(CDS_HARDWARE *pHardware, int modNum, int data);

#ifdef OVERSAMPLE
#ifdef SERVO2K
#define OVERSAMPLE_TIMES        32
#define FE_OVERSAMPLE_COEFF     feCoeff32x
#elif SERVO16K
#define OVERSAMPLE_TIMES        4
#define FE_OVERSAMPLE_COEFF     feCoeff4x
#elif SERVO32K
#define OVERSAMPLE_TIMES        2
#define FE_OVERSAMPLE_COEFF     feCoeff2x
#else
#error Unsupported system rate when in oversampling mode: only 2K, 16K and 32K are supported
#endif
#endif

#endif
