/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: xmlconv							*/
/*                                                         		*/
/* Module Description: program to convert data of an xml file		*/
/*                     into a normal ascii or binary file		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Includes: 								*/
/*                                                         		*/
/*----------------------------------------------------------------------*/
#include <time.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <unistd.h>
#include <complex>
#include <cmath>
#include <cstdlib>
#include "dtt/gdsutil.h"
#include "dtt/diagnames.h"
#include "dtt/gdsdatum.hh"
#include "gdsconst.h"


#ifdef __GNU_STDC_OLD
// lack of io manipulators
#define showpoint ""
#define left ""
#define showpos ""
#define noshowpos ""
#endif

   using namespace std;
   using namespace diag;


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Constants: _argHelp		argument for displaying help		*/
/*            _argBinary	binary flag				*/
/*            _argY		Y coord flag				*/
/*            _argFloat		float flag				*/
/*            help_text		help text				*/
/*            								*/
/*----------------------------------------------------------------------*/
   // const string		_argHelp ("-?");
   // const string 	_argBinary ("-b");
   // const string 	_argY ("-y");
   // const string 	_argZ ("-z");
   // const string 	_argFloat ("-f");
   // const string 	_argMagPhase ("-MP");
   // const string 	_argmagphase ("-mp");
   const string		help_text 
   ("Usage: xmlconv -flags filename1 filename2 data_object\n"
   "       filename1 xml file to be read\n"
   "       filename2 output file name\n"
   "       data_object name of data object to be converted\n"
   "       -flags control parameters\n"
   "Control parameters\n"
   "       -b   output binary file (default is ascii)\n"
   "       -f   force float output for binary files\n"
   "       -M   use magnitude(abs)/phase(arg) for complex ascii output\n"
   "            rather than real/imaginary (default)\n"
   "       -m   use magnitude(dB)/phase(deg) for complex ascii output\n"
   "       -y   output only y coordinate (default is xy)\n"
   "       -z   uses zero as the start time rather than GPS sec\n");


   complex<double> get_val (char* val, gdsDataType dtype)
   {
      switch (dtype) {
         case gds_int8: 
            return *((char*) val);
         case gds_int16:
            return *((short*) val);
         case gds_int32: 
            return *((int*) val);
         case gds_int64: 
            return *((long long*) val);
         case gds_float32: 
            return *((float*) val);
         case gds_float64:
            return *((double*) val);
         case gds_complex32: 
            return *((complex<float>*) val);
         case gds_complex64:
            return *((complex<double>*) val);
         case gds_string:
         case gds_channel:
         case gds_void:
         case gds_bool:
         default:
            return 0;
      }
   }


   void write_val (ostream& out, complex<double> x, gdsDataType dtype,
                  int magPhase = 0)
   {
      switch (dtype) {
         case gds_int8:
            {
               char xx = (char) (x.real() + 1E-12);
               out.write ((char*) &xx, sizeof (char));
               break;
            }
         case gds_int16:
            {
               short xx = (short) (x.real() + 1E-12);
               out.write ((char*) &xx, sizeof (short));
               break;
            }
         case gds_int32: 
            {
               int xx = (int) (x.real() + 1E-12);
               out.write ((char*) &xx, sizeof (int));
               break;
            }
         case gds_int64: 
            {
               long long xx = (long long) (x.real() + 1E-12);
               out.write ((char*) &xx, sizeof (long long));
               break;
            }
         case gds_float32: 
            {
               float xx = x.real();
               out.write ((char*) &xx, sizeof (float));
               break;
            }
         case gds_float64:
            {
               double xx = x.real();
               out.write ((char*) &xx, sizeof (double));
               break;
            }
         case gds_complex32: 
            {
               complex<float> xx;
               switch (magPhase) {
                  case 1:
                     {
                        xx = complex<float> (abs (x), arg (x));
                        break;
                     }
                  case 2:
                     {
                        xx = complex<float> (20. * log10 (abs(x)),
                                            arg(x) * 180. / PI);
                        break;
                     }
                  case 0:
                  default:
                     {
                        xx = complex<float> (x);
                        break;
                     }
               }
               out.write ((char*) &xx, sizeof (complex<float>));
               break;
            }
         case gds_complex64:
            {
               complex<double> xx;
               switch (magPhase) {
                  case 1:
                     {
                        xx = complex<double> (abs (x), arg (x));
                        break;
                     }
                  case 2:
                     {
                        xx = complex<double> (20. * log10 (abs(x)),
                                             arg(x) * 180. / PI);
                        break;
                     }
                  case 0:
                  default:
                     {
                        xx = x;
                        break;
                     }
               }
               out.write ((char*) &xx, sizeof (complex<double>));
               break;
            }
         case gds_string:
            {
               out << x.real();
               break;
            }
         case gds_channel:
            {
               switch (magPhase) {
                  case 1:
                     {
                        out << abs(x) << " " << setw (12) << 
                           setprecision (8) << arg(x);
                        break;
                     }
                  case 2:
                     {
                        out << 20 * log10 (abs(x)) << " " << setw (12) << 
                           setprecision (8) << arg(x) * 180 / PI;
                        break;
                     }
                  case 0:
                  default:
                     {
                        out << x.real() << " " << setw (12) << 
                           setprecision (8) << x.imag();
                        break;
                     }
               }
               break;
            }
         case gds_void:
         case gds_bool:
         default:
            break;
      }
   }


/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Main Program 							*/
/*                                                         		*/
/* Description: 							*/
/* 									*/
/*----------------------------------------------------------------------*/
   int main (int argc, char *argv[])
   {
      bool		fBinary = false;
      bool		fY = false;
      bool		fZero = false;
      bool		fFloat = false;
      int		fMagPhase = 0;
      string		filename1;
      string		filename2;
      string		oname;
   
      // no arguments
      if (argc <= 1) {
         cout << help_text;
         return 0;
      }
   
      // parse arguments
      int 		c;
      extern int	optind;
      bool		errflag = false;
   
      while ((c = getopt (argc, argv, "bfMmyzh")) != EOF) {
         switch (c) {
            case 'b':
               {
                  fBinary = true;
                  break;
               }
            case 'f':
               {
                  fFloat = true;
                  break;
               }
            case 'M':
               {
                  fMagPhase = 1;
                  break;
               }
            case 'm':
               {
                  fMagPhase = 2;
                  break;
               }
            case 'y':
               {
                  fY = true;
                  break;
               }
            case 'z':
               {
                  fZero = true;
                  break;
               }
            case 'h':
            case '?':
               {
                  errflag = true;
                  break;
               }
         }
      }
      if (!errflag && (optind + 2 < argc)) {
         filename1 = argv[optind];
         filename2 = argv[optind+1];
         oname = argv[optind+2];
      }
      else {
         errflag = true;
      }
   
      if (errflag) {
         cout << help_text;
         return 0;
      }
   
   
      // open xml file
      gdsStorage	st (filename1, gdsStorage::ioEverything);
      if (!st) {
         cout << "Unable to open " << filename1 << endl;
         return 1;
      }
   
      // find data object
      gdsDataObject* 	dat = st.findData (oname);
      if (dat == 0) {
         cout << "Data object " << oname << " not found" << endl;
         return 1;
      }
   
      // open output file
      ofstream 		out (filename2.c_str());
      if (!out) {
         cout << "Unable to open " << filename2 << endl;
         return 1;
      }
   
      // check data type 
      if ((dat->datatype == gds_void) || (dat->datatype == gds_bool) ||
         (dat->datatype == gds_string) || (dat->datatype == gds_channel)) {
         cout << "data type " << gdsDataTypeName (dat->datatype) <<
            " not suuported" << endl;
         return 1;
      }
      // check object flag
      if ((dat->getFlag() != gdsDataObject::resultObj) &&
         (dat->getFlag() != gdsDataObject::rawdataObj)) {
         cout << "unsupported data object" << endl;
         return 1;
      }
      // check size
      if ((dat->value == 0) || (dat->elNumber() <= 0)) {
         cout << "no data associated with " << oname << endl;
         return 1;
      }
   
      // get object type and subtype
      string otype = dat->getType ();
      gdsParameter* prmST = 
         st.findParameter (oname, stTimeSeriesSubtype);
      if (otype.empty() || (prmST == 0)) {
         cout << "data object must define a object (sub)type" << endl;
         return 1;
      }
      if ((prmST->datatype != gds_int32) || 
         (prmST->value == 0) || (prmST->elNumber() != 1)) {
         cout << "data object must define proper object subtype" << endl;
         return 1;
      }
      int stype = *((int*) prmST->value);
   
      // get x coordinate
      int	xLen = 0;	// number of points in x
      double	x0 = 0;		// x start value
      double    dx = 1.0;	// x coordinat spacing
      bool	xIncluded = false; // x coordinate included?
      bool 	dataStride = false; // data stored in stride format?
   
      // determine parameters names for x0, dx and xLen
      xLen = dat->dimension[0];
      string		startname;
      string		deltaname;
      string		Nname;
   
      if (otype == stObjectTypeTimeSeries) {
         startname = stTimeSeriest0;
         deltaname = stTimeSeriesdt;
         Nname = stTimeSeriesN;
         xIncluded = (stype >= 4);
         dataStride = true;
      }
      else if (otype == stObjectTypeSpectrum) {
         startname = stSpectrumf0;
         deltaname = stSpectrumdf;
         Nname = stSpectrumN;
         xIncluded = (stype >= 4);
         dataStride = true;
      }
      else if (otype == stObjectTypeTransferFunction) {
         startname = stTransferFunctionf0;
         deltaname = stTransferFunctiondf;
         Nname = stTransferFunctionN;
         xIncluded = (stype >= 3);
         dataStride = true;
      }
      else if (otype == stObjectTypeCoefficients) {
         xIncluded = (stype > 3) && (stype != 8);
         if (stype != 8) {
            Nname = stCoefficientsM;
            dataStride = false;
         }
         else {
            Nname = stCoefficientsN;
            dataStride = true;
            fY = true;
         }
      }
      else if (otype == stObjectTypeMeasurementTable) {
         Nname = stMeasurementTableTableLength;
         xIncluded = true;
         dataStride = false;
      }
      else {
         cout << "unsupported object type" << endl;
         return 1;
      }
     // get length, x0 and dx
      gdsParameter*	prm;
      string 		s;
      if (!fY && !xIncluded) {
         // start time
         prm = st.findParameter (oname, startname);
         // cout << (prm == 0) << "|" << (prm->value == 0) << "|" <<
            // (prm->elNumber() != 1) << "|" << 
            // (prm->datatype) << "|" <<
            // gdsStrDataType (prm->datatype, prm->value) << endl;
      
         if ((prm == 0) || (prm->value == 0) || 
            (prm->elNumber() != 1) || (prm->datatype == gds_string) ||
            (prm->datatype == gds_channel) || (prm->datatype == gds_bool) ||
            (prm->datatype == gds_void)) {
            cout << "can find start parameter" << endl;
            return 1;
         }
         s = gdsStrDataType (prm->datatype, prm->value);
         x0 = atof (s.c_str());
         if (prm->datatype == gds_int64) {
            x0 /= 1E9;
         }         
         if ((prm->datatype == gds_int64) && fZero) {
            x0 = 0.;
         }
      	 // delta time
         prm = st.findParameter (oname, deltaname);
         if ((prm == 0) || (prm->value == 0) || 
            (prm->elNumber() != 1) || (prm->datatype == gds_string) ||
            (prm->datatype == gds_channel) || (prm->datatype == gds_bool) ||
            (prm->datatype == gds_void)) {
            cout << "can find point spacing parameter" << endl;
            return 1;
         }
         s = gdsStrDataType (prm->datatype, prm->value);
         dx = atof (s.c_str());
      }
      // # of data points
      prm = st.findParameter (oname, Nname);
      if ((prm == 0) || (prm->value == 0) || 
         (prm->elNumber() != 1) || (prm->datatype == gds_string) ||
         (prm->datatype == gds_channel) || (prm->datatype == gds_bool) ||
         (prm->datatype == gds_void)) {
         cout << "can find number of data points parameter" << endl;
         return 1;
      }
      s = gdsStrDataType (prm->datatype, prm->value);
      xLen = atoi (s.c_str());
      if (xLen <= 0) {
         cout << "data length must be at least one" <<endl;
         return 1;
      }
   
      // get stride value
      int	xStride = dat->elNumber() / xLen;
   
      // treat direct binary special
      if (fBinary && (fMagPhase == 0) && !dataStride && 
         (xIncluded ^ fY) &&
         (!fFloat || (dat->datatype == gds_float32) || 
         (dat->datatype == gds_complex32))) {
         out.write ((char*) dat->value, dat->size());
         if (!out) {
            cout << "error while writing output file" << endl;
            return 1;
         }
      }
      else {
         // data type for output
         gdsDataType 	dtype;
         if (!fBinary) {
            if ((dat->datatype == gds_complex32) || 
               (dat->datatype == gds_complex64)) {
               dtype = gds_channel; // missuse channel for complex ascii
            }
            else {
               dtype = gds_string;
            }
         }
         else if (fFloat) {
            dtype = gds_float32;
            if ((dat->datatype == gds_complex32) || 
               (dat->datatype == gds_complex64)) {
               dtype = gds_complex32;
            }
            else {
               dtype = gds_float32;
            }
         }
         else {
            dtype = dat->datatype;
         }
         if ((dtype == gds_channel) || (dtype == gds_string)) {
            out << left << showpoint << setfill ('0');
         }
         // go through list one by one
         int	ndx = 0;
         for (int i = 0; i < xLen; i++) {
            int first = 0;
            if (!fY && !xIncluded) {
               if ((dtype == gds_channel) || (dtype == gds_string)) {
                  out << setw (20) << noshowpos << setprecision (18);
               }
               write_val (out, x0 + (double) i * dx, 
                         (dtype == gds_channel) ? gds_string : dtype);
               if ((dtype == gds_string) || (dtype == gds_channel)) {
                  out << " " << setw (12) << setprecision (8) << showpos;
               }
               first = 0;
            }
            else if (!fY && xIncluded) {
               if ((dtype == gds_channel) || (dtype == gds_string)) {
                  out << setw (20) << noshowpos << setprecision (18);
               }
               if (dataStride) {
                  ndx = i * dat->elSize();
               }
               else {
                  ndx = i * xStride * dat->elSize();
               }
               complex<double> y = 
                  get_val ((char*) dat->value + ndx, dat->datatype);
               write_val (out, y, (dtype == gds_channel) ? gds_string : dtype);
               if ((dtype == gds_string) || (dtype == gds_channel)) {
                  out << " " << setw (12) << setprecision (8) << showpos;
               }
               first = 1;
            }
            else if (fY && xIncluded) {
               first = 1;
            }
            else { // fY && !xIncluded
               first = 0;
            }
            for (int j = first;  j < xStride; j++) {
               if (dataStride) {
                  ndx = (i + j * xLen) * dat->elSize();
               }
               else {
                  ndx = (i * xStride + j) * dat->elSize();
               }
               out << setw (12) << setprecision (8);
               complex<double> y = 
                  get_val ((char*) dat->value + ndx, dat->datatype);
               write_val (out, y, dtype, fMagPhase);
               if ((j < xStride - 1) &&
                  ((dtype == gds_string) || (dtype == gds_channel))) {
                  out << " ";
               }
            }
            if (!fBinary) {
               out << endl;
            }
         }
         if (!out) {
            cout << "error while writing output file" << endl;
            return 1;
         }
      }
   
      return 0;
   }
