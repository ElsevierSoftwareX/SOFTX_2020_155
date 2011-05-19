/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdschannel						*/
/*                                                         		*/
/* Module Description: 	API for accessing channel information		*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 30June98 D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: gdschannel.html					*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8336  (509) 372-2178  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.6			*/
/*	Compiler Used: sun workshop C 4.2				*/
/*	Runtime environment: sparc/solaris				*/
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

#ifndef _GDS_GDSCHANNEL_H
#define _GDS_GDSCHANNEL_H

#ifdef __cplusplus
extern "C" {
#endif


/* Header File List: */


/** @name Channel Information API
    This API provides routines to query the channel information 
    database and to retreive infromation about specific channels.

    @memo Access to the channel information database
    @author Written June 1998 by Daniel Sigg
    @version 0.1
************************************************************************/

/*@{*/

#if 0
/** Compiler flag for a enabling dynamic configuration. When enabled
    the host address and interface information of the NDS and the channel 
    database server are queried from the network rather than read in 
    through a file.

    @author DS, June 98
    @see Test point API
************************************************************************/
#undef _CONFIG_DYNAMIC

/** Compiler flag specifying the source for the channel database.
    This define can be specified during compilation to choose the 
    database which will be used to obtain the channel information.
    It can be one of the following values:
    \begin{verbatim}
    0	default database
    1	parameter file stored at param/init/<site>/chninfo.par
    2	database provided by the DAQ system
    \end{verbatim}
  
    @author DS, June 98
    @version 0.1
    @see CDS channel info structure
************************************************************************/
#define _CHANNEL_DB		0

/** Compiler flag for disabling test points. If undefined test point
    channel information is read in from parameter files. Test point
    channels are added to the ones read in through the database 
    sepcified with _CHANNEL_DB.

    @author DS, June 98
    @see Test point API
************************************************************************/
#define _NO_TESTPOINTS

#endif

/** Channel information structure. Contains all the relevant information 
    associated with a specific channel.
  
    @author DS, June 98
    @version 0.1
    @see CDS channel info structure
************************************************************************/
   struct gdsChnInfo_t {
      /** channel name. 32 characters maximum; always \0 terminated! 
          The name must be the first member of the structure.  */
      char		chName[MAX_CHNNAME_SIZE];
      /** interferometer id:
          H0, L0 -> 0; H1, L1 -> 1; H2 -> 2 */
      short		ifoId;
      /** reflective memory loop id: 0 or 1 (LHO only) */
      short		rmId;
      /** DCU id number */
      short		dcuId;
      /** channel number */
      unsigned short	chNum;
      /** data type as defined by the nds:
      1 - int16, 2 - int32, 3 - int64, 4 - float, 5 - double, 6 - complex float */
      short		dataType;
      /** data rate: specified in Hz; must be a power of two  */
      int		dataRate;
      /** channel group */
      short 		chGroup;
      /** number of bytes per sample */
      short		bps;
      /** front-end gain */
      float		gain;
      /** calibration slope */
      float		slope;
      /** calibration offset */
      float		offset;
      /** unit name */
      char		unit[40];
      /** offset of channel in reflective memory */
      unsigned long	rmOffset;
      /** size of block channels belongs to */
      unsigned long	rmBlockSize;
      /** TP number */
      unsigned short	tpNum;
   };
   typedef struct gdsChnInfo_t gdsChnInfo_t;

/** Installs the channel information client interface. This function 
    should be called prior of using the channel information interface. 
    If not, the first call to any of the functions in this API will 
    call it. This function reads the channel information from file
    and/or the network data server.

    @param void
    @return 0 if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int channel_client (void);

/** Terminates the channel information client interface.

    @param void
    @return void
    @author DS, June 98
************************************************************************/
   void channel_cleanup (void);

/** Sets the network data server address. This function must
    be called before any other function (including channel_client).
    If a non-positive port number is specified, the default is used.

    @param hostname name of the network data server
    @param port port number of the network data server
    @return 0 if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int gdsChannelSetHostAddress (const char* hostname, int port);

/** Function prototype used for deciding which channels are returned by
    a channel query. Must return true (1), if channel has to be 
    included; and false (0), otherwise.
  
    @author DS, June 98
    @param info pointer to a channel information record
    @return true if valid, false otherwise
    @see CDS channel info structure
************************************************************************/
   typedef int (*gdsChannelQuery) (const gdsChnInfo_t* info);

/** Obtains the channel information associated with the given channel 
    name. The routine returns 0 if the channel was found in the database
    and -1 otherwise. This routine should also be used to check whether
    a given channel exists. If a returned channel information is not
    needed the info pointer can be set to NULL. 

    @param name channel name
    @param info pointer to a channel information record
    @return 0 if successful, <0 otherwise
    @author DS, June 98
************************************************************************/
   int gdsChannelInfo (const char* name, gdsChnInfo_t* info);

/** Returns the number of channels in the channel information database.
    The interferometer number is 1 for the 4K, 2 for the 2K, 0 for
    site specific channels and -1 for all channels. The flag parameter
    can be used to restrict the channels returned in the list 
    (currently not used).

    @param ifo the interferometer number
    @param query describes the type of requested channels
    @return number of channels if successful, <0 otherwise
    @author DS, July 98
************************************************************************/
   int gdsChannelListLen (int ifo, gdsChannelQuery query);

/** Returns a list of channels from the channel information database.
    The interferometer number is 1 for the 4K, 2 for the 2K, 0 for
    site specific channels and -1 for all channels. The query parameter
    can be used to restrict the channels returned in the list 
    (currently not used). The resulting list is stored at the 'info'
    pointer location. 'maxChn' decribes the length of the reserved
    data structure (in number of channels); it should be at least as
    long as the total number of channels returned by gdsChannelListLen.
    The returned function value specifies the number of channel 
    information structures copied into the 'info' array. The channels
    of the returned list are alphabetically ordered.

    @param ifo the interferometer number
    @param query describes the type of requested channels
    @param info pointer to a channel information record array
    @return number of returned channels if successful, <0 otherwise
    @author DS, July 98
************************************************************************/
   int gdsChannelList (int ifo, gdsChannelQuery query, gdsChnInfo_t* info, 
   int maxChn);

/** Returns a a space separated list of channel names from the channel 
    information database. The interferometer number is 1 for the 4K, 2 
    for the 2K, 0 for site specific channels and -1 for all channels. 
    The query parameter can be used to restrict the channels returned 
    in the list. It is a function taking a gdsChnInfo_t* as an argument
    and returning true if channel should be included. If no query 
    function is specified, all channels are returned. The function
    returns a character array which has to be freed by the caller.
    The channels of the returned list are alphabetically ordered.

    @param ifo the interferometer number
    @param query describes the type of requested channels
    @param info 0 - names only; 1 - names and rates
    @author DS, July 98
************************************************************************/
   char* gdsChannelNames (int ifo, gdsChannelQuery query, int info);

/* Returns the interferometer number. Uses the channel information 
    to determine to which interferometer subset the channel belongs.

    @param info pointer to channel information structure
    @return interferometer number
    @author DS, July 98
************************************************************************/
/*#define GDS_CHANNEL_IFO_SUBNET(info) \
	((info)->dcuId <= 16 ? 2 : 1)*/


/*@}*/

#ifdef __cplusplus
}
#endif

#endif /*_GDS_GDSCHANNEL_H */
