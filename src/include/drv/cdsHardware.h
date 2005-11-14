#define MAX_ADC_MODULES		4
#define MAX_DAC_MODULES		4
typedef struct CDS_HARDWARE{
	int dacCount;
	long pci_dac[MAX_DAC_MODULES];
	int adcCount;
	long pci_adc[MAX_ADC_MODULES];
}CDS_HARDWARE;
