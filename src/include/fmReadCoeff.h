#ifndef FM_READ_COEFF_H
#define FM_READ_COEFF_H

#define NO_FM10GEN_C_CODE


/* char vars' length */
#define CVL  40

typedef struct fmSubSysMap {
  char name[CVL]; /* Filter module name within this subsystem */
  int  fmModNum;  /* Filter module number within this subsystem */

  /* The rest of structure is filled by fmReadCoeffFile() */
  int filters;
  VME_FM_OP_COEF fmd;
} fmSubSysMap;


typedef struct fmReadCoeff {

  /*
    Date: Mon, 10 Jun 2002 11:03:54 -0700
    From: David Barker <barker_d@apex.ligo-wa.caltech.edu>
    To: rolf bork <rolf@ligo.caltech.edu>, Alex Ivanov <aivanov@ligo.caltech.edu>,
    nergis <nergis@ligo.mit.edu>, Peter Fritschel <pf@ligo.mit.edu>,
    richard mccarthy <mccarthy_r@ligo-wa.caltech.edu>
    Subject: filter files

    how about this for a naming standard.

    all 4k sus filter coef files called

    When any front end opens a coef file, it makes an archive of this file
    under the directory

    under filter_archive/h1 there are sus, asc, lsc subdirs. Under sus, one
    directory per optic.
  */

  /*
    site:      lho | llo | 40m
    ifo:       h1  | h2 | l1 | caltech
    system:    sus | asc | lsc
    subSystem: etx | etmy | itmx | itmy | rm |  bs | (etc. optic name) | (other subsystem for asc or lsc)

    Config file name:
       upper($ifo) + upper($system) + "_" + upper($subSystem) + ".txt"

    Archived config file name:
       upper($ifo) + upper($system) + "_" + upper($subSystem) + ".txt." + time("DDMMYY_hh:mm:ss")

    Config file directory:

    Archive file directory:
       + lower($ifo) + "/" + lower($system) + "/" + lower($subSystem) + "/"

   */

  char site[CVL];   /* lho, llo, 40m */
  char ifo[CVL];    /* h1, h2, l1, caltech */
  char system[CVL]; /* sus, asc, lsc */
  VME_COEF* pVmeCoeff; /* Pointer to coefficient area in shared VME window */

#ifndef FM_SUBSYS_NUM
#   error FM_SUBSYS_NUM has to be #defined to the number of coeff config files
#endif

  struct subSys {
    char name[CVL]; /* RM, BS (an optic) or WFS, QPD, OPTLEV or could be empty */
    char archiveNameModifier[CVL]; /* Can be used to alter archive file name */
    int numMap;           /* The number of elements in the map below */
    fmSubSysMap *map;       /* Connects filter names with filter module numbers for this subsys */
    unsigned long crc;    /* CRC sum calculated on the input file */
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

typedef struct filterThresh {                                            /* MA */
   char filterName[MAX_LINE_LEN];                                        /* MA */

   float gain[2];                                                        /* MA */
   float limit[2];                                                       /* MA */
   float offset[2];                                                      /* MA */
   float rampTime[2];                                                    /* MA */

   int switch1;                                                          /* MA */
   int switch2;                                                          /* MA */
};                                                                       /* MA */

int readThresholdsFile(fmReadCoeff*,                                     /* MA */
                       fmSubSysMap*,                                     /* MA */
                       struct filterThresh*,                             /* MA */
                       int,                                              /* MA */
                       int);                                             /* MA */

int readMatrixThreshFile(fmReadCoeff*,                                   /* MA */
                         char*,                                          /* MA */
                         float*,                                         /* MA */
                         float*,                                         /* MA */
                         int,                                            /* MA */
                         int,                                            /* MA */
                         int);                                           /* MA */

int fmReadCoeffFile(fmReadCoeff *, int);
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
