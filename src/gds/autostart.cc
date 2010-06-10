/* -*- mode: c++; c-basic-offset: 3; -*- */
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include "tconv.h"
#include "TROOT.h"
#include "TCint.h"


#ifndef GDS_NO_EPICS
/*#include "ezca.h"*/
/* Data Types */
#define ezcaByte   0
#define ezcaString 1
#define ezcaShort  2
#define ezcaLong   3
#define ezcaFloat  4
#define ezcaDouble 5
#define VALID_EZCA_DATA_TYPE(X) (((X) >= 0)&&((X)<=(ezcaDouble)))

/* Return Codes */
#define EZCA_OK                0
#define EZCA_INVALIDARG        1
#define EZCA_FAILEDMALLOC      2
#define EZCA_CAFAILURE         3
#define EZCA_UDFREQ            4
#define EZCA_NOTCONNECTED      5
#define EZCA_NOTIMELYRESPONSE  6
#define EZCA_INGROUP           7
#define EZCA_NOTINGROUP        8
/* Functions */
extern "C" {
   void ezcaAutoErrorMessageOff(void);
   int ezcaGet(char *pvname, char ezcatype, int nelem, void *data_buff);
   int ezcaPut(char *pvname, char ezcatype, int nelem, void *data_buff);
   int ezcaSetRetryCount(int retry);
   int ezcaSetTimeout(float sec);
}
#endif


   using namespace std;


   class ConfigurationEntry {
   public:
      mutable pthread_mutex_t	fMux;
   
      string 	fName;
      string	fCondition;
      double	fValidTime[2];
      double	fDelay[2];
      string	fScript[2];
   
      bool	fDelete;
      bool	fRestart;
   
      ConfigurationEntry ();
      ~ConfigurationEntry ();
      ConfigurationEntry (const ConfigurationEntry& e);
   
      ConfigurationEntry& operator= (const ConfigurationEntry& e);
      bool operator== (const ConfigurationEntry& e);
   
      bool start();
      bool stop();
      void run();
      bool evaluate (bool& val);
   
   private:
      pthread_t fTID;
   
   };
   typedef map <string, ConfigurationEntry> ConfigurationList;


//______________________________________________________________________________
   ConfigurationList gConf;
   string gFilename;
   TROOT root ("autostart", "autostart");
   TCint myCint ("autostart", "autostart");
   pthread_mutex_t muxCint;


//______________________________________________________________________________
   bool getnext (string& line, string& item)
   {
      // trim leading blanks
      while (!line.empty() && isspace (line[0])) {
         line.erase (0, 1);
      }
      // empty?
      if (line.empty()) {
         return false;
      }
      // check for quotes
      string::size_type size = 0;
      if (line[0] == '"') {
         // look for closing quote
         ++size;
         while ((size < line.size()) &&
               !((line[size] == '"') && (line[size-1] != '\\'))) {
            ++size;
         }
         if (size == line.size()) {
            return false;
         }
         // remove quotes
         line.erase (size, 1);
         line.erase (0, 1);
         --size;
         // remove trailing blanks
         while ((size > 0) && isspace (line[size-1])) {
            line.erase (size - 1, 1);
            --size;
         }
         // check for escaped quotes
         string::size_type pos;
         while ((pos = line.find ("\\\"")) != string::npos) {
            line.erase (pos, 1);
            --size;
         }
      }
      // else look for first space
      else {
         while ((size < line.size()) && !isspace (line[size])) {
            ++size;
         }
      }
      item = string (line, 0, size);
      line.erase (0, size);
      return true;
   }

//______________________________________________________________________________
   ConfigurationEntry::ConfigurationEntry ()
   : fName ("unknown"), fDelete (false), fRestart (false), fTID (0)
   {
      pthread_mutex_init (&fMux, 0);
      fValidTime[0] = 0;
      fValidTime[1] = 0;
      fDelay[0] = 0;
      fDelay[1] = 0;
   }


//______________________________________________________________________________
   ConfigurationEntry::~ConfigurationEntry ()
   
   {
      stop();
      pthread_mutex_lock (&fMux);
      pthread_mutex_destroy (&fMux);
   }


//______________________________________________________________________________
   ConfigurationEntry::ConfigurationEntry (const ConfigurationEntry& e)
   : fTID (0)
   {
      pthread_mutex_init (&fMux, 0);
      *this = e;
   }

//______________________________________________________________________________
   ConfigurationEntry& ConfigurationEntry::operator= (
                     const ConfigurationEntry& e)
   {
      if (this != &e) {
         pthread_mutex_lock (&fMux);
         pthread_mutex_lock (&e.fMux);
         fName = e.fName;
         fCondition = e.fCondition;
         for (int i = 0; i < 2; ++i) {
            fValidTime[i] = e.fValidTime[i];
            fDelay[i] = e.fDelay[i];
            fScript[i] = e.fScript[i];
         }
         fDelete = e.fDelete;
         fRestart = e.fRestart;
         pthread_mutex_unlock (&e.fMux);
         pthread_mutex_unlock (&fMux);
      }
      return *this;
   }

//______________________________________________________________________________
   bool ConfigurationEntry::operator== (const ConfigurationEntry& e)
   {
      return (fName == e.fName) && (fCondition == e.fCondition);
   }

//______________________________________________________________________________
extern "C"
   void* run_conf (void* p) 
   {
      ((ConfigurationEntry*)p)->run();
      return 0;
   }

//______________________________________________________________________________
   bool ConfigurationEntry::start()
   {
      if (fTID && !stop()) {
         return false;
      }
      pthread_attr_t		tattr;
      int			status;
      if (pthread_attr_init (&tattr) == 0) {
         pthread_attr_setdetachstate (&tattr, PTHREAD_CREATE_DETACHED);
         pthread_attr_setscope (&tattr, PTHREAD_SCOPE_PROCESS);
         status = pthread_create (&fTID, &tattr, &run_conf, (void*)this);
         pthread_attr_destroy (&tattr);
         return true;
      }
      else {
         cerr << "Unable to run configuration task" << endl;
         return false;
      }
   }

//______________________________________________________________________________
   bool ConfigurationEntry::stop()
   {
      if (fTID) {
         pthread_mutex_lock (&fMux);
         if (pthread_cancel (fTID) != 0) {
            pthread_mutex_unlock (&fMux);
            return false;
         }
         fTID = 0;
         pthread_mutex_unlock (&fMux);
      }
      return true;
   }

//______________________________________________________________________________
   void ConfigurationEntry::run()
   {
      enum state_t {
         kWait,
         kTest,
         kDelay
         };
   
      static const timespec tick = {0, 100000000};
   
      bool val = false;
      evaluate (val);
      bool newval;
      tainsec_t active = 0;
      state_t state = kWait;
      // run
      while (1) {
         nanosleep (&tick, 0);
         // evaluate new condition
         if (!evaluate (newval)) {
            continue;
         }
         // check if new condition
         if (state == kWait) {
            if (newval == val) {
               continue;
            }
            pthread_mutex_lock (&fMux);
            double validtime = fValidTime[(int)val];
            pthread_mutex_unlock (&fMux);
            if (validtime > 0) {
               state = kTest;
               active = TAInow() + (tainsec_t)(validtime * 1E9);
               continue;
            }
         }
         // check if test count-down
         else if (state == kTest) {
            if (val == newval) {
               // not long enough
               state = kWait;
               continue;
            }
            else if (active >= TAInow()) {
               // not yet, wait some more
               continue;
            }
         }
         // check if delay
         pthread_mutex_lock (&fMux);
         double delay = fDelay[(int)val];
         pthread_mutex_unlock (&fMux);
         if (delay > 0) {
            state = kDelay;
            timespec wait;
            tainsec_t w = (tainsec_t)(delay * 1E9);
            wait.tv_sec = w / 1000000000LL;
            wait.tv_nsec = w % 1000000000LL ;
            nanosleep (&wait, 0);
         }
         // start script!
         pthread_mutex_lock (&fMux);
         string script = fScript[(int)val];
         pthread_mutex_unlock (&fMux);
         if (!script.empty()) {
            ::system (script.c_str());
         }
         val = newval;
         state = kWait;
      }
   }

//______________________________________________________________________________
   bool ConfigurationEntry::evaluate (bool& val)
   {
      pthread_mutex_lock (&fMux);
      string cond = fCondition;
      pthread_mutex_unlock (&fMux);
   
      // get epics values
   #ifndef GDS_NO_EPICS
      string::size_type pos;
      while ((pos = cond.find ("epicsGet")) != string::npos) {
         const char* start = cond.c_str() + pos;
         const char* p = start + 8;
         while (*p && isspace (*p)) {
            ++p;
         }
         // opening bracket?
         if (*p != '(') {
            return false;
         }
         // look for closing bracket
         const char* arg = ++p;
         int bindx = 1;
         while (*p && bindx) {
            if (*p == '(') ++bindx;
            if (*p == ')') --bindx;
            ++p;
         }
         if (bindx) {
            return false;
         }
         // get channel name
         string rest (arg, p - 1);
         cond.erase (pos, (int)(p - start));
         string chn;
         if (!getnext (rest, chn)) {
            return false;
         }
         // get value of EPICS channel
         double value = 0;
         // static double test = -0.1;
         // test += 0.01;
         // value = ((int)test) % 2;
         if (ezcaGet ((char*)chn.c_str(), ezcaDouble, 
                     1, &value) != EZCA_OK) {
            return false;
         }
         char buf[256];
         sprintf (buf, "(%g)", value);
         cond.insert (pos, buf);
      }
   #endif
   
      // Calculate condition using CINT
      pthread_mutex_lock (&muxCint);
      Long_t res = myCint.Calc (cond.c_str());
      pthread_mutex_unlock (&muxCint);
      val = (res != 0);
      return true;
   }


//______________________________________________________________________________
   bool parseConf (const char* filename, ConfigurationList& list)
   {
      char* p;
      ifstream inp (filename);
      if (!inp) {
         return false;
      }
      while (inp) {
         string line;
         if (!getline (inp, line)) {
            break;
         }
         // trim leading blanks
         while (!line.empty() && isspace (line[0])) {
            line.erase (0, 1);
         }
         // check for empty and comment lines
         if (line.empty() || (line[0] == '#')) {
            continue;
         }
         ConfigurationEntry c;
         string rest = line;
         string item;
         // get name
         if (!getnext (rest, item)) {
            cout << "Missing name on line " << line << endl;
            continue;
         }
         for (string::iterator pp = item.begin(); pp != item.end(); ++pp) {
            *pp = tolower (*pp);
         }
         c.fName = item;
         // get condition
         if (!getnext (rest, item)) {
            cout << "Missing condition on line " << line << endl;
            continue;
         }
         c.fCondition = item;
         // get script for up transition
         if (!getnext (rest, item)) {
            cout << "Missing script on line " << line << endl;
            continue;
         }
         c.fScript[0] = item;
         // get valid time for up transition
         if (!getnext (rest, item)) {
            cout << "Missing valid time on line " << line << endl;
            continue;
         }
         c.fValidTime[0] = strtod (item.c_str(), &p);
         if (item.empty() || *p) {
            cout << "Invalid valid time on line " << line << endl;
            continue;
         }
         // get dealy time for up transition
         if (!getnext (rest, item)) {
            cout << "Missing delay time on line " << line << endl;
            continue;
         }
         c.fDelay[0] = strtod (item.c_str(), &p);
         if (item.empty() || *p) {
            cout << "Invalid delay time on line " << line << endl;
            continue;
         }
         // trim leading blanks
         while (!rest.empty() && isspace (rest[0])) {
            rest.erase (0, 1);
         }
         if (!rest.empty()) {
            // get script for down transition
            if (!getnext (rest, item)) {
               cout << "Missing script on line " << line << endl;
               continue;
            }
            c.fScript[1] = item;
            // get valid time for down transition
            if (!getnext (rest, item)) {
               cout << "Missing valid time on line " << line << endl;
               continue;
            }
            c.fValidTime[1] = strtod (item.c_str(), &p);
            if (item.empty() || *p) {
               cout << "Invalid valid time on line " << line << endl;
               continue;
            }
            // get dealy time for down transition
            if (!getnext (rest, item)) {
               cout << "Missing delay time on line " << line << endl;
               continue;
            }
            c.fDelay[1] = strtod (item.c_str(), &p);
            if (item.empty() || *p) {
               cout << "Invalid delay time on line " << line << endl;
               continue;
            }
         }
         list[c.fName] = c;
      }
      return true;
   }


//______________________________________________________________________________
   void sigCtrlC (void) 
   {
      // stop watchdogs
      for (ConfigurationList::iterator i = gConf.begin(); 
          i != gConf.end(); ++i) {
         i->second.stop();
      }
      exit (0);
   }


//______________________________________________________________________________
   void sigHUP (void) 
   {
      // parse configuration
      ConfigurationList newconf;
      if (!parseConf (gFilename.c_str(), newconf)) {
         return;
      }
      // update watchdog parameters
      for (ConfigurationList::iterator i = gConf.begin(); 
          i != gConf.end(); ++i) {
         i->second.fDelete = true;
      }
      for (ConfigurationList::iterator i = newconf.begin(); 
          i != newconf.end(); ++i) {
         // if it not exists, add it to global list
         ConfigurationList::iterator j = gConf.find (i->first);
         if (j == gConf.end()) {
            gConf[i->first] = i->second;
            gConf[i->first].fRestart = true;
            gConf[i->first].fDelete = false;
         }
         // if it exists, check if changed
         else if (j->second == i->second) {
            j->second.fDelete = false;
         }
         // otherwise just update values
         else {
            j->second = i->second;
            //j->fRestart = true;
            j->second.fDelete = false;
         }
      }
      // restart watchdogs if necessary
      for (ConfigurationList::iterator i = gConf.begin(); 
          i != gConf.end(); ) {
         if (i->second.fDelete) {
            gConf.erase (i++);
         }
         else {
            if (i->second.fRestart) {
               i->second.start();
            }
            ++i;
         }
      }
   }


//______________________________________________________________________________
extern "C"
   void* connect_signal_autostart (void*) 
   {
      // set signal mask to include Ctrl-C and HUP */
      sigset_t set;
      if ((sigemptyset (&set) != 0) ||
         (sigaddset (&set, SIGHUP) != 0) ||
         (sigaddset (&set, SIGTERM) != 0) ||
         (sigaddset (&set, SIGINT) != 0)) {
         cerr << "Unable to connect signals" << endl;
         return 0;
      }
      while (1) {
         // wait for masked signals
         int sig;
         sigwait (&set, &sig);
         switch (sig) {
            case SIGHUP:
               sigHUP();
               break;
            case SIGTERM:
            case SIGINT:
               sigCtrlC();
               break;
         }
      }
      return 0;
   }

//______________________________________________________________________________
   int main (int argc, char* argv[]) 
   {
      pthread_mutex_init (&muxCint, 0);
      // parse command line
      if ((argc != 2) || (argv[1][0] == '-')) {
         cout << "usage: autostart 'config. file'" << endl;
         return 1;
      }
      gFilename = argv[1];
   
      // install signal handlers for HUP and ^C
      // mask Ctrl-C signal
      sigset_t set;
      if ((sigemptyset (&set) != 0) ||
         (sigaddset (&set, SIGINT) != 0) ||
         (sigaddset (&set, SIGTERM) != 0) ||
         (sigaddset (&set, SIGHUP) != 0) ||
         (pthread_sigmask (SIG_BLOCK, &set, 0) != 0)) {
         cerr << "Unable to mask signals" << endl;
         return 1;
      }
      // create thread which handles Ctrl-C and HUP
      pthread_attr_t		tattr;
      int			status;
      if (pthread_attr_init (&tattr) == 0) {
         pthread_attr_setdetachstate (&tattr, PTHREAD_CREATE_DETACHED);
         pthread_attr_setscope (&tattr, PTHREAD_SCOPE_PROCESS);
         pthread_t tid;
         status = pthread_create (&tid, &tattr, connect_signal_autostart, 0);
         pthread_attr_destroy (&tattr);
      }
      else {
         cerr << "Unable to connect signal handler thread" << endl;
         return 1;
      }
   
      // initialize auto start parameters
      if (!parseConf (gFilename.c_str(), gConf)) {
         return 1;
      }
   
      // setup epics access
   #ifndef GDS_NO_EPICS
      ezcaAutoErrorMessageOff();
      ezcaSetTimeout (0.02);
      ezcaSetRetryCount (500);
   #endif
   
      // start watchdogs
      for (ConfigurationList::iterator i = gConf.begin(); 
          i != gConf.end(); ++i) {
         i->second.start();
      }
   
      // wait forever
      while (1) {
         sleep (10000);
      }
   }
