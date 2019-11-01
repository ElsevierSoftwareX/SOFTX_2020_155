///	@file param.c
///	@brief File contains routines for reading in DAQ config info by EPICS (skeleton.st).

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "daqmap.h"
#include "param.h"

#ifdef OS_VXWORKS
#define strcasecmp strcmp
#endif

static const CHAN_PARAM uninit = {
  -1,-1,-1,-1,-1,-1,0,0,-99999000.0,-99999999.0,-9999999.0,"none","none",
};

/*
 * Callback function should return 1 if OK and 0 if not.
 */
int testCallback(char *channel_name, struct CHAN_PARAM *params, void *user) { 
#if 0
  printf("channel_name=%s\n", channel_name);
  printf("dcuid=%d\n", params->dcuid);
  printf("datarate=%d\n", params->datarate);
  printf("acquire=%d\n", params->acquire);
  printf("ifoid=%d\n", params->ifoid);
  printf("datatype=%d\n", params->datatype);
  printf("chnnum=%d\n", params->chnnum);
  printf("testpoint=%d\n", params->testpoint);
  printf("gain=%f\n", params->gain);
  printf("slope=%f\n", params->slope);
  printf("offset=%f\n", params->offset);
  printf("units=%s\n", params->units);
  printf("system=%s\n", params->system);
#endif
  return 1; 
}

/* Cat string and make lower case */                              
static char *strcat_lower(char *dest, char *src) {             
  char *d = dest;                                        
  for( ; *d; d++);                                  
  for( ; (*d++ = tolower(*src)); src++);         
  return dest;                              
}                



#ifndef system_log
# define system_log(foo,str,one,two) fprintf(stderr, str, one, two)
#endif

int default_dcu_rate; 

int parseGdstpFile(char *fname,GDS_INFO_BLOCK *ginfo) {

  char *cp;
  char cbuf[128];
  char val[64];
  char id[64];
  int channum;
  int totalchans = 0;
  int inloop = 0;
  unsigned int cr;
  char chan_name[60];
  FILE *fp = fopen(fname, "r");
  if (fp == NULL) {
      return 0;
   }
   // Read file up to first TP channel name
    while((cp = fgets(cbuf, 128, fp)) && cbuf[0] != '[') {
    }
   while(!feof(fp)) {
       /* :TODO: there will be a problem if the closing square bracket is missing */
       // Capture the channel name
       for (cp = cbuf; *cp && *(cp+1) && *(cp+1) != ']'; cp++) *cp = *(cp+1);
       *cp = 0;
       strncpy(chan_name, cbuf, 60);
       chan_name[59] = 0;

	// Search for corresponding TP channel number, which should occur before next TP channel name.
       while((cp = fgets(cbuf, 128, fp)) && cbuf[0] != '[' ) {
       		for (cp = cbuf, cr = 0; *cp && *cp != '=' && cr < 64; cp++)  if (!isspace(*cp)) id[cr++] = *cp;
	      	if (*cp != '=') continue;
		// Get info descriptor and value based on '=' separator
	        id[cr]=0;
		for (cp++, cr = 0; *cp && cr < 64; cp++)  if (!isspace(*cp)) val[cr++] = *cp;
		val[cr]=0;

		if(!strcasecmp(id,"chnnum")) {
			char *endptr;
			channum = strtol(val,&endptr,0);
			// Load TP info from channel name and number info
			strcpy(ginfo->tpinfo[totalchans].tpname,chan_name);
			ginfo->tpinfo[totalchans].tpnumber = channum;
			totalchans ++;
			ginfo->totalchans = totalchans;
		}
       }
   }
   printf("EOF found\n");
   fclose(fp);

   return(totalchans);
   
}

 ///Parse DAQ system config file `fname' and call `callback' function
 /// for each data channel configured. Config files's CRC will be saved in
 /// `*crc'. Parameter `testpoint' is here for frame builder use.
 /// `arch_file' is a base name of archive file, ie. it is a full path
 /// Archive file name will be a base name with added date and time at the end.
 /// `user' is a user defined pointer passed to callback.
int
parseConfigFile(char *fname, unsigned long *crc,
		int (*callback)(char *channel_name, struct CHAN_PARAM *params, void *user),
		int testpoint, char *arch_file, void *user)
{
  unsigned int crc_ptr(char* cp, unsigned int bytes, unsigned int crc);
  unsigned int crc_len(unsigned int bytes, unsigned int crc);

  CHAN_PARAM deflt;
  CHAN_PARAM current;
  char cbuf[128];
  char *cp;
  unsigned int linenum = 1;
  unsigned int flen = 0;
  FILE *afp = 0;
  char afname[1024];
  FILE *fp = fopen(fname, "r");
  if (fp == NULL) {
    system_log(1, "failed to open `%s' for reading: errno=%d", fname, errno);
    return 0;
  }
  if (arch_file){
    long a = time(0);
    struct tm t;
    char buf[100];
    localtime_r(&a, &t);
    sprintf(buf, "_%02d%02d%02d_%02d%02d%02d.ini", 
	    t.tm_year-100, t.tm_mon+1, t.tm_mday,
	    t.tm_hour, t.tm_min, t.tm_sec);
    strcat(strcpy(afname, arch_file), buf);
    afp = fopen(afname, "w");
    if (!afp) {
      fclose(fp);
      system_log(1, "failed to open `%s' for writing: errno=%d", afname, errno);
      return 0;
    }
  }
  *crc = 0;

  while((cp = fgets(cbuf, 128, fp)) && cbuf[0] != '[') {
    int l = strlen(cbuf); flen += l;
    *crc = crc_ptr(cbuf, l, *crc);
    if (arch_file) {
      if (fputs(cbuf, afp) == EOF) {
	system_log(1, "writing to `%s' failed; errno=%d", afname, errno);
	fclose(fp);
	fclose(afp);
	unlink(afname);
	return 0;
      }
    }
    linenum++;
  }
  if (arch_file) {
    if (fputs(cbuf, afp) == EOF) {
      system_log(1, "writing to `%s' failed; errno=%d", afname, errno);
      fclose(fp);
      fclose(afp);
      unlink(afname);
      return 0;
    }
  }
  {
    int l = strlen(cbuf); flen += l;
    *crc = crc_ptr(cbuf, l, *crc);
    linenum++;
  }

  if(!cp || feof(fp)) {
    system_log(1, "failed to locate first section %s %s", "in", fname);
    fclose(fp);
    if (afp) fclose(afp);
    return 0;
  }

  deflt = uninit;

  for(;;) {
    char channel_name[60];
	/* :TODO: there will be a problem if the closing square bracket is missing */
    for (cp = cbuf; *cp && *(cp+1) && *(cp+1) != ']'; cp++) *cp = *(cp+1);
    *cp = 0;

    strncpy(channel_name, cbuf, 60);
    channel_name[59] = 0;

    current = uninit;
    while((cp = fgets(cbuf, 128, fp)) && cbuf[0] != '[') {
      unsigned int cr;
      char id[64];
      char val[64];

      int l = strlen(cbuf); flen += l;
      *crc = crc_ptr(cbuf, l, *crc);
      linenum++;
      if (arch_file) {
	if (fputs(cbuf, afp) == EOF) {
	  system_log(1, "writing to `%s' failed; errno=%d", afname, errno);
	  fclose(fp);
	  fclose(afp);
	  unlink(afname);
	  return 0;
	}
      }

      for (cp = cbuf, cr = 0; *cp && *cp != '=' && cr < 64; cp++)  if (!isspace(*cp)) id[cr++] = *cp;
      if (*cp != '=') continue; 
      id[cr]=0;
      for (cp++, cr = 0; *cp && cr < 64; cp++)	if (!isspace(*cp)) val[cr++] = *cp;
      val[cr]=0;

#define convert_int(varname) \
      if (!strcasecmp(id, #varname)) { \
	char *endptr; \
	varname = strtol(val, &endptr, 0); \
	if (endptr != (val + strlen(val))) { \
	  system_log(1, "not an integer number in %s:%d", fname, linenum); \
	  fclose(fp); \
          if (afp) { \
	    fclose(afp); \
	    unlink(afname); \
          } \
	  return 0; \
	} \
      }
#define convert_float(varname) \
      if (!strcasecmp(id, #varname)) { \
	char *endptr; \
	varname = strtod(val, &endptr); \
	if (endptr != (val + strlen(val))) { \
	  system_log(1, "not a floating point number in %s:%d", fname, linenum); \
	  fclose(fp); \
          if (afp) { \
	    fclose(afp); \
	    unlink(afname); \
          } \
	  return 0; \
	} \
      }

      /* convert_int(current.dcuid) */
      if(!strcasecmp(id,"dcuid")) {
	char *endptr;
	current.dcuid = strtol(val, &endptr,0);
	if (endptr != (val + strlen(val))) {
	  system_log(1, "not a integer number in %s:%d", fname, linenum);
	  fclose(fp);
          if (afp) {
	    fclose(afp);
	    unlink(afname);
          }
	  return 0;
	}
      }
      
      else if (!strcasecmp(id, "datarate")) { 
	char *endptr; 
	current.datarate = strtol(val, &endptr, 0); 
	if (endptr != (val + strlen(val))) { 
	  system_log(1, "not an integer number in %s:%d", fname, linenum); 
	  fclose(fp); 
          if (afp) {
	    fclose(afp);
	    unlink(afname);
          }
	  return 0; 
	}
	if (current.datarate < 16 || current.datarate > (256*1024)) {
	  system_log(1, "data rate out of range in %s:%d", fname, linenum);
	  fclose(fp);
          if (afp) {
	    fclose(afp);
	    unlink(afname);
          }
	  return 0;
	}
	{
	  unsigned int value = current.datarate;
	  do {
      	    if (value % 2) {
	  	system_log(1, "data rate is not a power of two in %s:%d", fname, linenum);
	        fclose(fp);
		if (afp) {
		  fclose(afp);
		  unlink(afname);
		}
	        return 0;
	    }
      	    value /= 2;
    	  } while (value > 1);
	}
      } 
	/* else convert_int(current.acquire) */
      else if(!strcasecmp(id,"acquire")) {
	char *endptr;
	current.acquire = strtol(val, &endptr,0);
	if (endptr != (val + strlen(val))) {
	  system_log(1, "not a integer number in %s:%d", fname, linenum);
	  fclose(fp);
          if (afp) {
	    fclose(afp);
	    unlink(afname);
          }
	  return 0;
	}
      }
      /* else convert_int(current.ifoid) */
      else if(!strcasecmp(id,"ifoid")) {
	char *endptr;
	current.ifoid = strtol(val, &endptr,0);
	if (endptr != (val + strlen(val))) {
	  system_log(1, "not a integer number in %s:%d", fname, linenum);
	  fclose(fp);
          if (afp) {
	    fclose(afp);
	    unlink(afname);
          }
	  return 0;
	}
      }
      else if(!strcasecmp(id,"rmid")) {
	char *endptr;
	current.rmid = strtol(val, &endptr,0);
	if (endptr != (val + strlen(val))) {
	  system_log(1, "not a integer number in %s:%d", fname, linenum);
	  fclose(fp);
          if (afp) {
	    fclose(afp);
	    unlink(afname);
          }
	  return 0;
	}
      }
      /* else convert_int(current.datatype) */
      else if(!strcasecmp(id,"datatype")) {
	char *endptr;
	current.datatype = strtol(val, &endptr,0);
	if (endptr != (val + strlen(val))) {
	  system_log(1, "not a integer number in %s:%d", fname, linenum);
	  fclose(fp);
          if (afp) {
	    fclose(afp);
	    unlink(afname);
          }
	  return 0;
	}
      }
      /* else convert_int(current.chnnum) */
      else if(!strcasecmp(id,"chnnum")) {
	char *endptr;
	current.chnnum = strtol(val, &endptr,0);
	if (endptr != (val + strlen(val))) {
	  system_log(1, "not a integer number in %s:%d", fname, linenum);
	  fclose(fp);
          if (afp) {
	    fclose(afp);
	    unlink(afname);
          }
	  return 0;
	}
      }

      /* else convert_float(current.gain) */
      else if (!strcasecmp(id, "gain")) { 
	char *endptr; 
	current.gain = strtod(val, &endptr);
	if (endptr != (val + strlen(val))) {
	  system_log(1, "not a floating point number in %s:%d", fname, linenum);
	  fclose(fp);
          if (afp) {
	    fclose(afp);
	    unlink(afname);
          }
	  return 0;
	}
      }
      /* else convert_float(current.slope) */
      else if (!strcasecmp(id, "slope")) { 
	char *endptr; 
	current.slope = strtod(val, &endptr);
	if (endptr != (val + strlen(val))) {
	  system_log(1, "not a floating point number in %s:%d", fname, linenum);
	  fclose(fp);
          if (afp) {
	    fclose(afp);
	    unlink(afname);
          }
	  return 0;
	}
      }
      /* else convert_float(current.offset) */
      else if (!strcasecmp(id, "offset")) { 
	char *endptr; 
	current.offset = strtod(val, &endptr);
	if (endptr != (val + strlen(val))) {
	  system_log(1, "not a floating point number in %s:%d", fname, linenum);
	  fclose(fp);
          if (afp) {
	    fclose(afp);
	    unlink(afname);
          }
	  return 0;
	}
      }
      else if (!strcasecmp(id, "units")
		|| (testpoint == 2 && !strcasecmp(id, "hostname"))) {
	strncpy(current.units, val, 32);
      }
      else if (!strcasecmp(id, "system")) {
	strncpy(current.system, val, 32);
      }
#undef convert_int
#undef convert_float
    }

    if (cp && !feof(fp)) {
      if (arch_file) {
        if (fputs(cbuf, afp) == EOF) {
          system_log(1, "writing to `%s' failed; errno=%d", afname, errno);
	  fclose(fp);
	  fclose(afp);
	  unlink(afname);
	  return 0;
        }
      }
      {
        int l = strlen(cbuf); flen += l;
        *crc = crc_ptr(cbuf, l, *crc);
      }
    }
    if (!strcasecmp(channel_name, "default")) {
      deflt = current;
      default_dcu_rate = deflt.datarate;
    } else {
#define setdefault(varname) if (current.varname == uninit.varname) current.varname = deflt.varname;
      setdefault(dcuid);
      setdefault(datarate);
      setdefault(acquire);
      setdefault(ifoid);
      setdefault(datatype);
      setdefault(chnnum);
      setdefault(gain);
      setdefault(slope);
      setdefault(offset);
#undef setdefault
      if (!strcmp(current.units, uninit.units)) strcpy(current.units, deflt.units);
      if (!strcmp(current.system, uninit.system)) strcpy(current.system, deflt.system);

      current.testpoint = testpoint;
      /* Allow for missing conversion data in testpoint config files */
      /* Allow missing dcu id in testpoint config files too */
      if (testpoint) {
	if (current.gain == uninit.gain) current.gain = 1;
	if (current.slope == uninit.slope) current.slope = 1;
	if (current.offset == uninit.offset) current.offset = 0;
	if (current.dcuid == uninit.dcuid) current.dcuid = DCU_ID_EX_16K;
      }
      if (0 == (*callback)(channel_name, &current, user)) {
	fclose(fp);
	if (afp) {
	  fclose(afp);
	  unlink(afname);
	}
	return 0;
      }
    }
    if (!cp || feof(fp)) {
      fclose(fp);
      if (afp) fclose(afp);
      *crc = crc_len(flen, *crc);
      return 1;
    }
  }
}


static int
infoCallback(char *channel_name, struct CHAN_PARAM *params, void *user) {
  DAQ_INFO_BLOCK *info = (DAQ_INFO_BLOCK *) user;
  testCallback(channel_name, params, user);
  if (info->numChans >= DCU_MAX_CHANNELS) {
    fprintf(stderr, "Too many channels. Hard limit is %d", DCU_MAX_CHANNELS);
    return 0;
  }
  if((params->chnnum >= 40000) && (params->chnnum < 50000) && ((params->datatype == 2) || (params->datatype == 7)))
  {
	info->numEpicsInts ++;
	info->numEpicsTotal ++;
  } else if ((params->chnnum >= 40000) && (params->chnnum < 50000) && (params->datatype == 4)){
	info->numEpicsFloats ++;
	info->numEpicsTotal ++;
  } else if (params->chnnum >= 50000) {
	info->numEpicsFilts ++;
	info->numEpicsTotal ++;
  } else {
  sprintf(info->tp[info->numChans].channel_name,"%s",channel_name);
  info->tp[info->numChans].tpnum = params->chnnum;
  info->tp[info->numChans].dataType = params->datatype;
  info->tp[info->numChans].dataRate = params->datarate;
  info->tp[info->numChans].dataGain = (int)params->gain;
  info->numChans++;
  }
  return 1;
}


 /// Load DAQ configuration file and store data in `info'.
 /// Input and archive file names are determined based on provided site, ifo
 /// and system names.
int
loadDaqConfigFile(DAQ_INFO_BLOCK *info, GDS_INFO_BLOCK *gdsinfo, char *site, char *ifo, char *sys)
{
  unsigned long crc = 0;
  char fname[256];         /* Input file name */
  char gds_fname[256];         /* Input file name */
  char archive_fname[256]; /* Archive file name */
  char perlCommand[256];   /* String that will contain the Perl command */
  int returnValue = -999;

  strcat(strcat(strcpy(fname, "/opt/rtcds/"), site), "/");
  strcat_lower(fname, ifo);

  strcpy(perlCommand, "iniChk.pl ");

  strcpy(gds_fname,fname);
  strcat(fname, "/chans/daq/");

  strcpy(archive_fname, fname);

  strcat(fname,sys);
  strcat(fname, ".ini");

  strcat(archive_fname, "archive/");
  strcat(archive_fname, sys);
  printf("%s\n%s\n", fname, archive_fname);

  strcat(perlCommand, fname);
/*  fprintf(stderr, "pC=%s\n", perlCommand);  */
  returnValue = system(perlCommand);
  if (returnValue != 0)  {
     fprintf(stderr, "Failed to load DAQ configuration file\n");
     return 0;
  }
  strcat(gds_fname,"/target/gds/param/tpchn_");
  strcat_lower(gds_fname,sys);
  strcat(gds_fname,".par");
  // printf("GDS TP FILE = %s\n",gds_fname);
int mytotal = parseGdstpFile(gds_fname,gdsinfo);
// printf("total gds chans = %d\n",mytotal);

  info->numChans = 0;
	info->numEpicsInts = 0;
	info->numEpicsTotal = 0;
	info->numEpicsFloats = 0;
	info->numEpicsFilts = 0;
  if (0 == parseConfigFile(fname, &crc, infoCallback, 0, archive_fname, info)) return 0;
  info->configFileCRC = crc;
  printf("CRC=0x%lx\n", crc);
  printf("dataSize=0x%lx 0x%lx\n", sizeof(DAQ_INFO_BLOCK), (0x1fe0000 + sizeof(DAQ_INFO_BLOCK)));
  printf("EPICS: INT = %d  FLOAT = %d  FILTERS = %d FAST = %d SLOW = %d \n",info->numEpicsInts,info->numEpicsFloats,info->numEpicsFilts,info->numChans,info->numEpicsTotal);
  return 1;
}
