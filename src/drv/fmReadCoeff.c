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

char *readSusCoeff_c_cvsid = "$Id: fmReadCoeff.c,v 1.1 2005/10/31 14:30:24 rolf Exp $";

#ifdef unix_test
/* The number of subsystems (optics) */
#define FM_SUBSYS_NUM  5
#define MAX_MODULES    100
#else
#ifdef SOLARIS
#define MAX_MODULES    1
#else
#define MAX_MODULES    0
#endif
#define FM_SUBSYS_NUM  1
#endif

#define NO_FM10GEN_C_CODE	1
#include "fm10Gen.h"
#include "fmReadCoeff.h"

/* Cat string and make upper case */
static char *strcat_upper(char *dest, char *src) {
  char *d=dest;
  for(;*d;d++);
  for(;(*d++ = toupper(*src));src++);
  return dest;
}

/* Cat string and make lower case */
static char *strcat_lower(char *dest, char *src) {
  char *d=dest;
  for(;*d;d++);
  for(;(*d++ = tolower(*src));src++);
  return dest;
}

/* Determine whether string is not a comment or white space */
static int info_line(char *c) {
  /* Skip white spaces in front */
  for(;isspace(*c);c++);
  return *c && *c != '#'; /* Not end of line or start of comment */
}

/* Max number of tokens allowed on a line */
#define MAX_TOKENS 12

/* Tokenize line */
static int lineTok(char *s, char *tokPtr[]) {
  int ntoks;
  for (ntoks = 0;
       ntoks < MAX_TOKENS && 
       (tokPtr[ntoks] = strtok_r(0, " \t\n", &s));
       ntoks++);

  return ntoks;
}

/* Convert string to number */
static int intToken(char *str, int *res) {
  char *ptr;

  *res = strtol (str, &ptr, 10);
  return *ptr == 0;
}

/* Convert string to number */
static int doubleToken(char *str, double *res) {
  char *ptr;
  *res = strtod (str, &ptr);
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


/* Read system 'fmc' coeffs for subsys 'n' */
int
fmReadCoeffFile(fmReadCoeff *fmc, int n)
{
  int i, j, k;
  char fname[256];         /* Input coeff file name */
  char archive_fname[256]; /* Archive file name */
  FILE *iFile;             /* Input file */
  FILE *aFile;             /* Archive file */
  char *p;                 /* Temporary variable */
  unsigned int iFile_len = 0; /* Input file length in bytes */
  unsigned long crc = 0;      /* Input file checksum */
  /* Pointers to tokens assigned by lineTok() */
  char *tokPtr[MAX_TOKENS];


  /* Close files, delete archive file */
#define fatal() \
  fclose(aFile); \
  fclose(iFile); \
  unlink(archive_fname)

  /* Clear filter coeff placeholder */
  for (i = 0; i < fmc->subSys[n].numMap; i++) {
    fmc->subSys[n].map[i].filters = 0;
    memset(&fmc->subSys[n].map[i].fmd, 0, sizeof(fmc->subSys[n].map[i].fmd));
  }

  /* Contruct filenames */
  strcat(strcat_lower(strcpy(fname, "/cvs/cds/"), fmc->site), "/chans/");
  strcpy(archive_fname, fname);

  strcat_upper(strcat_upper(fname, fmc->ifo), fmc->system);
  if (strlen(fmc->subSys[n].name) > 0) /* Only append non-empty subsystem name */
    strcat_upper(strcat(fname, "_"), fmc->subSys[n].name);
  strcat(fname,".txt");
  printf("Input %s\n",fname);

  strcat(archive_fname, "filter_archive/");
  strcat(strcat_lower(archive_fname, fmc->ifo), "/");
  strcat(strcat_lower(archive_fname, fmc->system), "/");
  if (strlen(fmc->subSys[n].name) > 0) /* Only append non-empty subsystem name */
    strcat(strcat_lower(archive_fname, fmc->subSys[n].name), "/");
  strcat_upper(strcat_upper(archive_fname, fmc->ifo), fmc->system);
  if (strlen(fmc->subSys[n].name) > 0)
    strcat_upper(strcat(archive_fname, "_"), fmc->subSys[n].name);
  if (strlen(fmc->subSys[n].archiveNameModifier) > 0) {
    strcat_upper(strcat(archive_fname, "_"), fmc->subSys[n].archiveNameModifier);
  }
  {
#if 1
    long a = time(0);
    struct tm t;
    char buf[100];
    localtime_r(&a, &t);
    sprintf(buf, "%02d%02d%02d_%02d%02d%02d", 
	    t.tm_year-100, t.tm_mon+1, t.tm_mday,
	    t.tm_hour, t.tm_min, t.tm_sec);
#else
#ifdef UNIX
    char buf[100];
    long t = time(0);
    ctime_r(&t, buf);
#else
    size_t s = 27;
    char buf[s];
    long t = time(0);
    ctime_r(&t, buf, &s);
#endif
    buf[24]=0;
#endif

    strcat(strcat(strcat(archive_fname,"_"), buf), ".txt");
  }
  for(p=archive_fname;(p=strchr(p,' '));*p++ = '_'); /* Replace spaces with underscores */
  printf("Archive %s\n",archive_fname);

  /* See if input file exists, not empty */
  /* Check whether archive file exists, can be created */
  {
    struct stat buf;
    if (stat(fname, &buf) < 0) {
      sprintf(fmErrMsgTxt, "Can't stat input file `%s'\n", fname);
      strncpy(fmShortErrMsgTxt, "Can't stat input file", 39);
      return FM_CANNOT_STAT_INPUT_FILE;
    }
    iFile_len = buf.st_size;
    if (buf.st_size == 0) {
      sprintf(fmErrMsgTxt, "Empty input file `%s'\n", fname);
      strncpy(fmShortErrMsgTxt, "Empty input file", 39);
      return FM_EMPTY_INPUT_FILE;
    }

    if (stat(archive_fname, &buf) == 0) {
#if 0
      sprintf(fmErrMsgTxt, "Archive file `%s' exists\n", archive_fname);
      strncpy(fmShortErrMsgTxt, "Archive file exists", 39);
      return FM_ARCHIVE_FILE_EXISTS;
#else
      printf("System time is not set? Archive file `%s' exists\n", archive_fname);
#endif
    }

  }

  /* Open input file for reading */
  iFile = fopen(fname, "r");
  if (!iFile) {
    sprintf(fmErrMsgTxt, "Can't open for reading input file `%s'\n", fname);
    strncpy(fmShortErrMsgTxt, "Can't open input file for reading", 39);
    return FM_CANNOT_OPEN_ARCHIVE_FILE;
  }

  /* Open archive file for writing */
  aFile = fopen(archive_fname, "w");
  if (!aFile) {
    fclose(iFile);
    sprintf(fmErrMsgTxt, "Can't open for writing archive file `%s'\n", archive_fname);
    strncpy(fmShortErrMsgTxt, "Can't open archive file for reading", 39);
    return FM_CANNOT_OPEN_ARCHIVE_FILE;
  }

  /* Read input file line by line, store lines in archive file */
  /* Record new coefficient data in memory */
  {
    const unsigned int linesize = 1024; /* Maximum length of input file line */
    unsigned int lineno; /* Line number counter */
    int inFilter = 0;            /* Set when reading continuation lines of multi-sos filter */
    fmSubSysMap * curFilter = 0; /* Set to currently read filter bank */
    int curFilterNum = 0;        /* Set to currently read filter number in the current bank */
#ifdef SOLARIS
    char linebuf[1024];      /* Lines are put in here when they are read */
#else
    char linebuf[linesize];      /* Lines are put in here when they are read */
#endif

    for (lineno = 1; fgets(linebuf, linesize, iFile); lineno++) {
      /* Checksum the line */
      crc = crc_ptr(linebuf, strlen(linebuf), crc);
      /* Write line into archive file */
      if (fputs(linebuf, aFile) == EOF) {
	fatal();
	sprintf(fmErrMsgTxt, "Can't write archive file `%s'\n", archive_fname);
	strncpy(fmShortErrMsgTxt, "Can't write archive file", 39);
	return FM_CANNOT_WRITE_ARCHIVE_FILE;
      }

      /* Do not process comment or empty lines */
      if (info_line(linebuf)) {
	/* Check invalid number of tokens on the line */
	/* There must be 12 tokens for the filter declaration line
	   and 4 tokens in the continuation line */
	unsigned int ntoks = lineTok(linebuf, tokPtr);
	if (ntoks < (inFilter? 4:MAX_TOKENS)) {
	  fatal();
	  sprintf(fmErrMsgTxt, "Invalid line %d in `%s'\n", lineno, fname);
	  sprintf(fmShortErrMsgTxt, "Invalid line %d", lineno);
	  return FM_INVALID_INPUT_FILE;
	} else {
	  /* Take care of discovering multi-line SOS inputs */
	  if (inFilter) {
	    inFilter--; /* Decrement the number of continuation lines required */
	    if (curFilter) {
	      double filtCoeff[4];

	      /* If the current filter is ours */
	      /* printf ("Cont for %s ntoks = %d; %s %s %s %s\n", curFilter->name, ntoks, tokPtr[0], tokPtr[1], tokPtr[2], tokPtr[3]); */

	      for (i = 0; i < 4; i++) {
		if (!doubleToken(tokPtr[i], filtCoeff + i)) {
		  fatal();
		  sprintf(fmErrMsgTxt, "Invalid cont coeff on line %d in `%s'\n", lineno, fname);
		  sprintf(fmShortErrMsgTxt, "Invalid line %d", lineno);
		  return FM_INVALID_INPUT_FILE;
		}
	      }

	      /* Put the data into placeholder */
	      for (j = 0; j < 4; j++)
		curFilter->fmd.filtCoeff[curFilterNum][5 + 4*(curFilter->fmd.filtSections[curFilterNum] - inFilter - 2) + j]
		  = filtCoeff[j];
	    }
	  } else {
	    int num;   /* Filter number in the bank 0-9 */
	    int sType; /* Switching type [12][1-4]*/
	    int filtSections;	    
	    int ramp;
	    int timeout;
	    char filtName[32];
	    double filtCoeff[5];

	    curFilter = 0; curFilterNum = 0;

	    /* NUM */
	    if (!intToken(tokPtr[1], &num)) {
	      fatal();
	      sprintf(fmErrMsgTxt, "Invalid filter number (field 2) on line %d in `%s'\n", lineno, fname);
	      sprintf(fmShortErrMsgTxt, "Bad field 2 on line %d", lineno);
	      return FM_INVALID_INPUT_FILE;
	    }
	    if (num < 0 || num > 9) {
	      fatal();
	      sprintf(fmErrMsgTxt, "Filter number (field 2) out of range on line %d in `%s'\n", lineno, fname);
	      sprintf(fmShortErrMsgTxt, "Out of range field 2 on line %d", lineno);
	      return FM_INVALID_INPUT_FILE;
	    }

	    /* STYPE */
	    if (!intToken(tokPtr[2], &sType)) {
	      fatal();
	      sprintf(fmErrMsgTxt, "Invalid switching type number on line %d in `%s'\n", lineno, fname);
	      sprintf(fmShortErrMsgTxt, "Bad field 3 on line %d", lineno);
	      return FM_INVALID_INPUT_FILE;
	    }
	    {
	      int inType = sType/10;
	      int outType = sType%10;
	      if ((inType != 1 && inType != 2) ||
		  (outType < 1 || outType > 4)) {
		fatal();
		sprintf(fmErrMsgTxt, "Switching type wrong on line %d in `%s'\n", lineno, fname);
		sprintf(fmShortErrMsgTxt, "Wrong field 3 on line %d", lineno);
		return FM_INVALID_INPUT_FILE;
	      }
	    }

	    /* SOS # */
	    if (!intToken(tokPtr[3], &filtSections)) {
	      fatal();
	      sprintf(fmErrMsgTxt, "Invalid SOS number on line %d in `%s'\n", lineno, fname);
	      sprintf(fmShortErrMsgTxt, "Bad field 4 on line %d", lineno);
	      return FM_INVALID_INPUT_FILE;
	    }

	    if (filtSections > 0  && filtSections <= MAX_SO_SECTIONS) {
	      inFilter = filtSections-1; /* Number of continuation lines there should be in the file next */
	    } else {
	      fatal();
	      sprintf(fmErrMsgTxt, "Invalid number of SOS on line %d in `%s'\n", lineno, fname);
	      sprintf(fmShortErrMsgTxt, "Wrong field 4 on line %d", lineno);
	      return FM_INVALID_INPUT_FILE;
	    }

	    /* RAMP */
	    if (!intToken(tokPtr[4], &ramp)) {
	      fatal();
	      sprintf(fmErrMsgTxt, "Invalid ramp count number on line %d in `%s'\n", lineno, fname);
	      sprintf(fmShortErrMsgTxt, "Bad field 5 on line %d", lineno);
	      return FM_INVALID_INPUT_FILE;
	    }
	    
	    if (ramp < 0) {
	      fatal();
	      sprintf(fmErrMsgTxt, "Invalid negative ramp count on line %d in `%s'\n", lineno, fname);
	      sprintf(fmShortErrMsgTxt, "Wrong field 5 on line %d", lineno);
	      return FM_INVALID_INPUT_FILE;
	    }

	    /* TIMEOUT */
	    if (!intToken(tokPtr[5], &timeout)) {
	      fatal();
	      sprintf(fmErrMsgTxt, "Invalid timeout number on line %d in `%s'\n", lineno, fname);
	      sprintf(fmShortErrMsgTxt, "Bad field 6 on line %d", lineno);
	      return FM_INVALID_INPUT_FILE;
	    }
	    
	    if (timeout < 0) {
	      fatal();
	      sprintf(fmErrMsgTxt, "Invalid negative timeout on line %d in `%s'\n", lineno, fname);
	      sprintf(fmShortErrMsgTxt, "Wrong field 6 on line %d", lineno);
	      return FM_INVALID_INPUT_FILE;
	    }

	    /* NAME */
	    if (strlen(tokPtr[6]) > 31) {
	      fatal();
	      sprintf(fmErrMsgTxt, "Filter name too long on line %d in `%s'\n", lineno, fname);
	      sprintf(fmShortErrMsgTxt, "Bad field 7 on line %d", lineno);
	      return FM_INVALID_INPUT_FILE;
	    }
	    strcpy(filtName, tokPtr[6]);

	    /* COEFFS */
	    for (i = 0; i < 5; i++) {
	      if (!doubleToken(tokPtr[7+i], filtCoeff + i)) {
		fatal();
		sprintf(fmErrMsgTxt, "Invalid coeff on line %d in `%s'\n", lineno, fname);
		sprintf(fmShortErrMsgTxt, "Bad coeffs on line %d", lineno);
		return FM_INVALID_INPUT_FILE;
	      }
	    }
	    
	    /* Put the data into placeholder if this is my filter */
	    for (i = 0; i < fmc->subSys[n].numMap; i++) {
	      if (!strcmp(fmc->subSys[n].map[i].name, tokPtr[0])) {
		/* Found one of my filters */

		curFilter = fmc->subSys[n].map + i;
		curFilterNum = num;
		
		/*		printf ("ntoks = %d; %s %s %s %s\n", ntoks, tokPtr[0], tokPtr[1], tokPtr[2], tokPtr[3] ); */
		curFilter->filters++;
		curFilter->fmd.bankNum = curFilter->fmModNum;
		curFilter->fmd.filtSections[num] = filtSections;
		curFilter->fmd.sType[num] = sType;
		curFilter->fmd.filtSections[num] = filtSections;
		curFilter->fmd.ramp[num] = ramp;
		curFilter->fmd.timout[num] = timeout;
		strcpy(curFilter->fmd.filtName[num], filtName);
		for (j = 0; j < 5; j++)
		  curFilter->fmd.filtCoeff[num][j] = filtCoeff[j];
		break;
	      }
	    }
	  }
	}
      }
    }
  }

#if 0
  /* File does not have to define all of the filters */
  /* Verify completeness */
  for (i = 0; i < fmc->subSys[n].numMap; i++) {
    if (fmc->subSys[n].map[i].filters == 0) {
      fatal();
      sprintf(fmErrMsgTxt, "Incomplete config in`%s'\n", fname);
      return FM_INVALID_INPUT_FILE;
    }
  }  
#endif

  if (fclose(iFile)) {
    fclose(aFile);
    sprintf(fmErrMsgTxt, "Failed to fclose input file `%s'\n", fname);
    strcpy(fmShortErrMsgTxt, "Failed to fclose input file");
    return FM_FCLOSE_FAILED;
  }

  if (fclose(aFile)) {
    sprintf(fmErrMsgTxt, "Failed to write (fclose) archive file `%s'\n", archive_fname);
    strcpy(fmShortErrMsgTxt, "Failed to write (fclose) archive file");
    return FM_FCLOSE_FAILED;
  }

  crc = crc_len(iFile_len, crc); /* Finish calculating input file checksum */
  if (crc != fmc->subSys[n].crc) fmc->subSys[n].crc = crc;
  else unlink(archive_fname); /* Delete archive file as it did not change */

  /* Update VME coeffs for the subsystem */
  /* Correct byte order on doubles for Pentium CPU */
  for (i = 0; i < fmc->subSys[n].numMap; i++) {
    unsigned int coef_crc = 0;
    unsigned int coef_crc_len = 0;
    for (j = 0; j < FILTERS; j++) {
      unsigned int nsections = fmc->subSys[n].map[i].fmd.filtSections[j]; /* The number of coeffs defined in this filter */
      unsigned int ncoefs = 1 + nsections * 4;

      coef_crc = crc_ptr ((char *)&nsections, sizeof(int), coef_crc);
      coef_crc_len += sizeof(int);
      if (nsections > 0) {
        coef_crc = crc_ptr ((char *)&(fmc->subSys[n].map[i].fmd.sType[j]), sizeof(int), coef_crc);
	coef_crc = crc_ptr ((char *)&(fmc->subSys[n].map[i].fmd.ramp[j]), sizeof(int), coef_crc);
	coef_crc = crc_ptr ((char *)&(fmc->subSys[n].map[i].fmd.timout[j]), sizeof(int), coef_crc);
        coef_crc_len += 3 * sizeof(int);
      }

      for (k = 0; k < ncoefs; k++) {
#if defined(__i386__)  || defined(__amd64__)
	fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].filtCoeff[j][k] = fmc->subSys[n].map[i].fmd.filtCoeff[j][k];
#else
#error
	memcpy_swap_words(fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].filtCoeff[j] + k,
			  fmc->subSys[n].map[i].fmd.filtCoeff[j] + k);
#endif
	if (nsections > 0) {
          coef_crc = crc_ptr ((char *)&(fmc->subSys[n].map[i].fmd.filtCoeff[j][k]), sizeof(double), coef_crc);
	  coef_crc_len += sizeof(double);
	}
      }
      strcpy(fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].filtName[j],fmc->subSys[n].map[i].fmd.filtName[j]);
      fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].filtSections[j] = nsections;
      fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].sType[j] = fmc->subSys[n].map[i].fmd.sType[j];
      fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].ramp[j] = fmc->subSys[n].map[i].fmd.ramp[j];
      fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].timout[j] = fmc->subSys[n].map[i].fmd.timout[j];
    }
    fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].crc = coef_crc;
    usleep(10000);

    fmc->pVmeCoeff->vmeCoeffs[fmc->subSys[n].map[i].fmModNum].bankNum = fmc->subSys[n].map[i].fmd.bankNum;
  }
  return 0;
}

void
printCoefs(fmReadCoeff *fmc, int subsystems) {
  int i,j,k,l;
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
	  for (l = 0; l < f->fmd.filtSections[k]-1; l++) {
	    printf("\t%e %e %e %e\n",
		   f->fmd.filtCoeff[k][5 + 4*l + 0],
		   f->fmd.filtCoeff[k][5 + 4*l + 1],
		   f->fmd.filtCoeff[k][5 + 4*l + 2],
		   f->fmd.filtCoeff[k][5 + 4*l + 3]);
	  }
	}
      }
    }
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

VME_COEF vme_win[MAX_MODULES];

fmReadCoeff fmc = {
  "test", /* Site */
  "h1",  /* Ifo  */
  "sus", /* System */
  vme_win,
  /* Subsystems */
  {
    {"itmx", "input", 5, itmxMap},
    {"itmy", "input", 5, itmyMap},
    {"rm",   "input", 5,   rmMap},
    {"bs",   "input", 5,   bsMap},
    {"mmt3", "", 26,mmt3Map},
  }
};


int main ()
{
  int i,j,k,l;

  for (i = 0; i < FM_SUBSYS_NUM; i++) {
    if (fmReadCoeffFile(&fmc,i) != 0) {
      fprintf(stderr, "Error: %s\n", fmReadErrMsg());
    }
  }

  for (i = 0; i < FM_SUBSYS_NUM; i++){ 
    printf ("%s has %d elements\n", fmc.subSys[i].name, fmc.subSys[i].numMap);
    for (j = 0; j < fmc.subSys[i].numMap; j++) {
      fmSubSysMap *f = fmc.subSys[i].map + j;
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
	  for (l = 0; l < f->fmd.filtSections[k]-1; l++) {
	    printf("\t%e %e %e %e\n",
		   f->fmd.filtCoeff[k][5 + 4*l + 0],
		   f->fmd.filtCoeff[k][5 + 4*l + 1],
		   f->fmd.filtCoeff[k][5 + 4*l + 2],
		   f->fmd.filtCoeff[k][5 + 4*l + 3]);
	  }
	}
      }
    }
  }
  return 0;
}
#endif
