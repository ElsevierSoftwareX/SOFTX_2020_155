///	@file fmReadCoeff.c
///	@brief Routines compiled with skeleton.st to allow EPICS to read in DAQ .ini configuration files.

#include <stdio.h>
#include <stdlib.h>
#ifdef SOLARIS
#include <strings.h>
#endif
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#ifdef unix_test
/* The number of subsystems (optics) */
#define FM_SUBSYS_NUM  1
#define MAX_MODULES    46
#else
#ifdef SOLARIS
#define MAX_MODULES    1
#else
#define MAX_MODULES    0
#endif
#define FM_SUBSYS_NUM  1
#endif

#include "fm10Gen.h"
#include "fmReadCoeff.h"
#include "crc.h"

/* Cat string and make upper case */
static char *strcat_upper(char *dest, char *src) {
  char *d = dest;

  for( ; *d; d++);
  for( ; (*d++ = toupper(*src)); src++);

  return dest;
}

/* Cat string and make lower case */
static char *strcat_lower(char *dest, char *src) {
  char *d = dest;

  for( ; *d; d++);
  for( ; (*d++ = tolower(*src)); src++);

  return dest;
}

/* Determine whether string is not a comment or white space */
static int info_line(char *c) {
  /* Skip white spaces in front */
  for( ; isspace(*c); c++);

  return *c && *c != '#'; /* Not end of line or start of comment */
}

/* Max number of tokens allowed on a line */
#define MAX_TOKENS 12

/* Tokenize line */
static int lineTok(char *s, char *tokPtr[]) {
  int nToks;

  for (nToks = 0;
       nToks < MAX_TOKENS && 
       (tokPtr[nToks] = strtok_r(0, " \t\n", &s));
       nToks++);

  return nToks;
}

/* Convert string to number */
static int intToken(char *str, int *res) {
  char *ptr;

  *res = strtol(str, &ptr, 10);
 
  return *ptr == 0;
}

/* Convert string to number */
static int doubleToken(char *str, double *res) {
  char *ptr;

  *res = strtod(str, &ptr);
 
  return *ptr == 0 || *ptr == '\n'; /* Trailing new line on last token */
}

/* Verbose error message text */
static char fmErrMsgTxt[256] = "";

/* Concise error message text */
static char fmShortErrMsgTxt[40] = "";

/* Return error message text to application */
char* fmReadErrMsg() { return fmErrMsgTxt; }
char* fmReadShortErrMsg() { return fmShortErrMsgTxt; }

/* Copy 8 bytes from 'src' to 'dst' and byteswap words */
void memcpy_swap_words(void *dest, void *src) {
  ((char *)dest)[0] = ((char *)src)[4];
  ((char *)dest)[1] = ((char *)src)[5];
  ((char *)dest)[2] = ((char *)src)[6];
  ((char *)dest)[3] = ((char *)src)[7];
  ((char *)dest)[4] = ((char *)src)[0];
  ((char *)dest)[5] = ((char *)src)[1];
  ((char *)dest)[6] = ((char *)src)[2];
  ((char *)dest)[7] = ((char *)src)[3];
}

#ifdef FIR_FILTERS
  /// Temporary storage for FIR filter coefficients 
  double firFiltCoeff[MAX_FIR_MODULES][FILTERS][1 + FIR_TAPS];
#endif

/// Read system 'fmc' coeffs for subsys 'n' 
int fmReadCoeffFile(fmReadCoeff *fmc, int n, unsigned long gps) {
  int i, j, k;
  int ix;

  char fname[2][256];                 /* Input coeff file names */
  char archiveFname[2][256];          /* Archive file names */

  FILE *iFile[2];                     /* Input files */
  FILE *aFile[2];                     /* Archive files */

  char *p;                            /* Temporary variable */

  unsigned int iFileLen[2] = {0, 0};  /* Input file lengths in bytes */
  unsigned int totalLength;

  unsigned long crc[2] = {0, 0};      /* Input file checksums */

  int FIRFileExists = 0;
  int inFileCount;

  char fileExt[2][5] = {".txt", ".fir"};

  /* Pointers to tokens assigned by lineTok() */
  char *tokPtr[MAX_TOKENS];


  /* Close files, delete archive file */
#define fatal(ix) \
  fclose(aFile[ix]); \
  fclose(iFile[ix]); \
  unlink(archiveFname[ix])

  /* Clear filter coeff placeholder */
  for (i = 0; i < fmc->subSys[n].numMap; i++) {
    fmc->subSys[n].map[i].filters = 0;
    memset(&fmc->subSys[n].map[i].fmd, 0, sizeof(fmc->subSys[n].map[i].fmd));
  }

#if 0
/* ###  DEBUG  ### */
#ifdef FIR_FILTERS
  fprintf(stderr, "%s", "\n###  FIR_FILTERS is defined\n\n");
#else
  fprintf(stderr, "%s", "\n###  FIR_FILTERS is NOT defined\n\n");
#endif
/* ###  DEBUG  ### */
#endif

  /* Construct filenames */
  strcat(strcat_lower(strcpy(fname[0], "/opt/rtcds/"), fmc->site), "/");
  strcat(strcat_lower(fname[0], fmc->ifo),"/");
  strcat(fname[0], "chans/");
  strcpy(archiveFname[0], fname[0]);
  strcpy(fname[1], fname[0]);
  strcat(fname[0], "tmp/");

  strcat_upper(fname[0], fmc->system);
  strcat_upper(fname[1], fmc->system);
  if (strlen(fmc->subSys[n].name) > 0) /* Only append non-empty subsystem name */
    strcat_upper(strcat(fname[0], "_"), fmc->subSys[n].name);


  strcat(fname[0], ".txt");
  printf("Input %s\n", fname[0]);

  strcat(fname[1], ".fir");
  printf("FIR input %s\n", fname[1]);

  strcat(archiveFname[0], "filter_archive/");

  strcat(strcat_lower(archiveFname[0], fmc->system), "/");
  if (strlen(fmc->subSys[n].name) > 0) /* Only append non-empty subsystem name */
    strcat(strcat_lower(archiveFname[0], fmc->subSys[n].name), "/");

  strcat_upper(archiveFname[0], fmc->system);
  if (strlen(fmc->subSys[n].name) > 0)
    strcat_upper(strcat(archiveFname[0], "_"), fmc->subSys[n].name);
  if (strlen(fmc->subSys[n].archiveNameModifier) > 0) {
    strcat_upper(strcat(archiveFname[0], "_"),
                 fmc->subSys[n].archiveNameModifier);
  }

  {
    char buf[128];
    unsigned long t = 0;

    // If GPS time is supplied by the front-end, use that
    if (gps > 0) {
	t = gps;
    } else {
      // See if we have /proc/gps time and read the GPS time from it
      FILE *f = fopen("/proc/gps", "r");
      if (f) {
        if (fgets(buf, 128, f)) {
          t = atol(buf);
        }
        fclose(f);
      }
    }

    // Failed to get the time from /proc/gps, so use system time
    if (t <= 0) {
      long a = time(0);
      struct tm t;

      localtime_r(&a, &t);
      sprintf(buf, "%02d%02d%02d_%02d%02d%02d", 
	    (t.tm_year - 100), (t.tm_mon + 1), t.tm_mday,
	    t.tm_hour, t.tm_min, t.tm_sec);
    } else {
      sprintf(buf, "%ld", t);
    }
    strcat(strcat(archiveFname[0], "_"), buf);
  }

  /* Replace spaces with underscores */
  for (p = archiveFname[0]; (p = strchr(p, ' ')); *p++ = '_');

  strcpy(archiveFname[1], archiveFname[0]);

  strcat(archiveFname[0], ".txt");
  printf("Archive %s\n", archiveFname[0]);

  strcat(archiveFname[1], ".fir");
  printf("FIR archive %s\n", archiveFname[1]);

  /* See if input file exists, not empty */
  /* Check whether archive file exists, can be created */
  {
    struct stat buf;

    if (stat(fname[0], &buf) < 0) {
      sprintf(fmErrMsgTxt, "Can't stat input file `%s'\n", fname[0]);
      strncpy(fmShortErrMsgTxt, "Can't stat input .txt file", 39);
      return FM_CANNOT_STAT_INPUT_FILE;
    }

    iFileLen[0] = buf.st_size;

    if (buf.st_size == 0) {
      sprintf(fmErrMsgTxt, "Empty input file `%s'\n", fname[0]);
      strncpy(fmShortErrMsgTxt, "Empty input .txt file", 39);
      return FM_EMPTY_INPUT_FILE;
    }

    if (stat(archiveFname[0], &buf) == 0) {
#if 0
      sprintf(fmErrMsgTxt, "Archive file `%s' exists\n", archiveFname[0]);
      strncpy(fmShortErrMsgTxt, "Archive .txt file exists", 39);
      return FM_ARCHIVE_FILE_EXISTS;
#else
      printf("System time is not set? Archive file `%s' exists\n",
             archiveFname[0]);
#endif
    }

    /* Repeat for FIR files */
    if (stat(fname[1], &buf) == 0) {
      iFileLen[1] = buf.st_size;

      if (iFileLen[1] > 0) {
        FIRFileExists = 1;

        if (stat(archiveFname[1], &buf) == 0) {
          printf("FIR archive file '%s' exists\n", archiveFname[1]);
        } else {
	  printf("Couldn't find FIR archive file %s\n", archiveFname[1]);
	}
      }
    }

  }

  /* Open input (.txt) file for reading */
  iFile[0] = fopen(fname[0], "r");

  if (!iFile[0]) {
    sprintf(fmErrMsgTxt, "Can't open for reading input file `%s'\n", fname[0]);
    strncpy(fmShortErrMsgTxt, "Can't open input .txt file for reading", 39);
    return FM_CANNOT_OPEN_ARCHIVE_FILE;
  }

  /* Open archive (.txt) file for writing */
  aFile[0] = fopen(archiveFname[0], "w");

  if (!aFile[0]) {
    fclose(iFile[0]);
    sprintf(fmErrMsgTxt, "Can't open for writing archive file `%s'\n",
            archiveFname[0]);
    strncpy(fmShortErrMsgTxt, "Can't open archive .txt file for writing", 39);
    return FM_CANNOT_OPEN_ARCHIVE_FILE;
  }

  /* Repeat for FIR file - if it exists */
  if (FIRFileExists) {
    iFile[1] = fopen(fname[1], "r");

    if (!iFile[1]) {
      FIRFileExists = 0;
    }
    else {
      aFile[1] = fopen(archiveFname[1], "w");

      if (!aFile[1]) {
        fclose(iFile[1]);
        FIRFileExists = 0;
      }
    }
  }

  if (FIRFileExists) {
    printf("FIR file found - opened\n");
  }
  else {
    printf("FIR file not found or file open failed\n");
  }

  /* Read input file line by line, store lines in archive file */
  /* Record new coefficient data in memory */
  {
    const unsigned int lineSize = 1024; /* Maximum length of input file line */
    unsigned int lineNo;         /* Line number counter */
    int inFilter = 0;            /* Set when reading continuation lines of multi-sos filter */
    fmSubSysMap * curFilter = 0; /* Set to currently read filter bank */
    int curFilterNum = 0;        /* Set to currently read filter number in the current bank */

#ifdef SOLARIS
    char lineBuf[1024];          /* Lines are put in here when they are read */
#else
    char lineBuf[lineSize];      /* Lines are put in here when they are read */
#endif

    int nFIRFilters = 0;         /* FIR filter count */

    int index1;
    int indexN;

    /* Repeat twice if FIR input file exists */
    for (inFileCount = 0; inFileCount < (FIRFileExists + 1); inFileCount++) {
      for (lineNo = 1; fgets(lineBuf, lineSize, iFile[inFileCount]); lineNo++) {
        /* Checksum the line */
        crc[inFileCount] = crc_ptr(lineBuf, strlen(lineBuf), crc[inFileCount]);
        /* Write line into archive file */
        if (fputs(lineBuf, aFile[inFileCount]) == EOF) {
          fatal(inFileCount);
          if (inFileCount != FIRFileExists) {
            fatal(1);
          }
          sprintf(fmErrMsgTxt, "Can't write archive file `%s'\n",
                  archiveFname[inFileCount]);
          sprintf(fmShortErrMsgTxt, "Can't write archive %s file",
                  fileExt[inFileCount]);
          return FM_CANNOT_WRITE_ARCHIVE_FILE;
        }

        /* Do not process comment or empty lines */
        if (info_line(lineBuf)) {
          /* Check invalid number of tokens on the line         */
          /* There must be 12 tokens for the filter declaration */
          /* line and 4 tokens in the continuation line         */
          unsigned int nToks = lineTok(lineBuf, tokPtr);

          if (nToks < (inFilter ? 4:MAX_TOKENS)) {
            fatal(inFileCount);
            if (inFileCount != FIRFileExists) {
              fatal(1);
            }
            sprintf(fmErrMsgTxt, "Invalid line %d in `%s'\n",
                    lineNo, fname[inFileCount]);
            sprintf(fmShortErrMsgTxt, "Invalid %s line %d",
                    fileExt[inFileCount], lineNo);
            return FM_INVALID_INPUT_FILE;
          }
          else {
            /* Take care of discovering multi-line SOS inputs */
            if (inFilter) {
              inFilter--; /* Decrement the number of continuation lines required */
              if (curFilter) {
                double filtCoeff[4];

                /* If the current filter is ours */
                /* printf("Cont for %s nToks = %d; %s %s %s %s\n", curFilter->name, */
                /*        nToks, tokPtr[0], tokPtr[1], tokPtr[2], tokPtr[3]);       */

                for (i = 0; i < 4; i++) {
                  if (!doubleToken(tokPtr[i], filtCoeff + i)) {
                    fatal(inFileCount);
                    if (inFileCount != FIRFileExists) {
                      fatal(1);
                    }
                    sprintf(fmErrMsgTxt, "Invalid cont coeff on line %d in `%s'\n",
                            lineNo, fname[inFileCount]);
                    sprintf(fmShortErrMsgTxt, "Invalid %s line %d",
                            fileExt[inFileCount], lineNo);
                    return FM_INVALID_INPUT_FILE;
                  }
                }

                /* Put the data into placeholder */
                if (curFilter->fmd.filterType[curFilterNum]) {
#ifdef FIR_FILTERS
                  /* FIR filter coeffs */
                  for (j = 0; j < 4; j++) {
                    index1 = curFilter->fmd.filterType[curFilterNum] - 1;
                    indexN = curFilter->fmd.filtSections[curFilterNum];
                    indexN = 5 + 4 * (indexN - inFilter - 2) + j;

                    firFiltCoeff[index1][curFilterNum][indexN] = filtCoeff[j];
                  }
#endif
                }
                else {
                  /* IIR filter coeffs */
                  for (j = 0; j < 4; j++) {
                    indexN = curFilter->fmd.filtSections[curFilterNum];
                    indexN = 5 + 4 * (indexN - inFilter - 2) + j;

                    curFilter->fmd.filtCoeff[curFilterNum][indexN] =
                                                      filtCoeff[j];
		    if (curFilter->biquad) {
		      // Calculate biquad form
		      switch (j) {
			case 0: // a11
			  curFilter->fmd.filtCoeff[curFilterNum][indexN]
				= - filtCoeff[j] - 1.0; // a11 = - a1 - 1
			  break;
			case 1: // a12
			  curFilter->fmd.filtCoeff[curFilterNum][indexN]
				= - filtCoeff[j] - filtCoeff[j - 1] - 1.0; // a12 = - a2 - a1 - 1
			  break;
			case 2: // c1
			  curFilter->fmd.filtCoeff[curFilterNum][indexN]
				= filtCoeff[j] - filtCoeff[j - 2]; // c1 = b1 - a1
			  break;
			case 3: // c2
			  curFilter->fmd.filtCoeff[curFilterNum][indexN]
				= filtCoeff[j] - filtCoeff[j - 2] + filtCoeff[j - 1] - filtCoeff[j - 3];
			  break;
		      }
/*
		      printf("first biquad filter %d coef #%d is %f\n", curFilterNum, indexN, curFilter->fmd.filtCoeff[curFilterNum][indexN]);
*/
		    }
                  }
                }
              }
            }
            else {
              int num;   /* Filter number in the bank 0-9 */
              int sType; /* Switching type [12][1-4]*/
              int filtSections;	    
              int ramp;
              int timeout;
              char filtName[32];
              double filtCoeff[5];
              int filterType;

              int maxSOS;

              curFilter = 0;
              curFilterNum = 0;

              /* NUM */
              if (!intToken(tokPtr[1], &num)) {
                fatal(inFileCount);
                if (inFileCount != FIRFileExists) {
                  fatal(1);
                }
                sprintf(fmErrMsgTxt,
                        "Invalid filter number (field 2) on line %d in `%s'\n",
                        lineNo, fname[inFileCount]);
                sprintf(fmShortErrMsgTxt, "Bad field 2 on %s line %d",
                        fileExt[inFileCount], lineNo);
                return FM_INVALID_INPUT_FILE;
              }

              if (num < 0 || num > 9) {
                fatal(inFileCount);
                if (inFileCount != FIRFileExists) {
                  fatal(1);
                }
                sprintf(fmErrMsgTxt,
                        "Filter number (field 2) out of range on line %d in `%s'\n",
                        lineNo, fname[inFileCount]);
                sprintf(fmShortErrMsgTxt, "Out of range field 2 on %s line %d",
                        fileExt[inFileCount], lineNo);
                return FM_INVALID_INPUT_FILE;
              }

              /* STYPE */
              if (!intToken(tokPtr[2], &sType)) {
                fatal(inFileCount);
                if (inFileCount != FIRFileExists) {
                  fatal(1);
                }
                sprintf(fmErrMsgTxt,
                        "Invalid switching type number on line %d in `%s'\n",
                        lineNo, fname[inFileCount]);
                sprintf(fmShortErrMsgTxt, "Bad field 3 on %s line %d",
                        fileExt[inFileCount], lineNo);
                return FM_INVALID_INPUT_FILE;
              }

              {
                int inType = sType/10;
                int outType = sType%10;

                if ((inType != 1 && inType != 2) ||
                    (outType < 1 || outType > 4)) {
                  fatal(inFileCount);
                  if (inFileCount != FIRFileExists) {
                    fatal(1);
                  }
                  sprintf(fmErrMsgTxt,
                          "Switching type wrong on line %d in `%s'\n",
                          lineNo, fname[inFileCount]);
                  sprintf(fmShortErrMsgTxt, "Wrong field 3 on %s line %d",
                          fileExt[inFileCount], lineNo);
                  return FM_INVALID_INPUT_FILE;
                }
              }

              /* SOS # */
              if (!intToken(tokPtr[3], &filtSections)) {
                fatal(inFileCount);
                if (inFileCount != FIRFileExists) {
                  fatal(1);
                }
                sprintf(fmErrMsgTxt, "Invalid SOS number on line %d in `%s'\n",
                        lineNo, fname[inFileCount]);
                sprintf(fmShortErrMsgTxt, "Bad field 4 on %s line %d",
                        fileExt[inFileCount], lineNo);
                return FM_INVALID_INPUT_FILE;
              }

#ifdef FIR_FILTERS
              maxSOS = MAX_FIR_SO_SECTIONS;
#else
              maxSOS = MAX_SO_SECTIONS;
#endif

              if (filtSections > 0  && filtSections <= maxSOS) {
	        /* Number of continuation lines there should be in the file next */
                inFilter = filtSections - 1;

#ifdef FIR_FILTERS
                /* 0 - IIR; N - FIR */
                if (filtSections > MAX_SO_SECTIONS) {
                  /* This is FIR filter */
                  if (nFIRFilters >= MAX_FIR_MODULES) {
                    sprintf(fmErrMsgTxt,
                            "Too many FIR filters in %s file; Maximum is %d\n",
                            fileExt[inFileCount], MAX_FIR_MODULES);
                    sprintf(fmShortErrMsgTxt, "Too many FIRs in %s file",
                            fileExt[inFileCount]);
                    return FM_INVALID_INPUT_FILE;
                  }
                  ++nFIRFilters;
                  filterType = nFIRFilters;
                }
                else {
                  filterType = 0;
                }
#else
                filterType = 0;
#endif
              }
              else {
                fatal(inFileCount);
                if (inFileCount != FIRFileExists) {
                  fatal(1);
                }
                sprintf(fmErrMsgTxt,
                        "Invalid number of SOS on line %d in `%s'\n",
                        lineNo, fname[inFileCount]);
                sprintf(fmShortErrMsgTxt, "Wrong field 4 on %s line %d",
                        fileExt[inFileCount], lineNo);
                return FM_INVALID_INPUT_FILE;
              }

              /* RAMP */
              if (!intToken(tokPtr[4], &ramp)) {
                fatal(inFileCount);
                if (inFileCount != FIRFileExists) {
                  fatal(1);
                }
                sprintf(fmErrMsgTxt,
                        "Invalid ramp count number on line %d in `%s'\n",
                        lineNo, fname[inFileCount]);
                sprintf(fmShortErrMsgTxt, "Bad field 5 on %s line %d",
                        fileExt[inFileCount], lineNo);
                return FM_INVALID_INPUT_FILE;
              }
	    
              if (ramp < 0) {
                fatal(inFileCount);
                if (inFileCount != FIRFileExists) {
                  fatal(1);
                }
                sprintf(fmErrMsgTxt,
                        "Invalid negative ramp count on line %d in `%s'\n",
                        lineNo, fname[inFileCount]);
                sprintf(fmShortErrMsgTxt, "Wrong field 5 on %s line %d",
                        fileExt[inFileCount], lineNo);
                return FM_INVALID_INPUT_FILE;
              }

              /* TIMEOUT */
              if (!intToken(tokPtr[5], &timeout)) {
                fatal(inFileCount);
                if (inFileCount != FIRFileExists) {
                  fatal(1);
                }
                sprintf(fmErrMsgTxt,
                        "Invalid timeout number on line %d in `%s'\n",
                        lineNo, fname[inFileCount]);
                sprintf(fmShortErrMsgTxt, "Bad field 6 on %s line %d",
                        fileExt[inFileCount], lineNo);
                return FM_INVALID_INPUT_FILE;
              }
	    
              if (timeout < 0) {
                fatal(inFileCount);
                if (inFileCount != FIRFileExists) {
                  fatal(1);
                }
                sprintf(fmErrMsgTxt,
                        "Invalid negative timeout on line %d in `%s'\n",
                        lineNo, fname[inFileCount]);
                sprintf(fmShortErrMsgTxt, "Wrong field 6 on %s line %d",
                        fileExt[inFileCount], lineNo);
                return FM_INVALID_INPUT_FILE;
              }

              /* NAME */
              if (strlen(tokPtr[6]) > 31) {
                fatal(inFileCount);
                if (inFileCount != FIRFileExists) {
                  fatal(1);
                }
                sprintf(fmErrMsgTxt,
                        "Filter name too long on line %d in `%s'\n",
                        lineNo, fname[inFileCount]);
                sprintf(fmShortErrMsgTxt, "Bad field 7 on %s line %d",
                        fileExt[inFileCount], lineNo);
                return FM_INVALID_INPUT_FILE;
              }

              strcpy(filtName, tokPtr[6]);

              /* COEFFS */
              for (i = 0; i < 5; i++) {
                if (!doubleToken(tokPtr[7 + i], filtCoeff + i)) {
                  fatal(inFileCount);
                  if (inFileCount != FIRFileExists) {
                    fatal(1);
                  }
                  sprintf(fmErrMsgTxt, "Invalid coeff on line %d in `%s'\n",
                          lineNo, fname[inFileCount]);
                  sprintf(fmShortErrMsgTxt, "Bad coeffs on %s line %d",
                          fileExt[inFileCount], lineNo);
                  return FM_INVALID_INPUT_FILE;
                }
              }
	    
              /* Put the data into placeholder if this is my filter */
              for (i = 0; i < fmc->subSys[n].numMap; i++) {
                if (!strcmp(fmc->subSys[n].map[i].name, tokPtr[0])) {
                  /* Found one of my filters */

                  curFilter = fmc->subSys[n].map + i;
                  curFilterNum = num;
		
                  /* printf("nToks = %d; %s %s %s %s\n", nToks,          */
                  /*        tokPtr[0], tokPtr[1], tokPtr[2], tokPtr[3]); */

                  curFilter->filters++;
                  curFilter->fmd.bankNum = curFilter->fmModNum;
                  curFilter->fmd.filtSections[num] = filtSections;
                  curFilter->fmd.sType[num] = sType;
                  curFilter->fmd.ramp[num] = ramp;
                  curFilter->fmd.timout[num] = timeout;
                  strcpy(curFilter->fmd.filtName[num], filtName);
                  curFilter->fmd.filterType[num] = filterType;

                  if (filterType > 0) {
#ifdef FIR_FILTERS
                    for (j = 0; j < 5; j++)
                      firFiltCoeff[filterType - 1][num][j] = filtCoeff[j];
#endif
                  }
                  else {
                    for (j = 0; j < 5; j++) {
                      curFilter->fmd.filtCoeff[num][j] = filtCoeff[j];

		      if (curFilter->biquad) {
		        // Calculate biquad form
		        switch (j) {
			  case 1: // a11
			    curFilter->fmd.filtCoeff[num][j]
				 = - filtCoeff[j] - 1.0; // a11 = - a1 - 1
			    break;
			  case 2: // a12
			    curFilter->fmd.filtCoeff[num][j]
				= - filtCoeff[j] - filtCoeff[j - 1] - 1.0; // a12 = - a2 - a1 - 1
			    break;
			  case 3: // c1
			    curFilter->fmd.filtCoeff[num][j]
				= filtCoeff[j] - filtCoeff[j - 2]; // c1 = b1 - a1
			    break;
			  case 4: // c2
			    curFilter->fmd.filtCoeff[num][j]
				= filtCoeff[j] - filtCoeff[j - 2] + filtCoeff[j - 1] - filtCoeff[j - 3];
			    break;
			}
/*
			printf("second biquad filter %d coef #%d is %f\n", num, j, curFilter->fmd.filtCoeff[num][j]);
*/
		      }
		    }
                  }
                  break;
                }
              }
            }
          }
        }
      }

      if (!inFileCount) {
        crc[1] = crc[0];
      }

    }
  }

#if 0
  /* File does not have to define all of the filters */
  /* Verify completeness */
  for (i = 0; i < fmc->subSys[n].numMap; i++) {
    if (fmc->subSys[n].map[i].filters == 0) {
      fatal(0);
      fatal(1);
      sprintf(fmErrMsgTxt, "Incomplete config in `%s'\n", fname[0]);
      return FM_INVALID_INPUT_FILE;
    }
  }  
#endif

  if (fclose(iFile[0])) {
    fclose(aFile[0]);
    sprintf(fmErrMsgTxt, "Failed to fclose input file `%s'\n", fname[0]);
    strcpy(fmShortErrMsgTxt, "Failed to fclose input .txt file");
    if (FIRFileExists) {
      fclose(iFile[1]);
      fclose(aFile[1]);
    }
    return FM_FCLOSE_FAILED;
  }

  if (fclose(aFile[0])) {
    sprintf(fmErrMsgTxt, "Failed to write (fclose) archive file `%s'\n",
            archiveFname[0]);
    strcpy(fmShortErrMsgTxt, "Failed to write (fclose) .txt archfile");
    if (FIRFileExists) {
      fclose(iFile[1]);
      fclose(aFile[0]);
    }
    return FM_FCLOSE_FAILED;
  }

  if (FIRFileExists) {
    if (fclose(iFile[1])) {
      fclose(aFile[1]);
      sprintf(fmErrMsgTxt, "Failed to fclose input file '%s'\n", fname[1]);
      strcpy(fmShortErrMsgTxt, "Failed to fclose input .fir file");
      return FM_FCLOSE_FAILED;
    }

    if (fclose(aFile[1])) {
      sprintf(fmErrMsgTxt, "Failed to write (fclose) archive file '%s'\n",
              archiveFname[1]);
      strcpy(fmShortErrMsgTxt, "Failed to write (fclose) .fir archfile");
      return FM_FCLOSE_FAILED;
    }
  }

/* ###  DEBUG  ### */
/*  fprintf(stderr, "\n###  New crc[%d] (pre) = %u\n\n", 0, crc[0]); */
/*  fprintf(stderr, "\n###  New crc[%d] (pre) = %u\n\n", 1, crc[1]); */
/*  fprintf(stderr, "\n###  Old crc = %u\n", fmc->subSys[n].crc);    */
/* ###  DEBUG  ### */
  if (FIRFileExists) {
    totalLength = iFileLen[0] + iFileLen[1];

    crc[1] = crc_len(totalLength, crc[1]);  /* Finish calculating input file checksum */
/* ###  DEBUG  ### */
/*    fprintf(stderr, "\n###  New crc[%d] (post) = %u\n\n", 1, crc[1]); */
/* ###  DEBUG  ### */

    if (crc[1] != fmc->subSys[n].crc) {
      fmc->subSys[n].crc = crc[1];
    }
    else {
      unlink(archiveFname[0]); /* Delete .txt archive file as it did not change */
      unlink(archiveFname[1]); /* Delete .fir archive file as it did not change */
    }
  }
  else {
    crc[0] = crc_len(iFileLen[0], crc[0]); /* Finish calculating input file checksum */
/* ###  DEBUG  ### */
/*    fprintf(stderr, "\n###  New crc[%d] (post) = %u\n\n", 0, crc[0]); */
/* ###  DEBUG  ### */

    if (crc[0] != fmc->subSys[n].crc) {
      fmc->subSys[n].crc = crc[0];
    }
    else {
      unlink(archiveFname[0]); /* Delete archive file as it did not change */
    }
  }

  /* Update VME coeffs for the subsystem */
  /* Correct byte order on doubles for Pentium CPU */
  for (i = 0; i < fmc->subSys[n].numMap; i++) {
    unsigned int coefCrc = 0;
    unsigned int coefCrcLen = 0;
    for (j = 0; j < FILTERS; j++) {
      unsigned int nSections = fmc->subSys[n].map[i].fmd.filtSections[j]; /* The number of coeffs defined in this filter */
      unsigned int nCoefs = 1 + nSections * 4;

      coefCrc = crc_ptr((char *)&nSections, sizeof(int), coefCrc);
      coefCrcLen += sizeof(int);
      if (nSections > 0) {
        coefCrc = crc_ptr((char *)&(fmc->subSys[n].map[i].fmd.sType[j]),
                           sizeof(int), coefCrc);
	coefCrc = crc_ptr((char *)&(fmc->subSys[n].map[i].fmd.ramp[j]),
                           sizeof(int), coefCrc);
	coefCrc = crc_ptr((char *)&(fmc->subSys[n].map[i].fmd.timout[j]),
                           sizeof(int), coefCrc);
        coefCrcLen += 3 * sizeof(int);
      }

      unsigned int filterType = fmc->subSys[n].map[i].fmd.filterType[j];
      fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].filterType[j] =
                                                                filterType;
      //printf("filter module %d; filter %d; type %d\n",
      //       fmc->subSys[n].map[i].fmModNum, j, filterType);
      //printf("nCoefs = %d; nSections = %d\n", nCoefs, nSections);
#ifdef FIR_FILTERS
      if (filterType) {
        for (k = 0; k < nCoefs; k++) {
#if defined(__i386__)  || defined(__amd64__)
	 //printf("fmc->pVmeCoeff->firFiltCoeff[%d][%d][%d] = firFiltCoeff[%d][%d][%d];\n",
         //       (filterType - 1), j, k, (filterType - 1), j, k);
	  fmc->pVmeCoeff->firFiltCoeff[filterType - 1][j][k] =
                          firFiltCoeff[filterType - 1][j][k];
#else
#error
#endif
	  if (nSections > 0) {
            coefCrc = crc_ptr((char *)&(firFiltCoeff[filterType - 1][j][k]),
                               sizeof(double), coefCrc);
	    coefCrcLen += sizeof(double);
	  }
        }
      } else
#endif
      for (k = 0; k < nCoefs; k++) {
#if defined(__i386__)  || defined(__amd64__)
	fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].filtCoeff[j][k] =
                                        fmc->subSys[n].map[i].fmd.filtCoeff[j][k];
#else
#error
	memcpy_swap_words(fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].filtCoeff[j] + k,
			  fmc->subSys[n].map[i].fmd.filtCoeff[j] + k);
#endif
	if (nSections > 0) {
          coefCrc = crc_ptr((char *)&(fmc->subSys[n].map[i].fmd.filtCoeff[j][k]),
                             sizeof(double), coefCrc);
	  coefCrcLen += sizeof(double);
	}
      }
      strcpy(fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].filtName[j],
                                             fmc->subSys[n].map[i].fmd.filtName[j]);
      fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].filtSections[j] =
                                                                   nSections;
      fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].sType[j] =
                                      fmc->subSys[n].map[i].fmd.sType[j];
      fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].ramp[j] =
                                      fmc->subSys[n].map[i].fmd.ramp[j];
      fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].timout[j] =
                                      fmc->subSys[n].map[i].fmd.timout[j];
    }

    fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].crc = coefCrc;
    fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].biquad = 
	    			fmc->subSys[n].map[i].biquad;
    /*printf("filt %d BIQUAD FLAG = %d\n", i,fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].biquad);*/

    usleep(10000);
    fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].bankNum =
                                    fmc->subSys[n].map[i].fmd.bankNum;
  }

  return 0;
}

int fmCreatePartial(char *cfDir, char *cfName, char *filtName)
{
  char searchPattern[64] = "### ";
  char stopPattern[10] = "########";
  char fName[NUM_FMC_FILE_TYPES][256];
  struct stat buf;
  int lineNo;
  int lineSize = 1024;
  char *line = NULL;
  FILE *fp;
  FILE *fptmp;
  size_t len;
  ssize_t read;
  int fstart = 0;
  int fstop = 0;
  char newcoeffs[50000] = "";
  int foundFilt = 0;
  char cpCmd[256];
  int status;
  char *words[10];
  int nwords;
  char myline[100];

  strcpy(fName[FMC_PHOTON],cfDir);
  strcpy(fName[FMC_LOAD],cfDir);
  strcat(fName[FMC_LOAD],"tmp/");
  strcpy(fName[FMC_TMP],fName[FMC_LOAD]);
  strcat(fName[FMC_PHOTON],cfName);
  strcat(fName[FMC_LOAD],cfName);
  strcat(fName[FMC_TMP],cfName);
  strcpy(fName[FIR_PHOTON],fName[0]);
  strcpy(fName[FIR_TMP],fName[1]);

  strcat(fName[FMC_PHOTON],".txt");
  strcat(fName[FMC_LOAD],".txt");
  strcat(fName[FMC_TMP],".tmp");
  strcat(fName[FIR_LOAD],".fir");
  strcat(fName[FIR_TMP],".fir");

  strcat(searchPattern,filtName);

  printf("In create partial: \n\t%s\n\t%s\n\t%s\n\t%s\n",fName[FMC_PHOTON],fName[FMC_LOAD],fName[FIR_PHOTON],fName[FIR_LOAD]);

  fp = fopen(fName[FMC_PHOTON],"r");
  if(fp == NULL) {
  	printf("Cannot open file %s\n",fName[FMC_PHOTON]);
        return FM_CANNOT_STAT_INPUT_FILE;
  }

  // Read the coeff file and pull out lines for single filter module
  while((read = getline(&line, &len, fp)) != -1) {
  	// printf("%s\n",line);
  	if(fstart && fstop < 2) {
		strcat(newcoeffs,line);
	}
	if(strstr(line,searchPattern) != NULL && fstart == 0 ) {
		strcpy(myline,line);
		nwords = getwords(myline,words,10);
		if(strcmp(filtName,words[1]) == 0) {
			fstart = 1;
			foundFilt = 1;
			strcat(newcoeffs,line);
			printf("Found the filter %s  %s\n",filtName,line);
			printf("Found %d words = %s %s\n",nwords,words[0],words[1]);
		}
	}
	if(fstart > 0 && strstr(line,stopPattern) != NULL) {
		fstop += 1;
	}
	if(fstop == 2) {
		fstart = 0;
	}
  }
  fclose(fp);

  fstart = 0;
  fstop = 0;

  fp = fopen(fName[FMC_LOAD],"r");
  if(fp == NULL) {
  	printf("Cannot open file %s\n",fName[FMC_LOAD]);
        return FM_CANNOT_STAT_INPUT_FILE;
  }

  fptmp = fopen(fName[FMC_TMP],"w");
  if(fptmp == NULL) {
  	printf("Cannot open file %s\n",fName[FMC_TMP]);
	fclose(fp);
        return FM_CANNOT_STAT_INPUT_FILE;
  }
  
  printf("Creating new file from %s to %s\n",fName[FMC_LOAD],fName[FMC_TMP]);
  while((read = getline(&line, &len, fp)) != -1) {
	if(strstr(line,searchPattern) == NULL && fstart == 0) {
		// No change in this line, so just write it to tmp file
		fprintf(fptmp,"%s",line);
	}
	if(strstr(line,searchPattern) != NULL && fstart == 0) {
		strcpy(myline,line);
		nwords = getwords(myline,words,10);
		if(strcmp(filtName,words[1]) == 0) {
			fstart = 1;
			foundFilt = 1;
			printf("Found the filter %s\n",filtName);
			// Write all of the new stuff extracted from the Photon file
			fprintf(fptmp,"%s",newcoeffs);
		} else {
			fprintf(fptmp,"%s",line);
		}
	}
	if(fstart > 0 && strstr(line,stopPattern) != NULL) {
		fstop += 1;
	}
	if(fstop == 2) {
		// We are done adding new stuff
		fstart = 0;
	}
  }
  fclose(fp);
  fclose(fptmp);

  if(line) free(line);

  // Copy the newly formed tmp file to the file to be loaded by fmReadCoeff()
  sprintf(cpCmd,"%s %s %s","cp",fName[FMC_TMP],fName[FMC_LOAD]);
  status = system(cpCmd);

  if(foundFilt) {
  	printf("Found the filter %s\n",filtName);
	printf("%s\n",newcoeffs);
	printf("copy status = %d\n",status);
  }

  int mychksum = checkFileCrc(fName[FMC_LOAD]);
  int mychksumP = checkFileCrc(fName[FMC_PHOTON]);
  printf("My load file CRC = %d \n",mychksum);
  if(mychksum == mychksumP) printf ("Photon and Load Files Match \n");
  else printf ("Photon and Load Files DO NOT Match \n");



  return(0);

}
/// Test routine which will print out filter coefs to screen.
///	@param[in] *fmc		Pointer to filter coef data.
///	@param[in] subsystems	Number of subsystems
void
printCoefs(fmReadCoeff *fmc, int subsystems) {
  int i, j, k, l;
  for (i = 0; i < subsystems; i++){ 
    printf ("%s has %d elements\n", fmc->subSys[i].name, fmc->subSys[i].numMap);
    for (j = 0; j < fmc->subSys[i].numMap; j++) {
      fmSubSysMap *f = fmc->subSys[i].map + j;
      printf ("\t%s -> %d has %d filters\n", f->name, f->fmModNum, f->filters);
      for (k = 0; k < FILTERS; k++) {
	if (f->fmd.filtSections[k]) {
	  printf("%d %d %d %d %d %s %e %e %e %e %e\n",
		 k,
		 f->fmd.sType[k],
		 f->fmd.filtSections[k],
		 f->fmd.ramp[k],
		 f->fmd.timout[k],
		 f->fmd.filtName[k],
		 f->fmd.filtCoeff[k][0],
		 f->fmd.filtCoeff[k][1],
		 f->fmd.filtCoeff[k][2],
		 f->fmd.filtCoeff[k][3],
		 f->fmd.filtCoeff[k][4]);
	  for (l = 0; l < f->fmd.filtSections[k] - 1; l++) {
	    printf("\t%e %e %e %e\n",
		   f->fmd.filtCoeff[k][5 + 4 * l + 0],
		   f->fmd.filtCoeff[k][5 + 4 * l + 1],
		   f->fmd.filtCoeff[k][5 + 4 * l + 2],
		   f->fmd.filtCoeff[k][5 + 4 * l + 3]);
	  }
	}
      }
    }
  }
}

int getwords(char *line, char *words[], int maxwords)
{
char *p = line;
int nwords = 0;

while(1)
	{
	while(isspace(*p))
		p++;

	if(*p == '\0')
		return nwords;

	words[nwords++] = p;

	while(!isspace(*p) && *p != '\0')
		p++;

	if(*p == '\0')
		return nwords;

	*p++ = '\0';

	if(nwords >= maxwords)
		return nwords;
	}
}

#ifdef unix_test
fmSubSysMap itmxMap[5] = { { "ULSEN", 0 }, { "LLSEN", 1 }, { "URSEN", 2 }, { "LRSEN", 3 }, { "SDSEN", 4 } };
fmSubSysMap itmyMap[5] = { { "ULSEN", 5 }, { "LLSEN", 6 }, { "URSEN", 7 }, { "LRSEN", 8 }, { "SDSEN", 9 } };
fmSubSysMap   rmMap[5] = { { "ULSEN", 10}, { "LLSEN", 11}, { "URSEN", 12}, { "LRSEN", 13}, { "SDSEN", 14} };
fmSubSysMap   bsMap[5] = { { "ULSEN", 15}, { "LLSEN", 16}, { "URSEN", 17}, { "LRSEN", 18}, { "SDSEN", 19} };

fmSubSysMap   mmt3Map[26] = {
  {"ULSEN", 20}, {"LLSEN", 21}, {"URSEN", 22}, {"LRSEN", 23}, {"SDSEN", 24},

  {"SUSPOS", 25}, {"SUSPIT", 26}, {"SUSYAW", 27},

  {"ULPOS", 28}, {"ULPIT", 29}, {"ULYAW", 30},
  {"LLPOS", 31}, {"LLPIT", 32}, {"LLYAW", 33},
  {"URPOS", 34}, {"URPIT", 35}, {"URYAW", 36},
  {"LRPOS", 37}, {"LRPIT", 38}, {"LRYAW", 39},

  {"ULCOIL", 40}, {"LLCOIL", 41}, {"URCOIL", 42}, {"LRCOIL", 43},

  {"OPLEVPIT", 44}, {"OPLEVYAW", 45}
};


VME_COEF vme_win;

fmReadCoeff fmc = {
  "test",  /* Site */
  "h1",    /* Ifo  */
  "sus",   /* System */
  &vme_win,
  /* Subsystems */
  {
    {"itmx", "input", 5, itmxMap},
    {"itmy", "input", 5, itmyMap},
    {"rm",   "input", 5,   rmMap},
    {"bs",   "input", 5,   bsMap},
    {"mmt3", "",     26, mmt3Map},
  }
};


int main ()
{
  int i, j, k, l;

  for (i = 0; i < FM_SUBSYS_NUM; i++) {
    if (fmReadCoeffFile(&fmc, i) != 0) {
      fprintf(stderr, "Error: %s\n", fmReadErrMsg());
    }
  }

  for (i = 0; i < MAX_MODULES; i++) {
    for (j = 0; j < FILTERS; j++) {
      if (vme_win.vmeCoeffs[i].filterType[j] > 0) {
	unsigned int fnum = vme_win.vmeCoeffs[i].filterType[j] - 1;
	printf("Filter module %d; bank %d is FIR\n", i, j);
	unsigned int nSect = vme_win.vmeCoeffs[i].filtSections[j];
	unsigned int nCoef = nSect * 4 + 1;
	printf("Sections = %d; nCoef = %d\n", nSect, nCoef);
	printf("Gain = %f\n", vme_win.firFiltCoeff[fnum][j][0]);
	for (k = 1; k < nCoef; k += 4) {
	  printf("%f %f %f %f\n", vme_win.firFiltCoeff[fnum][j][k],
				  vme_win.firFiltCoeff[fnum][j][k + 1],
				  vme_win.firFiltCoeff[fnum][j][k + 2],
				  vme_win.firFiltCoeff[fnum][j][k + 3]);
	}
      }
    }
  }
  return 0;
}
#endif
