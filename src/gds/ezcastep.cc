
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string>
#include <iostream>
#include <vector>

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


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// utility functions                                                    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
   static string trim (const char* p)
   {
      while (isspace (*p)) p++;
      string s = p;
      while ((s.size() > 0) && isspace (s[s.size()-1])) {
         s.erase (s.size() - 1);
      }
      return s;
   }


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// step class                                                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
   class step {
   public:
      enum steptype {
      invalid,
      linear,
      geometric
      };
   
      steptype	fType;
      double	fSize;
   
      step () 
      : fType (invalid), fSize (0) {
      }
      step (const char* s);
      bool isValid() const {
         return fType != invalid; }
      double stepvalue (int i, double x);
   };

//______________________________________________________________________________
   step::step (const char* p) : fType (invalid), fSize (0)
   {
      if (p == 0) {
         printf ("Invalid step specification:\n");
         return;
      }
      string s = trim (p);
      if (s.empty()) {
         printf ("Invalid step specification: %s\n", p);
         return;
      }
      bool up = true;
      switch (s[0]) {
         case '+':
            {
               fType = linear;
               s.erase (0, 1);
               break;
            }
         case '-':
            {
               fType = linear;
               s.erase (0, 1);
               up = false;
               break;
            }
         case '*':
            {
               fType = geometric;
               s.erase (0, 1);
               break;
            }
         case '/':
            {
               fType = geometric;
               s.erase (0, 1);
               up = false;
               break;
            }
         default:
            {
               if (!isdigit (s[0])) {
                  printf ("Invalid step specification: %s\n", p);
                  return;
               }
               fType = linear;
               break;
            }
      }
      char* pp = 0;
      fSize = strtod (s.c_str(), &pp);
      string unit = trim (pp);
      if ((unit == "dB") && (fType == linear)) {
         fType = geometric;
      	 // round 3dB/6dB to sqrt(2) and 2!
         if (fabs (fSize - 3.0) < 1e-5) {
            fSize = sqrt(2.);
         }
         else if (fabs (fSize + 3.0) < 1e-5) {
            fSize = 1./sqrt(2.);
         }
         else if (fabs (fSize - 6.0) < 1e-5) {
            fSize = 2.;
         }
         else if (fabs (fSize + 6.0) < 1e-5) {
            fSize = 0.5;
         }
         else {
            fSize = ::pow (10, fSize/20);
         }
      }
      else if (!unit.empty()) {
         printf ("Invalid step specification: %s\n", p);
         fType = invalid;
         return;
      }
      if (!up) {
         fSize = (fType == linear) ? -fSize : 1./fSize;
      }
   }

//______________________________________________________________________________
   double step::stepvalue (int i, double x)
   {
      if (i < 0) {
         return 0;
      }
      else if (i == 0) {
         return fSize;
      }
      else if (fType == linear) {
         return x + fSize * i;
      }
      else {
         return x * ::pow (fSize, i);
      }
   }



//////////////////////////////////////////////////////////////////////////
//                                                                      //
// channel class                                                        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
   class channel {
   public:
      enum stepformat {
      automatic,
      fromlist
      };
      typedef std::vector<step> step_list;   
   
      stepformat	fFormat;
      std::string	fName;
      step_list		fSteps;
      int		fStepNum;
      bool		fInitValue;
      double		fValue;
   
      channel (const char* name = "") 
      : fFormat (automatic), fStepNum (0), fInitValue (false), fValue (0.0) {
         setname (name);
      }
      bool isValid() const {
         return !fSteps.empty(); }
      int steps() const {
         return (fFormat == automatic) ? fStepNum : fSteps.size(); }
   
      double stepvalue (int step);
      bool setname (const char* s);
      bool parse (const char* s);
   };

//______________________________________________________________________________
   bool channel::setname (const char* p)
   {
      string s (p);
      string::size_type pos = s.find ("=");
      if (pos != string::npos) {
         fInitValue = true;
         fValue = atof (s.data() + pos + 1);
         s.erase (pos);
      }
      fName = trim (s.c_str());
      return !fName.empty();
   }

//______________________________________________________________________________
   bool channel::parse (const char* p)
   {
      fSteps.clear();
      fStepNum = 0;
   
      string s = trim (p);
      if (s.empty()) {
         return false;
      }
      // list
      if (s[0] == '(') {
         fFormat = fromlist;
         s = trim (s.c_str() + 1);
         while (!s.empty() && (s[0] != ')')) {
            string ss = s;
            string::size_type pos = s.find (',');
            if (pos != string::npos) {
               ss.erase (pos);
               s.erase (0, pos + 1);
            }
            else if ((pos = s.find (')')) != string::npos) {
               ss.erase (pos);
               s.erase (0, pos + 1);
            }
            else {
               s = "";
            }
            step val (ss.c_str());
            if (val.isValid()) {
               fSteps.push_back (val);
            }
         }
      }
      // automatic
      else {
         fFormat = automatic;
         string::size_type pos = s.find (',');
         if (pos != string::npos) {
            fStepNum = atoi (s.data() + pos + 1);
            s.erase (pos);
         }
         step val (s.c_str());
         if (val.isValid()) {
            fSteps.push_back (val);
         }
      }
   
      return !fSteps.empty();
   }

//______________________________________________________________________________
   double channel::stepvalue (int step)
   {
      if ((step < 0) || !isValid()) {
         return 0;
      }
      else if (step == 0) {
         return fValue;
      }
      if ((steps() > 0) && (step > steps())) {
         step = steps();
      }
      if (fFormat == automatic) {
         return fSteps[0].stepvalue (step, fValue);
      }
      else {
         return fSteps[step-1].stepvalue (1, fValue);
      }
   }


//______________________________________________________________________________
   typedef std::vector<channel> channel_list;


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// main                                                                 //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
   int main (int argc, char* argv[])
   {
   #ifdef GDS_NO_EPICS
      printf ("Epics channel access not supported\n");
   #else
      int 		c;		/* option */
      extern char*	optarg;		/* option argument */
      extern int	optind;		/* option ind */
      int		errflag = 0;	/* error flag */
      double		delay = 1.0;	/* delay between steps */
      timespec		wait;		/* wait delay */
      channel_list	channels;	/* channel list*/
   
      while ((c = getopt (argc, argv, "s:h")) != EOF) {
         switch (c) {
            /* delay */
            case 's':
               {
                  delay = atof (optarg);
                  break;
               }
            /* help */
            case 'h':
            case '?':
               {
                  errflag = 1;
                  break;
               }
         }
      }
      while (optind + 1 < argc) {
         channel chn (argv[optind]);
         if (chn.parse (argv[optind+1])) {
            channels.push_back (chn);
         }
         else {
            errflag = true;
         }
         optind += 2;
      }
   
      /* help */
      if (errflag || (optind != argc) || channels.empty()) {
         printf ("Usage: [options] ezcastep {'channel[=value]' 'steps'}\n"
                "       -s 'delay' : delay between steps; default 1s\n"
                "       -h : help\n"
                "       'channel' : Name of channel with optional initial value\n"
                "       'steps' : format 'type''size'[,'number'] or ('type''size',...)\n"
                "            +1.0 - linear up\n"
                "            -1.0 - linear down\n"
                "            *2.0 - geometric up\n"
                "            /2.0 - geometric down\n"
                "            -3dB - geometric 3dB down\n"
                "            +1.0,4 - linear up, 4 times by one\n"
                "            (*3,*10,*30,*100) - steps of 3, 10, 30, and 100\n"
                "       use quotes when necessary\n"
                "       3dB and 6dB are rounded to sqrt(2) and 2, respectively\n"
                "   Example: ezcastep \"H2:LSC-GAIN1\" \"+1,10\" \"H2:LSC-GAIN2\" \"*0.8\"\n"
                "       GAIN1 is increased 10 times by one, whereas GAIN2 is\n"
                "       simultanously ramped down by factors of 0.8\n"
                );
         return 1;
      }
   
      // Maximum number of steps
      int maxsteps = 1;
      for (channel_list::iterator i = channels.begin(); 
          i != channels.end(); ++i) {
         if (i->steps() > maxsteps) maxsteps = i->steps();
      }
   
      // Initialization
      ezcaAutoErrorMessageOff();
      ezcaSetTimeout (0.02);
      ezcaSetRetryCount (500);
      wait.tv_sec = (long long) (delay * 1E9) / 1000000000LL;
      wait.tv_nsec = (long long) (delay * 1E9) % 1000000000LL;
   
      // Get initial values
      for (channel_list::iterator i = channels.begin(); 
          i != channels.end(); ++i) {
         if (i->fInitValue) {
            continue;
         }
         if (ezcaGet ((char*)i->fName.c_str(), 
                     ezcaDouble, 1, &i->fValue) != EZCA_OK) {
            printf ("channel %s not accessible\n", (i->fName.c_str()));
            return 1;
         }
      }
   
      // step it
      for (int j = 0; j < maxsteps; ++j) {
         for (channel_list::iterator i = channels.begin(); 
             i != channels.end(); ++i) {
            double x = i->stepvalue (j + 1);
            if (ezcaPut ((char*)i->fName.c_str(), 
                        ezcaDouble, 1, &x) != EZCA_OK) {
               printf ("channel %s not accessible\n", (i->fName.c_str()));
               return 1;
            }
            printf ("%s = %g\n", i->fName.c_str(), x);
         }
      	 // wait
         if ((delay > 0) && (j + 1 < maxsteps)) {
            nanosleep (&wait, 0);
         }
      }
   #endif
   
      return 0;
   }

