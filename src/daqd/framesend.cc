#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
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
#include <pthread.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sys/syscall.h>
#include <sys/prctl.h>

#include "debug.h"
#undef DEBUG

#include "framesend.hh"

unsigned long dmt_retransmit_count = 0;
unsigned long dmt_failed_retransmit_count = 0;


   using namespace std;

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

diag::mutex cmnInUseMux;

   struct interface_t {
      char		name[IFNAMSIZ];
      in_addr_t		addr;
   };
   typedef vector<interface_t> interfaceList;


   static bool getInterfaces (int sock, interfaceList& iList) 
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
          cout << "found " << iface.name << " (" << 
             inet_ntoa (*(in_addr*)&iface.addr) << ")" << endl;
         iList.push_back (iface);
      } 
   
      return true;
   }


   static bool matchInterface (int sock, const char* net, in_addr_t& i_addr)
   {
      // check net address
      if ((net == 0) || (inet_addr (net) == (unsigned long)-1)) {
         // use default host address
         char		myname[256];	// local host name
         hostent 	hostinfo;
         char		buf[1024];
         int		my_errno;
         if ((gethostname (myname, sizeof (myname)) != 0) ||
            (__gethostbyname_r (myname, &hostinfo, buf, 1023, &my_errno)) == 0) {
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
            if ((i->addr > inet_addr (net)) && 
               (i->addr - inet_addr (net) < diff)) {
               i_addr = i->addr;
               diff = i->addr - inet_addr (net);
//printf("matchInterface(): %d; diff %d\n", i_addr, diff);
            }
         }
         return (i_addr != 0);
      }
   }



   frameSend::buffer::buffer ()
   : seq (0), own (false), data (0), len (0), used (0), inUseMux (0),
   timestamp (0), duration (0), sofar (0) {
   }

   frameSend::buffer::buffer (const buffer& buf) 
   {
      *this = buf;
   }

   frameSend::buffer::buffer (char* Data, int Len, unsigned int Seq, 
                     bool Own, bool* Used, mutex* InUseMux,
                     unsigned int Timestamp, unsigned int Duration) 
   : seq (Seq), own (Own), data (Data), len (Len), used (Used), 
   inUseMux (InUseMux), timestamp (Timestamp), duration (Duration), 
   sofar (0) {
   }

   frameSend::buffer::~buffer () 
   {
      if (own) {
         delete [] data;
      }
      if (inUseMux != 0) {
         inUseMux->lock();
      }
      if (used != 0) {
         *used = false;
      }
      if (inUseMux != 0) {
         inUseMux->unlock();
      }
   }

   frameSend::buffer& frameSend::buffer::operator= (const buffer& buf) 
   {
      if (this != &buf) {
         own = buf.own;
         seq = buf.seq;
         data = buf.data;
         len = buf.len;
         timestamp = buf.timestamp;
         duration = buf.duration;
         sofar = buf.sofar;
         used = buf.used;
         inUseMux = buf.inUseMux;
         buf.own = false;
         buf.used = 0;
      }
      return *this;
   }


   void xmitDaemonCallback (frameSend& th) 
   {
      th.xmitDaemon();
   }

extern "C" 
   void xmitDaemonCallback2 (frameSend& th) 
   {
      xmitDaemonCallback (th);
   }

   bool frameSend::open (const char* dest_addr, const char* interface, 
                     int port)
   {
      if (sock >= 0) {
         close();
      }
   
//printf("frameSend::open(dest_addr=%s, interface=%s)\n", dest_addr, interface);

      // set destination address
      struct hostent 	hostinfo;
      in_addr_t		addr;
      char		buf[1024];
      int		my_errno;
   
      if (__gethostbyname_r (dest_addr, &hostinfo, buf, 
                           1023, &my_errno) != 0) {
         memcpy (&addr, hostinfo.h_addr_list[0], sizeof (addr));
      }
      else {
         return false;
      }
      name.sin_addr.s_addr = addr;
      name.sin_family = AF_INET;
      name.sin_port = htons (port);
   
      // open socket
      sock = socket (PF_INET, SOCK_DGRAM, 0);
      if (sock < 0) {
         return false;
      }
   
      /* set receive buffer size */
      int bufsize = sndInBuffersize;
      if (setsockopt (sock, SOL_SOCKET, SO_RCVBUF, 
                     (char*) &bufsize, sizeof (int)) != 0) {
         ::close (sock);
         sock = -1;
         return false;
      }
   
      /* set send buffer size */
      bufsize = sndOutBuffersize;
      if (setsockopt (sock, SOL_SOCKET, SO_SNDBUF, 
                     (char*) &bufsize, sizeof (int)) != 0) {
         ::close (sock);
         sock = -1;
         return false;
      }
   
      // bind socket
      sockaddr_in name2;
      name2.sin_family = AF_INET;
      name2.sin_port = 0;
      name2.sin_addr.s_addr = htonl (INADDR_ANY);
      if (::bind (sock, (struct sockaddr*) &name2, sizeof (name2))) {
         ::close (sock); 
         sock = -1;
         return false;
      }
   
      // broadcast/multicast options
      if (IN_MULTICAST (addr)) {
         multicast = true;
         // multicast: set number of hopes
         unsigned char ttl = mcast_TTL;
         if (setsockopt (sock, IPPROTO_IP, IP_MULTICAST_TTL, 
                        (char*) &ttl, sizeof (ttl)) == -1) {
            ::close (sock);
            sock = -1;
            return false;
         }
         // multicast: disable loopback
         char loopback = 0;
         if (setsockopt (sock, IPPROTO_IP, IP_MULTICAST_LOOP, 
                        (char*) &loopback, sizeof (loopback)) == -1) {
            ::close (sock);
            sock = -1;
            return false;
         }
      	 // specify interface
         in_addr_t i_addr;
         if (!matchInterface (sock, interface, i_addr) ||
            (setsockopt (sock, IPPROTO_IP, IP_MULTICAST_IF, 
                        (char*) &i_addr, sizeof (i_addr)) == -1)) {
            ::close (sock);
            sock = -1;
            return false;
         }	 
      }
      else {
         multicast = false;
	 //printf("Broadcast enabled\n");
         // enable broadcast
         int bset = 1;
         if (setsockopt (sock, SOL_SOCKET, SO_BROADCAST, 
                        (char*) &bset, sizeof (bset)) == -1) {
            ::close (sock);
            sock = -1;
            return false;
         }
      }
   
      // clear buffers
      mux.lock();
      buffers.clear();
      curbuf = -1;
      skippedDataBuffers = 0;
      mux.unlock();
   
      // create transmit daemon
      int attr = PTHREAD_SCOPE_SYSTEM | PTHREAD_CREATE_DETACHED;
      if (taskCreate (attr, daemonPriority, &daemon, "tXmit",
                     (taskfunc_t) xmitDaemonCallback2, (taskarg_t) this) < 0) {
         ::close (sock);
         sock = -1;
         return false;
      } 
   
      return true;
   }


   void frameSend::close ()
   {
      if (sock < 0) {
         return;
      }
   
      mux.lock();
      buffers.clear();
      curbuf = -1;
      mux.unlock();
   
      taskCancel (&daemon);  
   
      ::close (sock);
      sock = -1;
   }


   bool frameSend::send (buffer& data)
   {
      semlock		lockit (mux);
      typedef deque<buffer>::size_type size_type;
   
      //printf("Bcast buffers %d; max %d\n", buffers.size(), maxbuffers);

      // add send buffer to list
      if (curbuf == -1) {
         // no buffer in list
         buffers.push_back (data);
         curbuf = 0;
      }
      else if (curbuf >= maxbuffers / 2) {
         // too many old buffers
         buffers.pop_front ();
         curbuf--;
         buffers.push_back (data);
      }
      else if (buffers.size() < (size_type) maxbuffers) {
         // enough free space
         buffers.push_back (data);
      }
      else {
         // overload! flush pipe
         while (buffers.size() > (size_type) curbuf + 1) {
            buffers.pop_back();
            skippedDataBuffers++;
         }
         buffers.push_back (data);
         cout << "flush pipe" << endl;
	 abort();
      }
   
      return true;
   }


   bool frameSend::send (char* data, int len, bool* used, bool copy,
                     unsigned int timestamp, unsigned int duration)
   {
      if ((sock < 0) || (data == 0)) {
         return false;
      }
   
      // copy data if necessary
      char* p;
      if (copy) {
         p = new (std::nothrow) char [len];
         if (p == 0) {
            return false;
         }
         memcpy (p, data, len);
      }
      else  {
         p = data;
      }
   
      // set in use variable
      cmnInUseMux.lock();

      if (used != 0) {
         *used = true;
      }
      cmnInUseMux.unlock();
   
      // fill into buffer
      buffer buf (p, len, seq++, copy, used, &cmnInUseMux, timestamp, duration);
      return send (buf);
   }


   bool frameSend::getRetransmitPacket (retransmitpacket& pkt)
   {
      // poll socket
      struct timeval 	wait;		// timeout=0
      wait.tv_sec = 0;
      wait.tv_usec = 0;
      fd_set 		readfds;	
      FD_ZERO (&readfds);
      FD_SET (sock, &readfds);
      int nset = select (FD_SETSIZE, &readfds, 0, 0, &wait);
      if (nset < 0) {
         return false;
      }
      else if (nset == 0) {
         return false;
      }
   
      // receive a packet from socket
      struct sockaddr_in 	from;
      int n;
      socklen_t max = sizeof (from);
      n = recvfrom (sock, (char*) &pkt, sizeof (packet), 0, 
                   (struct sockaddr*) &from, &max);
      if (n < 0) {
         return false;
      }
      // swap if necessary
      pkt.ntoh();
      // check if valid retransmit packet
      if (n < (int)sizeof (packetHeader) || 
         (pkt.header.pktType != PKT_REQUEST_RETRANSMIT) ||
         (n != (int)sizeof (packetHeader) + pkt.header.pktLen)) {
         return false;
      }
      else {
         return true;
      }
   }


   bool frameSend::putPackets (packet pkts[], int n)
   {
      static int pktnum = 0;
   
      for (int i = 0; i < n; i++) {
         // send it
         int sbytes = sizeof (packetHeader) + pkts[i].header.pktLen;

         // swap if necessary
         pkts[i].hton();
         if (sendto (sock, (char*) (pkts + i), sbytes, 0, 
                    (struct sockaddr*) &name, 
                    sizeof (struct sockaddr_in)) != sbytes) {
            ::close (sock);
            sock = -1;
            return false;
         }
         pktnum++;
         if (pktnum == packetBurst) {
            if (packetBurstInterval > 0) {
               struct timespec wait;
               wait.tv_sec = packetBurstInterval / 1000000;
               wait.tv_nsec = 1000 * (packetBurstInterval % 1000000);
               nanosleep (&wait, 0);
            }
            pktnum = 0;
         }
      }
      return true;
   }


   bool frameSend::compSeqeuence (const frameSend::buffer& b, 
                     const retransmitpacket& p)
   {
      return (b.seq < p.header.seq);
   }


extern "C" {
   typedef void (*cleanup_type)(void*);

   void xmitDaemonCleanup (packet* pkts)
   {
      delete [] pkts;
   }
}

   void frameSend::xmitDaemon ()
   {
      // alocate packet buffer/install cleanup function
      packet*	pkts = 0;
      pthread_cleanup_push ((cleanup_type)xmitDaemonCleanup, pkts);
      pkts = new (nothrow) packet [
         max (packetBurst, maximumRetransmit)];
      if (pkts == 0) {
         ::close (sock);
         sock = -1;
         return;
      }
      // Name the thread
      pid_t my_tid;
      char my_thr_label[16]="dqxmit";
      my_tid = (pid_t) syscall(SYS_gettid);
      prctl(PR_SET_NAME,my_thr_label,0, 0, 0);
      system_log(1, "Broadcast transmit thread - label %s pid=%d\n", my_thr_label, (int) my_tid);

     // main loop
      while (1) {
         pthread_testcancel();
      
         // check if buffer is ready
         mux.lock();
         if ((curbuf != -1) && (curbuf < (int)buffers.size())) {
         
            // assemble packets
            int n;
            int pktTotal = (buffers[curbuf].len + packetSize - 1) / packetSize;
            int pktNum = buffers[curbuf].sofar / packetSize;
            for (n = 0; (n < packetBurst) && (pktNum + n < pktTotal); n++) {
               memset (&pkts[n].header, 0, sizeof (packetHeader));
               pkts[n].header.pktType = PKT_BROADCAST;
               pkts[n].header.seq = buffers[curbuf].seq;
               pkts[n].header.timestamp = buffers[curbuf].timestamp;
               pkts[n].header.duration = buffers[curbuf].duration;
               pkts[n].header.pktTotal = pktTotal;
               pkts[n].header.pktNum = pktNum + n;
               pkts[n].header.pktLen = (pktNum + n + 1 == pktTotal) ?
                  (buffers[curbuf].len -  buffers[curbuf].sofar) : packetSize;
               memcpy (pkts[n].payload, buffers[curbuf].data + buffers[curbuf].sofar, 
                      pkts[n].header.pktLen);
               buffers[curbuf].sofar += pkts[n].header.pktLen;
            }
            // check if buffer is done
            if (buffers[curbuf].sofar >= buffers[curbuf].len) {
               curbuf++;
            }
            mux.unlock();
         
            // now send n packets
            if (!putPackets (pkts, n)) {
            #ifdef DEBUG
               cout << "packet error 1" << endl;
            #endif
               break;
            }
         }
         else {
            mux.unlock();
            timespec	wait = {(1000LL * sndDelayTick) / 1000000000LL, 
               (1000LL * sndDelayTick) % 1000000000LL};
            nanosleep (&wait, 0);
         }
      
      	 // check if retransmit packets are here
         retransmitpacket rpkt;
         if (!getRetransmitPacket (rpkt)) {
            continue;
         }
      #ifdef DEBUG
         cout << "received a retransmit packet " << 
            rpkt.header.pktLen / sizeof (int) << endl;
      #endif
      
      	 // check if buffers are available
         mux.lock();
         if (curbuf == -1) {
            mux.unlock();
            continue;
         }
      	 // find buffer with same sequence
         deque<buffer>::iterator pos = 
            lower_bound (buffers.begin(), buffers.end(), rpkt, compSeqeuence);
         if ((pos == buffers.end()) ||
            (pos->seq != rpkt.header.seq)) {
            mux.unlock();
         #ifdef DEBUG
            cout << "old data no longer available" << endl;
         #endif
            dmt_failed_retransmit_count++;
            continue;
         }
      	 // retransmit
         packet pkt;
         int n = 0;
         memset (&pkt.header, 0, sizeof (packetHeader));
         //pkts[n].header.timestamp = pos->timestamp;
         pkt.header.timestamp = pos->timestamp;
         //pkts[n].header.duration = pos->duration;
         pkt.header.duration = pos->duration;
         pkt.header.pktType = PKT_REBROADCAST;
         pkt.header.pktTotal = (pos->len + packetSize - 1) / packetSize;
         if (rpkt.header.pktLen / (int)sizeof (int) > maximumRetransmit) {
            mux.unlock();
            continue;
         }
         pkt.header.seq = rpkt.header.seq;
         for (int i = rpkt.header.pktLen / sizeof (int) - 1; i >= 0; i--) {
            // assemble retransmit packet
            pkt.header.pktNum = rpkt.pktResend[i];
            if (pkt.header.pktNum < 0) {
               continue;
            }
            pkt.header.pktLen = (pkt.header.pktNum + 1 >= pkt.header.pktTotal) ?
               (pos->len -  pkt.header.pktNum * packetSize) : packetSize;
            if ((pkt.header.pktLen <= 0) || 
               (pkt.header.pktLen > packetSize)) {
               continue;
            }
            memcpy (&pkts[n].header, &pkt.header, sizeof (packetHeader));
            memcpy (pkts[n].payload, pos->data + pkt.header.pktNum * packetSize, 
                   pkt.header.pktLen);
            n++;
         }
         mux.unlock();
      
      	 // send packets
      #ifdef DEBUG
         cout << "send retransmit packet " << n << " time=" << pos->timestamp
		<< " duration=" << pos->duration << endl;
      #endif

         dmt_retransmit_count++;

         if (!putPackets (pkts, n)) {
         #ifdef DEBUG
            cout << "packet error 2" << endl;
         #endif
            break;
         }
      }
   
      // remove cleanup function
      pthread_cleanup_pop (1);
   }


}
