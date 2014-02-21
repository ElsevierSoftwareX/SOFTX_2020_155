#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

// define DBUG to enable log messages on std out
// DBUG = 1: basic messages
// DBUG = 2: verbose
// DBUG = 3: verbose and random packet receiving errors
#define DBUG 2
#undef DBUG

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#ifdef __linux__
#include <sys/time.h>
#include <linux/types.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
typedef u_int32_t in_addr_t;
#elif defined (P__WIN32)
#include <sys/ioctl.h>
#define IFF_POINTOPOINT 0
#else
#include <sys/sockio.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <unistd.h> 
#include <memory>
#include <vector>
#include <algorithm>
#include <string>
#include "framerecv.hh"
#include <iostream>
#include "tconv.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "shutdown.h"

#include "debug.h"
   using namespace std;

#if defined(USE_MAIN)
  static const int buflen = 1024*1024*20;
  char buffer[buflen];

main() {
  diag::frameRecv* NDS = new diag::frameRecv();
// DMT broadcast port needs to be different from IB ports !
  if (!NDS->open("225.0.0.1", "10.110.144.0", 7097 /*diag::frameXmitPort + 1*/)) {
  //if (!NDS->open("225.0.0.1", "10.12.0.0", 7032 /*diag::frameXmitPort + 1*/)) {
        perror("Multicast receiver open failed.");
        exit(1);
  }
  char *bufptr = buffer;
  unsigned int seq, gps, gps_n;
/* Get Thread ID */
  pid_t rcvfr_tid;
  rcvfr_tid = (pid_t) syscall(SYS_gettid);
  printf("Opened broadcaster receiver thread pid=%d\n",(int) rcvfr_tid);
  while (1) {
    int length = NDS->receive(bufptr, buflen, &seq, &gps, &gps_n);
    printf("len=%d seq=%d gps=%d gps_n=%d data=%d\n", length, seq, gps, gps_n, ntohl(*((unsigned int *)bufptr)));

    // Print the header
    unsigned int *bufp = (unsigned int *)bufptr;
    unsigned int num_dcu = ntohl(*bufp);bufp++;
    printf("num_dcu=%d\n", num_dcu);
    while(num_dcu--) {
    	printf("dcu=%d size=%d config_crc=0x%x crc=0x%x status=0x%x cycle=%d\n",
		ntohl(bufp[0]), 
		ntohl(bufp[1]), 
		ntohl(bufp[2]), 
		ntohl(bufp[3]), 
		ntohl(bufp[4]), 
		ntohl(bufp[5]));
    	bufp += 6;
    }

    // The data starts at bufptr +2048
    //
#if 0
    char fname[1024];
    sprintf(fname, "%d", gps);
    int fd = open(fname, O_CREAT | O_WRONLY, 0777);
    write(fd, bufptr, length);
    close(fd);
#endif
  }
  exit(0);

}
#endif

   static struct hostent* __gethostbyname_r (const  char  *name,  
                     struct hostent *result, char *buffer, int buflen, int *h_errnop)
   {
   #if defined(sun)
      return gethostbyname_r (name, result, buffer, buflen, h_errnop);
   #elif defined(__linux__)
      struct hostent* ret = 0;
      if (gethostbyname_r (name, result, buffer, buflen, &ret, h_errnop) < 0) {
         return 0;
      }
      else {
         return ret;
      }
   #elif defined (P__WIN32)
   // quick and dirty
   struct hostent* ret = gethostbyname (name);
   if (ret) memcpy (result, ret, sizeof (struct hostent));
   return ret;
   #else
   #error define gethostbyname_r for this platform
   #endif 
   }

namespace diag {


   struct interface_t {
      char		name[IFNAMSIZ];
      in_addr_t		addr;
   };
   typedef vector<interface_t> interfaceList;


   bool getInterfaces (int sock, interfaceList& iList) 
   {
      // clear list
      iList.clear();
   
      // get list of interfaces
      ifconf 	ifc;
      char 	buf[2048];
      ifc.ifc_len = sizeof (buf);
      ifc.ifc_buf = buf;
      if (ioctl (sock, SIOCGIFCONF, (char*) &ifc) < 0) {
         return false;
      }
   
      // obtaining interface flags
      ifreq*	ifr = ifc.ifc_req;
      for (int n = ifc.ifc_len / sizeof (ifreq); --n >=0; ifr++) {
         // inet interfaces only
         if (ifr->ifr_addr.sa_family != AF_INET) {
            continue;
         }
      	 // get flags
         if (ioctl (sock, SIOCGIFFLAGS, (char*) ifr) < 0) {
            return -1;
         }
      	 // skip unintersting cases
         if (((ifr->ifr_flags & IFF_UP) == 0) ||
            ((ifr->ifr_flags & IFF_LOOPBACK) != 0) ||
            ((ifr->ifr_flags & (IFF_BROADCAST | IFF_POINTOPOINT)) == 0)) {
            continue;
         }
         interface_t iface;
         strcpy (iface.name, ifr->ifr_name);
         iface.addr = ((sockaddr_in*)(&ifr->ifr_addr))->sin_addr.s_addr;
         // cout << "found " << iface.name << " (" << 
            // inet_ntoa (*(in_addr*)&iface.addr) << ")" << endl;
         iList.push_back (iface);
      } 
   
      return true;
   }


   bool matchInterface (int sock, const char* net, in_addr_t& i_addr)
   {
      // check net address
      if ((net == 0) || (inet_addr (net) == (unsigned long)-1)) {
         // use default host address
         char		myname[256];	// local host name
         hostent 	hostinfo;
         char		buf[1024];
         int		errno;
         if ((gethostname (myname, sizeof (myname)) != 0) ||
            (__gethostbyname_r (myname, &hostinfo, buf, 1023, &errno)) == 0) {
            return false;
         }
         else {
            memcpy (&i_addr, hostinfo.h_addr_list[0], sizeof (in_addr_t));
            return true;
         }
      }
      else {
         // get a list of all interface addresses
         interfaceList iList;
         if (!getInterfaces (sock, iList)) {
            return false;
         }
         // go through interface list and take closest match
         i_addr = 0;
         unsigned int diff = (unsigned int) -1;
         for (interfaceList::iterator i = iList.begin(); 
             i != iList.end(); i++) {
            if ((ntohl(i->addr) > ntohl(inet_addr (net))) && 
               (ntohl(i->addr) - ntohl(inet_addr (net)) < diff)) {
               i_addr = i->addr;
               diff = ntohl(i->addr) - ntohl(inet_addr (net));
            }
         }
         return (i_addr != 0);
      }
   }


   bool frameRecv::open (const char* mcast_addr, const char* interface,
                     int port)
   {
      if (sock >= 0) {
         close();
      }
   
      // open socket
      sock = socket (PF_INET, SOCK_DGRAM, 0);
      if (sock < 0) {
      #ifdef DBUG
         cerr << "socket() failed" << endl;
      #endif
         return false;
      }
   
      // set reuse socket option 
      int reuse = 1;
      if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, 
                     (char*) &reuse, sizeof (int)) != 0) {
         ::close (sock);
         sock = -1;
      #ifdef DBUG
         cerr << "socketopt() failed" << endl;
      #endif
         return false;
      }
   
      // set receive buffer size 
      int bufsize = rcvInBuffersize;
      //for (int bTest=bufsize; bTest<=rcvInBufferMax; bTest+=rcvInBuffersize) {
//	 if (setsockopt (sock, SOL_SOCKET, SO_RCVBUF, 
			 //(char*) &bTest, sizeof (int)) != 0) break;
	 //bufsize = bTest;
      //}
      if (setsockopt (sock, SOL_SOCKET, SO_RCVBUF, 
                     (char*) &bufsize, sizeof (int)) != 0) {
         ::close (sock);
         sock = -1;
      #ifdef DBUG
         cerr << "socketopt(2) failed" << endl;
      #endif
         return false;
      }
      printf("broadcast receiver socket buffer size set to %d\n", bufsize);
   
      // set send buffer size 
      bufsize = rcvOutBuffersize;
      if (setsockopt (sock, SOL_SOCKET, SO_SNDBUF, 
                     (char*) &bufsize, sizeof (int)) != 0) {
         ::close (sock);
         sock = -1;
         return false;
      }
   
      // bind socket
      name.sin_family = AF_INET;
      this->port = port;
      name.sin_port = htons (port);
      name.sin_addr.s_addr = htonl (INADDR_ANY);
      if (bind (sock, (struct sockaddr*) &name, sizeof (name))) {
         ::close (sock);
         sock = -1;
      #ifdef DBUG
         cerr << "bind() failed" << endl;
      #endif
         return false;
      }
   
      // options
      if (mcast_addr != 0) {
         multicast = true;
      
         // check multicast address
         if (!IN_MULTICAST (ntohl (inet_addr (mcast_addr)))) {
            ::close (sock);
            sock = -1;
            return false;
         }
      
         // get interface address
         in_addr_t i_addr;
         if (!matchInterface (sock, interface, i_addr)) {
            ::close (sock);
            sock = -1;
            return false;
         }
      
         // multicast: join
         group.imr_multiaddr.s_addr = inet_addr (mcast_addr);
         group.imr_interface.s_addr = i_addr;
         if (setsockopt (sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
                        (char*) &group, sizeof (ip_mreq)) == -1) {
            ::close (sock);
            sock = -1;
            return false;
         }
         if (logison) {
            char buf[256];
            sprintf (buf, "Join multicast %s at %s", mcast_addr,
                    inet_ntoa (*(in_addr*) &i_addr));
            addLog (buf);
         }
      #ifdef DBUG
         cout << "join multicast " << mcast_addr << " at " <<
            inet_ntoa (*(in_addr*) &i_addr) << endl;
      #endif
      }
      else {
         multicast = false;
         // broadcast: enable
         int bset = 1;
         if (setsockopt (sock, SOL_SOCKET, SO_BROADCAST, 
                        (char*) &bset, sizeof (bset)) == -1) {
            ::close (sock);
            sock = -1;
            return false;
         }
      #ifdef DBUG
         cout << "broadcast port "  << port << endl;
      #endif
      }
   
      // check quality of service
      if ((qos < 0) || (qos > 2)) {
         qos = 2;
      }
      pkts.clear();
      first = true;
   
      return true;
   }


   void frameRecv::close ()
   {
      if (sock < 0) {
         return;
      }
   
      if (multicast) {
         // multicast drop
         if (setsockopt (sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, 
                        (char*) &group, sizeof (ip_mreq)) == -1) {
            // do nothing; close anyway
         }
      }
   
      pkts.clear();
   
      ::close (sock);
      sock = -1;
   }


   class packetControl {
   public:
      packetControl () : received (false) {
      }
      bool 		received;
   };

   bool isNotValidPacket (const packetControl& cntrl) {
      return !cntrl.received;
   }


   bool frameRecv::purge ()
   {
      // empty input queue
      int max = 10;
      while (getPacket (false)) {
         if (--max <= 0) {
            break;
         }
      }
      return !pkts.empty();
   }


   long long frameRecv::calcDiff (const frameRecv::packetlist& pkts, 
                     unsigned int newseq) {
      long long diff = 0;
      if (pkts.size() > 0) {
         diff = (long long) pkts[0]->header.seq - 
            (long long) newseq;
//printf("packet sequence is %d\n", pkts[0]->header.seq);
        if (diff > (long long)0x80000000LL) {
            diff -= (long long)0x100000000LL;
         }
         if (diff < -(long long)0x80000000LL) {
            diff += (long long)0x100000000LL;
         }
      }
      return diff;
   }

int drop_seq = 0; // sequence to drop (for debugging)

   int frameRecv::receive (char*& data, int maxlen, unsigned int* sequence,
                     unsigned int* recvtime, unsigned int* duration)
   {
      // check socket
      if (sock < 0) {
         return -1;
      }
   
      // check on data array and max. length
      if (data == 0) {
         maxlen = -1;
      }
      else if (maxlen <= 0) {
         return -2;
      }
   
      // determine qos parameter
      double qosFrac;
      switch (qos) {
         case 0: 
            qosFrac = 0.3;
            break;
         case 1: 
            qosFrac = 0.1;
            break;
         default: 
            qosFrac = 0.03;
            break;
      }
   
      // read a packet if buffer empty
      if (pkts.size() == 0) {
         // request a packet
         if (!getPacket()) {
            return -3;
         }
      }
   #ifdef DBUG
      //cout << "first packet received" << endl;
   #endif
      // if first, set the old sequence number to one of the first packet
      if (first) {
         oldseq = pkts[0]->header.seq;
         retransmissionRate = 0.0;
         first = false;
      }
   
      // calculate new sequnence number from old
      unsigned int newseq = oldseq + 1;
   
      // main receiver loop
      bool requestPacket = false;	// request a new packet
      bool firstPacket = true;		// first packet received of this seq.?
      bool seqDone = false;		// sequence transmission done
      int len = 0;			// bytes received so far
      int retry = 0;			// # of rebroadcasts
      long long timestamp = 0;		// time stamp (in us) of retry timer
      bool retryExpired = true;		// keeps track of retry timer
      double newRetransmissionRate = 0;	// new retransmission rate
      vector<packetControl> pktCntrl;	// marks received packets
   
      while (1) {
      
      //    // check if packet buffer empty
         // if ((pkts.size() == 0) || requestPacket) {
         //    // request a packet
            // if (!getPacket()) {
               // return -4;
            // }
            // requestPacket = false;
         // }
      
      	 // empty input queue (or maximum of 10 packets)
         int max = 1;
         while (getPacket (false)) {
            if (--max <= 0) {
               break;
            }
         }
      
         // calculate difference between new seq. and the one from the pkt
         long long diff = calcDiff (pkts, newseq);
         // if (pkts.size() > 0) {
            // diff = (long long) pkts[0]->header.seq - 
               // (long long) newseq;
            // if (diff > 0x80000000LL) {
               // diff -= 0x100000000LL;
            // }
            // if (diff < -0x80000000LL) {
               // diff += 0x100000000LL;
            // }
         // }
      
         // wait receiver tick if no packets were received or
         // only more recent packets are in the queue
         if (pkts.empty() || (!firstPacket && (diff > 0))) {
            timespec	wait = {(1000LL * rcvDelayTick) / 1000000000LL, 
               (1000LL * rcvDelayTick) % 1000000000LL};
            nanosleep (&wait, 0);
            // continue if sequence wasn't done yet
            // if (!seqDone) {
               // continue;
            // }
         }
      
         // process packets at front of queue if some are there */
         if (!pkts.empty()) {
         
            // check if sequence out of sync
	    if (diff >= 1 && len == 0) {
		if (retry > 0) { // Waiting for it now
            	   // check if retry is expired
            	   timeval tp;
            	   if (gettimeofday (&tp, 0) < 0) {
               		retryExpired = true;
            	   } else {
		   	long long timestampNow = (long long) tp.tv_sec * 1000000 + 
                  					(long long) tp.tv_usec;
                	//cout << "tNow = " << timestampNow << " timestamp = " <<
                   		//timestamp << " retryT = " << retryTimeout << endl;
               		retryExpired = ((timestampNow - timestamp) > retryTimeout);
            	   }
		   if (!retryExpired) continue; // Keep waiting
		   else {
			printf("Retry expired; requesting again\n");
			if (retry >= maxRetry) {
				printf("Too many retries; max = %d\n", maxRetry);
				abort();
			}
		   }
		}
		//printf("DIFF=%d and len==0; missed the first packet\n", diff);
		// Looks like we missed the packet (or packets)!
		// Request its retransmission and wait
	//	abort();
		
                retransmitpacket	rpkt;
            	memset (&rpkt.header, 0, sizeof (packetHeader));
            	rpkt.header.pktType = PKT_REQUEST_RETRANSMIT;
            	rpkt.header.seq = newseq;
            	rpkt.header.pktNum = 0;
            	rpkt.header.pktTotal = 1;
            	int n = 0;
            	rpkt.pktResend[n++] = 0;

         #ifdef DBUG
            	cout << "ask for retransmission " << n << " (rate " <<
               	newRetransmissionRate << "; seq " << newseq << ") at " << endl;
               	//(TAInow() % 1000000000000LL) / 1E9 << endl;
         #endif
            	rpkt.header.pktLen = n * sizeof (int32_t);
            	if (!putPacket (rpkt)) {
               		return -6;
            	}
         
            	// start new timer
            	retryExpired = false;
            	timeval tp;
            	if (gettimeofday (&tp, 0) < 0) {
               		timestamp = 0;
            	} else {
               		timestamp = (long long) tp.tv_sec * 1000000 + (long long) tp.tv_usec;
            	}
            	retry++;

	    	continue;

            } else if (((diff<0 ? -diff : diff) > maxSequenceOutOfSync)
                /*|| ((diff >= 1) && (len == 0)) */) {
               newseq = pkts.back()->header.seq + 1;
               pkts.clear();
               firstPacket = true;
               retry = 0;
               len = 0;
               seqDone = false;
               newRetransmissionRate = 0.0;
               if (logison) {
                  char buf[512];
                  sprintf (buf, "Have to skip %i sequence numbers\n"
                          "New sequence number is %i", (int)diff, 
                          newseq);
                  addLog (buf);
               }
            #ifdef DBUG
	       if (diff != 0) 
                  cout << "have to skip (" << (int)diff << ")" << endl;
            #endif
#ifndef USE_UDP
	       //abort();
	        server_is_shutting_down = true;
		exit(1);
//#error
#endif
               continue;
            }
         
            // reject negative sequence difference (i.e. old packets)
            while (!pkts.empty() && (diff < 0)) {
               // skip old packets
#if 0
               if (logison) {
                  char buf[512];
                  sprintf (buf, "Skip packet %li of out-of-order sequence %lu",
                          pkts[0]->header.pktNum, pkts[0]->header.seq);
                  addLog (buf);
               }
#endif
               pkts.pop_front();
               diff = calcDiff (pkts, newseq);
            }
            if (pkts.empty()) {
               continue;
            }
         
            // found a valid packet
            while (!pkts.empty() && (diff == 0)) {
               // allocate memory and setup qos packet control if first packet
               if (firstPacket) {
                  // check if data array is long enough
                  int needed = pkts[0]->header.pktTotal * packetSize;
                  if ((maxlen > 0) && (maxlen < needed)) {
                     return -needed;
                  }
                  // allocate data array if necessary
                  if (maxlen == -1) {
                     if (data != 0) {
                        delete [] data;
                     }
                     data = new (std::nothrow) char [needed];
                  }
                  if (logison && !data) {
                     char buf[256];
                     sprintf (buf, "Memory allocation for %i failed",
                             needed);
                     addLog (buf);
                  }
                  // check if data array is valid
                  if (data == 0) {
                     return -5;
                  }
                  // set sequence, time stamp & duration
                  if (sequence != 0) {
                     *sequence = newseq;
                  }
                  if (recvtime != 0) {
                     *recvtime = pkts[0]->header.timestamp;
                  }
                  if (duration != 0) {
                     *duration = pkts[0]->header.duration;
                  }
                  // setup qos & packet control
                  pktCntrl = vector<packetControl> (pkts[0]->header.pktTotal);
                  newRetransmissionRate = 
                     (1.0 - 1.0/retransmissionAverage) * retransmissionRate;
                  firstPacket = false;
               }
            
               // copy packet payload into receiver buffer
               int pktNum = pkts[0]->header.pktNum;
               if ((pktNum >= 0) && (pktNum < (int)pktCntrl.size()) &&
                  (!pktCntrl[pktNum].received)) {
                  memcpy (data + pktNum * packetSize, pkts[0]->payload,
                         pkts[0]->header.pktLen);
                  len += pkts[0]->header.pktLen;
                  pktCntrl[pktNum].received = true;
               //#if DBUG - 0 > 2 // add a random error rate!
		#if 0
                  if ((double)rand() < rcvErrorRate * RAND_MAX) {
                     pktCntrl[pktNum].received = false;
                     len -= pkts[0]->header.pktLen;
                  }
               #endif
               }
            
               // check if last packet
               if (pkts[0]->header.pktTotal == pkts[0]->header.pktNum + 1) {
                  seqDone = true;
               #ifdef DBUG
                  //cout << "received last package at " << TAInow() << endl;
                  cout << "received last package len=" << len << endl;
               #endif
               }
               // remove packet
               pkts.pop_front();
               diff = calcDiff (pkts, newseq);
            }
         
            // sequence done if next sequence is already in buffer
            if (diff > 0 && len) {
               //cout << "#2 received last package len=" << len << endl;
               seqDone = true;
            }
         }
      
         // continue if sequence not yet finished
         if (!seqDone) {
            continue;
         }
      
         // calculate # of missing packets
      #ifdef __SUNPRO_CC
         int missingPackets = 0;
         count_if (pktCntrl.begin(), pktCntrl.end(), 
                  isNotValidPacket, missingPackets);
      #else
         int missingPackets = count_if (pktCntrl.begin(), pktCntrl.end(), 
                              isNotValidPacket);
      #endif
      
         // check if complete
         if (missingPackets == 0) {
            oldseq = newseq;
            retransmissionRate = newRetransmissionRate;
            return len; // done!
         }
         #ifdef DBUG
          cout << "missing packets " << missingPackets << 
             " (packets pending " << pkts.size() << ")" << endl;
         #endif
      
         if ((retry > 0) && !retryExpired) {
            // check if retry is expired
            timeval tp;
            if (gettimeofday (&tp, 0) < 0) {
               retryExpired = true;
            }
            else {
               long long timestampNow = (long long) tp.tv_sec * 1000000 + 
                  (long long) tp.tv_usec;
                //cout << "tNow = " << timestampNow << " timestamp = " <<
                   //timestamp << " retryT = " << retryTimeout << endl;
               retryExpired = ((timestampNow - timestamp) > retryTimeout);
            }
         }
      
         // check qos
         if ((missingPackets > maximumRetransmit) ||
            /*(retransmissionRate > qosFrac) || */
            ((retry >= maxRetry) && (retryExpired)) ||
            ((int)pkts.size() >= rcvpacketbuffersize)) {
            if (logison) {
               char buf[256];
               sprintf (buf, "Have to skip %i packets (%s)", 
                       missingPackets, 
                       (missingPackets > maximumRetransmit) ? 
                       "retransmit limit exceeded" :
                       (retransmissionRate > qosFrac) ?
                       "quality of service limit exceeded" :
                       ((retry >= maxRetry) && (retryExpired)) ?
                       "retry limit exceeded" :
                       ((int)pkts.size() >= rcvpacketbuffersize) ?
                       "packet buffer limit exceeded" : "");
               addLog (buf);
            }
         #ifdef DBUG
            // status message
            cout << "have to skip (missing " << missingPackets << "): ";
            if (missingPackets > maximumRetransmit) {
               cout << "max. retransm. "; 
            }
            if (retransmissionRate > qosFrac) {
               cout << "QoS limit "; 
            }
            if ((retry >= maxRetry) && (retryExpired)) {
               cout << "retry limit "; 
            }
            if ((int)pkts.size() >= rcvpacketbuffersize) {
               cout << "pkt buffer limit "; 
            }
            cout << endl;
         #endif
            // skip sequence if too many packets are missing or
            // if too many retries or if packet buffer is full
            if (!pkts.empty()) {
               newseq =  pkts.back()->header.seq + 1;
            }
            else {
               newseq++;
            }
            pkts.clear();
            firstPacket = true;
            retry = 0;
            len = 0;
            seqDone = false;
            retransmissionRate = newRetransmissionRate;
	    abort();
         }
         else if ((retry == 0) || (retryExpired)) {
            // calculate new retransmission rate
            if (retry == 0) {
               newRetransmissionRate = 
                  (1.0 - 1.0/retransmissionAverage) * retransmissionRate + 
                  1.0/retransmissionAverage * 
                  (double) missingPackets / pktCntrl.size();
            }
            // ask for retransmit
            retransmitpacket	rpkt;
            memset (&rpkt.header, 0, sizeof (packetHeader));
            rpkt.header.pktType = PKT_REQUEST_RETRANSMIT;
            rpkt.header.seq = newseq;
            rpkt.header.pktNum = 0;
            rpkt.header.pktTotal = 1;
            int n = 0;
            int i = 0;
            for (vector<packetControl>::iterator iter = pktCntrl.begin();
                iter != pktCntrl.end(); iter++, i++) {
               // check if packet has to be resent
               if (!iter->received) {
                  rpkt.pktResend[n++] = i;
                  // check if packet is full
                  if (n >= packetSize / (int)sizeof (int32_t)) {
                     break;
                  }
               }
            }
            if (logison) {
/*
               char buf[256];
               sprintf (buf, "Ask for retransmission of %i packets; port %d", n, port);
		
               addLog (buf);
*/
               system_log(1, "Ask for retransmission of %i packets; port %d", n, port);		
            }
         #ifdef DBUG
            cout << "ask for retransmission " << n << " (rate " <<
               newRetransmissionRate << "; seq " << newseq << ") at " << endl;
               //(TAInow() % 1000000000000LL) / 1E9 << endl;
         #endif
         #if DBUG - 0 > 1
            cout << "Packets =";
            for (int jj = 0; jj < n; ++jj) {
               cout << " " << rpkt.pktResend[jj];
            }
            cout << endl;
         #endif
            rpkt.header.pktLen = n * sizeof (int32_t);
            if (!putPacket (rpkt)) {
               return -6;
            }
         
            // start new timer
            retryExpired = false;
            timeval tp;
            if (gettimeofday (&tp, 0) < 0) {
               timestamp = 0;
            }
            else {
               timestamp = (long long) tp.tv_sec * 1000000 + 
                  (long long) tp.tv_usec;
            }
            retry++;
         }
      
         requestPacket = true;
      };
   
      // never reached
      return false;
   }


   bool frameRecv::getPacket (bool block)
   {
      #if DBUG > 1
      //cout << "packet buffer size = " << pkts.size() << endl;
      #endif
      // check if enough space for a new packet
      if ((int)pkts.size() >= rcvpacketbuffersize) {
         if (logison) {
            addLog ("Packet buffer is full");
         }
      #ifdef DBUG
         cout << "packet buffer full" << endl;
      #endif
	 abort();
         return false;
      }
   
      // allocate pakcet buffer
      auto_pkt_ptr	pkt = auto_pkt_ptr (new (nothrow) packet);
      if (pkt.get() == 0) {
         return false;
      }
   
      // receive a packet from socket
      int n;
      do {
         // if not blocking poll socket
         if (!block) {
            timeval 	wait;		// timeout=0
            wait.tv_sec = 0;
            wait.tv_usec = 0;
            fd_set 		readfds;	
            FD_ZERO (&readfds);
            FD_SET (sock, &readfds);
            int nset = select (FD_SETSIZE, &readfds, 0, 0, &wait);
            if (nset < 0) {
            #ifdef DBUG
               cout << "select on receiver socket failed" << endl;
            #endif
               return false;
            }
            else if (nset == 0) {
               return false;
            }
         }
      	 // read packet
         socklen_t max = sizeof (name);
         n = recvfrom (sock, (char*) pkt.get(), sizeof (packet), 0, 
                      (struct sockaddr*) &name, &max);
         if (n < 0) {
            return false;
         }
         // swap if necessary
         pkt->ntoh();
#if DBUG
	printf("packet type=%d received\n", pkt->header.pktType);
#endif
      // repeat until valid packet
      } while (n < (int)sizeof (packetHeader) || 
              ((pkt->header.pktType != PKT_BROADCAST) && 
              (pkt->header.pktType != PKT_REBROADCAST)) ||
              (n != (int)sizeof (packetHeader) + pkt->header.pktLen));
   
      #if DBUG - 0 > 1
         cout << "received packet " << pkt->header.pktNum << 
             (pkt->header.pktType == PKT_REBROADCAST ? "R " : " ") <<
             " seq=" << pkt->header.seq << "time=" << pkt->header.timestamp
		<< "duration=" << pkt->header.duration << endl;
      #endif 

#if 0
// Drop some packets in their entirety
// This is what we see on Solaris x86, but not on Linux
//
		  if (drop_seq) {
			if (pkt->header.seq == drop_seq) {
				printf("dropping seq=%d pkt=%d\n", pkt->header.seq, pkt->header.pktNum);
				return true;
			} else drop_seq = 0;
		  } else  if ((double)rand() < .2 * RAND_MAX) {
			if (pkt->header.pktNum == 0) {
				drop_seq = pkt->header.seq;
				printf("dropping seq=%d pkt=%d\n", pkt->header.seq, pkt->header.pktNum);
				return true;
			}
                  }
#endif

      // now add packet to packet buffer
      packetlist::iterator pos = lower_bound (pkts.begin(), pkts.end(), pkt);
      // check if end of list
      if (pos == pkts.end()) {
         pkts.push_back (pkt);
      }
      // check if duplicate
      else if (*pos == pkt) {
         // do nothing
      }
      // check if front of list
      else if (pos == pkts.begin()) {
         pkts.push_front (pkt);
      }
      // otherwise insert at position
      else {
         pkts.insert (pos, pkt);
      }
      return true;
   }


   bool frameRecv::putPacket (retransmitpacket& pkt)
   {
      int sbytes = sizeof (packetHeader) + pkt.header.pktLen;
      // swap if necessary
      pkt.hton();
      if (sendto (sock, (char*) &pkt, sbytes, 0, 
                 (struct sockaddr*) &name, 
                 sizeof (struct sockaddr_in)) != sbytes) {
         ::close (sock);
         sock = -1;
         return false;
      }
      else {
         return true;
      }
   }


   const char* frameRecv::logmsg ()
   {
      return copyLog();
   }

   void frameRecv::addLog (const string& s)
   {
	//cerr << s << endl;
	printf("%s\n", s.c_str());
#if 0
      logs.push_back (s + "\n");
      if ((int)logs.size() > maxlog) {
         logs.pop_front();
      }
#endif
   }

   void frameRecv::addLog (const char* s)
   {
	//cerr << s << endl;
      string l (s);
      addLog (l);
   }

   const char* frameRecv::copyLog ()
   {
      logbuf = "framexmit log:\n";
      for (deque<string>::iterator i = logs.begin(); 
          i != logs.end(); i++) {
         logbuf += *i;
      }
      return logbuf.c_str();
   }

   void frameRecv::clearlog ()
   {
      logs.clear();
      logbuf = "";
   }


}
