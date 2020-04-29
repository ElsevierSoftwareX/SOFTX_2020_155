///	\file gsc16ai64.h 
///	\brief GSC 16bit, 32 channel ADC Module Definitions. See  
///<	<a href="http://www.generalstandards.com/view-products2.php?BD_family=16ai64ssc">GSC 16AI64SSC Manual</a>
///< for more info on board registers.

#define ADC_SS_ID       0x3101  ///< Subsystem ID to identify and locate module on PCI bus

int gsc16ai64AdcStop(void);
int gsc16ai64CheckAdcBuffer(int);
int gsc16ai64Init(CDS_HARDWARE *, struct pci_dev *);

/// Structure defining ADC module PCI register layout as per user manual
typedef struct GSA_ADC_REG{
        unsigned int BCR;       ///< Board Control Register at 0x0  
        unsigned int INTCR;     ///< Interrupt Control Register at 0x4
        unsigned int IDB;       ///< Input Data Buffer at 0x8 
        unsigned int IDBC;      ///< Input Buffer Control at 0xc */
        unsigned int RAG;       ///< Rate A Generator at 0x10 
        unsigned int RBG;       ///< Rate B Generator at 0x14 
        unsigned int BUF_SIZE;  ///< Buffer Size at 0x18
        unsigned int BRT_SIZE;  ///< Burst Size at 0x1c 
        unsigned int SSC;       ///< Scan and Sync Control at 0x20 
        unsigned int ACA;       ///< Active Channel Assignment Register at 0x24
        unsigned int ASSC;      ///< Board Configuration Register at 0x28
        unsigned int AC_VAL;    ///< Autocal Values register at 0x2c
        unsigned int AUX_RWR;   ///< Auxillary Read/Write Register at 0x30
        unsigned int AUX_SIO;   ///< Auxillary Sync Control Register at 0x34
        unsigned int SMUW;      ///< Scan Marker Upper Word 0x38
        unsigned int SMLW;      ///< Scan Maker Lower Word at 0x3c
}GSA_ADC_REG;

#define GSAI_FULL_DIFFERENTIAL  0x200
#define GSAI_64_CHANNEL         0x6
#define GSAI_32_CHANNEL         0x5
#define GSAI_8_CHANNEL          0x3
#define GSAI_SOFT_TRIGGER       0x1000
#define GSAI_RESET              0x8000
#define GSAI_DATA_PACKING       0x40000
#define GSAI_DMA_MODE_NO_INTR   0x10943
#define GSAI_DMA_MODE_NO_INTR_DEMAND   0x20943
#define GSAI_DMA_MODE_INTR      0x10D43
#define GSAI_DMA_LOCAL_ADDR     0x8
#define GSAI_DMA_TO_PCI         0xA
#define GSAI_DMA_START          0x3
#define GSAI_DMA1_START          0x300
#define GSAI_DMA_DONE           0x10
#define GSAI_DMA_BYTE_COUNT     (0x80 * UNDERSAMPLE)
#define GSAI_ISR_ON_SAMPLE      0x3
#define PLX_INT_ENABLE          0x900
#define PLX_INT_DISABLE         0x800
#define GSAI_SAMPLE_START       0x10000
#define GSAI_SET_2S_COMP        0x40
#define GSAI_EXTERNAL_SYNC      0x10
#define GSAI_ENABLE_X_SYNC      0x80
#define GSAI_CLEAR_BUFFER       0x40000
#define GSAI_THRESHOLD          0x001f
#define GSAI_AUTO_CAL           0x2000
#define GSAI_AUTO_CAL_PASS      0x4000
#define GSAI_DMA_DEMAND_MODE    0x80000
#define GSAI_18BIT_DATA         0x100000
#define GSAI_DATA_CODE_OFFSET   0x8000
#define GSAI_DATA_MASK          0xffff
#define GSAI_CHAN_COUNT         32
#define GSAI_CHAN_COUNT_M1      31
#define GSAI_64_OFFSET          (GSAI_CHAN_COUNT * UNDERSAMPLE - 1)

