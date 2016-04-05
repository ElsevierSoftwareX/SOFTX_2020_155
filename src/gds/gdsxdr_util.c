static char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsxdr_util						*/
/*                                                         		*/
/* Module Description: implements functions for encodeing and decoding	*/
/* data into or from xdr streams					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

/* Header File List: */
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "dtt/gdsutil.h"
#include "dtt/gdsxdr_util.h"


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _DEFAULT_XDR_SIZE   max. length of an XDR'd task arg	*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#define _DEFAULT_XDR_SIZE	100000	/* 100kByte */


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: xdr_decodeArgument				*/
/*                                                         		*/
/* Procedure Description: decodes an xdr struct from an xdr stream	*/
/*                                                         		*/
/* Procedure Arguments: xdr_struct - pointer to structure pointer	*/
/*				     (return arg)			*/
/*			xdr_struct_len - structure length		*/
/*			xdr_stream - pointer to xdr_stream	 	*/
/*			xdr_func - xdr en-/de-codeing function		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int xdr_decodeArgument (char** xdr_struct, unsigned int xdr_struct_len,
                     const char* xdr_stream, unsigned int xdr_stream_len,
                     xdrproc_t xdr_func)
   {
      XDR		xdr;	/* xdr handle */
      bool_t		retval;
   
      if ((xdr_struct == NULL) || (xdr_stream == NULL)) {
         return -1;
      }
   
      /* if xdr_func is NULL we assume that xdr_stream points to an integer
         argument */
   
      /* allocate memory */
      if (xdr_func != NULL) {
         *xdr_struct = malloc (xdr_struct_len);
         if (*xdr_struct != NULL) {
            memset (*xdr_struct, 0, xdr_struct_len);
         }
      }
      else {
         *xdr_struct = malloc (sizeof (int));
      }
      if (*xdr_struct == NULL) {
         return -32;
      }
   
      /* create xdr stream handle */
      xdr.x_ops = NULL;
      xdrmem_create (&xdr, (char*) xdr_stream, 
                    xdr_stream_len, XDR_DECODE);
      if (xdr.x_ops == NULL) {
         free (*xdr_struct);
         *xdr_struct = NULL;
         return -32;
      }
   
      if (xdr_func != NULL) {
         retval = xdr_func (&xdr, *xdr_struct);
      }
      else {
         retval = xdr_int (&xdr, (int*) xdr_struct);
      }
      xdr_destroy (&xdr);
      if (!retval) {
         free (*xdr_struct);
         *xdr_struct = NULL;
         return -33;
      }
   
      return 0;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* External Procedure Name: xdr_encodeArgument				*/
/*                                                         		*/
/* Procedure Description: encodes an xdr struct into an xdr stream	*/
/*                                                         		*/
/* Procedure Arguments: xdr_struct - structure to encode, 		*/
/*			xdr_stream - pointer to xdr_stream pointer 	*/
/*				     (return arg)			*/
/*			xdr_stream_len - returned xdr stream length	*/
/*			xdr_func - xdr en-/de-codeing function		*/
/*                                                         		*/
/* Procedure Returns: 0 if successful, <0 if failed			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   int xdr_encodeArgument (const char* xdr_struct, char** xdr_stream, 
                     unsigned int* xdr_stream_len, xdrproc_t xdr_func)
   {
      XDR		xdr;	/* xdr handle */
      int		size;	/* size of argument */
   
      if ((xdr_struct == NULL) || (xdr_stream == NULL) || 
         (xdr_stream_len == NULL)) {
         return -1;
      }
   
      /* if xdr_func is NULL we assume that xdr_struct points to an integer
         argument */
   
      /* get size of argument and allocate memory */
      if (xdr_func != NULL) {
      #if defined(OS_VXWORKS) || defined(__CYGWIN__)
         size = _DEFAULT_XDR_SIZE;
      #else
         size = xdr_sizeof (xdr_func, (char*) xdr_struct);
      #endif
      }
      else {
         size = sizeof (int);
      }
      /* set return arguments */
      *xdr_stream_len = size;
      *xdr_stream = malloc (size);
      if (*xdr_stream == NULL) {
         return -31;
      }
   
      /* create xdr stream */
      xdr.x_ops = NULL;
      xdrmem_create (&xdr, *xdr_stream, size, XDR_ENCODE);
      if (xdr.x_ops == NULL) {
         free (*xdr_stream);
         *xdr_stream = NULL;
         return -32;
      }
         /* encode argument into xdr stream */
      if (xdr_func != NULL) {
         if (!xdr_func (&xdr, (char*)xdr_struct)) {
            xdr_destroy (&xdr);
            free (*xdr_stream);
            *xdr_stream = NULL;
            return -33;
         }
         /* readjust size of xdr stream (VxWorks only) */
      #if defined(OS_VXWORKS) || defined(__CYGWIN__)
         size = xdr_getpos (&xdr);
         *xdr_stream_len = size;
         {
            void*		newptr;
            newptr = realloc (*xdr_stream, size);
            if (newptr != NULL) {
               *xdr_stream = newptr;
            }
         }
      #endif
         /* printf ("xdr size = %i and pos = %i\n", size, xdr_getpos (&xdr)); */
      }
      else {
         /* now encode integer into xdr stream */
         if (!xdr_int (&xdr, (int*) &xdr_struct)) {
            xdr_destroy (&xdr);
            free (*xdr_stream);
            *xdr_stream = NULL;
            return -33;
         }
      }
      xdr_destroy (&xdr);
   
      return 0;
   }
