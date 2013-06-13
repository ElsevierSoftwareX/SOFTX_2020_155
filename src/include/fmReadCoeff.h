///	@file fmReadCoeff.h
///	@brief Contains stuctures and defs for loading filer module info from file.

#ifndef FM_READ_COEFF_H
#define FM_READ_COEFF_H

/* char vars' length */
#define CVL  40

typedef struct fmSubSysMap {
  char name[CVL]; 	///< Filter module name within this subsystem 
  int  fmModNum;  	///< Filter module number within this subsystem
  int  biquad;	  	///< 0 -- IIR or FIR; 1 -- biquad form IIR 

  /* The rest of structure is filled by fmReadCoeffFile() */
  int filters;
  VME_FM_OP_COEF fmd;
} fmSubSysMap;


typedef struct fmReadCoeff {
  char site[CVL];   			///< lho, llo, 40m 
  char ifo[CVL];    			///< h1, h2, l1, caltech 
  char system[CVL]; 			///< sus, asc, lsc 
  VME_COEF* pVmeCoeff; 			///< Pointer to coefficient area in shared memory window 

#ifndef FM_SUBSYS_NUM
#   error FM_SUBSYS_NUM has to be #defined to the number of coeff config files
#endif

  struct subSys {
    char name[CVL]; 			///< RM, BS (an optic) or WFS, QPD, OPTLEV or could be empty 
    char archiveNameModifier[CVL]; 	///< Can be used to alter archive file name 
    int numMap;           		///< The number of elements in the map below 
    fmSubSysMap *map;       		///< Connects filter names with filter module numbers for this subsys 
    unsigned long crc;    		///< CRC sum calculated on the input file 
  }  subSys[FM_SUBSYS_NUM];

  /* !!! KEEP subSys sub-structure last in this struct !!! */

} fmReadCoeff;

#define BIG_NUMBER                9.999e+03                              /* MA */
#define HI                        1                                      /* MA */
#define LOW                       0                                      /* MA */
#define MAX_FNAME_LEN             256                                    /* MA */
#define MAX_LINE_LEN              80                                     /* MA */
#define TF_FILE_OPEN_ERROR        1                                      /* MA */
#define TF_THRESHOLDS_NOT_FOUND   2                                      /* MA */
#define TF_PARSE_ERROR            3                                      /* MA */

int fmReadCoeffFile(fmReadCoeff *, int, unsigned long);
char* fmReadErrMsg();
char* fmReadShortErrMsg();

#undef CVL

/* Error codes */ 
#define FM_CANNOT_STAT_INPUT_FILE    1
#define FM_EMPTY_INPUT_FILE          2
#define FM_ARCHIVE_FILE_EXISTS       3
#define FM_CANNOT_OPEN_ARCHIVE_FILE  4
#define FM_CANNOT_WRITE_ARCHIVE_FILE 5
#define FM_INVALID_INPUT_FILE        6
#define FM_FCLOSE_FAILED             7

#endif
