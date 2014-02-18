static const char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: channelinput						*/
/*                                                         		*/
/* Module Description: Manages an input channel				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

//#define DEBUG

// Header File List:
#include "dtt/channelinput.hh"
#include <math.h>
#include <errno.h>
#include <iostream>
#include <vector>
#include <iterator>
#include <algorithm>
#include "gdsconst.h"
#include "decimate.h"
#include "gdssigproc.h"
#include "dtt/gdstask.h"
#include "tconv.h"
#include "dtt/map.h"

namespace diag {
   using namespace std;
   using namespace thread;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: decimationflag	  flags used for decimation filter	*/
/*            __ONESEC		  one second (in nsec)			*/
/*            _PREPROC_STARTUP	  startup factor for preprocessing	*/
/*            _PREPROC_CONTINUE	  continuation factor for preprocessing	*/
/*            taskDataUpdateName  nds update task priority		*/
/*            taskDataUpdatePriority nds update task name		*/
/*            								*/
/*----------------------------------------------------------------------*/
#define __ONESEC		1E9
#define _PREPROC_STARTUP	10
#define _PREPROC_CONTINUE	2
   const int	kDecimationflag = 1;
   const int	kDecimationflagZoom = 4;
   const char	taskDataUpdateName[] = "tNDSupdate";
   const int	taskDataUpdatePriority = 4;

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Class Name: chnCallback						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   chnCallback::chnCallback (const string& Chnname) 
   : chnname (Chnname), idnum (-1) 
   {
   }

   chnCallback::~chnCallback () 
   {
      unsubscribe ();
   }


   chnCallback::chnCallback (const chnCallback& chncb) 
   {
      *this = chncb;
   }


   chnCallback& chnCallback::operator= (const chnCallback& chncb) 
   {
      if (this != &chncb) {
         semlock	lockit (mux);
         idnum = -1;
         chnname = chncb.chnname;
      }
      return *this;
   }


   bool chnCallback::subscribe (tainsec_t start, 
                     tainsec_t* active) 
   {
      semlock		lockit (mux);	// lock mutex
   
      // check if already subscribed
      if (idnum >= 0) {
         return true;
      }
      // subscribe
   #if !defined (_RTDD_PROVIDER) || (_RTDD_PROVIDER  == 2)
      idnum = 1;
   #else
      idnum = gdsSubscribeData (chnname.c_str(), 0, 
                           (gdsDataCallback_t) callbackC, this);
   #endif
      // set active time
      if (active != 0) {
         *active = max (start, TAInow());
      }
      return (idnum >= 0);
   }


   bool chnCallback::unsubscribe () 
   {
      semlock		lockit (mux);	// lock mutex
   
      // unsubscribe
      if (idnum >= 0) {
      #if defined (_RTDD_PROVIDER) && (_RTDD_PROVIDER  != 2)
         gdsUnsubscribeData (idnum);
      #endif
         idnum = -1;
      }
      return true;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Class Name: dataChannel::partition					*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   dataChannel::partition::partition (
                     string Name, tainsec_t Start, tainsec_t Duration, 
                     double Dt, tainsec_t tp) 
   : name (Name), start (Start), duration (Duration), 
   precursor (tp), dt (Dt), decimate1 (1), decimate2 (1),
   zoomstart (0), zoomfreq (0), removeDelay (true), 
   decdelay (0.0), delaytaps (0), timedelay (0.0), done (false)
   {
      length = (int) ((double) duration / _ONESEC / dt + 0.5);
   }


   dataChannel::partition::partition (const partition& p) 
   {
      *this = p;
   }


   void dataChannel::partition::setup (double Dt, 
                     int Decimate1, int Decimate2, 
                     tainsec_t Zoomstart, double Zoomfreq, bool rmvDelay)
   {
      dt = Dt;
      decimate1 = Decimate1;
      decimate2 = Decimate2;
      zoomstart = Zoomstart;
      zoomfreq = Zoomfreq;
      removeDelay = rmvDelay;
      length = (int) ((double) duration / _ONESEC / dt + 0.5);
   }


   void dataChannel::partition::fill (
                     const float data[], int len, int bufnum) 
   {
   #ifdef DEBUG
      cerr << "size = " << buf[bufnum].size() << " len = " << len << endl;
   #endif
      if ((bufnum < 0) || (bufnum >= 2)) {
         return;
      }
      int ncopy = min (len, length - (int) buf[bufnum].size());
      buf[bufnum].insert (buf[bufnum].end(), data, data + ncopy);
   #ifdef DEBUG
      cerr << "size = " << buf[bufnum].size() << " length = " << length << endl;
   #endif
   }


   void dataChannel::partition::copy (
                     float data[], int max, bool cmplx) 
   {
      int 		len;          
      if (cmplx) {
         len = min (buf[0].size(), buf[1].size());
      } 
      else {
         len = buf[0].size();
      }
   
      for (int i = 0; i < len; i++) {
         if (i >= max) {
            return;
         }
         if (!cmplx) {
            data[i] = buf[0][i];
         }
         else {
            data[2*i] = buf[0][i];
            data[2*i+1] = buf[1][i];
         }
      }
   }


   int dataChannel::partition::index (
                     tainsec_t Start, int Length) const
   {
      tainsec_t 	time;	// time of first sample needed
      time = start + (tainsec_t) (buf[0].size() * dt * _ONESEC);
   #ifdef DEBUG
      cerr << time << ":" << Start << "|" << Start - time << endl;
   #endif
      if (time < Start - (tainsec_t) (_ONESEC * dt / 2.0)) {
         // failure: gap in data
         cerr << "gap in data dt = " << (Start - time) / 1E9 << endl;
         return -2;
      }
      else if (time >= Start + Length * dt * _ONESEC) {
         // not yet
         return -1;
      }
      else {
         return (int) ((double) (time - Start) / 
                      (double) _ONESEC / dt + 0.5);
      }
   }


   int dataChannel::partition::range (
                     tainsec_t Start, int Length) const       
   {  
      int		ndx;
   
      if ((ndx = index (Start, Length)) < 0) {
         return 0;
      }
      else {
      #ifdef DEBUG
         cerr << "Length = " << Length << " length = " << length <<
            " buf[0].size = " << buf[0].size() << endl;
      #endif
         return min (Length - ndx, length - (int) buf[0].size());
      }
   }


   inline void dataChannel::partition::nomore ()
   {
      done = true;
   }


   inline bool dataChannel::partition::isFull () const 
   {                 
      return ((int)buf[0].size() >= length);
   }


   inline bool dataChannel::partition::isDone () const      
   {                 
      return isFull() || done;
   }


   inline bool dataChannel::partition::operator< 
   (const partition& p) const 
   {           
      return (start < p.start);
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Class Name: dataChannel::preprocessing				*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   dataChannel::preprocessing::preprocessing (int dataRate, 
                     int Decimate1, int Decimate2, 
                     tainsec_t Zoomstart, double Zoomfreq,
                     bool rmvDelay)
   : datarate (dataRate), cmplx (Decimate1 < 0), 
   decimate1 (Decimate1), decimate2 (Decimate2),
   decimationflag (kDecimationflag), zoomstart (Zoomstart), 
   zoomfreq (Zoomfreq), removeDelay (rmvDelay), 
   useActiveTime (false), start (0), stop (0), bufTime (0), bufSize (0),
   t0Grid (0), buf (0), hpinit (false), hpval (0), 
   tmpdelay1 (0), tmpdec1 (0), tmpdec2I (0), tmpdec2Q (0)
   {
      float		tmp[1];
      if (cmplx) decimate1 = 1;
   
      // set time constants
      dt[0] = 1.0 / (double) datarate;
      dt[1] = dt[0] * decimate1;
      dt[2] = dt[1] * decimate2;
   
      // set decimation filter delay constants
      if (zoomfreq != 0) {
         decimationflag = kDecimationflagZoom;
      #ifdef DEBUG
         cerr << "Decimatoin flag for zoom = " << decimationflag << endl;
      #endif
      }
      int dec = decimate1 * decimate2;
      decdelay = firphase (decimationflag, dec) / TWO_PI * dt[0];
      if (removeDelay) {
         int sDelay = (int)(firphase (decimationflag, dec) / TWO_PI + 0.5);
         int tDelay = dec * (int)((sDelay + dec - 1) / dec);
         delaytaps = tDelay - sDelay;
         delayshift = 
            (tainsec_t) ((double) tDelay * dt[0] * __ONESEC + 0.5);
         delay1 = (tainsec_t) 
            ((firphase (decimationflag, decimate1) / TWO_PI +
             (double) delaytaps) * dt[0] * __ONESEC + 0.5);
      #ifdef DEBUG
         cerr << "dec=" << dec << " sD=" << sDelay << " tD=" << tDelay << 
            " dtaps=" << delaytaps << " dshift=" << delayshift <<
            " decd=" << decdelay << " d1=" << delay1 << endl;
      #endif
      }
      else {
         delaytaps = 0;
         delayshift = 0;
         delay1 = 0;
      }
   
      // new memory for buffer
      if (dec >= 0) {
         buf = new (nothrow) float [(cmplx ? 2 : 1) * dec];
      }
   
      // new temp memory for delay / decimation filter
      timedelay (tmp, tmp, 0, (cmplx ? 2 : 1) * delaytaps, 0, &tmpdelay1);
      decimate (decimationflag, tmp, tmp, 0, decimate1, 0, &tmpdec1);
      decimate (decimationflag, tmp, tmp, 0, decimate2, 0, &tmpdec2I);
      decimate (decimationflag, tmp, tmp, 0, decimate2, 0, &tmpdec2Q);
   }


   dataChannel::preprocessing::preprocessing (const preprocessing& p) 
   : buf (0), hpinit (false), hpval (0), tmpdelay1 (0), tmpdec1 (0), 
   tmpdec2I (0), tmpdec2Q (0)
   {
      *this = p;
   }


   dataChannel::preprocessing& dataChannel::preprocessing::operator= (
                     const preprocessing& p)
   {
      if (this != &p) {
         float		tmp[1];
      
         // release buffer memory
         if (buf != 0) {
            delete [] buf;
         }
         // release old temp memory for decimation filter
         if (tmpdelay1 != 0) {
            timedelay (tmp, tmp, 0,  (cmplx ? 2 : 1) * delaytaps, tmpdelay1, 0);
            tmpdelay1 = 0;
         }
         if (tmpdec1 != 0) {
            decimate (decimationflag, tmp, tmp, 0, decimate1, tmpdec1, 0);
            tmpdec1 = 0;
         }
         if (tmpdec2I != 0) {
            decimate (decimationflag, tmp, tmp, 0, decimate2, tmpdec2I, 0);
            tmpdec2I = 0;
         }
         if (tmpdec2Q != 0) {
            decimate (decimationflag, tmp, tmp, 0, decimate2, tmpdec2Q, 0);
            tmpdec2Q = 0;
         }
      
         // copy parameters
         datarate = p.datarate;
         cmplx = p.cmplx;
         decimate1 = p.decimate1;
         decimate2 = p.decimate2;
         decimationflag = p.decimationflag;
         zoomstart = p.zoomstart;
         zoomfreq = p.zoomfreq;
         memcpy (dt, p.dt, sizeof (dt));
         removeDelay = p.removeDelay;
         decdelay = p.decdelay;
         delaytaps = p.delaytaps;
         delayshift = p.delayshift;
         delay1 = p.delay1;
         useActiveTime = p.useActiveTime;
         start = p.start;
         stop = p.stop;
         bufTime = p.bufTime;
         bufSize = p.bufSize;
         t0Grid = p.t0Grid;
      
         // new memory for buffer
         if (decimate1 * decimate2 >= 0) {
            buf = new (nothrow) float [(cmplx ? 2 : 1) * decimate1 * decimate2];
         }
      
         // new temp memory for decimation filter
         hpinit = false; hpval = 0;
         timedelay (tmp, tmp, 0,  (cmplx ? 2 : 1) * delaytaps, 0, &tmpdelay1);
         decimate (decimationflag, tmp, tmp, 0, decimate1, 0, &tmpdec1);
         decimate (decimationflag, tmp, tmp, 0, decimate2, 0, &tmpdec2I);
         decimate (decimationflag, tmp, tmp, 0, decimate2, 0, &tmpdec2Q);
      }
      return *this;
   }


   dataChannel::preprocessing::~preprocessing ()
   {
      // release buffer memory
      if (buf != 0) {
         delete [] buf;
      }
      // release temp memory for decimation filter
      float		tmp[1];
      if (tmpdelay1 != 0) {
         timedelay (tmp, tmp, 0,  (cmplx ? 2 : 1) * delaytaps, tmpdelay1, 0);
         tmpdelay1 = 0;
      }
      if (tmpdec1 != 0) {
         decimate (decimationflag, tmp, tmp, 0, decimate1, tmpdec1, 0);
         tmpdec1 = 0;
      }
      if (tmpdec2I != 0) {
         decimate (decimationflag, tmp, tmp, 0, decimate2, tmpdec2I, 0);
         tmpdec2I = 0;
      }
      if (tmpdec2Q != 0) {
         decimate (decimationflag, tmp, tmp, 0, decimate2, tmpdec2Q, 0);
         tmpdec2Q = 0;
      }
   }


   bool dataChannel::preprocessing::operator== (
                     const preprocessing& pre) const
   {
      return  (cmplx == pre.cmplx) &&
         (decimate1 == pre.decimate1) && 
         (decimate2 == pre.decimate2) &&
         (decimationflag == pre.decimationflag) &&
         (zoomstart == pre.zoomstart) &&
         (fabs (zoomfreq - pre.zoomfreq) < 1E-6) &&
         (removeDelay == pre.removeDelay);
   }


   void dataChannel::preprocessing::setActiveTime (
                     tainsec_t Start, tainsec_t Stop,
                     bool obey, bool newValues)
   {
      if (newValues) {
         start = Start;
         stop = Stop;
      }
      else {
         if (start == 0) {
            start = Start;
         }
         else if (Start == -1) {
            start = -1;
         }
         else if (start != -1) {
            start = min (start, Start);
         }
         if (stop == 0) {
            stop = Stop;
         }
         else if (Stop == -1) {
            stop = -1;
         }
         else if (stop != -1) {
            stop = max (stop, Stop);
         }
      }
      useActiveTime = obey;
   }


   bool dataChannel::preprocessing::operator() (
                     taisec_t time, int epoch,
                     float data[], int ndata, int err,
                     partitionlist& partitions, 
                     mutex& mux, bool& update)
   {
      semlock		lockit (mux);
      int		dec1 = decimate1;
      int		dec2 = decimate2;
      int		dec = dec1 * dec2;
   
      // not enough data points for decimation
      if (ndata < dec) {
         // cerr << "not enough data points (" << ndata << "|" <<
            // bufSize << "|" << dec << ")" << endl;
         // check buffer allocation & size
         if ((buf == 0) || (ndata + bufSize > dec)) {
            bufSize = 0;
            return false;
         }
         // caluclate buffer epoch
         tainsec_t dStart = (tainsec_t) time * _ONESEC + epoch * _EPOCH;
         int	   bufEpoch;
         if (dt[2] >= 1.0 - 1E-12) {
            tainsec_t 	step = (tainsec_t) (dt[2] + 0.5);
            tainsec_t	tsec = 
               step * ((dStart - t0Grid) / _ONESEC / step);
            double rest = (dStart - t0Grid - tsec * _ONESEC) / __ONESEC;
            bufEpoch = (int) (rest / (ndata * dt[0]) + 0.5);
         }
         else {
            tainsec_t	tnsec  = (dStart - t0Grid) % _ONESEC;
            bufEpoch = (int) (((double) tnsec / __ONESEC) / 
                             (ndata * dt[0]) + 0.5);
            bufEpoch %= (int) (dt[2] / (ndata * dt[0]) + 0.5);
         }
         // set buffer start time first time around
         if (bufEpoch == 0) {
            bufTime = dStart;
         }
         // check for error, check start time
         if (err ||  (bufEpoch != bufSize / ndata)) {
            cerr << "buffer ERROR size = " << bufSize << " bufEpoch = " <<
               bufEpoch << endl;
            // ignore if buffer not yet filled
            if ((bufSize == 0) && !err) {
               return true;
            }
            // otherwise call process with error flag set
            else {
               bufSize = 0;
               bool ret = 
                  process (bufTime / _ONESEC, 
                          (bufTime % _ONESEC + _EPOCH / 10) / _EPOCH, 
                          buf, dec, true, partitions, mux, update);
               // if first epoch try to resync; otherwise quit
               if ((bufEpoch != 0) || !ret) {
                  return ret;
               }
            }
         }
      
         // copy data into buffer
         if (cmplx) {
            memcpy (buf + 2 * bufSize, data, 2 * ndata * sizeof (float));
         }
         else {
            memcpy (buf + bufSize, data, ndata * sizeof (float));
         }
         bufSize += ndata;
      
         // process if buffer is full; else return
         if (bufSize < dec) {
            return true;
         }
         else {
            bufSize = 0;
            return process (bufTime / _ONESEC, 
                           (bufTime % _ONESEC + _EPOCH / 10) / _EPOCH, 
                           buf, dec, false, partitions, mux, update);
         }
      }
      
      // enough data points: data just passed through
      else {
         return process (time, epoch, data, ndata, err, 
                        partitions, mux, update);
      }
   }


   bool dataChannel::preprocessing::process (
                     taisec_t time, int epoch,
                     float data[], int ndata, int err,
                     partitionlist& partitions, 
                     mutex& mux, bool& update)
   {
   #ifdef DEBUG
      cerr << "process time = " << time << " epoch = " << epoch << endl;
   #endif
   
      // setup parameters
      tainsec_t	dStart = (tainsec_t) time * _ONESEC + epoch * _EPOCH;
      tainsec_t duration = (tainsec_t) (ndata * dt[0] * __ONESEC);
   
      // check active time
      tainsec_t tpre = (tainsec_t) (decdelay * __ONESEC);
      if (useActiveTime && (start > 0) && 
         (dStart + duration < start - _PREPROC_STARTUP * tpre)) {
         return true;
      }
      if (useActiveTime && (stop > 0) &&
         (dStart > stop + _PREPROC_CONTINUE * tpre)) {
         return true;
      }
   
      mux.lock();
      int		dec1 = decimate1;
      int		dec2 = decimate2;
      tainsec_t		zoom0 = zoomstart;
      double		zoomf = zoomfreq;
      double 		zoomdt = dt[1];
      mux.unlock();
      /* setup temp storage */
      int		size = ((zoomf == 0) && !cmplx) ? ndata : 2 * ndata;
      float* 		yy1 = new (nothrow) float [size];
      float*		yy2 = new (nothrow) float [size];
      float*		y1 = data;
      float*		y2 = yy1;
      int		len = ndata;
      int		len2;
      // check allocation
      if ((yy1 == 0) || (yy2 == 0)) {
         delete [] yy1;
         delete [] yy2;
         return false;
      }
   #ifdef DEBUG
      cerr << "dec1 = " << dec1 << " dec2 = " << dec2 << 
         " zoom = " << zoomf << " tzoom = " << zoom0 << 
         " dtzoom = " << zoomdt << " cmplx = " << cmplx <<
         " len = " << len << endl;
   #endif
   
      /* Primitive high pass filter for zoom analysis 
         This 'filter' will take the average from the first received time series
         and subtract this value from all subsequently received ones */
      if ((zoomf != 0) && !cmplx) {
      	 // first time around: init the hp value
         if (!hpinit) {
            double val = 0;
            for (int i = 0; i < len; i++) {
               val += y1[i];
            }
            hpinit = true;
            hpval = -val / len;
         #ifdef DEBUG
            cerr << "init hp filter to " << hpval << endl;
         #endif
         }
      	 // subtract the hp value from each point
         for (int i = 0; i < len; i++) {
            y2[i] = y1[i] + hpval;
         }
         y1 = y2;
         y2 = (y1 == yy1) ? yy2 : yy1;
      }
      /* delay stage */
      if (removeDelay) {
         timedelay (y1, y2,  (cmplx ? 2 : 1) * len,  
                   (cmplx ? 2 : 1) * delaytaps, tmpdelay1, &tmpdelay1);
         y1 = y2;
         y2 = (y1 == yy1) ? yy2 : yy1;
      }
      /* first decimation stage */
      if ((dec1 > 1) && !cmplx) {
         decimate (decimationflag, y1, y2, len, dec1, 
                  tmpdec1, &tmpdec1);
         len = ndata / dec1;
         y1 = y2;
         y2 = (y1 == yy1) ? yy2 : yy1;
      }
      /* zoom stage */
      if ((zoomf != 0) && !cmplx) {
         double tzoom = (removeDelay) ? 
            (double) (dStart - delay1 - zoom0) / __ONESEC:
            (double) (dStart - zoom0) / __ONESEC;
      #ifdef DEBUG
         cerr << "tzoom = " << tzoom << " len = " << len << endl;
      #endif
         sMixdown (0, y1, NULL, y2, y2+len, len, tzoom, zoomdt, zoomf);
         y1 = y2;
         y2 = (y1 == yy1) ? yy2 : yy1;
      }
      /* reorder the data if complex */
      if (cmplx) {
         for (int i = 0; i < len; ++i) {
            y2[i] = y1[2*i];
            y2[i+len] = y1[2*i+1];
         }
         y1 = y2;
         y2 = (y1 == yy1) ? yy2 : yy1;
      }
      /* second decimation stage */
      if (dec2 > 1) {
         decimate (decimationflag, y1, y2, len, dec2, 
                  tmpdec2I, &tmpdec2I);
         len2 = len / dec2;
         if ((zoomf != 0) || cmplx) {
            decimate (decimationflag, y1+len, y2+len2, len, dec2, 
                     tmpdec2Q, &tmpdec2Q);
         }
         len = len2;
         y1 = y2;
         y2 = (y1 == yy1) ? yy2 : yy1;
      }
      /* remove delay */
      if (removeDelay) {
         dStart -= delayshift;
      }
      /* partition data */
      mux.lock();
      for (dataChannel::partitionlist::iterator iter = partitions.begin(); 
          iter != partitions.end(); iter++) {
      #ifdef DEBUG
         cerr << "inspect partition " << iter->name << endl;
         cerr << "dec1 " << iter->decimate1 << "|" << decimate1 << endl;
         cerr << "dec2 " << iter->decimate2 << "|" << decimate2 << endl;
         cerr << "zoom " << iter->zoomstart << "|" << zoomstart << endl;
         cerr << "zoomf " << iter->zoomfreq << "|" << zoomfreq << endl;
         cerr << "is done = " << (iter->isDone() ? "true" : "false") << endl;
         cerr << "part size = " << iter->buf[0].size() << "$" << iter->length << endl;
      #endif
         // check partition preprocessing needs
         if (cmplx) {
            if ((iter->decimate1 * iter->decimate2 != decimate2) ||
               (iter->zoomstart != zoomstart) ||
               (fabs (iter->zoomfreq - zoomfreq) >= 1E-6) ||
               (iter->removeDelay != removeDelay) ||
               iter->isDone()) {
               continue;
            }
         }
         else {
            if ((iter->decimate1 != decimate1) || 
               (iter->decimate2 != decimate2) ||
               (iter->zoomstart != zoomstart) ||
               (fabs (iter->zoomfreq - zoomfreq) >= 1E-6) ||
               (iter->removeDelay != removeDelay) ||
               iter->isDone()) {
               continue;
            }
         }
      #ifdef DEBUG
         cerr << "found partition " << time << "." << epoch << 
            " dat[0] = " << y1[0] << endl;
      #endif
         // now copy data into partition if necessary
         int 	ndx = iter->index (dStart, len);
      #ifdef DEBUG
         cerr << "index = " << ndx << endl;
      #endif
         if (ndx == -1) {
            break;
         }	 
         if (err | (ndx == -2)) {
            iter->nomore();
            update = true;
            continue;
         }
         len2 = iter->range (dStart, len);
      #ifdef DEBUG
         cerr << "range = " << len2 << endl;
      #endif
         iter->fill (y1 + ndx, len2);
         if ((zoomf != 0) || cmplx) {
            iter->fill (y1 + len + ndx, len2, 1);
         }
         if (iter->isDone()) {
            iter->decdelay = decdelay;
            iter->delaytaps = delaytaps;
            iter->timedelay = (decdelay + delaytaps * dt[0]) - 
               (double) delayshift / __ONESEC;
            if (iter->timedelay < 1E-9) {
               iter->timedelay = 0.0;
            }
         
            update = true;
         }
      }
      mux.unlock();
      delete [] yy1;
      delete [] yy2;
      return true;
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Class Name: dataChannel						*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
   dataChannel::dataChannel (string Chnname, gdsStorage& dat,
                     int dataRate, int dataType)
   : chnCallback (Chnname), datarate (dataRate), datatype (dataType), 
   storage (&dat), timestamp (0), inUse (1), isTP (false),
   asyncUpdate (false)
   {
      switch (datatype) {
         case DAQ_DATATYPE_16BIT_INT:
            databps = sizeof (short);
            break;
         case DAQ_DATATYPE_32BIT_INT:
            databps = sizeof (int);
            break;
         case DAQ_DATATYPE_64BIT_INT:
            databps = sizeof (long long);
            break;
         case DAQ_DATATYPE_FLOAT:
            databps = sizeof (float);
            break;
         case DAQ_DATATYPE_DOUBLE:
            databps = sizeof (double);
            break;
         case DAQ_DATATYPE_COMPLEX:
            databps = 2 * sizeof (float);
            break;
         case DAQ_DATATYPE_32BIT_UINT:
            databps = sizeof (int);
            break;
         default:
            databps = 0;
            break;
      }
   }


   dataChannel::dataChannel (const dataChannel& chn)
   : chnCallback (chn.chnname)
   {
      *this = chn;
   }


   dataChannel::~dataChannel ()
   {
      updatelock.writelock();
   }


   dataChannel& dataChannel::operator= (const dataChannel& chn)
   {
      if (this != &chn) {
         chn.updatelock.writelock();
         updatelock.writelock();
         this->chnCallback::operator= (chn);
         datarate = chn.datarate;
         datatype = chn.datatype;
         databps = chn.databps;
         storage = chn.storage;
         partitions = chn.partitions;
         preprocessors = chn.preprocessors;
         timestamp = chn.timestamp;
         inUse = chn.inUse;
         isTP = chn.isTP;
         asyncUpdate = false;
         updatelock.unlock();
         chn.updatelock.unlock();
      }
      return *this;
   }


   bool dataChannel::addPreprocessing (int Decimate1, int Decimate2, 
                     tainsec_t Zoomstart, double Zoomfreq, 
                     bool rmvDelay, bool useActiveTime,
                     tainsec_t Start, tainsec_t Stop)
   {
      preprocessing	pre (datarate, Decimate1, Decimate2, 
                         Zoomstart, Zoomfreq, rmvDelay);
   
      updatelock.writelock();
      semlock		lockit (mux);
      for (preprocessinglist::iterator iter = preprocessors.begin();
          iter != preprocessors.end(); iter++) {
         if (*iter == pre) {
            iter->setActiveTime (Start, Stop, useActiveTime);
            updatelock.unlock();
            return true;
         }
      }
      // not found, add it
      pre.setActiveTime (Start, Stop, useActiveTime);
      preprocessors.push_back (pre);
      updatelock.unlock();
      return true;
   }


   bool dataChannel::addPartitions (const partitionlist& newPartitions,
                     bool useActiveTime) 
   {
      // check if preprocessing exists
      for (partitionlist::const_iterator iter = newPartitions.begin();
          iter != newPartitions.end(); ++iter) {
         if (datatype == DAQ_DATATYPE_COMPLEX) {
            addPreprocessing (-1, iter->decimate1 * iter->decimate2,
                             iter->zoomstart, 
                             iter->zoomfreq == 0 ? 1 : iter->zoomfreq, 
                             iter->removeDelay, useActiveTime, 
                             iter->start, iter->start + iter->duration);
         }
         else {
            addPreprocessing (iter->decimate1, iter->decimate2,
                             iter->zoomstart, iter->zoomfreq, 
                             iter->removeDelay, useActiveTime, 
                             iter->start, iter->start + iter->duration);
         }
      }
   
      // add partitions to list
      updatelock.writelock();
      semlock		lockit (mux);
      copy (newPartitions.begin(), newPartitions.end(), 
           back_inserter (partitions));
      sort (partitions.begin(), partitions.end());
      /* make sure they zoomf is non-zero if complex time series */
      for (partitionlist::iterator iter = partitions.begin();
          iter != partitions.end(); ++iter) {
         if ((datatype == DAQ_DATATYPE_COMPLEX) && 
            (iter->zoomfreq == 0)) {
            iter->zoomfreq = 1;
         }
      }
      updatelock.unlock();
      return true;
   }


   void dataChannel::reset ()
   {
      updatelock.writelock();
      semlock		lockit (mux);
      partitions.clear();
      preprocessors.clear();
      updatelock.unlock();
   }


   void dataChannel::skip (tainsec_t stop)
   {
      bool		update = false;
      semlock		lockit (mux);
   
      for (partitionlist::iterator iter = partitions.begin();
          iter != partitions.end(); iter++) {
         if (iter->start + iter->duration <= stop) {
            iter->nomore();
            update = true;
         }
      }
      if (update) {
         updateStorage (true);
      }
   }


   tainsec_t dataChannel::maxDelay () const
   {
      tainsec_t		delay = 0;
      semlock		lockit (mux);
   
      for (preprocessinglist::const_iterator iter = preprocessors.begin();
          iter != preprocessors.end(); iter++) {
         if (iter->removeDelay) {
            delay = max (delay, iter->delayshift);
         }
      }
      return delay;
   }


   int dataChannel::callback (taisec_t time, int epoch,
                     float data[], int ndata, int err)
   {
      bool	update = false;
   
   #ifdef DEBUG
      cerr << "callback for " << chnname << " (pre # = " << 
         preprocessors.size() << ")" << endl;
   #endif
      // cycle through preprocessing objects
      mux.lock();
      for (preprocessinglist::iterator iter = preprocessors.begin();
          iter != preprocessors.end(); iter++) {
         if (!(*iter)(time, epoch, data, ndata, err, 
                     partitions, mux, update)) {
            cerr << "PREPROCESSING ERROR " << chnname << endl;
            // error
         }
      }
      mux.unlock();
   
      if (update) {
         updateStorage (true);
      }
      return 0;
   }


   void dataChannel::updateStorage (bool async)
   {
      if (async) {
         mux.lock();
         // do nothing if update task is already running
         if (asyncUpdate) {
            mux.unlock();
            return;
         }
         // else set flag and create task
         asyncUpdate = true;
         mux.unlock();
         taskID_t	taskID;
         int		attr;
      #ifdef OS_VXWORKS
         attr = VX_FP_TASK;
      #else
         attr = PTHREAD_CREATE_DETACHED | PTHREAD_SCOPE_PROCESS;
      #endif
         updatelock.readlock();
         if (taskCreate (attr, taskDataUpdatePriority, &taskID, 
                        taskDataUpdateName, (taskfunc_t) updateStorageTask, 
                        (taskarg_t) this) < 0) {
            updatelock.unlock();
            mux.lock();
            asyncUpdate = false;
            mux.unlock();
         }
      }
      else {
      #ifdef DEBUG
         cerr << "update " << chnname << endl;
      #endif
      
         while (1) {
            // find oldest parititon which is done
            mux.lock();
            partitionlist::iterator iter = partitions.end();
            for (partitionlist::iterator i = partitions.begin();
                i != partitions.end(); i++) {
               // found one which is done
               if (i->isDone()) {
                  // first one found
                  if (iter == partitions.end()) {
                     iter = i;
                  }
                  // else: test if older stop time
                  else if (i->start + i->duration < 
                          iter->start + iter->duration) {
                     iter = i;
                  }
               }
            }
            // if none found, exit
            if (iter == partitions.end()) {
               asyncUpdate = false;
               mux.unlock();
            #ifdef DEBUG
               cerr << "update done" << endl;
            #endif
               return;
            }
            // get partition parameters
            string		name = iter->name;
            tainsec_t		start = iter->start;
            tainsec_t		stop = start + iter->duration;
            double		precursor = 
               (double) iter->precursor / __ONESEC;
            double		dt = iter->dt;
            int			len = iter->length;
            int			decimate1 = iter->decimate1;
            int			decimate2 = iter->decimate2;
            tainsec_t		zoomstart = iter->zoomstart;
            double		zoomfreq = iter->zoomfreq;
            bool		cmplx = (iter->zoomfreq != 0);
            int			decimationflag = 
               cmplx ? kDecimationflagZoom : kDecimationflag;
            double 		decdelay = iter->decdelay;
            int 		delaytaps = iter->delaytaps;
            double		timedelay = iter->timedelay;
            bool		error = !iter->isFull();
            mux.unlock();
         
            // set time stamp
            if (error) {
               cerr << "UPDATE ERROR 3 " << name << endl;
            }
            /* setup channel if necessary */
            if (storage->findData (name) == 0) {
               gdsDataObject* 	dobj = 
                  storage->newChannel (name, start, dt, cmplx);
               if (dobj != 0) {
                  /* add parameters describing the preprocessing */
                  gdsParameter		prm;
                  prm = gdsParameter ("tp", precursor, "s");
                  storage->addParameter (name, prm);
                  prm = gdsParameter ("tf0", zoomstart, "ns");
                  storage->addParameter (name, prm);
                  prm = gdsParameter ("f0", zoomfreq, "Hz");
                  storage->addParameter (name, prm);
                  prm = gdsParameter ("N", len);
                  storage->addParameter (name, prm);
                  prm = gdsParameter ("AverageType", (int) 0);      
                  storage->addParameter (name, prm);
                  prm = gdsParameter ("Averages", (int) 1);
                  storage->addParameter (name, prm);
                  prm = gdsParameter ("Decimation", decimate1*decimate2);
                  storage->addParameter (name, prm);
                  prm = gdsParameter ("Decimation1", decimate1);
                  storage->addParameter (name, prm);
                  prm = gdsParameter ("DecimationType", decimationflag);
                  storage->addParameter (name, prm);
                  char		dectype[256];
                  decimationFilterName (decimationflag, dectype, 
                                       sizeof (dectype));
                  prm = gdsParameter ("DecimationFilter", dectype);
                  storage->addParameter (name, prm);
                  prm = gdsParameter ("DecimationDelay", decdelay, "s");
                  storage->addParameter (name, prm);
                  prm = gdsParameter ("DelayTaps", delaytaps);
                  storage->addParameter (name, prm);
                  prm = gdsParameter ("TimeDelay", timedelay, "s");
                  storage->addParameter (name, prm);
                  prm = gdsParameter ("Channel", chnname);
                  storage->addParameter (name, prm);
               }
            }
            float*	dat = storage->allocateChannelMem (name, len);
         
            // find partition again
            mux.lock();
            iter = partitions.end();
            for (partitionlist::iterator i = partitions.begin();
                i != partitions.end(); i++) {
               if ((i->name == name) && (i->start == start) &&
                  (i->length == len)) {
                  iter = i;
                  break;
               }
            }
            // error if not found
            if ((iter == partitions.end()) || !iter->isDone()) {
               asyncUpdate = false;
               mux.unlock();
               if (dat != 0) {
                  storage->notifyChannelMem (name, error);
               }
               cerr << "UPDATE ERROR " << name << endl;
               return;
            }
         
            // copy data
            if ((dat != 0) && (!error)) {
               iter->copy (dat, len, cmplx);
            }
            // remove partition
            partitions.erase (iter);
         
            // notify storage object of new data
            if (dat != 0) {
               mux.unlock();
               storage->notifyChannelMem (name, error);
               mux.lock();
            }
            // set time stamp
            if (error) {
               cerr << "UPDATE ERROR 2 " << name << endl;
            }
            timestamp = stop;
            mux.unlock();
         }
      }
   }


   void dataChannel::updateStorageTask (dataChannel* chn) 
   {
      chn->updateStorage (false);
      chn->updatelock.unlock();
   }



}
