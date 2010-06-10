/* Header File List: */
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif 
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include "dtt/gdsmain.h" 
#include <string.h>

#ifdef OS_VXWORKS
/* for VxWorks */
#include <vxWorks.h>
#include <sockLib.h>
#include <inetLib.h>
#include <stdioLib.h>
#include <hostLib.h>
#include <ioLib.h>

/* for others */
#else
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include "sockutil.h"
#endif

#define DAQD_PORT 8088

   struct DAQDChannel {
      /* The channel name */
      char mName[40];
      /* The channel group number */
      int  mGroup;
      /* The channel sample rate */
      int  mRate;
      /* Set if trend data are calculated for this channel */
      int  mTrend;
      /* Data type */
      int  mDatatype;
      /* byte per sample */
      int  mbps;
   };
   typedef struct DAQDChannel DAQDChannel;

/*--------------------------------------  Hex conversion */
   static int CVHex(const char *text, int N) {
      int v = 0;
      int i;
      for (i=0 ; i<N ; i++) {
         v<<=4;
         if      ((text[i] >= '0') && (text[i] <= '9')) v += text[i] - '0';
         else if ((text[i] >= 'a') && (text[i] <= 'f')) v += text[i] - 'a';
         else if ((text[i] >= 'A') && (text[i] <= 'F')) v += text[i] - 'A';
         else 
            return -1;
      }
      return v;
   }

#if 0
/*--------------------------------------  Swap ints and shorts in place */
   static void SwapI(int *array, int N) {
      char temp;
      int i;
      for (i=0 ; i<N ; i++) {
         char* ptr = (char*) (array + i);
         temp   = ptr[0];
         ptr[0] = ptr[3];
         ptr[3] = temp;
         temp   = ptr[1];
         ptr[1] = ptr[2];
         ptr[2] = temp;
      }
      return;
   }

   static void SwapS(short *array, int N) {
      char temp;
      int i;
      for (i=0 ; i<N ; i++) {
         char* ptr = (char*) (array + i);
         temp   = ptr[0];
         ptr[0] = ptr[1];
         ptr[1] = temp;
      }
      return;
   }
#endif

   static int RecvRec(int mSocket, char *buffer, int length, 
                     int readall) {
      char* point = buffer;
      int   nRead = 0;
      do {
         int nB = recv(mSocket, point, length - nRead, 0);
         if (nB <= 0) 
            return -1;
         point += nB;
         nRead += nB;
      } while (readall && (nRead < length));
      return nRead;
   }

   static int SendRequest(int mSocket, const char* text, char *reply, 
                     int length, int *Size) {
      char status[4];
      int rc;
    /* Send the request */
   #if defined(OS_VXWORKS) || defined(__CYGWIN__)
      rc  = send (mSocket, (char*) text, strlen(text), 0 /*MSG_EOR*/);
   #else
      rc = send (mSocket, (char*) text, strlen(text), MSG_EOR);
   #endif
      if (rc <= 0) 
         return rc;
   
    /* Return if no reply expected. */
      if (reply == 0) 
         return 0;
   
    /* Read the reply status. */
      rc = RecvRec (mSocket, status, 4, 0);
      if (rc != 4) 
         return -1;
      rc = CVHex(status, 4);
      if (rc) 
         return rc;
   
    /* Read the reply text. */
      if (length != 0) {
         rc = RecvRec(mSocket, reply, length, 0);
         if (rc < 0)      
            return rc;
         if (rc < length) reply[rc] = 0;
         if (Size)       *Size = rc;
      }
      return 0;
   }


   static int chnnet (char* server, int port) 
   {
   #ifdef OS_VXWORKS
      typedef int in_addr_t;
   #endif
   
      struct in_addr	serveraddr;	/* IP addr of server */
      int		sock;		/* socket */
      struct sockaddr_in	name;	/* socket name */
      char		buf[1024];	/* buffer */
      int		i;		/* index */
      int 		channelnum;	/* channel number */
      int		mVersion;	/* version # of server */
      int		mRevision;	/* revision # of server */
   
   #ifdef OS_VXWORKS
      {
         in_addr_t	saddr;
      
         if (((saddr = inet_addr (server)) < 0) &&
            ((saddr = hostGetByName (server)) < 0)) {
            saddr = 0;
         }
         serveraddr.s_addr = saddr;
      }
   #else
      /* use DNS if necessary */
      if (nslookup (server, &serveraddr) < 0) {
         return -1;
      }
#if 0
      {
         struct 	hostent hostinfo;
         struct 	hostent* phostinfo;
         in_addr_t	addr;
         char		buf[1024];
      
         phostinfo = gethostbyname_r (server, &hostinfo, 
                                     buf, 1024, &i);
         if (phostinfo != NULL) {
            memcpy (&addr, hostinfo.h_addr_list[0], 
                   sizeof (addr));
            serveraddr = addr; /* network byte order */
         }
         else {
            return -1;
         }
      }
#endif
   #endif
   
      /* create the socket */
      /* printf ("create socket\n"); */
      sock = socket (PF_INET, SOCK_STREAM, 0);
      if (sock == -1) {
         return -1;
      }
   
      /* fill destination address */
      name.sin_family = AF_INET;
      name.sin_port = htons (port);  /* convert to network byte order */
      name.sin_addr.s_addr = serveraddr.s_addr;   /* already in network byte order */
   
      /* printf ("connect socket\n"); */
      if (connect (sock, (struct sockaddr*) &name, 
         sizeof (struct sockaddr_in)) != 0) {
         return -1;   
      }
   
      /* Get the server version number */
      mVersion = 0;
      mRevision = 0;
      if (SendRequest(sock, "version;", buf, 4, 0)) 
         return -1;
      mVersion = CVHex(buf, 4);
      if (SendRequest(sock, "revision;", buf, 4, 0)) 
         return -1;
      mRevision = CVHex(buf, 4);
      printf ("server = %s   port = %i   version = %i.%i\n",
             server, port, mVersion, mRevision);
   
      /* Request a list of channels. */
      if (SendRequest(sock, "status channels;", buf, 4, 0)) 
         return -1;
   
      /* Get the number of channels. */
      channelnum = CVHex(buf, 4);
      printf ("number of channels = %i\n", channelnum);
   
      /* Skip undocumented field. */
      RecvRec (sock, buf, 4, 1);
   
      printf (" chn name                                      "
             "rate type bps group\n");
      for (i = 0; i < channelnum; i++) {
         int recsz;
         int rc;
         DAQDChannel chn;
         recsz = (mVersion == 9) ? 60 : 52;
         rc = RecvRec(sock, buf, recsz, 1);
         if (rc < recsz) 
            return -1;
         memcpy(chn.mName, buf, 40);
         chn.mRate  = CVHex(buf+40, 4);
         chn.mTrend = CVHex(buf+44, 4);
         chn.mGroup = CVHex(buf+48, 4);
         if (recsz > 52) {
            chn.mbps = CVHex(buf+52, 4);
            chn.mDatatype = CVHex(buf+56, 4);
         } 
         else {
            chn.mbps = 0;
            chn.mDatatype = 0;
         }
         printf ("%4i %40s %5i %4i %3i %5i\n", i, chn.mName, 
                chn.mRate, chn.mDatatype, chn.mbps, chn.mGroup);
      }
   
      /* quit */
      /* printf ("quit\n"); */
      strcpy (buf, "quit;");
      write (sock, buf, strlen (buf));
      close (sock);
      return 0;
   }


/* main program */

   #ifdef OS_VXWORKS
   int chnnetdump (char* server) 
   {
   #else
   int main (int argc, char *argv[])
   {
      char	server[256];
   
      if (argc <= 1) {
         strcpy (server, "fb0");
      }
      else {
         strcpy (server, argv[1]);
      }
   #endif
      return chnnet (server, DAQD_PORT);
   }

