static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: GDS Parameter File API 					*/
/*                                                         		*/
/* Module Description:  GDS Parameter File API  source code		*/
/*									*/
/*                                                         		*/
/* Module Arguments:					   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date    Engineer   Comments			   		*/
/* 0.1   4/1/98  MRP/PG	    Initial release              		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: HTML documentation  					*/
/*	References:							*/
/*                                                         		*/
/* Author Information:							*/
/*	Name          Telephone    Fax          e-mail 			*/
/*	Mark Pratt    617-253-6410		mrp@mit.edu		*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Sun Ultra					*/
/*	Compiler Used: CC & gcc						*/
/*	Runtime environment: Solaris, vxWorks				*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	OK			*/
/*			  Lint.			TBD			*/
/*			  ANSI			TBD			*/
/*			  POSIX			TBD			*/
/*									*/
/* Known Bugs, Limitations, Caveats:					*/
/*								 	*/
/*									*/
/*                                                         		*/
/*                      -------------------                             */
/*                                                         		*/
/*                             LIGO					*/
/*                                                         		*/
/*        THE LASER INTERFEROMETER GRAVITATIONAL WAVE OBSERVATORY.	*/
/*                                                         		*/
/*                     (C) The LIGO Project, 1996.			*/
/*                                                         		*/
/*                                                         		*/
/* California Institute of Technology			   		*/
/* LIGO Project MS 51-33				   		*/
/* Pasadena CA 91125					   		*/
/*                                                         		*/
/* Massachusetts Institute of Technology		   		*/
/* LIGO Project MS 20B-145				   		*/
/* Cambridge MA 01239					   		*/
/*                                                         		*/
/* LIGO Hanford Observatory				   		*/
/* P.O. Box 1970 S9-02					   		*/
/* Richland WA 99352					   		*/
/*                                                         		*/
/* LIGO Livingston Observatory		   				*/
/* 19100 LIGO Lane Rd.					   		*/
/* Livingston, LA 70754					   		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dtt/gdsutil.h"

/*
 * Parameter File Management
 * 
 */

   char *
   nextParamFileSection (FILE *fp, char *cbuf)
   {
      char *cp;
   
      if(!fp || !cbuf)
         return 0;				/* arg error */
   
      while((cp = fgets(cbuf, PARAM_ENTRY_LEN, fp)) && cbuf[0] != '[')
         ;
   
      if(!cp || feof(fp)) {
         rewind(fp);
         return 0;
      }
   
      for (cp = cbuf; *cp && *(cp+1) && *(cp+1) != ']'; cp++)
         *cp = *(cp+1);
      *cp = 0;
      return cbuf;
   }

   int
   findParamFileSection (FILE *fp, const char *sec_name, int rwd)
   {
      char cbuf[PARAM_ENTRY_LEN], *cp;
      int stoff;
   
      if(!fp || !sec_name)
         return 0;		/* arg error */
      if(rwd)
         rewind(fp);
      stoff = ftell(fp);
   
      while((cp = nextParamFileSection(fp, cbuf)) &&
           gds_strcasecmp(cbuf, sec_name))
         ;
   
      if(!cp && rwd)
         return 0;
   
      if(!cp) {					/* try again from top */
         while((cp = nextParamFileSection(fp, cbuf)) &&
              gds_strcasecmp(cbuf, sec_name))
            if(ftell(fp) >= stoff)
               return 0;
         if(!cp)
            return 0;
      }
      return 1;
   }

   char *
   getParamFileSection (FILE *fp, const char *sec_name,
                     int *nentry, int rwd )
   {
      char cbuf[PARAM_ENTRY_LEN], *cp, *ctail;
      char *section, *sp;
      int stoff;
      int n = 0;
   
      if(!fp || !nentry)
         return 0;			/* arg error */
   
      if(sec_name) {
         if(!findParamFileSection(fp, sec_name, rwd))
            return 0;
      } 
   
      /* first read to count param entries */
      *nentry = 0;
      stoff = ftell(fp);
      ctail = cbuf + PARAM_ENTRY_LEN - 1;
   
      while((cp = fgets(cbuf, PARAM_ENTRY_LEN, fp))) {
      
         while(*cp && isspace((int)*cp) && (++cp < ctail))
            ;
      
         if(!*cp || cp == ctail)			/* fluff */
            continue;	
         if(*cp == '[')				/* next section */
            break;	
         if(*cp == ';' ||				/* fluff */
           *cp == '#' ||				/* fluff */
           *cp == '\n' )				/* fluff */
            continue;
         ++*nentry;	
      }
      if(!*nentry)					/* empty section */
         return 0;
   
   /* second read to copy */
      fseek(fp, stoff-ftell(fp), SEEK_CUR);	/* rewind to section head */
      sp = section = (char*) calloc(*nentry * PARAM_ENTRY_LEN, sizeof(char));
   
      while((cp = fgets(cbuf, PARAM_ENTRY_LEN, fp))) {
         while(*cp && isspace((int)*cp) && (++cp < ctail))
            ;
         if(!*cp || (cp == ctail))			/* fluff */ 
            continue;
         if(*cp == ';' ||				/* fluff */
           *cp == '#' ||				/* fluff */
           *cp == '\n' )				/* fluff */
            continue;
      
         memcpy(sp, cp, strlen(cp)-1);		/* remove newline */
         if(++n == *nentry)  
            return
               section;
         sp += PARAM_ENTRY_LEN;
      }
      return 0;				/* error */
   }

/*
 * Parameter Section Parsing
 * 
 */

   char *
   nextParamSectionEntry (char *section, int nentry, int *cursor)
   {
      int c;
      if(nentry <= 0) {
         gdsError( GDS_ERR_PRM, "nextParamSectionEntry()");
         return 0;
      }
      if(cursor && ++*cursor >= 0 && *cursor < nentry)
         c = *cursor;
      else
         c = 0;
      if(cursor)
         *cursor = c;
      return
         (c * PARAM_ENTRY_LEN + section);
   }

   int
   findParamSectionEntry (const char *par_name, char *section,
                     int nentry, int *cursor)
   {
      int c, clast, ncmp;
      char *entry;
   
      if(!section){
         gdsError( GDS_ERR_PRM, "findParamSectionEntry()");
         return 0;
      }
   
      if(cursor && *cursor >= 0 && *cursor < nentry)
         c = *cursor;
      else
         c = nentry - 1;				/* start at top */
      clast = c;
      ncmp = strlen(par_name);
      while((entry = nextParamSectionEntry(section, nentry, &c))) {
         if(gds_strncasecmp(par_name, entry, ncmp) == 0) {
            if(cursor)
               *cursor = c;
            return c;
         }
         if(c == clast)
            return -1;				/* not found */
      }
      gdsError( GDS_ERR_PROG, "findParamSectionEntry()");
      return GDS_ERR_PROG;				/* error */
   }

   char*
   getParamSectionEntry (const char *par_name, char *section,
                     int nentry, int *cursor)
   {
      int c;
      char *val, *vtail;
   
      if(cursor && *cursor >= 0 && *cursor < nentry)
         c = *cursor;
      else
         c = nentry - 1;
   
      if((c = findParamSectionEntry(par_name, section, nentry, &c)) < 0)
         return 0;
   
      if(cursor)
         *cursor = c;
      val = c * PARAM_ENTRY_LEN + section;
      vtail = val + PARAM_ENTRY_LEN - 1 ;
   
   /* skip ahead to '=' sign */
      while(*val && *val != '=' && ++val < vtail)
         ;
   
      if(!*val || val == vtail) {
         gdsError( GDS_ERR_FORMAT, "getParamSectionEntry() no =");
         return 0;				/* bad format */
      }
   
   /* skip '=' and trailing spaces*/
      ++val;
      while(*val && isspace((int)*val) && ((++val) < vtail));
   
      if(!*val || val == vtail) {
         gdsError( GDS_ERR_MISSING, "getParamSectionEntry() no value");
         return 0;				/* bad format */
      }  
      else
         return val;
   }

   int
   loadParamSectionEntry (const char *par_name, char *sec,
                     int nentry, int *cursor, int type, void *def)
   {
      char *val;
      if(!par_name || !sec) {
         gdsError( GDS_ERR_PRM, "loadParamSectionEntry()");
         return -1;
      }
   
      if(!(val = getParamSectionEntry(par_name, sec, nentry, cursor)) || !*val)
         return 0;		/* no match, no error */
   
      switch(type) {
         case 0 :
            *((int*)def) = (*val != '0' && tolower(*val) != 'f' &&
                           tolower(*val) != 'n' ); 
            break;
         case 1:
            *((int*)def) = atoi(val);
            break;
         case 2 :
            *((double*)def) = atof(val);
            break;
         case 3 :
            strcpy((char*)def, val);
            break;
         case 4:
            *((unsigned long*) def) = strtoul (val, NULL, 0); 
            break;
         case 5:
            *((short*)def) = atoi(val);
            break;
         case 6:
            *((float*)def) = atof(val);
            break;
         default :
            gdsError( GDS_ERR_PRM, "loadParamSectionEntry() Invalid type");
            return -1;
      }
      return 0;
   }


   int
   loadBoolParam (const char *filename, const char *sec_name,
                 const char *par_name, int *def)
   {
      FILE *fp;
      int nentry;
      int err;
      char *sec;
   
      if(!filename || !sec_name || !par_name || !def) {
         gdsError( GDS_ERR_PRM, "loadBoolParam() bad args");
         return GDS_ERR_PRM;
      }
   
      if((fp = fopen(filename, "r")) == NULL) {
         gdsError (GDS_ERR_MISSING, "loadBoolParam() bad args");
         return GDS_ERR_MISSING;
      }
      sec = getParamFileSection(fp, sec_name, &nentry, 0);
      fclose (fp);
      if (sec == NULL) {
         return GDS_ERR_MISSING;
      }
   
      err = loadParamSectionEntry(par_name, sec, nentry, 0, 0, (void*) def);
      free(sec);
      return err;
   }

   int
   loadIntParam (const char *filename, const char *sec_name,
                const char *par_name, int *def)
   {
      FILE *fp;
      int nentry;
      int err;
      char *sec;
   
      if(!filename || !sec_name || !par_name || !def) {
         gdsError( GDS_ERR_PRM, "loadIntParam() bad args");
         return GDS_ERR_PRM;
      }
   
      if((fp = fopen(filename, "r")) == NULL) {
         gdsError (GDS_ERR_MISSING, "loadIntParam() bad args");
         return GDS_ERR_MISSING;
      }
   
      sec = getParamFileSection(fp, sec_name, &nentry, 0);
      fclose (fp);
      if (sec == NULL) {
         return GDS_ERR_MISSING;
      }
      err = loadParamSectionEntry(par_name, sec, nentry, 0, 1, (void*) def);
      free(sec);
      return err;
   }


   int
   loadNumParam (const char *filename, const char *sec_name,
                const char *par_name, unsigned long *def)
   {
      FILE *fp;
      int nentry;
      int err;
      char *sec;
   
      if(!filename || !sec_name || !par_name || !def) {
         gdsError( GDS_ERR_PRM, "loadNumParam() bad args");
         return GDS_ERR_PRM;
      }
   
      if((fp = fopen(filename, "r")) == NULL) {
         gdsError (GDS_ERR_MISSING, "loadNumParam() bad args");
         return GDS_ERR_MISSING;
      }
      sec = getParamFileSection(fp, sec_name, &nentry, 0);
      fclose (fp);
      if (sec == NULL) {
         return GDS_ERR_MISSING;
      }
   
      err = loadParamSectionEntry (par_name, sec, nentry, 0, 4, (void*) def);
      free(sec);
      return err;
   }


   int 
   loadFloatParam (const char *filename, const char *sec_name, 
                  const char *par_name, double *def)
   {
      FILE *fp;
      int nentry;
      int err;
      char *sec;
   
      if(!filename || !sec_name || !par_name || !def) {
         gdsError( GDS_ERR_PRM, "loadFloatParam() bad args");
         return GDS_ERR_PRM;
      }
   
      if((fp = fopen(filename, "r")) == NULL) {
         gdsError (GDS_ERR_MISSING, "loadFloatParam() bad args");
         return GDS_ERR_MISSING;
      }
      sec = getParamFileSection(fp, sec_name, &nentry, 0);
      fclose (fp);
      if (sec == NULL) {
         return GDS_ERR_MISSING;
      }
   
      err = loadParamSectionEntry(par_name, sec, nentry, 0, 2, (void*) def);
      free(sec);
      return err;
   }

   int 
   loadStringParam (const char *filename, const char *sec_name,
                   const char *par_name, char *def)
   {
      FILE *fp;
      int nentry;
      int err;
      char *sec;
   
      if(!filename || !sec_name || !par_name || !def) {
         gdsError( GDS_ERR_PRM, "loadStringParam() bad args");
         return GDS_ERR_PRM;
      }
   
      if((fp = fopen(filename, "r")) == NULL) {
         gdsError (GDS_ERR_MISSING, "loadStringParam() bad args");
         return GDS_ERR_MISSING;
      }
      sec = getParamFileSection(fp, sec_name, &nentry, 0);
      fclose (fp);
      if (sec == NULL) {
         return GDS_ERR_MISSING;
      }
   
      err = loadParamSectionEntry(par_name, sec, nentry, 0, 3, (void*) def);
      free(sec);
      return err;
   }

