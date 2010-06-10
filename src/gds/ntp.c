/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: ntp							*/
/*                                                         		*/
/* Module Description: implements API for NTP servers			*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Header File List: */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#ifdef OS_VXWORKS
#include <vxWorks.h>
#include <sockLib.h>
#include <inetLib.h>
#include <stdioLib.h>
#include <strLib.h>
#include <hostLib.h>
#include <selectLib.h>

#else
#include <strings.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#endif

#include "dtt/ntp.h"

#define PORT 123
#define MY_OFFSET 2208988800UL


   int sntp_sync (const char* hostname)
   {
      return sntp_sync_year (hostname, NULL);
   }


/* Sync the time */
   int sntp_sync_year (const char* hostname, int* year)
   {
      struct {
         unsigned char livnmode, stratum, poll, prec;
         unsigned long delay, disp, refid;
         unsigned long reftime[2], origtime[2], rcvtime[2], txtime[2];
      } pkt;
   
      int fd = -1, len, n;
      struct sockaddr_in saddr, myaddr;
      char answer[2000];
      unsigned long myt;
      long offset;
      fd_set readfds;
      struct timeval tv;
      struct timespec t1;
   
      /* Get server name */
      n = strlen(hostname);
      if (n <= 0) {
         perror("hostname");
         return -1;
      }
      sprintf(answer, "sntp: Connecting to %s\n", hostname);
      printf(answer);
   
      /* Resolve server name */
      bzero((char *)&saddr, sizeof (saddr));
      saddr.sin_family = AF_INET;
      saddr.sin_port = htons(PORT);
      if ((saddr.sin_addr.s_addr = inet_addr(hostname)) < 0) {
         perror("unknown server");
         return -1;
      }
      sprintf(answer, "sntp: Hostname resolved, getting the socket\n");
      printf(answer);
   
      /* Get a socket */
      if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
         perror("socket");
         return -1;
      }
   
      sprintf(answer, "sntp: Got socket, bind to port\n");
      printf(answer);
      /* Bind it to some port */
      bzero((char *)&myaddr, sizeof (myaddr));
      myaddr.sin_family = AF_INET;
      myaddr.sin_port = htons(0);
      myaddr.sin_addr.s_addr = INADDR_ANY;
   
      if (bind(fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0) {
         perror("bind error");
         close(fd);
         return -1;
      }
   
      /* Fill in the request packet */
      bzero((char *)&pkt, sizeof (pkt));
      pkt.livnmode = 0x0b;        /* version 1, mode 3 (client) */
   
      sprintf(answer, "sntp: Send packet\n");
      printf(answer);
      /* Send packet to server */
      if ((n = sendto(fd, (char *)&pkt, sizeof(pkt), 0, 
         (struct sockaddr *) &saddr, 
         sizeof (saddr))) != sizeof (pkt)) {
         sprintf(answer, "sendto %d err %d", n, errno);
         perror(answer);
         close(fd);
         return -1;
      }
   
      sprintf(answer, "sntp: Wait for reply\n");
      printf(answer);
      /* Read reply packet from server */
      len = sizeof (pkt);
      FD_ZERO(&readfds);
      FD_SET(fd, &readfds);
      tv.tv_sec = 5;
      tv.tv_usec = 0;
      if(select(FD_SETSIZE, &readfds, NULL, NULL, &tv) &&
        FD_ISSET (fd, &readfds)) {
         if ((n = recvfrom(fd, (char *)&pkt, sizeof(pkt), 0, 
            (struct sockaddr *) &saddr, 
            &len)) != sizeof(pkt)) {
            sprintf(answer, "recvfrom %d err %d", n, errno);
            perror(answer);
            close(fd);
            return -1;
         }
      }
      else {
         perror("Timeout");
         close(fd);
         return -1;
      }
      sprintf(answer, "sntp: Sanity check\n");
      printf(answer);

#if 0
      /* Sanity checks */
   
      if ((pkt.livnmode & 0xc0) == 0xc0 || !pkt.stratum || !pkt.txtime[0]) {
         perror("server not synchronized");
         close(fd);
         return -1;
      }
#endif
   
      /*
       * NTP timestamps are represented as a 64-bit unsigned fixed-
       * point number, in seconds relative to 0h on 1 January 1900. The integer
       * part is in the first 32 bits and the fraction part in the last 32 bits.
       */
      myt = ntohl(pkt.txtime[0]) - MY_OFFSET;
      offset = myt - time(NULL);
      sprintf(answer, "Time: %sstratum %d offset %ld\n", 
             ctime((long*)&myt), (int) pkt.stratum, offset);
      if (fd >= 0) {
         close(fd);
      }
      printf(answer);
   
      /* set year info */
      if (year) {
         struct tm gm;
         #ifdef OS_VXWORKS	
         if (gmtime_r (&myt, &gm) == 0) {
            *year = 1900 + gm.tm_year;
         }
      	#else
         if (gmtime_r (&myt, &gm) != NULL) {
            *year = 1900 + gm.tm_year;
         }
      	#endif
      }

      if (year < 2004) {
	fprintf(stderr, "FATAL ERROR: Bogus year received from NTP server\n");
	return -1;
      }
   
      /* set realtime clock */
      t1.tv_sec = ntohl(pkt.txtime[0]) - MY_OFFSET;
      t1.tv_nsec = ntohl(pkt.txtime[1]);
      clock_settime (CLOCK_REALTIME, &t1);
   
      return 0;
   }


   /*int main ()
   {
      sntp_sync ("10.1.240.1");
   }*/
