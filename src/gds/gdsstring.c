/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsstring						*/
/*                                                         		*/
/* Module Description: implements functions for dealing with strings	*/
/* and the LIGO channel naming convention				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Header File List: */
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gds_strcasecmp				*/
/*                                                         		*/
/* Procedure Description: same as strcmp, but without case-sensitivity	*/
/*                                                         		*/
/* Procedure Arguments: s1, s2 - strings to comapre			*/
/*                                                         		*/
/* Procedure Returns: 0: equal, (-) s1 < s2, (+) s1> s2			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int
   gds_strcasecmp (const char *s1, const char *s2) {
      int d, i = 0;
      while (s1[i] || s2[i]) {
         if ((d = tolower (s1[i]) - tolower (s2[i])))
            return d;
         ++i;
      }
      return 0;
   }

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: gds_strncasecmp				*/
/*                                                         		*/
/* Procedure Description: same as strncmp, but without case-sensitivity	*/
/*                                                         		*/
/* Procedure Arguments: s1, s2 - strings to comapre			*/
/*                                                         		*/
/* Procedure Returns: 0: equal, (-) s1 < s2, (+) s1> s2			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int
   gds_strncasecmp (const char *s1, const char *s2, int n) {
      int d, i = 0;
      if (n <= 0) {
         return 0;
      }
      while (s1[i] || s2[i]) {
         if ((d = tolower (s1[i]) - tolower (s2[i])))
            return d;
         if (++i == n)
            break;
      }
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: strend					*/
/*                                                         		*/
/* Procedure Description: returns the end of a string			*/
/*                                                         		*/
/* Procedure Arguments: s						*/
/*                                                         		*/
/* Procedure Returns: end of s						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* strend (const char* s)
   {
      return (char*) (s + strlen(s));
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: strecpy					*/
/*                                                         		*/
/* Procedure Description: Like strcpy, but returns the end of string	*/
/*                                                         		*/
/* Procedure Arguments: s1, s2						*/
/*                                                         		*/
/* Procedure Returns: end of s1						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* strecpy (char* s1, const char* s2)
   {
      return strend (strcpy (s1, s2));
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: strencpy					*/
/*                                                         		*/
/* Procedure Description: Like strncpy, but returns the end of string	*/
/*                                                         		*/
/* Procedure Arguments: s1, s2, n					*/
/*                                                         		*/
/* Procedure Returns: end of s1						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* strencpy (char* s1, const char* s2, size_t n)
   
   {
      strncpy (s1, s2, n);
      s1[n-1] = 0;
      return strend (s1);
   }


#ifdef OS_VXWORKS
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: strdup					*/
/*                                                         		*/
/* Procedure Description: duplicates a string				*/
/*                                                         		*/
/* Procedure Arguments: s						*/
/*                                                         		*/
/* Procedure Returns: end of s1						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* strdup (const char* s)
   {
      char*		p;
   
      if (s == NULL) {
         return NULL;
      }
      p = malloc (strlen (s) + 1);
      if (p != NULL) {
         strcpy (p, s);
      }
      return p;
   }
#endif


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: chnIsValid					*/
/*                                                         		*/
/* Procedure Description: check if it is a valid channel		*/
/*                                                         		*/
/* Procedure Arguments: name						*/
/*                                                        		*/
/* Procedure Returns: non zero if valid, 0 otherwise			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int chnIsValid (const char* name)
   {
      return ((name != NULL) && 
             (strlen (name) > 0) &&
             (strchr (name, ':') != NULL) && 
             (strcspn (name, ":") > 1) &&
             (strchr (name, '-') != NULL) &&
             (strcspn (name, "-") > strcspn (name, ":")));
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: chnSitePrefix				*/
/*                                                         		*/
/* Procedure Description: returns site prefix				*/
/*                                                         		*/
/* Procedure Arguments: name, site					*/
/*                                                         		*/
/* Procedure Returns: pointer to site prefix				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* chnSitePrefix (const char* name, char* site)
   {
      if ((name == NULL) || (site == NULL) || (strlen (name) == 0)) {
         return NULL;
      }
   
      /* just copy first character */
      site [0] = name [0];
      site [1] = '\0';
   
      return site;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: chnIfoPrefix				*/
/*                                                         		*/
/* Procedure Description: returns interferometer prefix			*/
/*                                                         		*/
/* Procedure Arguments: name, ifo					*/
/*                                                         		*/
/* Procedure Returns: pointer to ifo prefix				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* chnIfoPrefix (const char* name, char* ifo)
   {
      int		colpos;		/* colon position */
   
      if ((name == NULL) || (ifo == NULL) || (strlen (name) <= 1)) {
         return NULL;
      }
   
      /* look for colon; neglect first character */
      colpos = strcspn (name + 1, ":");
      /* make sure it is actually there */
      if (colpos == strlen (name + 1)) {
         return NULL;
      }
      /* copy ifo prefix */
      strncpy (ifo, name + 1, colpos);
      ifo [colpos] = '\0';
   
      return ifo;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: chnSubsystemName				*/
/*                                                         		*/
/* Procedure Description: returns subsystem name			*/
/*                                                         		*/
/* Procedure Arguments: name, subsys					*/
/*                                                         		*/
/* Procedure Returns: pointer to subsystem name				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* chnSubsystemName (const char* name, char* subsys)
   {
      char*	colpos;		/* colon position */
      int	hyppos;		/* hyphen position */
   
      if ((name == NULL) || (subsys == NULL)) {
         return NULL;
      }
   
      /* look for colon */
      colpos = strchr (name, ':');
      if (colpos == NULL) {
         return NULL;
      }
      /* look for hyphen */
      hyppos = strcspn (colpos + 1, "-");
      /* make sure it is actually there */
      if (hyppos == strlen (colpos + 1)) {
         return NULL;
      }
      /* copy result */
      strncpy (subsys, colpos + 1, hyppos);
      subsys[hyppos] = '\0';
   
      return subsys;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: chnRemName					*/
/*                                                         		*/
/* Procedure Description: returns identification within subsystem	*/
/*                                                         		*/
/* Procedure Arguments: name, rem					*/
/*                                                         		*/
/* Procedure Returns: pointer to remaining characters			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* chnRemName (const char* name, char* rem)
   {
      char*	colpos;		/* colon position */
      char*	hyppos;		/* hyphen position */
   
      if ((name == NULL) || (rem == NULL)) {
         return NULL;
      }
   
      /* look for colon */
      colpos = strchr (name, ':');
      if (colpos == NULL) {
         return NULL;
      }
      /* look for hyphen */
      hyppos = strchr (colpos + 1, '-');
      if (hyppos == NULL) {
         return NULL;
      }
      /* copy result */
      return strcpy (rem, hyppos + 1);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: chnShortName				*/
/*                                                         		*/
/* Procedure Description: returns the channel short name		*/
/*                                                         		*/
/* Procedure Arguments: name, sname					*/
/*                                                         		*/
/* Procedure Returns: pointer to short name				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* chnShortName (const char* name, char* sname)
   {
      char*	colpos;		/* colon position */
   
      if ((name == NULL) || (sname == NULL)) {
         return NULL;
      }
   
      /* look for colon */
      colpos = strchr (name, ':');
      if (colpos == NULL) {
         return NULL;
      }
      /* copy result */
      return strcpy (sname, colpos + 1);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: chnMakeName					*/
/*                                                         		*/
/* Procedure Description: builds a channel name from its components	*/
/*                                                         		*/
/* Procedure Arguments: name, site, ifo, sname, srem			*/
/*                                                         		*/
/* Procedure Returns: pointer to channel name				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   char* chnMakeName (char* name, const char* site, const char* ifo,
                     const char* sname, const char* srem)
   {
      if ((name == NULL) || (site == NULL) || (sname == NULL)) {
         return NULL;
      }
   
      /* copy prefixes */
      if (ifo != NULL) {
         strcpy (strecpy (strecpy (name, site), ifo), ":");
      }
      else {
         strcpy (strecpy (name, site), ":");
      }
   
      /* copy short name */
      strcpy (strend (name), sname);
      if (srem != NULL) {
         strcpy (strecpy (strend (name), "-"), srem);
      }
   
      return name;
   }

