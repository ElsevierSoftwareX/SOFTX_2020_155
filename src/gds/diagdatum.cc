/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: diagdatum						*/
/*                                                         		*/
/* Module Description: implements a storage object to store data and	*/
/* parameters, results and settings of a diagnostics test		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Header File List: */

#include <strings.h>
#include <iostream>
#include <algorithm>
#include "dtt/diagdatum.hh"
#include "tconv.h"
#include <time.h>
#include <string>
#include <stdio.h>
#include "dtt/gdsstring.h"
#include "gmutex.hh"
#include "dtt/diagnames.h"


namespace diag {
   using namespace std;
   using namespace thread;

   // Maximum allowed memory after purge
   const int64_t kMaxTimeSeriesMemory = 512*1024*1024; // 512MB


   // Diagnostics object templates
   const diagGlobal 		diagGlobal::myself = diagGlobal ();
   const diagDef 		diagDef::myself = diagDef ();
   const diagLidax 		diagLidax::myself = diagLidax ();
   const diagSync 		diagSync::myself = diagSync ();
   const diagEnv 		diagEnv::myself = diagEnv ();
   const diagScan 		diagScan::myself = diagScan ();
   const diagFind 		diagFind::myself = diagFind ();
   const diagIndex 		diagIndex::myself = diagIndex ();
   diagResult::diagResultList 	diagResult::myself = diagResultList ();
   const diagTimeSeries 	result1;
   const diagSpectrum		result2;
   const diagTransferFunction	result3;
   const diagCoefficients	result4;
   const diagMeasurementTable	result5;
   const diagChn 		diagChn::myself = diagChn ();
   const diagPlot 		diagPlot::myself = diagPlot ();
   const diagCalibration	diagCalibration::myself = diagCalibration ();
   diagTest::diagTestList 	diagTest::myself = diagTestList ();
   const testSineResponse	test1;
   const testSweptSine		test2;
   const testFFT		test3;
   const testTimeSeries		test4;


   diagObjectName::~diagObjectName ()
   {
   }


   bool diagObjectName::isValid (const string& Name, bool write,
                     string* normName) const
   {
      if (write && !writeaccess) {
         return false;
      }
   
      string		n;
      int		index1;
      int		index2;
   
      if (!diagStorage::analyzeName (Name, n, index1, index2)) {
         return false;
      }
   
      // first index 
      if (((index1 < 0) && (maxIndex1 > 0)) || 
         (index1 >= maxIndex1)) {
         return false;
      }
      // second index
      if (((index2 < 0) && (maxIndex2 > 0)) || 
         (index2 >= maxIndex2)) {
         return false;
      }
   
      bool 		ret = compareTestNames (name, n) == 0;
      if (ret && (normName != 0)) {
         *normName = makeName (name, index1, index2);
      }
      return ret;
   }


   string diagObjectName::makeName (const string& Name, int index1,
                     int index2)
   {
      string::size_type	pos =  Name.find ('[');
      string		n = Name;
      char		buf[256];
   
      if (pos != string::npos) {
         n.erase (pos, string::npos);
      }
      if (index1 >= 0) {
         if (index2 >= 0) {
            sprintf (buf, "[%i][%i]", index1, index2);
         }
         else {
            sprintf (buf, "[%i]", index1);
         }
      }
      else {
         buf[0] = 0;
      }
      return n + buf;
   }


   gdsParameter* diagObject::diagParam::newParam (const void* value, 
                     int dim1, int index1, int index2) const
   {
      // check value
      if ((value == 0) && (defValue != 0)) {
         // use default
         value = (void*) defValue;
         dim1 = (maxDim1 < 0) ? 0 : maxDim1;
      }
      if (value == 0) {
         return 0;
         // value = &empty;
         // dim1 = 1;
      }
   
      // check dim
      if ((dim1 < 1) || ((maxDim1 >= 0) && (dim1 > maxDim1))) {
         return 0;
      }
   
      // check index
      if (((maxIndex1 > 0) && 
          ((index1 < 0) || (index1 >= maxIndex1))) || 
         ((maxIndex2 > 0) && 
         ((index2 < 0) || (index2 >= maxIndex2)))) {
         return 0;
      }
      string 		n (name);
      if (maxIndex1 > 0) {
         char		s[100];
         sprintf (s, "[%d]", index1);
         n += s;
      }
      if (maxIndex2 > 0) {
         char		s[100];
         sprintf (s, "[%d]", index2);
         n += s;
      }
   
      // return parameter
      return new (nothrow) gdsParameter (n, datatype, value, dim1, unit);
   }


   gdsParameter* diagObject::diagParam::newParam (const gdsDatum& value, 
                     int index1, int index2) const
   {
      if (value.dimension.size() == 1) {
         return newParam (value.value, value.dimension[0], 
                         index1, index2);
      }
      else {
         return 0;
      }
   }


   bool diagObject::isValid (const string& Name, bool write,
                     string* normName) const
   {
      // separate name into data object and parameter name
      string		nameA;		// object name
      string 		nameB;		// parameter name
      int		indexA1;	// 1st object index
      int		indexA2;	// 2nd object index
      int		indexB1;	// 1st parameter index
      int		indexB2;	// 2nd parameter index
      string		norm1;		// 1st normalized name
      string		norm2;		// 2nd normalized name
   
      if (!diagStorage::analyzeName (Name, nameA, indexA1, indexA2, 
                           nameB, indexB1, indexB2)) {
         return false;
      }
   
      // check object name
      if (!diagObjectName::isValid (makeName (nameA, indexA1, indexA2), 
                           write, &norm1)) {
         return false;
      }
      if (nameB.size() == 0) {
         return (maxDim1 != 0);
      }
   
   
      // check parameter name
      for (diagParamList::const_iterator iter = dParams.begin();
          iter != dParams.end(); iter++) {
         if (iter->isValid (makeName (nameB, indexB1, indexB2), 
                           write, &norm2)) {
            if (normName != 0) {
               *normName = norm1 + "." + norm2;
            }
            return true;
         }
      }
      return false;
   }


   gdsDataObject* diagObject::newObject (void* value, int dim1, 
                     int dim2, int index1, int index2, 
                     gdsDataType Datatype) const
   {
      // check value
      if ((value == 0) && (defValue != 0)) {
         // use default
         value = (void*) defValue;
         dim1 = (maxDim1 < 0) ? 0 : maxDim1;
         dim2 = (maxDim2 < 0) ? 0 : maxDim2;
      }
   
      // check dim
      if ((dim1 < 0) || ((maxDim1 >= 0) && (dim1 > maxDim1))) {
         return 0;
      }
      if ((dim2 < 0) || ((maxDim2 >= 0) && (dim2 > maxDim2))) {
         return 0;
      }
   
      // check index
      if (((maxIndex1 > 0) && 
          ((index1 < 0) || (index1 >= maxIndex1))) || 
         ((maxIndex2 > 0) && 
         ((index2 < 0) || (index2 >= maxIndex2)))) {
         return 0;
      }
      string 		n (name);
      if (maxIndex1 > 0) {
         char		s[100];
         sprintf (s, "[%d]", index1);
         n += s;
      }
      if (maxIndex2 > 0) {
         char		s[100];
         sprintf (s, "[%d]", index2);
         n += s;
      }
   
      // check data type
      gdsDataType	dtype = datatype;
      if (Datatype != gds_void) {
         dtype = Datatype;
      }
   
      // make new data object
      gdsDataObject* obj = new (nothrow) gdsDataObject (n, dtype, value,
                           dim1, dim2, "", "", flag);
      if (obj == 0) {
         return 0;
      }
      obj->setType (type);
      obj->setFlag (flag);
   
      // add default parameters
      for (diagParamList::const_iterator iter = dParams.begin();
          iter != dParams.end(); iter++) {
         if ((iter->maxIndex1 > 0) && (iter->maxIndex2 == 0)) {
            // create array of default parameters
            for (int idx = 0; (idx < iter->maxIndex1) && 
                (idx < maxDefaultParameters); idx++) {
               gdsParameter* 	prm = iter->newParam (0, 1, idx);
               if (prm != 0) {
                  obj->parameters.push_back (
                                       gdsStorage::gdsParameterPtr (prm));
               }
            }
         }
         else {
            // create single parameter
            gdsParameter* 	prm = iter->newParam (0);
            if (prm != 0) {
               obj->parameters.push_back (
                                    gdsStorage::gdsParameterPtr (prm));
            }
         }
      }
      return obj;
   }


   bool diagObject::clone (gdsDataObject& obj, 
                     const gdsDataObject* templ,
                     bool copydata) const
   {
      if (templ == 0) {
         return true;
      }
      semlock		lockit (((gdsDataObject*) templ)->mux);
   
      // clone type/flag
      obj.setFlag (templ->getFlag());
      obj.setType (templ->getType());
   
      // clone parameters
      for (gdsDataObject::gdsParameterList::const_iterator iter2 = 
          templ->parameters.begin(); 
          iter2 != templ->parameters.end(); iter2++) {
         for (diagParamList::const_iterator iter = dParams.begin();
             iter != dParams.end(); iter++) {
            if (iter->isValid ((*iter2)->name)) {
               // found a parameter to clone
               if (((*iter2)->dimension.size() == 1) && 
                  ((*iter2)->datatype == iter->datatype)) {
                  setParam (obj, (*iter2)->name, (**iter2));
               }
            }
         }
      }
   
      // clone data
      if (copydata && (templ->dimension.size() <= 2)) {
         int		dim1;
         int		dim2;
         dim1 = (templ->dimension.size() > 0) ? templ->dimension[0] : 0;
         dim2 = (templ->dimension.size() > 1) ? templ->dimension[1] : 0;
         setData (obj, templ->value, dim1, dim2, templ->datatype);
      }
      return true;
   }


   bool diagObject::setParamHook (gdsDataObject& obj, 
                     const string& pName, 
                     const gdsDatum& value) const 
   {
      if ((compareTestNames (pName, stObjectType) == 0) &&
         (value.datatype == gds_string)) {
         // if (value.value != 0) {
            // obj.setType ((const char*) (value.value));
         // }      
         return true;
      }
      else if ((compareTestNames (pName, stObjectFlag) == 0) &&
              (value.datatype == gds_string)) {
         // if (value.value != 0) {
            // obj.setFlag ((const char*) value.value);
         // }
         return true;
      }
      return false;
   }


   bool diagObject::getParamHook (gdsDataObject& obj, 
                     const string& pName, 
                     gdsDatum& value) const 
   {
      //cerr << "check param hook for " << pName << endl;
      if (compareTestNames (pName, stObjectType) == 0) {
         value = gdsDatum (gds_string, obj.getType().c_str());     
         //cerr << "check param hook found " << obj.getType() << endl;
         return true;
      }
      else if (compareTestNames (pName, stObjectFlag) == 0) {
         int val = obj.getFlag();
         value = gdsDatum (gds_int32, &val);     
         return true;
      }
      return false; 
   }


   bool diagObject::setParam (gdsDataObject& obj, const string& Name, 
                     const gdsDatum& value) const
   {
      semlock		lockit (obj.mux);
      if (setParamHook (obj, Name, value)){
         return true;
      }
      for (diagParamList::const_iterator iter = dParams.begin();
          iter != dParams.end(); iter++) {
         if (iter->isValid (Name)) {
            // found parameter name
            if ((value.dimension.size() != 1) || 
               ((iter->maxDim1 >= 0) && 
               (value.dimension[0] != iter->maxDim1))) {
               return false;
            }
            // now check if an old parameter object is already defined
            for (gdsDataObject::gdsParameterList::const_iterator iter2 = 
                obj.parameters.begin(); 
                iter2 != obj.parameters.end(); iter2++) {
               if (iter2->get() == 0) {
                  continue;
               }
               if (compareTestNames (Name, (*iter2)->name) == 0) {
                  // found an old one
                  return (*iter2)->assignDatum (value);
               }
            }
         
            // no old one; need to make new parameter object
            gdsParameter*	prm;
            string		n;
            int			index1;
            int			index2;
            diagStorage::analyzeName (Name, n, index1, index2);
            prm = iter->newParam (value, index1, index2);
            if (prm == 0) {
               return false;
            }
            else {
               obj.parameters.push_back (
                                    gdsStorage::gdsParameterPtr (prm));
               return true;
            }
         }
      }
      // not a valid parameter name
      return false;
   }


   bool diagObject::setParam (gdsDataObject& obj, const string& Name, 
                     const string& value) const
   {
      semlock		lockit (obj.mux);
   
      for (diagParamList::const_iterator iter = dParams.begin();
          iter != dParams.end(); iter++) {
         if (iter->isValid (Name)) {
            // found parameter name
            gdsDatum 		dat;
         
            if ((iter->datatype == gds_string) ||
               (iter->datatype == gds_channel)) {
               dat = gdsDatum (iter->datatype, value.c_str(), 1);
            }
            else {
               int		dim;
               int		num;
            
               dim = (iter->maxDim1 < 0) ? value.size() : iter->maxDim1;
               dat = gdsDatum (iter->datatype, 0, dim);
               num = dat.readValues (value);
               if (iter->maxDim1 < 0) {
                  dat = gdsDatum (iter->datatype, dat.value, num);
               }
            }
            return setParam (obj, Name, dat);
         }
      }
      // not a valid parameter name
      return false;
   }


   bool diagObject::setParam (gdsDataObject& obj, const string& Name, 
                     char value) const
   {
     return setParam (obj, Name, int64_t(value));
   }


   bool diagObject::setParam (gdsDataObject& obj, const string& Name, 
                     short value) const
   {
     return setParam (obj, Name, int64_t(value));
   }


   bool diagObject::setParam (gdsDataObject& obj, const string& Name, 
                     int value) const
   {
     return setParam (obj, Name, int64_t(value) );
   }


   bool diagObject::setParam (gdsDataObject& obj, const string& Name, 
                     int64_t value) const
   {
      semlock		lockit (obj.mux);
   
      for (diagParamList::const_iterator iter = dParams.begin();
          iter != dParams.end(); iter++) {
         if (iter->isValid (Name)) {
            // found parameter name
            gdsDatum 		dat;
            switch (iter->datatype) {
               case gds_int8:
                  {
                     char	v = value;
                     dat = gdsDatum (iter->datatype, &v);
                     break;
                  }
               case gds_int16:
                  {
                     short	v = value;
                     dat = gdsDatum (iter->datatype, &v);
                     break;
                  }
               case gds_int32:
                  {
                     int	v = value;
                     dat = gdsDatum (iter->datatype, &v);
                     break;
                  }
               case gds_int64:
                  {
                     int64_t	v = value;
                     dat = gdsDatum (iter->datatype, &v);
                     break;
                  }
               default: 
                  {
                     return false;
                  }
            }
            return setParam (obj, Name, dat);
         }
      }
      // not a valid parameter name
      return false;
   }


   bool diagObject::setParam (gdsDataObject& obj, const string& Name, 
                     float value) const
   {
      return setParam (obj, Name, (double) value);
   }


   bool diagObject::setParam (gdsDataObject& obj, const string& Name, 
                     double value) const
   {
      semlock		lockit (obj.mux);
   
      for (diagParamList::const_iterator iter = dParams.begin();
          iter != dParams.end(); iter++) {
         if (iter->isValid (Name)) {
            // found parameter name
            gdsDatum 		dat;
            switch (iter->datatype) {
               case gds_float32:
                  {
                     float	v = value;
                     dat = gdsDatum (iter->datatype, &v);
                     break;
                  }
               case gds_float64:
                  {
                     double	v = value;
                     dat = gdsDatum (iter->datatype, &v);
                     break;
                  }
               case gds_complex32:
                  {
                     complex<float>	v = value;
                     dat = gdsDatum (iter->datatype, &v);
                     break;
                  }
               case gds_complex64:
                  {
                     complex<double>	v = value;
                     dat = gdsDatum (iter->datatype, &v);
                     break;
                  }
               default: 
                  {
                     return false;
                  }
            }
            return setParam (obj, Name, dat);
         }
      }
      // not a valid parameter name
      return false;
   }


   bool diagObject::setParam (gdsDataObject& obj, const string& Name, 
                     complex<float> value) const
   {
      return setParam (obj, Name, (complex<double>) value);
   }


   bool diagObject::setParam (gdsDataObject& obj, const string& Name, 
                     complex<double> value) const
   {
      semlock		lockit (obj.mux);
   
      for (diagParamList::const_iterator iter = dParams.begin();
          iter != dParams.end(); iter++) {
         if (iter->isValid (Name)) {
            // found parameter name
            gdsDatum 		dat;
            switch (iter->datatype) {
               case gds_complex32:
                  {
                     complex<float>	v (value);
                     dat = gdsDatum (iter->datatype, &v);
                     break;
                  }
               case gds_complex64:
                  {
                     complex<double>	v = value;
                     dat = gdsDatum (iter->datatype, &v);
                     break;
                  }
               default: 
                  {
                     return false;
                  }
            }
            return setParam (obj, Name, dat);
         }
      }
      // not a valid parameter name
      return false;
   }


   bool diagObject::setParam (gdsDataObject& obj, const string& Name, 
                     bool value) const
   {
      semlock		lockit (obj.mux);
   
      for (diagParamList::const_iterator iter = dParams.begin();
          iter != dParams.end(); iter++) {
         if (iter->isValid (Name)) {
            // found parameter name
            gdsDatum 		dat;
            switch (iter->datatype) {
               case gds_bool:
                  {
                     bool	v = value;
                     dat = gdsDatum (iter->datatype, &v);
                     break;
                  }
               default: 
                  {
                     return false;
                  }
            }
            return setParam (obj, Name, dat);
         }
      }
      // not a valid parameter name
      return false;
   }



   bool diagObject::getParam (gdsDataObject& obj, const string& Name, 
                     gdsDatum& value) const
   {
      semlock		lockit (obj.mux);
      if (getParamHook (obj, Name, value)){
         return true;
      }
      for (diagParamList::const_iterator iter = dParams.begin();
          iter != dParams.end(); iter++) {
         if (iter->isValid (Name, false)) {
            // found parameter name
            // now check if an parameter object is defined
            for (gdsDataObject::gdsParameterList::const_iterator iter2 = 
                obj.parameters.begin(); 
                iter2 != obj.parameters.end(); iter2++) {
               if (compareTestNames (Name, (*iter2)->name) == 0) {
                  // found one
                  value = **iter2;
                  return true;
               }
            }
         }
      }
      // not a valid parameter
      return false;
   }


   bool diagObject::getParam (gdsDataObject& obj, const string& pName,
                     string& value) const
   {
      // get parameter
      gdsDatum 		val;
      if (!getParam (obj, pName, val)) {
         return false;
      }
      // verify type and dimension
      if ((val.elNumber() != 1) || 
         ((val.datatype != gds_string) && 
         (val.datatype != gds_channel))) {
         return false;
      } 
      // copy value
      value = val.value ? string ((char*) val.value) : string ("");
      return true;
   }


   bool diagObject::getParam (gdsDataObject& obj, const string& pName,
                     char& value) const
   {
      // get parameter
      gdsDatum 		val;
      if (!getParam (obj, pName, val)) {
         return false;
      }
      // verify type and dimension
      if ((val.elNumber() != 1) || 
         ((val.datatype != gds_int8))) {
         return false;
      } 
      // copy value
      value = *((char*) val.value);
      return true;
   }


   bool diagObject::getParam (gdsDataObject& obj, const string& pName,
                     short& value) const
   {
      // get parameter
      gdsDatum 		val;
      if (!getParam (obj, pName, val)) {
         return false;
      }
      // verify type and dimension
      if ((val.elNumber() != 1) || 
         ((val.datatype != gds_int8) &&
         (val.datatype != gds_int16))) {
         return false;
      } 
      // copy value
      switch (val.datatype) {
         case gds_int8:
            value = *((char*) val.value);
            break;
         case gds_int16:
            value = *((short*) val.value);
            break;
         default: 
            return false;
      }
      return true;
   }


   bool diagObject::getParam (gdsDataObject& obj, const string& pName,
                     int& value, int max) const
   {
      // get parameter
      gdsDatum 		val;
      if (!getParam (obj, pName, val)) {
         return false;
      }
      // verify type and dimension
      if ((val.elNumber() < max) || 
         ((val.datatype != gds_int8) &&
         (val.datatype != gds_int16) &&
         (val.datatype != gds_int32))) {
         return false;
      } 
      // copy value
      int*	v = &value;
      for (int i = 0; i < max; i++, v++) {
         switch (val.datatype) {
            case gds_int8:
               *v = *((char*) val.value + i);
               break;
            case gds_int16:
               *v = *((short*) val.value + i);
               break;
            case gds_int32:
               *v = *((int*) val.value + i);
               break;
            default: 
               return false;
         }
      }
      return true;
   }


   bool diagObject::getParam (gdsDataObject& obj, const string& pName,
			      int64_t& value) const
   {
      // get parameter
      gdsDatum 		val;
      if (!getParam (obj, pName, val)) {
         return false;
      }
      // verify type and dimension
      if ((val.elNumber() != 1) || 
         ((val.datatype != gds_int8) &&
         (val.datatype != gds_int16) &&
         (val.datatype != gds_int32) &&
         (val.datatype != gds_int64))) {
         return false;
      } 
      // copy value
      switch (val.datatype) {
         case gds_int8:
	    value = *reinterpret_cast<char*>(val.value);
            break;
         case gds_int16:
	    value = *reinterpret_cast<short*>(val.value);
            break;
         case gds_int32:
	    value = *reinterpret_cast<int*>(val.value);
            break;
         case gds_int64:
	    value = *reinterpret_cast<int64_t*>(val.value);
            break;
         default: 
            return false;
      }
      return true;
   }


   bool diagObject::getParam (gdsDataObject& obj, const string& pName,
                     float& value) const
   {
      // get parameter
      gdsDatum 		val;
      if (!getParam (obj, pName, val)) {
         return false;
      }
      // verify type and dimension
      if ((val.elNumber() != 1) || 
         ((val.datatype != gds_float32))) {
         return false;
      } 
      // copy value
      value = *((float*) val.value);
      return true;
   }


   bool diagObject::getParam (gdsDataObject& obj, const string& pName,
                     double& value, int max) const
   {
      // get parameter
      gdsDatum 		val;
      if (!getParam (obj, pName, val)) {
         return false;
      }
      // verify type and dimension
      if ((val.elNumber() < max) || 
         ((val.datatype != gds_float32) &&
         (val.datatype != gds_float64))) {
         return false;
      } 
      // copy value
      double*	v = &value;
      for (int i = 0; i < max; i++, v++) {
         switch (val.datatype) {
            case gds_float32:
	      *v = reinterpret_cast<float*>(val.value)[i];
               break;
            case gds_float64:
	      *v = reinterpret_cast<double*>(val.value)[i];
               break;
            default: 
               return false;
         }
      }
      return true;
   }


   bool diagObject::getParam (gdsDataObject& obj, const string& pName,
                     complex<float>& value) const
   {
      // get parameter
      gdsDatum 		val;
      if (!getParam (obj, pName, val)) {
         return false;
      }
      // verify type and dimension
      if ((val.elNumber() != 1) || 
         ((val.datatype != gds_float32) &&
         (val.datatype != gds_float64) &&
         (val.datatype != gds_complex32))) {
         return false;
      } 
      // copy value
      switch (val.datatype) {
         case gds_float32:
	    value = *reinterpret_cast<float*>(val.value);
            break;
         case gds_float64:
	    value = *reinterpret_cast<double*>(val.value);
            break;
         case gds_complex32:
	    value = *reinterpret_cast<complex<float>*>(val.value);
            break;
         default: 
            return false;
      }
      return true;
   }


   bool diagObject::getParam (gdsDataObject& obj, const string& pName,
                     complex<double>& value) const
   {
      // get parameter
      gdsDatum 		val;
      if (!getParam (obj, pName, val)) {
         return false;
      }
      // verify type and dimension
      if ((val.elNumber() != 1) || 
         ((val.datatype != gds_float32) &&
         (val.datatype != gds_float64) &&
         (val.datatype != gds_complex32) &&
         (val.datatype != gds_complex64))) {
         return false;
      } 
      // copy value
      switch (val.datatype) {
         case gds_float32:
            value = *((float*) val.value);
            break;
         case gds_float64:
            value = *((double*) val.value);
            break;
         case gds_complex32:
            value = *((complex<float>*) val.value);
            break;
         case gds_complex64:
            value = *((complex<double>*) val.value);
            break;
         default: 
            return false;
      }
      return true;
   }


   bool diagObject::getParam (gdsDataObject& obj, const string& pName,
                     bool& value) const
   {
      // get parameter
      gdsDatum 		val;
      if (!getParam (obj, pName, val)) {
         return false;
      }
      // verify type and dimension
      if ((val.elNumber() != 1) || 
         ((val.datatype != gds_bool))) {
         return false;
      } 
      // copy value
      value = *((bool*) val.value);
      return true;
   }


   bool diagObject::setData (gdsDataObject& obj, const void* value,
                     int dim1, int dim2, gdsDataType Datatype) const
   {
      semlock		lockit (obj.mux);
   
      // determine data type
      if (Datatype == gds_void) {
         Datatype = datatype;
      }
   
      gdsDatum 		dat (Datatype, value, dim1, dim2);
      obj.assignDatum (dat);
      return true;
   }


   bool diagGlobal::isValid (const string& Name, bool write,
                     string* normName) const
   {
      if (Name.find ('.') != string::npos) {
         return false;
      }
   
      return diagObject::isValid (string (".") + Name, write, normName);
   }


   diagGlobal::diagGlobal ()
   : diagObject (gdsDataObject::parameterObj, stObjectTypeGlobal,
                stGlobal, 0, 0, gds_void, 0, 0, 0)
   {
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stInputSource, 0, 0, gds_string, 
                                   stInputSourceDef, 1));
      dParams.push_back (diagParam (stTestType, 0, 0, gds_string, 
                                   stTestTypeDef, 1));
      dParams.push_back (diagParam (stTestName, 0, 0, gds_string, 
                                   stTestNameDef, 1));
      dParams.push_back (diagParam (stSupervisory, 0, 0, gds_string, 
                                   stSupervisoryDef, 1));
      dParams.push_back (diagParam (stTestIterator, 0, 0, gds_string, 
                                   stTestIteratorDef, 1));
      dParams.push_back (diagParam (stTestComment, 0, 0, gds_string, 
                                   stTestCommentDef, 1));
      dParams.push_back (diagParam (stTestTime, 0, 0, gds_int64, 
                                   &stTestTimeDef, 1, "ns"));
      dParams.push_back (diagParam (stTestTimeUTC, 0, 0, gds_string, 
                                   stTestTimeUTCDef, 1, "ISO -8601", false));
   }


   diagDef::diagDef ()
   : diagObject (gdsDataObject::parameterObj, stObjectTypeDef,
                stDef, 0, 0, gds_void, 0, 0, 0)
   {
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stDefAllowCancel, 0, 0, gds_bool, 
                                   &stDefAllowCancelDef, 1));
      dParams.push_back (diagParam (stDefNoStimulus, 0, 0, gds_bool, 
                                   &stDefNoStimulusDef, 1));
      dParams.push_back (diagParam (stDefNoAnalysis, 0, 0, gds_bool, 
                                   &stDefNoAnalysisDef, 1));
      dParams.push_back (diagParam (stDefKeepTraces, 0, 0, gds_int32, 
                                   &stDefKeepTracesDef, 1));
      dParams.push_back (diagParam (stDefSiteDefault, 0, 0, gds_int8, 
                                   &stDefSiteDefaultDef, 1));
      dParams.push_back (diagParam (stDefSiteForce, 0, 0, gds_int8, 
                                   &stDefSiteForceDef, 1));
      dParams.push_back (diagParam (stDefIfoDefault, 0, 0, gds_int8, 
                                   &stDefIfoDefaultDef, 1));
      dParams.push_back (diagParam (stDefIfoForce, 0, 0, gds_int8, 
                                   &stDefIfoForceDef, 1));
      dParams.push_back (diagParam (stDefPlotWindowLayout, 10, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stDefPlotWindows, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stDefCalibrationRecords, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stDefReconnect, 0, 0, gds_bool, 
                                   &stDefReconnectDef, 1));
   }


   diagLidax::diagLidax ()
   : diagObject (gdsDataObject::parameterObj, stObjectTypeLidax,
                stLidax, 0, 0, gds_void, 0, 0, 0)
   {
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stLidaxServer, maxLidaxServer, 0, gds_string, 
                                   0, 1));
      dParams.push_back (diagParam (stLidaxUDN, maxLidaxServer, 0, gds_string, 
                                   0, 1));
      dParams.push_back (diagParam (stLidaxChannel, maxLidaxServer, 
                                   maxLidaxChannels, gds_channel, 0, 1));
      dParams.push_back (diagParam (stLidaxRate, maxLidaxServer, 
                                   maxLidaxChannels, gds_float64, 0, 1));
   }


   diagSync::diagSync ()
   : diagObject (gdsDataObject::parameterObj, stObjectTypeSync,
                stSync, 0, 0, gds_void, 0, 0, 0)       
   {
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stSyncType, 0, 0, gds_int32, 
                                   &stSyncTypeDef, 1));
      dParams.push_back (diagParam (stSyncStart, 0, 0, gds_int64, 
                                   &stSyncStartDef, 1, "ns"));
      dParams.push_back (diagParam (stSyncWait, 0, 0, gds_float64, 
                                   &stSyncWaitDef, 1, "s"));
      dParams.push_back (diagParam (stSyncRepeat, 0, 0, gds_int32, 
                                   &stSyncRepeatDef, 1));
      dParams.push_back (diagParam (stSyncRepeatRate, 0, 0, gds_float64, 
                                   &stSyncRepeatRateDef, 1, "s"));
      dParams.push_back (diagParam (stSyncSlowDown, 0, 0, gds_float64, 
                                   &stSyncSlowDownDef, 1, "s"));
      dParams.push_back (diagParam (stSyncWaitForStart, 0, 0, gds_channel, 
                                   0, 1));
      dParams.push_back (diagParam (stSyncWaitAtEachStep, 0, 0, gds_channel, 
                                   0, 1));
      dParams.push_back (diagParam (stSyncSignalEndOfStep, 0, 0, gds_channel, 
                                   0, 1));
      dParams.push_back (diagParam (stSyncSignalEnd, 0, 0, gds_channel, 
                                   0, 1));
   }


   diagEnv::diagEnv ()
   : diagObject (gdsDataObject::parameterObj, stObjectTypeEnv,
                stEnv, maxEnv, 0, gds_void, 0, 0, 0)
   {
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stEnvActive, 0, 0, gds_bool, 
                                   &stEnvActiveDef, 1));
      dParams.push_back (diagParam (stEnvChannel, 0, 0, gds_channel, 
                                   0, 1));
      dParams.push_back (diagParam (stEnvWaveform, 0, 0, gds_string, 
                                   0, 1));
      dParams.push_back (diagParam (stEnvPoints, 0, 0, gds_float32, 
                                   0, -1));
      dParams.push_back (diagParam (stEnvWait, 0, 0, gds_float64, 
                                   &stEnvWaitDef, 1, "s"));
   }


   diagScan::diagScan ()
   : diagObject (gdsDataObject::parameterObj, stObjectTypeScan,
                stScan, maxScan, 0, gds_void, 0, 0, 0)
   {
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32,
                                   0, 1, "", false));
      dParams.push_back (diagParam (stScanActive, 0, 0, gds_bool, 
                                   &stScanActiveDef, 1));
      dParams.push_back (diagParam (stScanChannel, 0, 0, gds_channel, 
                                   0, 1));
      dParams.push_back (diagParam (stScanType, 0, 0, gds_int32, 
                                   &stScanTypeDef, 1));
      dParams.push_back (diagParam (stScanDirection, 0, 0, gds_int32, 
                                   &stScanDirectionDef, 1));
      dParams.push_back (diagParam (stScanParameter, 0, 0, gds_int32, 
                                   &stScanParameterDef, 1));
      dParams.push_back (diagParam (stScanFrequency, 0, 0, gds_float64, 
                                   &stScanFrequencyDef, 1, "Hz"));
      dParams.push_back (diagParam (stScanAmplitude, 0, 0, gds_float64, 
                                   &stScanAmplitudeDef, 1));
      dParams.push_back (diagParam (stScanOffset, 0, 0, gds_float64, 
                                   &stScanOffsetDef, 1));
      dParams.push_back (diagParam (stScanStart, 0, 0, gds_float64, 
                                   &stScanStartDef, 1));
      dParams.push_back (diagParam (stScanStop, 0, 0, gds_float64, 
                                   &stScanStopDef, 1));
      dParams.push_back (diagParam (stScanN, 0, 0, gds_int32, 
                                   &stScanNDef, 1));
      dParams.push_back (diagParam (stScanPoints, 0, 0, gds_float32, 
                                   0, -1));
      dParams.push_back (diagParam (stScanWait, 0, 0, gds_float64, 
                                   stScanWaitDef, 2, "s"));
   }


   diagFind::diagFind ()
   : diagObject (gdsDataObject::parameterObj, stObjectTypeFind,
                stFind, 0, 0, gds_void, 0, 0, 0)       
   {
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stFindEnable, 0, 0, gds_bool, 
                                   &stFindEnableDef, 1));
      dParams.push_back (diagParam (stFindChange, 0, 0, gds_bool, 
                                   &stFindChangeDef, 1));
      dParams.push_back (diagParam (stFindType, 0, 0, gds_int32, 
                                   &stFindTypeDef, 1));
      dParams.push_back (diagParam (stFindValue, 0, 0, gds_float64, 
                                   &stFindValueDef, 1));
      dParams.push_back (diagParam (stFindFunction, 0, 0, gds_int32, 
                                   &stFindFunctionDef, 1));
      dParams.push_back (diagParam (stFindParam, 0, 0, gds_float64, 
                                   stFindParamDef, 2));
      dParams.push_back (diagParam (stFindMethod, 0, 0, gds_int32, 
                                   &stFindMethodDef, 1));
   }


   diagPlot::diagPlot ()
   : diagObject (gdsDataObject::settingsObj, stObjectTypePlot,
                stPlot, maxPlot, maxPlot, gds_void, 0, 0, 0)
   {
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stPlotName, 0, 0, gds_string, 0, 1));
      dParams.push_back (diagParam (stPlotTracesGraphType, 0, 0, gds_string, 0, 1));
      dParams.push_back (diagParam (stPlotTracesActive, maxTraces, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotTracesAChannel, maxTraces, 0, gds_channel, 0, 1));
      dParams.push_back (diagParam (stPlotTracesBChannel, maxTraces, 0, gds_channel, 0, 1));
      dParams.push_back (diagParam (stPlotTracesPlotStyle, 0, 0, gds_int32, 0, maxTraces));
      dParams.push_back (diagParam (stPlotTracesLineAttrColor, 0, 0, gds_int32, 0, maxTraces));
      dParams.push_back (diagParam (stPlotTracesLineAttrStyle, 0, 0, gds_int32, 0, maxTraces));
      dParams.push_back (diagParam (stPlotTracesLineAttrWidth, 0, 0, gds_float64, 0, maxTraces));
      dParams.push_back (diagParam (stPlotTracesMarkerAttrColor, 0, 0, gds_int32, 0, maxTraces));
      dParams.push_back (diagParam (stPlotTracesMarkerAttrStyle, 0, 0, gds_int32, 0, maxTraces));
      dParams.push_back (diagParam (stPlotTracesMarkerAttrSize, 0, 0, gds_float64, 0, maxTraces));
      dParams.push_back (diagParam (stPlotTracesBarAttrColor, 0, 0, gds_int32, 0, maxTraces));
      dParams.push_back (diagParam (stPlotTracesBarAttrStyle, 0, 0, gds_int32, 0, maxTraces));
      dParams.push_back (diagParam (stPlotTracesBarAttrWidth, 0, 0, gds_float64, 0, maxTraces));
      dParams.push_back (diagParam (stPlotRangeAxisScale, 0, 0, gds_int32, 0, 2));
      dParams.push_back (diagParam (stPlotRangeRange, 0, 0, gds_int32, 0, 2));
      dParams.push_back (diagParam (stPlotRangeRangeFrom, 0, 0, gds_float64, 0, 2));
      dParams.push_back (diagParam (stPlotRangeRangeTo, 0, 0, gds_float64, 0, 2));
      dParams.push_back (diagParam (stPlotRangeBin, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotRangeBinLogSpacing, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotUnitsXValues, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotUnitsYValues, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotUnitsXUnit, 0, 0, gds_string, 0, 1));
      dParams.push_back (diagParam (stPlotUnitsYUnit, 0, 0, gds_string, 0, 1));
      dParams.push_back (diagParam (stPlotUnitsXMag, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotUnitsYMag, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotUnitsXSlope, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotUnitsYSlope, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotUnitsXOffset, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotUnitsYOffset, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotCursorActive, 0, 0, gds_bool, 0, 2));
      dParams.push_back (diagParam (stPlotCursorTrace, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotCursorStyle, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotCursorType, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotCursorX, 0, 0, gds_float64, 0, 2));
      dParams.push_back (diagParam (stPlotCursorH, 0, 0, gds_float64, 0, 2));
      dParams.push_back (diagParam (stPlotCursorValid, 0, 0, gds_bool, 0, maxTraces));
      dParams.push_back (diagParam (stPlotCursorY, 0, 0, gds_float64, 0, 2*maxTraces));
      dParams.push_back (diagParam (stPlotCursorN, 0, 0, gds_float64, 0, maxTraces));
      dParams.push_back (diagParam (stPlotCursorXDiff, 0, 0, gds_float64, 0, maxTraces));
      dParams.push_back (diagParam (stPlotCursorYDiff, 0, 0, gds_float64, 0, maxTraces));
      dParams.push_back (diagParam (stPlotCursorMean, 0, 0, gds_float64, 0, maxTraces));
      dParams.push_back (diagParam (stPlotCursorRMS, 0, 0, gds_float64, 0, maxTraces));
      dParams.push_back (diagParam (stPlotCursorStdDev, 0, 0, gds_float64, 0, maxTraces));
      dParams.push_back (diagParam (stPlotCursorSum, 0, 0, gds_float64, 0, maxTraces));
      dParams.push_back (diagParam (stPlotCursorSqrSum, 0, 0, gds_float64, 0, maxTraces));
      dParams.push_back (diagParam (stPlotCursorArea, 0, 0, gds_float64, 0, maxTraces));
      dParams.push_back (diagParam (stPlotCursorRMSArea, 0, 0, gds_float64, 0, maxTraces));
      dParams.push_back (diagParam (stPlotCursorPeakX, 0, 0, gds_float64, 0, maxTraces));
      dParams.push_back (diagParam (stPlotCursorPeakY, 0, 0, gds_float64, 0, maxTraces));
      dParams.push_back (diagParam (stPlotConfigAutoConfig, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotConfigRespectUser, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotConfigAutoAxes, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotConfigAutoBin, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotConfigAutoTimeAdjust, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotStyleTitle, 0, 0, gds_string, 0, 1));
      dParams.push_back (diagParam (stPlotStyleTitleAlign, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotStyleTitleAngle, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotStyleTitleColor, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotStyleTitleFont, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotStyleTitleSize, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotStyleMargin, 0, 0, gds_float64, 0, 4));
      dParams.push_back (diagParam (stPlotAxisXTitle, 0, 0, gds_string, 0, 1));
      dParams.push_back (diagParam (stPlotAxisXAxisAttrAxisColor, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotAxisXAxisAttrLabelColor, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotAxisXAxisAttrLabelFont, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotAxisXAxisAttrLabelOffset, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotAxisXAxisAttrLabelSize, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotAxisXAxisAttrNdividions, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotAxisXAxisAttrTickLength, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotAxisXAxisAttrTitleOffset, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotAxisXAxisAttrTitleSize, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotAxisXAxisAttrTitleColor, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotAxisXGrid, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotAxisXBothSides, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotAxisXCenterTitle, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotAxisYTitle, 0, 0, gds_string, 0, 1));
      dParams.push_back (diagParam (stPlotAxisYAxisAttrAxisColor, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotAxisYAxisAttrLabelColor, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotAxisYAxisAttrLabelFont, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotAxisYAxisAttrLabelOffset, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotAxisYAxisAttrLabelSize, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotAxisYAxisAttrNdividions, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotAxisYAxisAttrTickLength, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotAxisYAxisAttrTitleOffset, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotAxisYAxisAttrTitleSize, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotAxisYAxisAttrTitleColor, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotAxisYGrid, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotAxisYBothSides, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotAxisYCenterTitle, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotLegendShow, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotLegendPlacement, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotLegendXAdjust, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotLegendYAdjust, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotLegendSymbolStyle, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotLegendTextStyle, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stPlotLegendSize, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stPlotLegendText, maxTraces, 0, gds_string, 0, 1));
      dParams.push_back (diagParam (stPlotParamShow, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotParamT0, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotParamAvg, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotParamSpecial, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotParamTimeFormatUTC, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stPlotParamTextSize, 0, 0, gds_float64, 0, 1));
   }


   diagCalibration::diagCalibration ()
   : diagObject (gdsDataObject::settingsObj, stObjectTypeCalibration,
                stCal, maxCalibration, 0, gds_void, 0, 0, 0)
   {
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stCalChannel, 0, 0, gds_string, 0, 1));
      dParams.push_back (diagParam (stCalTime, 0, 0, gds_int64, 0, 1, "ns"));
      dParams.push_back (diagParam (stCalDuration, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stCalReference, 0, 0, gds_string, 0, 1));
      dParams.push_back (diagParam (stCalUnit, 0, 0, gds_string, 0, 1));
      dParams.push_back (diagParam (stCalConversion, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stCalOffset, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stCalTimeDelay, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stCalTransferFunction, 0, 0, gds_float64, 0, -1));
      dParams.push_back (diagParam (stCalGain, 0, 0, gds_float64, 0, 1));
      dParams.push_back (diagParam (stCalPoles, 0, 0, gds_complex64, 0, -1));
      dParams.push_back (diagParam (stCalZeros, 0, 0, gds_complex64, 0, -1));
      dParams.push_back (diagParam (stCalDefault, 0, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (stCalPreferredMag, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stCalPreferredD, 0, 0, gds_int32, 0, 1));
      dParams.push_back (diagParam (stCalComment, 0, 0, gds_string, 0, 1));
   }



   const string diagIndex::indexIndent = "      ";
   const string diagIndex::indexCat = ":\n";
   const string diagIndex::indexEqual = " = ";
   const string diagIndex::indexEnd = ";\n";

   recursivemutex diagIndex::indexmux = recursivemutex();

   diagIndex::diagIndex ()
   : diagObject (gdsDataObject::resultObj, stObjectTypeIndex,
                stIndex, 0, 0, gds_void, 0, 0, 0)
   {
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stIndexEntry, 2 * maxResult, 0, 
                                   gds_string, 0, 1));
   }


   string masterindexEntry (int index, const string& entry) 
   {
      return  diagIndex::indexIndent + 
         diagObject::makeName (stIndexEntry, index) + 
         diagIndex::indexEqual + entry +  diagIndex::indexEnd;
   }


   ostream& diagIndex::channelEntry (ostream& os, int num, 
                     const string& name, char type)
   {
      string		channel;
      switch (type) {
         case 'A':
         case 'a':
            {
               channel = ieChannelA;
               break;
            }
         case 'B':
         case 'b':
            {
               channel = ieChannelB;
               break;
            }
         default:
            {
               channel = ieChannel;
               break;
            }
      }
      os << diagIndex::indexIndent << makeName (channel, num) <<
         diagIndex::indexEqual << name << diagIndex::indexEnd;
      return os;
   }


   ostream& diagIndex::resultEntry (ostream& os, int res, int ofs, 
                     int len, int i1, int i2) 
   {
      os << diagIndex::indexIndent <<
         diagObject::makeName (ieName, i1, i2) << 
         diagIndex::indexEqual << 
         diagObject::makeName (stResult, res) << diagIndex::indexEnd;
      os << diagIndex::indexIndent <<
         diagObject::makeName (ieOffset, i1, i2) << 
         diagIndex::indexEqual << ofs << diagIndex::indexEnd;
      os << diagIndex::indexIndent <<
         diagObject::makeName (ieLength, i1, i2) << 
         diagIndex::indexEqual << len << diagIndex::indexEnd;
      return os;
   }


   bool diagIndex::getMasterIndex (gdsDataObject& obj,
                     masterindex& index) const
   {
      semlock		lockit (indexmux);
      string		mindex;
   
      // get master index entry
      string mentry = makeName (stIndexEntry, 0);
      if (!getParam (obj, mentry, mindex)) {
         // first time: create master
         mindex = string (icMasterIndex) + indexCat +
            masterindexEntry (0, icMasterIndex);
         if (!setParam (obj, mentry, mindex)) {
            return false;
         }
      }
   
      // fill map structure
      index.clear();
      istringstream	is (mindex);
      // skip category
      char		c;
      while ((is >> c) && (c != ':')) {
      }
      // read entries
      do {
         while ((is >> c) && (c != '[')) {
         }
         int	nentry;
         is >> nentry;
         while ((is >> c) && (c != '=')) {
         }
         ostringstream	entry;
         while ((is >> c) && (c != ';')) {
            entry << c;
         }
         if (!is) {
            break;
         }
         index[entry.str()] = nentry;
      } while (is);
   
      return true;
   }


   bool diagIndex::setMasterIndex (gdsDataObject& obj,
                     const masterindex& index) const
   {
      semlock		lockit (indexmux);
      ostringstream		entry;
   
      // sort entries in master index
      string mentry = makeName (stIndexEntry, 0);
      vector<string>	entries;
      for (masterindex::const_iterator iter = index.begin();
          iter != index.end(); iter++) {
         entries.push_back (masterindexEntry (iter->second, iter->first));
      }
      sort (entries.begin(), entries.end());
   
      // write master index
      entry << icMasterIndex << indexCat;
      for (vector<string>::const_iterator iter = entries.begin();
          iter != entries.end(); iter++) {
         entry << *iter;
      }
      string eval = entry.str();
      if (!eval.empty() && (eval[eval.size() - 1] == '\n')) {
         eval.erase (eval.size() - 1, 1);
      }
      return setParam (obj, mentry, eval);
   }


   bool diagIndex::isCategory (const string& cat, int step,
                     string* catname) const 
   {
      // check if category is valid
      const char* const*	ic = icAll;
      while (*ic != 0) {
         if (compareTestNames (cat, *ic) == 0) {
            if (catname != 0) {
               *catname = makeName (*ic, step, -1);
            }
            return true;
         }
         ic++;
      }
      if (catname != 0) {
         *catname = "";
      }
      return false;
   }


   bool diagIndex::setEntry (gdsDataObject& obj, const string& category,
                     int step, const string& entry) const
   {
      semlock		lockit (indexmux);
      masterindex	index;
      string		catname;
      int		nentry;
   
      // get master index
      if (!isCategory (category, step, &catname) || 
         !getMasterIndex (obj, index)) {
         return false;
      }
      // find entry number
      if (index.find (catname) == index.end()) {
         // new categroy: find a free slot
         vector<bool> 	entryset (index.size() + 1, false);
         for (masterindex::iterator iter = index.begin();
             iter != index.end(); iter++) {
            if (iter->second < (int)entryset.size()) {
               entryset[iter->second] = true;
            }
         }
         nentry = 0;
         for (vector<bool>::iterator iter = entryset.begin();
             iter != entryset.end(); iter++, nentry++) {
            if (!*iter) {
               break;
            }
         }
         index[catname] = nentry;
         // write updated master index
         if (!setMasterIndex (obj, index)) {
            return false;
         }
      }
      else {
         nentry = index[catname];
      }
   
      // write entry
      string ename = makeName (stIndexEntry, nentry);
      string eval = catname + indexCat + entry;
      if (!eval.empty() && (eval[eval.size() - 1] == '\n')) {
         eval.erase (eval.size() - 1, 1);
      }
      return setParam (obj, ename, eval);
   }


   bool diagIndex::getEntry (gdsDataObject& obj, const string& category,
                     int step, string& entry) const
   {
      semlock		lockit (indexmux);
      masterindex	index;
      string		catname;
   
      // get master index
      if (!isCategory (category, step, &catname) || 
         !getMasterIndex (obj, index)) {
         return false;
      }
      // find entry number
      if (index.find (catname) == index.end()) {
         return false;
      }
   
      // read entry
      string ename = makeName (stIndexEntry, index[catname]);
      if (!getParam (obj, ename, entry)) {
         return false;
      }
      string::size_type pos = entry.find (":");
      if (pos == string::npos) {
         return false;
      }
      entry.erase (0, pos + 1);
      while (entry.find_first_of (" \n\t") == 0) {
         entry.erase (0, 1);
      }
      return true;
   }


   bool diagIndex::delEntry (gdsDataObject& obj, 
                     const string& category, int step) const
   {
      semlock		lockit (indexmux);
      masterindex	index;
      string		catname;
   
      // get master index
      if (!isCategory (category, step, &catname) || 
         !getMasterIndex (obj, index)) {
         return false;
      }
      // find entry number
      if (index.find (catname) == index.end()) {
         return false;
      }
      // can't delete master index
      if (index[catname] == 0) {
         return false;
      }
   
      // delete entry
      string ename = makeName (stIndexEntry, index[catname]);
      gdsStorage::prm_iterator iter = 
         find (obj.parameters.begin(), obj.parameters.end(), ename);
      if (iter == obj.parameters.end()) {
         return false;
      }
      else {
         obj.parameters.erase (iter);
         return true;
      }
   }


   diagResult::diagResult (const string& ID, 
                     int MaxDim1, int MaxDim2)
   : diagMultiObject (ID, gdsDataObject::resultObj, ID, stResult, 
                     maxResult, 0, gds_float32, 0, MaxDim1, MaxDim2) {
   
      subscribe (ID);
   }


   bool diagResult::subscribe (const string& ID) 
   {
      if (ID.empty()) {
         return false;
      }
      for (diagResultList::const_iterator iter = myself.begin();
          iter != myself.end(); iter++) {
         if (compareTestNames ((*iter)->ID(), ID) == 0) {
            return true;
         }
      }
      myself.push_back (this);
      return true;
   }


   const diagResult* diagResult::self (const string& ID) 
   {
      for (diagResultList::const_iterator iter = myself.begin();
          iter != myself.end(); iter++) {
         if (compareTestNames ((*iter)->ID(), ID) == 0) {
            return *iter;
         }
      }
      return 0;
   }


   diagTimeSeries::diagTimeSeries (bool subscribe)
   : diagResult (subscribe ? stObjectTypeTimeSeries : "", -1, -1) {
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stTimeSeriesSubtype, 0, 0, gds_int32, 
                                   &stTimeSeriesSubtypeDef, 1));
      dParams.push_back (diagParam (stTimeSeriest0, 0, 0, gds_int64, 
                                   &stTimeSeriest0Def, 1, "ns"));
      dParams.push_back (diagParam (stTimeSeriesdt, 0, 0, gds_float64, 
                                   &stTimeSeriesdtDef, 1, "s"));
      dParams.push_back (diagParam (stTimeSeriestp, 0, 0, gds_float64, 
                                   &stTimeSeriestpDef, 1, "s"));
      dParams.push_back (diagParam (stTimeSeriestf0, 0, 0, gds_int64, 
                                   &stTimeSeriestf0Def, 1, "s"));
      dParams.push_back (diagParam (stTimeSeriesf0, 0, 0, gds_float64, 
                                   &stTimeSeriesf0Def, 1, "Hz"));
      dParams.push_back (diagParam (stTimeSeriesAverageType, 0, 0, gds_int32, 
                                   &stTimeSeriesAverageTypeDef, 1));
      dParams.push_back (diagParam (stTimeSeriesAverages, 0, 0, gds_int32, 
                                   &stTimeSeriesAveragesDef, 1));
      dParams.push_back (diagParam (stTimeSeriesDecimation, 0, 0, 
                                   gds_int32, 
                                   &stTimeSeriesDecimationDef, 1));
      dParams.push_back (diagParam (stTimeSeriesDecimation1, 0, 0, 
                                   gds_int32, 
                                   &stTimeSeriesDecimation1Def, 1));
      dParams.push_back (diagParam (stTimeSeriesDecimationType, 0, 0, 
                                   gds_int32, 
                                   &stTimeSeriesDecimationTypeDef, 1));
      dParams.push_back (diagParam (stTimeSeriesDecimationFilter, 0, 0, 
                                   gds_string, 0, 1));
      dParams.push_back (diagParam (stTimeSeriesDecimationDelay, 0, 0, 
                                   gds_float64, 
                                   &stTimeSeriesDecimationDelayDef, 1, "s"));
      dParams.push_back (diagParam (stTimeSeriesTimeDelay, 0, 0, 
                                   gds_float64, 
                                   &stTimeSeriesTimeDelayDef, 1, "s"));
      dParams.push_back (diagParam (stTimeSeriesDelayTaps, 0, 0, gds_int32, 
                                   &stTimeSeriesDelayTapsDef, 1));
      dParams.push_back (diagParam (stTimeSeriesChannel, 0, 0, gds_channel, 
                                   stTimeSeriesChannelDef, 1));
      dParams.push_back (diagParam (stTimeSeriesN, 0, 0, gds_int32, 
                                   &stTimeSeriesNDef, 1));
      dParams.push_back (diagParam (stTimeSeriesMeasurementNumber, 0, 0, gds_int32, 
                                   0, 1));
   }


   diagSpectrum::diagSpectrum ()
   : diagResult (stObjectTypeSpectrum, -1, -1) {
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stSpectrumSubtype, 0, 0, gds_int32, 
                                   &stSpectrumSubtypeDef, 1));
      dParams.push_back (diagParam (stSpectrumf0, 0, 0, gds_float64, 
                                   &stSpectrumf0Def, 1, "Hz"));
      dParams.push_back (diagParam (stSpectrumdf, 0, 0, gds_float64, 
                                   &stSpectrumdfDef, 1, "Hz"));
      dParams.push_back (diagParam (stSpectrumt0, 0, 0, gds_int64, 
                                   &stSpectrumt0Def, 1, "ns"));
      dParams.push_back (diagParam (stSpectrumdt, 0, 0, gds_float64, 
                                   &stSpectrumdtDef, 1, "s"));
      dParams.push_back (diagParam (stSpectrumBW, 0, 0, gds_float64, 
                                   &stSpectrumBWDef, 1, "Hz"));
      dParams.push_back (diagParam (stSpectrumWindow, 0, 0, gds_int32, 
                                   &stSpectrumWindowDef, 1));
      dParams.push_back (diagParam (stSpectrumAverageType, 0, 0, gds_int32, 
                                   &stSpectrumAverageTypeDef, 1));
      dParams.push_back (diagParam (stSpectrumAverages, 0, 0, gds_int32, 
                                   &stSpectrumAveragesDef, 1));
      dParams.push_back (diagParam (stSpectrumChannelA, 0, 0, gds_channel, 
                                   0, 1));
      dParams.push_back (diagParam (stSpectrumChannelB, maxCoefficients, 0, 
                                   gds_channel, 0, 1));
      dParams.push_back (diagParam (stSpectrumN, 0, 0, gds_int32, 
                                   &stSpectrumNDef, 1));
      dParams.push_back (diagParam (stSpectrumM, 0, 0, gds_int32, 
                                   &stSpectrumMDef, 1));
      dParams.push_back (diagParam (stSpectrumMeasurementNumber, 0, 0, gds_int32, 
                                   0, 1));
   }


   diagTransferFunction::diagTransferFunction ()
   : diagResult (stObjectTypeTransferFunction, -1, -1) {
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stTransferFunctionSubtype, 0, 0, gds_int32, 
                                   &stTransferFunctionSubtypeDef, 1));
      dParams.push_back (diagParam (stTransferFunctionf0, 0, 0, gds_float64, 
                                   &stTransferFunctionf0Def, 1, "Hz"));
      dParams.push_back (diagParam (stTransferFunctiondf, 0, 0, gds_float64, 
                                   &stTransferFunctiondfDef, 1, "Hz"));
      dParams.push_back (diagParam (stTransferFunctiont0, 0, 0, gds_int64, 
                                   &stTransferFunctiont0Def, 1, "ns"));
      dParams.push_back (diagParam (stTransferFunctionBW, 0, 0, gds_float64, 
                                   &stTransferFunctionBWDef, 1, "Hz"));
      dParams.push_back (diagParam (stTransferFunctionWindow, 0, 0, gds_int32, 
                                   &stTransferFunctionWindowDef, 1));
      dParams.push_back (diagParam (stTransferFunctionAverageType, 0, 0, gds_int32, 
                                   &stTransferFunctionAverageTypeDef, 1));
      dParams.push_back (diagParam (stTransferFunctionAverages, 0, 0, gds_int32, 
                                   &stTransferFunctionAveragesDef, 1));
      dParams.push_back (diagParam (stTransferFunctionChannelA, 
                                   0, 0, gds_channel, 0, 1));
      dParams.push_back (diagParam (stTransferFunctionChannelB, 
                                   maxCoefficients, 0, gds_channel, 0, 1));
      dParams.push_back (diagParam (stTransferFunctionN, 0, 0, gds_int32, 
                                   &stTransferFunctionNDef, 1));
      dParams.push_back (diagParam (stTransferFunctionM, 0, 0, gds_int32, 
                                   &stTransferFunctionMDef, 1));
      dParams.push_back (diagParam (stTransferFunctionMeasurementNumber, 0, 0, 
                                   gds_int32, 0, 1));
   }


   diagCoefficients::diagCoefficients ()
   : diagResult (stObjectTypeCoefficients, -1, -1) {
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stCoefficientsSubtype, 0, 0, gds_int32, 
                                   &stCoefficientsSubtypeDef, 1));
      dParams.push_back (diagParam (stCoefficientsf, 0, 0,
                                   gds_float64, 0, -1, "Hz"));
      dParams.push_back (diagParam (stCoefficientst0, 0, 0, gds_int64, 
                                   &stCoefficientst0Def, 1, "ns"));
      dParams.push_back (diagParam (stCoefficientsBW, 0, 0, gds_float64, 
                                   &stCoefficientsBWDef, 1, "Hz"));
      dParams.push_back (diagParam (stCoefficientsWindow, 0, 0, gds_int32, 
                                   &stCoefficientsWindowDef, 1));
      dParams.push_back (diagParam (stCoefficientsAverageType, 0, 0, gds_int32, 
                                   &stCoefficientsAverageTypeDef, 1));
      dParams.push_back (diagParam (stCoefficientsAverages, 0, 0, gds_int32, 
                                   &stCoefficientsAveragesDef, -1));
      dParams.push_back (diagParam (stCoefficientsChannelA, 
                                   maxCoefficients, 0, gds_channel, 0, 1));
      dParams.push_back (diagParam (stCoefficientsChannelB, 
                                   maxCoefficients, 0, gds_channel,0, 1));
      dParams.push_back (diagParam (stCoefficientsN, 0, 0, gds_int32, 
                                   &stCoefficientsNDef, 1));
      dParams.push_back (diagParam (stCoefficientsM, 0, 0, gds_int32, 
                                   &stCoefficientsMDef, 1));
      dParams.push_back (diagParam (stCoefficientsMeasurementNumber, 0, 0, 
                                   gds_int32, 0, 1));
   }


   diagMeasurementTable::diagMeasurementTable ()
   : diagResult (stObjectTypeMeasurementTable, -1, -1) {
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stMeasurementTableSubtype, 0, 0, gds_int32, 
                                   &stMeasurementTableSubtypeDef, 1));
      dParams.push_back (diagParam (stMeasurementTablet0, 0, 0, gds_int64, 
                                   &stMeasurementTablet0Def, 1, "ns"));
      dParams.push_back (diagParam (stMeasurementTableTableLength, 0, 0, 
                                   gds_int32, 
                                   &stMeasurementTableTableLengthDef, 1));
      dParams.push_back (diagParam (stMeasurementTableName, 
                                   maxMeasurementTableLength, 0, 
                                   gds_string, 
                                   stMeasurementTableNameDef, 1));
      dParams.push_back (diagParam (stMeasurementTableUnit, 
                                   maxMeasurementTableLength, 0, 
                                   gds_string, 
                                   stMeasurementTableUnitDef, 1));
      dParams.push_back (diagParam (stMeasurementTableDescription, 
                                   maxMeasurementTableLength, 0, 
                                   gds_string, 
                                   stMeasurementTableDescriptionDef, 1));
      dParams.push_back (diagParam (stMeasurementTableValueType, 
                                   maxMeasurementTableLength, 0, 
                                   gds_string, 
                                   stMeasurementTableValueTypeDef, 1));
   }


   diagChn::diagChn () : diagTimeSeries (false)
   {
      maxIndex2 = -1;
      name = stChannel;
      flag = gdsDataObject::rawdataObj;
   }


   bool diagChn::isValid (const string& Name, bool write,
                     string* normName) const
   {
      // separate name into data object and parameter name
      string		n1;
      string		n2;
      string		norm1;
      string		norm2;
      int		i11;
      int		i12;
      int		i21;
      int		i22;
   
      // check channel name
      diagStorage::analyzeName (Name, n1, i11, i12, n2, i21, i22);
      if (!chnIsValid (n1.c_str())) {
         return false;
      }
      n1 = makeName (stChannel, i11, i12);
      n2 = makeName (n2, i21, i22);
   
      // check object name
      if (!diagObjectName::isValid (n1, write, &norm1)) {
         return false;
      }
      if (n2.size() == 0) {
         return (maxDim1 != 0);
      }
      // check parameter name
      for (diagParamList::const_iterator iter = dParams.begin();
          iter != dParams.end(); iter++) {
         if (iter->isValid (n2, write, &norm2)) {
            if (normName != 0) {
               *normName = norm1 + "." + norm2;
            }
            return true;
         }
      }
      return false;
   }


   diagTest::diagTest (const string& ID)
   : diagMultiObject (ID, gdsDataObject::parameterObj, 
                     stObjectTypeTestParameter, stTestParameter, 
                     0, 0, gds_void, 0, 0, 0) {
      subscribe (ID);
      dParams.push_back (diagParam (stObjectType, 0, 0, gds_string, 
                                   0, 1, "", false));
      dParams.push_back (diagParam (stObjectFlag, 0, 0, gds_int32, 
                                   0, 1, "", false));
   }


   bool diagTest::subscribe (const string& ID) 
   {
      for (diagTestList::const_iterator iter = myself.begin();
          iter != myself.end(); iter++) {
         if (compareTestNames ((*iter)->ID(), ID) == 0) {
            return true;
         }
      }
      myself.push_back (this);
      return true;
   }


   const diagTest* diagTest::self (const string& ID) 
   {
      for (diagTestList::const_iterator iter = myself.begin();
          iter != myself.end(); iter++) {
         if (compareTestNames ((*iter)->ID(), ID) == 0) {
            return *iter;
         }
      }
      return 0;
   }


   testSineResponse::testSineResponse ()
   : diagTest (stSineResponse)
   {
      dParams.push_back (diagParam (stTestParameterSubtype, 0, 0, 
                                   gds_string, stSineResponse, 1, "", false));
      dParams.push_back (diagParam (srMeasurementTime, 0, 0, gds_float64, 
                                   srMeasurementTimeDef, 2, "s"));
      dParams.push_back (diagParam (srSettlingTime, 0, 0, gds_float64, 
                                   &srSettlingTimeDef, 1, "s"));
      dParams.push_back (diagParam (srAverageType, 0, 0, gds_int32, 
                                   &srAverageTypeDef, 1));
      dParams.push_back (diagParam (srAverages, 0, 0, gds_int32, 
                                   &srAveragesDef, 1));
      dParams.push_back (diagParam (srStimulusActive, 
                                   maxCoefficients, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (srStimulusChannel, 
                                   maxCoefficients, 0, gds_channel, 0, 1));
      dParams.push_back (diagParam (srStimulusReadback, 
                                   maxCoefficients, 0, gds_channel, 0, 1));
      dParams.push_back (diagParam (srStimulusFrequency, 
                                   maxCoefficients, 0, gds_float64, 
                                   &srStimulusFrequencyDef, 1, "Hz"));
      dParams.push_back (diagParam (srStimulusAmplitude, 
                                   maxCoefficients, 0, gds_float64, 
                                   &srStimulusAmplitudeDef, 1));
      dParams.push_back (diagParam (srStimulusOffset, 
                                   maxCoefficients, 0, gds_float64, 
                                   &srStimulusOffsetDef, 1));
      dParams.push_back (diagParam (srStimulusPhase, 
                                   maxCoefficients, 0, gds_float64, 
                                   &srStimulusPhaseDef, 1));
      dParams.push_back (diagParam (srMeasurementChannelActive, 
                                   maxCoefficients, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (srMeasurementChannel, 
                                   maxCoefficients, 0, gds_channel, 0, 1));
      dParams.push_back (diagParam (srHarmonicOrder, 0, 0, gds_int32, 
                                   &srHarmonicOrderDef, 1));
      dParams.push_back (diagParam (srWindow, 0, 0, gds_int32, 
                                   &srWindowDef, 1));
      dParams.push_back (diagParam (srFFTResult, 0, 0, gds_bool, 
                                   &srFFTResultDef, 1));
   }


   testSweptSine::testSweptSine ()
   : diagTest (stSweptSine)
   {
      dParams.push_back (diagParam (stTestParameterSubtype, 0, 0, 
                                   gds_string, stSweptSine, 1, "", false));
      dParams.push_back (diagParam (ssSweepType, 0, 0, gds_int32, 
                                   &ssSweepTypeDef, 1));
      dParams.push_back (diagParam (ssSweepDirection, 0, 0, gds_int32, 
                                   &ssSweepDirectionDef, 1));
      dParams.push_back (diagParam (ssStartFrequency, 0, 0, gds_float64, 
                                   &ssStartFrequencyDef, 1, "Hz"));
      dParams.push_back (diagParam (ssStopFrequency, 0, 0, gds_float64, 
                                   &ssStopFrequencyDef, 1, "Hz"));
      dParams.push_back (diagParam (ssNumberOfPoints, 0, 0, gds_int32, 
                                   &ssNumberOfPointsDef, 1));
      dParams.push_back (diagParam (ssSweepPoints, 0, 0, gds_float64, 
                                   0, -1, "Hz"));
      dParams.push_back (diagParam (ssAChannels, 0, 0, gds_int32, 
                                   &ssAChannelsDef, 1));
      dParams.push_back (diagParam (ssAverages, 0, 0, gds_int32, 
                                   &ssAveragesDef, 1));
      dParams.push_back (diagParam (ssMeasurementTime, 0, 0, gds_float64, 
                                   ssMeasurementTimeDef, 2, "s"));
      dParams.push_back (diagParam (ssSettlingTime, 0, 0, gds_float64, 
                                   &ssSettlingTimeDef, 1));
      dParams.push_back (diagParam (ssStimulusChannel, 
                                   0, 0, gds_channel, 0, 1));
      dParams.push_back (diagParam (ssStimulusReadback, 
                                   0, 0, gds_channel, 0, 1));
      dParams.push_back (diagParam (ssStimulusAmplitude, 0, 0, gds_float64, 
                                   &ssStimulusAmplitudeDef, 1));
      dParams.push_back (diagParam (ssMeasurementChannelActive, 
                                   maxCoefficients, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (ssMeasurementChannel, 
                                   maxCoefficients, 0, gds_channel, 0, 1));
      dParams.push_back (diagParam (ssHarmonicOrder, 0, 0, gds_int32, 
                                   &ssHarmonicOrderDef, 1));
      dParams.push_back (diagParam (ssWindow, 0, 0, gds_int32, 
                                   &ssWindowDef, 1));
      dParams.push_back (diagParam (ssFFTResult, 0, 0, gds_bool, 
                                   &ssFFTResultDef, 1));
   }


   testFFT::testFFT ()
   : diagTest (stFFT)  
   {
      dParams.push_back (diagParam (stTestParameterSubtype, 0, 0, 
                                   gds_string, stFFT, 1, "", false));
      dParams.push_back (diagParam (fftStartFrequency, 0, 0, gds_float64, 
                                   &fftStartFrequencyDef, 1, "Hz"));
      dParams.push_back (diagParam (fftStopFrequency, 0, 0, gds_float64, 
                                   &fftStopFrequencyDef, 1, "Hz"));
      dParams.push_back (diagParam (fftBW, 0, 0, gds_float64, 
                                   &fftBWDef, 1, "Hz"));
      dParams.push_back (diagParam (fftOverlap, 0, 0, gds_float64, 
                                   &fftOverlapDef, 1));
      dParams.push_back (diagParam (fftWindow, 0, 0, gds_int32, 
                                   &fftWindowDef, 1));
      dParams.push_back (diagParam (fftRemoveDC, 0, 0, gds_bool, 
                                   &fftRemoveDCDef, 1));
      dParams.push_back (diagParam (fftAChannels, 0, 0, gds_int32, 
                                   &fftAChannelsDef, 1));
      dParams.push_back (diagParam (fftAverageType, 0, 0, gds_int32, 
                                   &fftAverageTypeDef, 1));
      dParams.push_back (diagParam (fftAverages, 0, 0, gds_int32, 
                                   &fftAveragesDef, 1));
      dParams.push_back (diagParam (fftSettlingTime, 0, 0, gds_float64, 
                                   &fftSettlingTimeDef, 1));
      dParams.push_back (diagParam (fftStimulusActive, 
                                   maxCoefficients, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (fftStimulusType, maxCoefficients, 0, 
                                   gds_int32, &fftStimulusTypeDef, 1));
      dParams.push_back (diagParam (fftStimulusChannel, 
                                   maxCoefficients, 0, gds_channel, 0, 1));
      dParams.push_back (diagParam (fftStimulusReadback, 
                                   maxCoefficients, 0, gds_channel, 0, 1));
      dParams.push_back (diagParam (fftStimulusFrequency, maxCoefficients, 
                                   0, gds_float64, 
                                   &fftStimulusFrequencyDef, 1, "Hz"));
      dParams.push_back (diagParam (fftStimulusAmplitude, maxCoefficients, 
                                   0, gds_float64, 
                                   &fftStimulusAmplitudeDef, 1));
      dParams.push_back (diagParam (fftStimulusOffset, maxCoefficients, 0, 
                                   gds_float64, &fftStimulusOffsetDef, 1));
      dParams.push_back (diagParam (fftStimulusPhase, maxCoefficients, 0, 
                                   gds_float64, &fftStimulusPhaseDef, 1));
      dParams.push_back (diagParam (fftStimulusRatio, maxCoefficients, 0, 
                                   gds_float64, &fftStimulusRatioDef, 1));
      dParams.push_back (diagParam (fftStimulusFrequencyRange, 
                                   maxCoefficients, 0, gds_float64, 
                                   &fftStimulusFrequencyRangeDef, 1, "Hz"));
      dParams.push_back (diagParam (fftStimulusAmplitudeRange, 
                                   maxCoefficients, 0, gds_float64, 
                                   &fftStimulusAmplitudeRangeDef, 1));
      dParams.push_back (diagParam (fftStimulusFilterCmd, 
                                   maxCoefficients, 0, gds_string, 0, 1));
      dParams.push_back (diagParam (fftStimulusPoints, maxCoefficients, 0, 
                                   gds_float64, 0, -1));
      dParams.push_back (diagParam (fftMeasurementChannelActive, 
                                   maxCoefficients, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (fftMeasurementChannel, 
                                   maxCoefficients, 0, gds_channel, 0, 1));
   }


   testTimeSeries::testTimeSeries ()
   : diagTest (stTimeSeries)
   {
      dParams.push_back (diagParam (stTestParameterSubtype, 0, 0, 
                                   gds_string, stTimeSeries, 1, "", false));
      dParams.push_back (diagParam (tsMeasurementTime, 0, 0, gds_float64, 
                                   &tsMeasurementTimeDef, 1, "s"));
      dParams.push_back (diagParam (tsPreTriggerTime, 0, 0, gds_float64, 
                                   &tsPreTriggerTimeDef, 1, "s"));
      dParams.push_back (diagParam (tsSettlingTime, 0, 0, gds_float64, 
                                   &tsSettlingTimeDef, 1));
      dParams.push_back (diagParam (tsDeadTime, 0, 0, gds_float64, 
                                   &tsDeadTimeDef, 1));
      dParams.push_back (diagParam (tsBW, 0, 0, gds_float64, 
                                   &tsBWDef, 1, "Hz"));
      dParams.push_back (diagParam (tsIncludeStatistics, 0, 0, gds_bool, 
                                   &tsIncludeStatisticsDef, 1, ""));
      dParams.push_back (diagParam (tsAverages, 0, 0, gds_int32, 
                                   &tsAveragesDef, 1));
      dParams.push_back (diagParam (tsAverageType, 0, 0, gds_int32, 
                                   &tsAverageTypeDef, 1));
      dParams.push_back (diagParam (tsFilter, 0, 0, gds_string, 
                                   tsFilterDef, 1));
      dParams.push_back (diagParam (tsStimulusActive, 
                                   maxCoefficients, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (tsStimulusType, maxCoefficients, 0, 
                                   gds_int32, &tsStimulusTypeDef, 1));
      dParams.push_back (diagParam (tsStimulusChannel, 
                                   maxCoefficients, 0, gds_channel, 0, 1));
      dParams.push_back (diagParam (tsStimulusReadback, 
                                   maxCoefficients, 0, gds_channel, 0, 1));
      dParams.push_back (diagParam (tsStimulusFrequency, maxCoefficients, 
                                   0, gds_float64, 
                                   &tsStimulusFrequencyDef, 1, "Hz"));
      dParams.push_back (diagParam (tsStimulusAmplitude, maxCoefficients, 
                                   0, gds_float64, 
                                   &tsStimulusAmplitudeDef, 1));
      dParams.push_back (diagParam (tsStimulusOffset, maxCoefficients, 0, 
                                   gds_float64, &tsStimulusOffsetDef, 1));
      dParams.push_back (diagParam (tsStimulusPhase, maxCoefficients, 0, 
                                   gds_float64, &tsStimulusPhaseDef, 1));
      dParams.push_back (diagParam (tsStimulusRatio, maxCoefficients, 0, 
                                   gds_float64, &tsStimulusRatioDef, 1));
      dParams.push_back (diagParam (tsStimulusFrequencyRange, 
                                   maxCoefficients, 0, gds_float64, 
                                   &tsStimulusFrequencyRangeDef, 1, "Hz"));
      dParams.push_back (diagParam (tsStimulusAmplitudeRange, 
                                   maxCoefficients, 0, gds_float64, 
                                   &tsStimulusAmplitudeRangeDef, 1));
      dParams.push_back (diagParam (tsStimulusFilterCmd, 
                                   maxCoefficients, 0, gds_string, 0, 1));
      dParams.push_back (diagParam (tsStimulusPoints, maxCoefficients, 0, 
                                   gds_float64, 0, -1));
      dParams.push_back (diagParam (tsMeasurementChannelActive, 
                                   maxCoefficients, 0, gds_bool, 0, 1));
      dParams.push_back (diagParam (tsMeasurementChannel, 
                                   maxCoefficients, 0, gds_channel, 0, 1));
   }


   diagStorage::diagStorage (const string& test)
   : gdsStorage (stCreator, ""),
   TestType (0), TestName (0), Supervisory (0),
   TestIterator (0), TestTime (0), TestTimeUTC (0), Def (0), Lidax (0),
   Sync (0), Env (), Scan (), Find (0), Test (0), Channel (), Index (0),
   Result (), Plot (), Calibration (), ReferenceTraces ()
   {
      init (test);
   }


   diagStorage::~diagStorage ()
   {
   }


   void diagStorage::init (const string& test) 
   {
      // test
      //cerr << "Init TEST with " << test << endl;
      setType (diagGlobal::self().getType());
      setFlag (diagGlobal::self().getFlag());
      if (TestType == 0) {
         TestType = new (nothrow) gdsParameter 
            (stTestType, test.c_str());
         if (TestType != 0) {
            addParameter (*TestType, false);
         }
      }
      if (TestName == 0) {
         TestName = new (nothrow) 
            gdsParameter (stTestName, "1998-2001, by Daniel Sigg");
         if (TestName != 0) {
            addParameter (*TestName, false);
         }
      }
      if (Supervisory == 0) {
         Supervisory = new (nothrow) 
            gdsParameter (stSupervisory, stSupervisoryDef);
         if (Supervisory != 0) {
            addParameter (*Supervisory, false);
         }
      }
      if (TestIterator == 0) {
         TestIterator = new (nothrow) 
            gdsParameter (stTestIterator, stTestIteratorDef);
         if (TestIterator != 0) {
            addParameter (*TestIterator, false);
         }
      }
   
      // time
      if (TestTime == 0) {
         tainsec_t		now = TAInow ();
         utc_t		utc;
         char		s[100];
         TestTime = new (nothrow) gdsParameter (stTestTime, gds_int64, 
                              &now, "ns");
         if (TestTime != 0) {
            addParameter (*TestTime, false);
         }
         TAIntoUTC (now, &utc);
         strftime (s, 100, "%Y-%m-%d %H:%M:%S", &utc);
         if (TestTimeUTC != 0) {
            eraseParameter (TestTimeUTC->name);
            TestTimeUTC = 0;
         }
         TestTimeUTC = new (nothrow)
            gdsParameter (stTestTimeUTC, s, "ISO-8601");
         if (TestTimeUTC != 0) {
            addParameter (*TestTimeUTC, false);
         }
      }
   
      // globals
      if (Def == 0) {
         Def = diagDef::self().newObject (0);
         if (Def != 0) {
            addData (*Def, false);
         }
      }
      if (Lidax == 0) {
         Lidax = diagLidax::self().newObject (0);
         if (Lidax != 0) {
            addData (*Lidax, false);
         }
      }
      if (Sync == 0) {
         Sync = diagSync::self().newObject (0);
         if (Sync != 0) {
            addData (*Sync, false);
         }
      }
      if (Find == 0) {
         Find = diagFind::self().newObject (0);
         if (Find != 0) {
            addData (*Find, false);
         }
      }
      if ((int)Env.size() < maxEnv) {
         Env.resize (maxEnv, 0);
      }
      if ((int)Scan.size() < maxScan) {
         Scan.resize (maxScan, 0);
      }
   
      // results
      if ((int)Result.size() < maxResult) {
         Result.assign (maxResult, reinterpret_cast<gdsDataObject*>(0));
      }
      // reference traces
      if ((int)ReferenceTraces.size() < maxReferences) {
         ReferenceTraces.assign (maxReferences, reinterpret_cast<gdsDataObject*>(0));
      }
   
      // test
      if (Test == 0) {
         updateTest (string ((char*) TestType->value));
      }
   }


   bool diagStorage::isAuxiliaryResult (gdsDataObject& obj)
   {
      string n;
      int i1;
      int i2;
      if (!analyzeName (obj.name, n, i1, i2)) {
         return false;
      }
      if ((compareTestNames (n, stResult) != 0) ||
         (i1 < 0) || (i1 >= maxResult) || (i2 >= 0)) {
         return false;
      }
      if (Index == 0) {
         return true;
      }
      n = diagObject::makeName (n, i1);
      for (gdsDataObject::gdsParameterList::const_iterator i = 
          Index->parameters.begin(); i != Index->parameters.end(); i++) {
         if (((*i)->value != 0) && ((*i)->datatype == gds_string) &&
            (strstr ((const char*)((*i)->value), n.c_str()) != 0)) {
            return false;
         }
      }
      return true;
   }


   bool diagStorage::fsave (string filename, ioflags saveflags, 
                     filetype FileType)
   {
      // set xml type
      setType ("Global");
   
      // now call gdsStorage save method
      return gdsStorage::fsave (filename, saveflags, FileType);
   }


   bool diagStorage::frestore (string filename, ioflags restoreflags, 
                     filetype FileType)
   {
      // delete all old objects first
      objects.clear();
      parameters.clear();
      TestType = 0;
      TestName = 0;
      Supervisory = 0;
      TestIterator = 0;
      TestTime = 0;
      TestTimeUTC = 0;
      Def = 0;
      Lidax = 0;
      Sync = 0;
      Env.clear();
      Scan.clear();
      Find = 0;
      Test = 0;
      Channel.clear();
      Index = 0;
      Result.clear();
      ReferenceTraces.clear();
      Plot.clear();
      Calibration.clear();
      Env.assign (maxEnv, reinterpret_cast<gdsDataObject*>(0));
      Scan.assign (maxScan, reinterpret_cast<gdsDataObject*>(0));
      Result.assign (maxResult, reinterpret_cast<gdsDataObject*>(0));
      ReferenceTraces.assign (maxReferences, reinterpret_cast<gdsDataObject*>(0));
   
      // read in new file
      bool ret = gdsStorage::frestore (filename, restoreflags, FileType);
   
      // make sure we have a conformant diag object
      TestType = findParameter (stTestType);
      TestName = findParameter (stTestName);
      Supervisory = findParameter (stSupervisory);
      TestIterator = findParameter (stTestIterator);
      TestTime = findParameter (stTestTime);
      TestTimeUTC = findParameter (stTestTimeUTC);
      init (fftname);
      return ret;
   }


   bool diagStorage::updateTest (const string& newtest)
   {
      // test if cloning
      cerr << "Set new test " << newtest << endl;
      string newtestname (newtest);
      bool cloning = ((newtestname.size() >= 0) && 
                     (newtestname[newtestname.size() - 1] == '*'));
      if (cloning) {
         newtestname.erase (newtestname.size() - 1, 1);
         while ((newtestname.size() > 0) && 
               (newtestname[newtestname.size() - 1] == ' ')) {
            newtestname.erase (newtestname.size() - 1, 1);
         }
      }
      // test if name is supplied
      if (newtestname.size() <= 0) {
         return false;
      }
   
      // find test class
      const diagTest* 		t = diagTest::self (newtestname);
      if (t == 0) {
         return false;
      }
   
      // new test
      gdsDataObject* 	newt = t->newObject (0);
      if (newt == 0) {
         return false;
      }
      // cloning
      if (cloning) {
         //cerr << "Clone a test..." << endl;
         t->clone (*newt, Test);
      }
   
      // remove old test
      if (Test != 0) {
         eraseData (Test->name);
      }
      Test = newt;
      addData (*Test, false);
      diagGlobal::self().setParam (*this, string (stTestType), t->ID());
   
      return true;
   }


   bool diagStorage::addData (gdsDataObject& dat, bool copy)
   {
      gdsDataObject*	pdat;
      if (gdsStorage::addData (dat, copy) &&
         (pdat = findData (dat.name)) != 0) {
         string n;
         int indx1;
         int indx2;
         if (!analyzeName (pdat->name, n, indx1, indx2)) {
            return true;
         }
         switch (pdat->getFlag()) {
            case resultObj:
               {
                  if ((compareTestNames (n, stResult) == 0) &&
                     (indx1 >= 0) && (indx1 < maxResult) && 
                     (indx2 == -1)) {
                     Result[indx1] = pdat;
                  }
                  else if ((compareTestNames (n, stIndex) == 0) &&
                          (indx1 == -1) && (indx2 == -1)) {
                     Index = pdat;
                  }
                  if ((compareTestNames (n, stReference) == 0) &&
                     (indx1 >= 0) && (indx1 < maxReferences) && 
                     (indx2 == -1)) {
                     ReferenceTraces[indx1] = pdat;
                  }
                  return true;
               }
            case rawdataObj: 
               {
                  Channel.push_back (pdat);
                  return true;
               }
            case parameterObj:
               {
                  if ((compareTestNames (n, stDef) == 0) &&
                     (indx1 == -1) && (indx2 == -1)) {
                     Def = pdat;
                  }
                  else if ((compareTestNames (n, stLidax) == 0) &&
                          (indx1 == -1) && (indx2 == -1)) {
                     Lidax = pdat;
                  }
                  else if ((compareTestNames (n, stSync) == 0) &&
                          (indx1 == -1) && (indx2 == -1)) {
                     Sync = pdat;
                  }
                  else if ((compareTestNames (n, stEnv) == 0) &&
                          (indx1 >= 0) && (indx1 < maxEnv) && 
                          (indx2 == -1)) {
                     Env[indx1] = pdat;
                  }
                  else if ((compareTestNames (n, stScan) == 0) &&
                          (indx1 >= 0) && (indx1 < maxScan) && 
                          (indx2 == -1)) {
                     Scan[indx1] = pdat;
                  }
                  else if ((compareTestNames (n, stFind) == 0) &&
                          (indx1 == -1) && (indx2 == -1)) {
                     Find = pdat;
                  }
                  else if ((compareTestNames (n, stTestParameter) == 0) &&
                          (indx1 == -1) && (indx2 == -1)) {
                     Test = pdat;
                  }
                  return true;
               }
            case settingsObj:
               {
                  if (compareTestNames (n, stPlot) == 0) {
                     Plot.push_back (pdat);
                  }
                  else if ((compareTestNames (n, stCal) == 0) &&
                          (indx1 >= 0) && (indx1 < maxCalibration) && 
                          (indx2 == -1)) {
                     if (indx1 >= (int)Calibration.size()) {
                        Calibration.resize (indx1 + 1, 0);
                     }
                     if (indx1 < (int)Calibration.size()) {
                        Calibration[indx1] = pdat;
                     }
                  }
                  return true;
               }
            default:
               {
                  return true;
               }
         }
      }
      else {
         return false;
      }
   }


   bool diagStorage::eraseData (const string& objname)
   {
      gdsDataObject*	pdat = findData (objname);
      if (pdat == 0) {
         return gdsStorage::eraseData (objname);
      }
   
      string n;
      int indx1;
      int indx2;
      if (!analyzeName (pdat->name, n, indx1, indx2)) {
         return gdsStorage::eraseData (objname);
      }
   
      switch (pdat->getFlag()) {
         // case resultObj:
            // {
               // if ((compareTestNames (n, stResult) == 0) &&
                  // (indx1 >= 0) && (indx1 < maxResult) && 
                  // (indx2 == -1)) {
                  // Result[indx1] = 0;
               // }
               // else if ((compareTestNames (n, stIndex) == 0) &&
                       // (indx1 == -1) && (indx2 == -1)) {
                  // Index = 0;
               // }
               // break;
            // }
         case rawdataObj: 
            {
               gdsDataObjectList::iterator iter = 
                  find (Channel.begin(), Channel.end(), pdat);
               if (iter != Channel.end()) {
                  Channel.erase (iter);
               }
               break;
            }
         // case parameterObj:
            // {
               // if ((compareTestNames (n, stDef) == 0) &&
                  // (indx1 == -1) && (indx2 == -1)) {
                  // Def = 0;
               // }
               // else if ((compareTestNames (n, stSync) == 0) &&
                       // (indx1 == -1) && (indx2 == -1)) {
                  // Sync = 0;
               // }
               // else if ((compareTestNames (n, stEnv) == 0) &&
                       // (indx1 >= 0) && (indx1 < maxEnv) && 
                       // (indx2 == -1)) {
                  // Env[indx1] = 0;
               // }
               // else if ((compareTestNames (n, stScan) == 0) &&
                       // (indx1 >= 0) && (indx1 < maxScan) && 
                       // (indx2 == -1)) {
                  // Scan[indx1] = 0;
               // }
               // else if ((compareTestNames (n, stFind) == 0) &&
                       // (indx1 == -1) && (indx2 == -1)) {
                  // Find = 0;
               // }
               // else if ((compareTestNames (n, stTestParameter) == 0) &&
                       // (indx1 == -1) && (indx2 == -1)) {
                  // Test = 0;
               // }
               // break;
            // }
         // case settingsObj: 
            // {
               // gdsDataObjectList::iterator iter = 
                  // find (Plot.begin(), Plot.end(), pdat);
               // if (iter != Plot.end()) {
                  // Plot.erase (iter);
               // }
               // break;
            // }
         default:
            {
               break;
            }
      }
   
      return gdsStorage::eraseData (objname);
   }


   bool diagStorage::eraseResults () 
   {
      semlock 		lockit (mux);
   
      // loop over existig data objects and delete results
      gdsObjectList::iterator iter = objects.begin();
      while (iter != objects.end()) {
         if ((((*iter)->getFlag() == resultObj) && 
             ((strncasecmp ((*iter)->name.c_str(), stReference, 
                           strlen (stReference)) != 0))) ||
            ((*iter)->getFlag() == rawdataObj)) {
            eraseData ((*iter)->name);
         }
         else {
            iter++;
         }
      }
   
      // clean up result lists
      int	nochn = 0;
      Channel.assign (nochn, reinterpret_cast<gdsDataObject*>(0));
      Result.assign (maxResult, reinterpret_cast<gdsDataObject*>(0));
      Index = 0;
   
      return true;
   }


   bool diagStorage::eraseReferenceTraces () 
   {
      semlock 		lockit (mux);
   
      // loop over existig data objects and delete reference traces
      gdsObjectList::iterator iter = objects.begin();
      while (iter != objects.end()) {
         string n;
         int indx1;
         int indx2;
         if (!analyzeName ((*iter)->name, n, indx1, indx2)) {
            continue;
         }
         if (((*iter)->getFlag() == resultObj) &&
            (compareTestNames (n, stReference) == 0)) {
            eraseData ((*iter)->name);
         }
         else {
            iter++;
         }
      }
   
      // clean up reference trace list
      ReferenceTraces.assign (maxReferences, reinterpret_cast<gdsDataObject*>(0));
      return true;
   }


   bool diagStorage::erasePlotSettings () 
   {
      semlock 		lockit (mux);
   
      // loop over existig data objects and delete results
      gdsObjectList::iterator iter = objects.begin();
      while (iter != objects.end()) {
         string n;
         int i1;
         int i2;
         if (((*iter)->getFlag() == settingsObj) &&
            (analyzeName ((*iter)->name, n, i1, i2) &&
            compareTestNames (n, stPlot) == 0)) {
            eraseData ((*iter)->name);
         }
         else {
            iter++;
         }
      }
   
      // clean up plot list
      Plot.clear();
      return true;
   }


   bool diagStorage::eraseCalibration () 
   {
      semlock 		lockit (mux);
   
      // loop over existig data objects and delete calibration records
      gdsObjectList::iterator iter = objects.begin();
      while (iter != objects.end()) {
         string n;
         int i1;
         int i2;
         if (((*iter)->getFlag() == settingsObj) &&
            (analyzeName ((*iter)->name, n, i1, i2) &&
            compareTestNames (n, stCal) == 0)) {
            eraseData ((*iter)->name);
         }
         else {
            iter++;
         }
      }
   
      // clean up calibration list
      Calibration.clear();
      return true;
   }


   bool diagStorage::purgeChannelData (int left, int step, int firstindex)
   {
      if (left < 0) {
         return true;
      }
      int64_t total = 0;
      for (gdsDataObjectList::iterator i = Channel.begin();
          i != Channel.end(); ++i) {
         total += (*i)->size();
      }
      int size;
      while (((size = Channel.size()) > left) ||
            (total > kMaxTimeSeriesMemory)) {
         if ((total > kMaxTimeSeriesMemory) && (size <= left)) {
            cerr << "===========================================" 
               "==========================================" << endl;
            cerr << "Maximum memory exceeded in purge by " << total / 1024 << 
               " kB (# of channels = " << size << ")" << endl;
         }
         //cerr << "erase channel" << endl;
         // check name
         string n;
         int i1;
         int i2;
         if ((step >= 0) && (firstindex >= 0) &&
            analyzeName (Channel[0]->name, n, i1, i2)) {
            if ((i1 > step) ||
               (i1 == step) && (i2 >= firstindex)) {
               return true; // done
            }
         }
         total -= Channel[0]->size();
         eraseData (Channel[0]->name);
         //cerr << "erased channel" << endl;
         //Channel.erase(Channel.begin());
         //cerr << "erased2 channel" << endl;
         if ((int)Channel.size() >= size) {
            cerr << "ERROR in purge: Tried to delete " << Channel[0]->name <<
               " but couldn't" << endl;
            return false;
         }
      }
      return true;
   }


   bool diagStorage::getChannelNames (
                     std::vector<string>& names)
   {
      names.clear();
      semlock		lockit (mux);
      for (data_iterator iter = objects.begin(); iter != objects.end(); 
          iter++) {
         if (((*iter)->getFlag() == rawdataObj) && 
            (((*iter)->datatype == gds_float32) || 
            ((*iter)->datatype == gds_complex32)) &&
            ((*iter)->size() > 0)) {
            names.push_back ((*iter)->name);
         }
      }
      return true;
   }


   bool diagStorage::getAuxiliaryResultNames (
                     std::vector<string>& names)
   {
      names.clear();
      semlock		lockit (mux);
      for (data_iterator iter = objects.begin(); iter != objects.end(); 
          iter++) {
         if (isAuxiliaryResult (**iter)) {
            names.push_back ((*iter)->name);
         }
      }
      return true;
   }


   bool diagStorage::getReferenceTraceNames (
                     std::vector<string>& names)
   {
      names.clear();
      semlock		lockit (mux);
      for (data_iterator iter = objects.begin(); iter != objects.end(); 
          iter++) {
         string n;
         int indx1;
         int indx2;
         if (!analyzeName ((*iter)->name, n, indx1, indx2)) {
            continue;
         }
         if (((*iter)->getFlag() == resultObj) &&
            (compareTestNames (n, stReference) == 0)&& 
            (((*iter)->datatype == gds_float32) || 
            ((*iter)->datatype == gds_complex32)) &&
            ((*iter)->size() > 0)) {
            names.push_back ((*iter)->name);
         }
      }
      return true;
   }


   bool diagStorage::analyzeName (const string& name,
                     string& n, int& index1, int& index2)
   {
      index1 = -1;
      index2 = -1;
   
      // first index 
      string::size_type	pos = name.find ('[');
      if (pos == string::npos) {
         // no index in name
         n = name;
      }
      else {
         n.assign (name, 0, pos);
         index1 = atoi (name.c_str() + pos + 1);
         if (index1 < 0) {
            return false;
         }
         string::size_type	pos2 = name.find ('[', pos + 1);
         if (pos2 != string::npos) {
            index2 = atoi (name.c_str() + pos2 + 1);
            if (index2 < 0) {
               return false;
            } 
         }
      }
      // remove blanks, etc.
      while ((pos = n.find_first_of (" \t")) != string::npos) {
         n.erase (pos, 1);
      }
      return true;
   }


   bool diagStorage::analyzeName (const string& name,
                     string& nameA, int& indexA1, int& indexA2,
                     string& nameB, int& indexB1, int& indexB2)
   {
      // separate name into data object and parameter name
      string::size_type	pos;
      string		n1;
      string		n2;
   
      if ((pos = name.find ('.')) == string::npos) {
         n1 = name;
         n2 = "";
      }
      else {
         n1.assign (name, 0, pos);
         n2.assign (name, pos + 1, name.size());
      }
      return (analyzeName (n1, nameA, indexA1, indexA2) &&
             analyzeName (n2, nameB, indexB1, indexB2));
   }


   bool diagStorage::set (const string& var, const string& val)
   {
      string		norm;		// normalized name 
      string		nameA;		// object name
      string 		nameB;		// parameter name
      int		indexA1;	// 1st object index
      int		indexA2;	// 2nd object index
      int		indexB1;	// 1st parameter index
      int		indexB2;	// 2nd parameter index
      semlock		lockit (mux);
   
      // globals
      if (diagGlobal::self().isValid (var, true, &norm) &&
         analyzeName (norm, nameA, indexA1, indexA2, 
                     nameB, indexB1, indexB2)) {
         // treat TestType separately
         if (nameB == stTestType) {
            bool ret = updateTest (val);
            if (ret) {
               ret = eraseResults();
            }
            return ret;
         }
         else {
            return diagGlobal::self().setParam 
               (*this, diagObject::makeName (nameB, indexB1, indexB2), 
               val);
         }
      }
   
      // Def
      if (diagDef::self().isValid (var, true, &norm) &&
         analyzeName (norm, nameA, indexA1, indexA2, 
                     nameB, indexB1, indexB2) &&
         (Def != 0)) {
         return diagDef::self().setParam 
            (*Def, diagObject::makeName (nameB, indexB1, indexB2), val);
      }
   
      // Lidax
      if (diagLidax::self().isValid (var, true, &norm) &&
         analyzeName (norm, nameA, indexA1, indexA2, 
                     nameB, indexB1, indexB2) &&
         (Lidax != 0)) {
         return diagLidax::self().setParam 
            (*Lidax, diagObject::makeName (nameB, indexB1, indexB2), val);
      }
   
      // Sync
      if (diagSync::self().isValid (var, true, &norm) &&
         analyzeName (norm, nameA, indexA1, indexA2, 
                     nameB, indexB1, indexB2) &&
         (Sync != 0)) {
         return diagSync::self().setParam 
            (*Sync, diagObject::makeName (nameB, indexB1, indexB2), val);
      }
   
      // Find
      if (diagFind::self().isValid (var, true, &norm) &&
         analyzeName (norm, nameA, indexA1, indexA2, 
                     nameB, indexB1, indexB2) &&
         (Find != 0)) {
         return diagFind::self().setParam 
            (*Find, diagObject::makeName (nameB, indexB1, indexB2), val);
      }
   
      // Env
      if (diagEnv::self().isValid (var, true, &norm) &&
         analyzeName (norm, nameA, indexA1, indexA2, 
                     nameB, indexB1, indexB2) &&
         (indexA1 >= 0) && (indexA1 < (int)Env.size())) {
         if (Env[indexA1] == 0) {
            Env[indexA1] = diagEnv::self().newObject (0, 0, 0, indexA1);
            if (Env[indexA1] == 0) {
               return false;
            }
            addData (*Env[indexA1], false);
         }
         return diagEnv::self().setParam 
            (*Env[indexA1], diagObject::makeName (nameB, 
                              indexB1, indexB2), val);
      }
   
      // Scan
      if (diagScan::self().isValid (var, true, &norm) &&
         analyzeName (norm, nameA, indexA1, indexA2, 
                     nameB, indexB1, indexB2) &&
         (indexA1 >= 0) && (indexA1 < (int)Scan.size())) {
         if (Scan[indexA1] == 0) {
            Scan[indexA1] = diagScan::self().newObject (0, 0, 0, indexA1);
            if (Scan[indexA1] == 0) {
               return false;
            }
            addData (*Scan[indexA1], false);
         }
         return diagScan::self().setParam 
            (*Scan[indexA1], diagObject::makeName (nameB, 
                              indexB1, indexB2), val);
      }
   
      // Test
      if ((Test != 0) && (TestType != 0) && (TestType->value != 0)) {
         const diagTest* 	t =
            diagTest::self ((char*) TestType->value);
         if (t == 0) {
            return false;
         }
         if (t->isValid (var, true, &norm) &&
            analyzeName (norm, nameA, indexA1, indexA2, 
                        nameB, indexB1, indexB2)) {
            return t->setParam 
               (*Test, diagObject::makeName (nameB, 
                                 indexB1, indexB2), val);
         }
      }
   
      // Plot
      if (diagPlot::self().isValid (var, true, &norm) &&
         analyzeName (norm, nameA, indexA1, indexA2, 
                     nameB, indexB1, indexB2)) {
         gdsDataObject* pl = 0;
         for (gdsDataObjectList::iterator iter = Plot.begin();
             iter != Plot.end(); iter++) {
            string objname;
            int objindex1;
            int objindex2;
            analyzeName ((*iter)->name, objname, objindex1, objindex2);
            if ((compareTestNames (nameA, objname) == 0) && 
               (indexA1 == objindex1) && (indexA2 == objindex2)) {
               pl = *iter;
               break;
            }
         }
         if (pl == 0) {
            pl = diagPlot::self().newObject (0, 0, 0, indexA1, indexA2);
            if (pl == 0) {
               return false;
            }
            addData (*pl, false);
         }
         return diagPlot::self().setParam 
            (*pl, diagObject::makeName (nameB, indexB1, indexB2), val);
      }
   
      // Calibration
      if (diagCalibration::self().isValid (var, true, &norm) &&
         analyzeName (norm, nameA, indexA1, indexA2, 
                     nameB, indexB1, indexB2) &&
         (indexA1 >= 0) && (indexA1 < maxCalibration)) {
         if (indexA1 >= (int)Calibration.size()) {
            Calibration.resize (indexA1 + 1, 0);
         }
         if (indexA1 >= (int)Calibration.size()) {
            return false;
         }
         if (Calibration[indexA1] == 0) {
            Calibration[indexA1] = 
               diagCalibration::self().newObject (0, 0, 0, indexA1);
            if (Calibration[indexA1] == 0) {
               return false;
            }
            addData (*Calibration[indexA1], false);
         }
         return diagCalibration::self().setParam 
            (*Calibration[indexA1], diagObject::makeName (nameB, 
                              indexB1, indexB2), val);
      }
   
      // Reference traces
      if (!analyzeName (var, nameA, indexA1, indexA2, 
                       nameB, indexB1, indexB2)) {
         return false;
      }
      if ((compareTestNames (nameA, stReference) == 0) &&
         (indexA1 >= 0) && (indexA1 < maxReferences) && (indexA2 == -1) &&
         ((int)ReferenceTraces.size() > indexA1) && 
         (ReferenceTraces[indexA1] != 0)) {
         const diagResult* res = 
            diagResult::self (ReferenceTraces[indexA1]->getType());
         if (res == 0) {
            return false;
         }
         return res->setParam 
            (*ReferenceTraces[indexA1], diagObject::makeName (nameB, 
                              indexB1, indexB2), val);
      }
      // Auxiliary results
      if (!analyzeName (var, nameA, indexA1, indexA2, 
                       nameB, indexB1, indexB2)) {
         return false;
      }
      if ((compareTestNames (nameA, stResult) == 0) &&
         (indexA1 >= 0) && (indexA1 < maxResult) && (indexA2 == -1) &&
         ((int)Result.size() > indexA1) && (Result[indexA1] != 0) &&
         (isAuxiliaryResult (*Result[indexA1]))) {
         const diagResult* res = 
            diagResult::self (Result[indexA1]->getType());
         if (res == 0) {
            return false;
         }
         return res->setParam 
            (*Result[indexA1], diagObject::makeName (nameB, 
                              indexB1, indexB2), val);
      }
   
      return false;
   }


   bool diagStorage::erase (const string& var, string* nrm)
   {
      string		n;		// normalized name 
      string		nameA;		// object name
      string 		nameB;		// parameter name
      int		indexA1;	// 1st object index
      int		indexA2;	// 2nd object index
      int		indexB1;	// 1st parameter index
      int		indexB2;	// 2nd parameter index
      semlock		lockit (mux);
   
      if (!analyzeName (var, nameA, indexA1, indexA2, 
                       nameB, indexB1, indexB2)) {
         return false;
      }
      n = diagObject::makeName (nameA, indexA1, indexA2);
      // Lidax
      if (diagLidax::self().diagObjectName::isValid (n, true, nrm)) {
         eraseData (Lidax->name);
         Lidax = diagLidax::self().newObject (0);
         if (Lidax != 0) {
            addData (*Lidax, false);
         }
         return true;
      }   
      // Env
      if (diagEnv::self().diagObjectName::isValid (n, true, nrm)) {
         if ((indexA1 >= 0) && (indexA1 < (int)Env.size()) &&
            (Env[indexA1] != 0) &&
            eraseData (Env[indexA1]->name)) {
            Env[indexA1] = 0;
            return true;
         }
         else {
            return false;
         }
      }
   
      // Scan
      if (diagScan::self().diagObjectName::isValid (n, true, nrm)) {
         if ((indexA1 >= 0) && (indexA1 < (int)Scan.size()) &&
            (Scan[indexA1] != 0) &&
            eraseData (Scan[indexA1]->name)) {
            Scan[indexA1] = 0;
            return true;
         }
         else {
            return false;
         }
      }
   
      // Plot
      if (diagPlot::self().diagObjectName::isValid (n, true, nrm)) {
         analyzeName (n, nameA, indexA1, indexA2);
         gdsDataObject* pl = 0;
         gdsDataObjectList::iterator iter;
         for (iter = Plot.begin(); iter != Plot.end(); iter++) {
            string objname;
            int objindex1;
            int objindex2;
            analyzeName ((*iter)->name, objname, objindex1, objindex2);
            if ((compareTestNames (nameA, objname) == 0) && 
               (indexA1 == objindex1) && (indexA2 == objindex2)) {
               pl = *iter;
               break;
            }
         }
         if (pl != 0) {
            eraseData (Plot[indexA1]->name);
            Plot.erase (iter);
            return true;
         }
         else {
            return false;
         }
      }
   
      // Calibration
      if (diagCalibration::self().diagObjectName::isValid (n, true, nrm)) {
         if ((indexA1 >= 0) && (indexA1 < (int)Calibration.size()) &&
            (Calibration[indexA1] != 0) &&
            eraseData (Calibration[indexA1]->name)) {
            Calibration[indexA1] = 0;
            return true;
         }
         else {
            return false;
         }
      }
   
      // Result
      const diagResult*	res = diagResult::self (stObjectTypeTimeSeries);
      if ((res != 0) && 
         (res->diagObjectName::isValid (n, true, nrm))) {
         if ((indexA1 >= 0) && (indexA1 < (int)Result.size()) &&
            (Result[indexA1] != 0) &&
            eraseData (Result[indexA1]->name)) {
            Result[indexA1] = 0;
            return true;
         }
         else {
            return false;
         }
      }
   
      return false;
   }


   bool diagStorage::get (const string& var, gdsDatum& dat, 
                     string* nrm) const
   {
      string		norm;		// normalized name 
      string		nameA;		// object name
      string 		nameB;		// parameter name
      int		indexA1;	// 1st object index
      int		indexA2;	// 2nd object index
      int		indexB1;	// 1st parameter index
      int		indexB2;	// 2nd parameter index
      const gdsNamedDatum*	obj = 0;	// data pointer
      semlock		lockit (mux);
   
      // check rest
      if (var.find ('.') == string::npos) {
         norm = string (".") + var;
      }
      else {
         norm = var;
      }
      if ((!analyzeName (norm, nameA, indexA1, indexA2, 
                        nameB, indexB1, indexB2)) ||
         ((nameA.size() == 0) && (nameB.size() == 0))) {
         return false;
      }
      // check special case: xml
      if (compareTestNames (nameB, "xml") == 0) {
         ostringstream os;
         const gdsDataObject* obj = 
            findData (diagObject::makeName (nameA, indexA1, indexA2));
         if (obj != 0) {
            os << endl;
         //    gdsParameter flagprm (stObjectFlag, (int)obj->getFlag());
         //    os << flagprm;
            for (gdsDataObject::gdsParameterList::const_iterator i = 
                obj->parameters.begin(); i != obj->parameters.end(); i++) {
               if (compareTestNames ((*i)->name, stObjectType) == 0) {
                  continue;
               }
               if (compareTestNames ((*i)->name, stObjectFlag) == 0) {
                  continue;
               }
               if (compareTestNames ((*i)->name, stResultSubtype) == 0) {
                  continue;
               }
               os << **i;
            }
            string xml (os.str());
            dat = gdsDatum (gds_string, xml.c_str());
            if (nrm != 0) {
               *nrm = obj->name + ".xml";
            }
            return true;
         }
      }
      
      // special parameter: object type
      else if (compareTestNames (nameB, stObjectType) == 0) {
         gdsDataObject* obj2 = findData 
            (diagObject::makeName (nameA, indexA1, indexA2));
         if (obj2 == 0) {
            return false; 
         }
         else {
            dat = gdsDatum (gds_string, obj2->getType().c_str());     
            return true;
         }
      }
      // special parameter: object flag
      else if (compareTestNames (nameB, stObjectFlag) == 0) {
         gdsDataObject* obj2 = findData 
            (diagObject::makeName (nameA, indexA1, indexA2));
         if (obj2 == 0) {
            return false; 
         }
         else {
            int dflag = obj2->getFlag();
            dat = gdsDatum (gds_int32, &dflag);     
            return true;
         }
      }
      
      // search for global parameter
      else if (nameA.size() == 0) {
         // global
         obj = findParameter
            (diagObject::makeName (nameB, indexB1, indexB2));
         if (obj == 0) {
            obj = findData 
               (diagObject::makeName (nameB, indexB1, indexB2));
         }
         if ((obj != 0) && (nrm != 0)) {
            *nrm = obj->name;
         }
      }
      
      // treat all others as normal parameters
      else {
         obj = findParameter 
            (diagObject::makeName (nameA, indexA1, indexA2),
            diagObject::makeName (nameB, indexB1, indexB2));
         if ((obj != 0) && (nrm != 0)) {
            gdsNamedDatum* obj2 = findData 
               (diagObject::makeName (nameA, indexA1, indexA2));
            if (obj2 != 0) {
               *nrm = obj2->name + '.' + obj->name;
            }
         }
      }
      if (obj != 0) {
         dat = *obj;
         return true;
      }
      return false;
   }


   bool diagStorage::get (const string& var, string& val, 
                     string* norm) const
   {
      gdsDatum		dat;
   
      if (!get (var, dat, norm)) {
         return false;
      }
      if ((dat.datatype == gds_string) || 
         (dat.datatype == gds_channel)) {
         val = (dat.value == 0) ? "" : (char*) dat.value;
      }
      else {
         ostringstream 	os;
         os << dat;
         val = os.str();
      }
      return true;
   }


   string& oneline (string& s, bool brief = true) 
   {
      string::size_type	pos;
      if (!brief && ((pos = s.find ('\n')) != string::npos)) {
         s.erase (pos, s.size()).append ("...");
      }
      return s;
   }


   void diagStorage::parameterInfo (const gdsDataObject& obj, 
                     ostringstream& os, const string& Name, 
                     bool brief, bool nameonly) const
   {
      string		norm;		// normalized name 
      string		val;		// value
      string		objName (obj.name);	// object name
   
      // make sure global object is empty string
      if (objName == name) {
         objName = "";
      }
   
      for (gdsStorage::const_prm_iterator 
          iter = obj.parameters.begin();
          iter != obj.parameters.end(); iter++) {
      
         if ((gds_strncasecmp ((*iter)->name.c_str(), 
                              Name.c_str(), Name.size()) == 0) &&
            (get (objName + '.' + (*iter)->name, val, &norm))) {
            if (nameonly) {
               os << norm << endl;
            }
            else {
               os << norm << " = " << oneline (val, brief) << endl;
            }
         }
      }
   }


   bool diagStorage::getMultiple (const string& var, 
                     string& info, bool brief, bool nameonly) const
   {
      string		norm;		// normalized name 
      string		val;		// value
      string		nameA;		// object name
      string 		nameB;		// parameter name
      int		indexA1;	// 1st object index
      int		indexA2;	// 2nd object index
      int		indexB1;	// 1st parameter index
      int		indexB2;	// 2nd parameter index
      ostringstream 	os;		// output string stream
   
      semlock		lockit (mux);
   
      // no wildcard
      if (var.find ('*') == string::npos) {
         if (get (var, val, &norm)) {
            if (!nameonly) {
               os << norm << " = " << oneline (val, brief) << endl;
               info = os.str();
            }
            else {
               info = "yes";
            }
            return true;
         }
         else {
            info = nameonly ? "no" : "";
            return true;
         }
      }
   
      // find wildcard character
      string		v (var, 0, var.find ('*'));
      bool 		w1st = (v.find ('.') == string::npos);
      if (!analyzeName (v, nameA, indexA1, indexA2, 
                       nameB, indexB1, indexB2)) {
         info = "";
         return true;
      }
   
      if (!w1st) {
         nameA = diagObject::makeName (nameA, indexA1, indexA2);
      }
      else {
         // search through global parameters
         parameterInfo (*this, os, nameA, brief, nameonly);
      }
   
      // search through data objects 
      for (const_data_iterator iter = objects.begin();
          iter != objects.end(); iter++) {
         if ((w1st && 
             (gds_strncasecmp ((*iter)->name.c_str(), 
                              nameA.c_str(), nameA.size()) == 0)) ||
            (!w1st && ((*iter) == nameA))) {
            parameterInfo (**iter, os, nameB, brief, nameonly);
         }
      }
   
      info = os.str();
      return true;
   }


   bool diagStorage::getData (const string& Name, int Datatype, int len,
                     int ofs, float*& data, int& datalength) const
   {
      data = 0;
      datalength = 0;
   
      gdsDataObject* dat = findData (Name);
      if (dat == 0) {
         return false;
      }
      // check data type
      if ((Datatype < 0) || (Datatype > 3)) {
         return false;
      }
      if ((dat->datatype != gds_float32) &&
         (dat->datatype != gds_complex32)) {
         return false;
      }
      if ((dat->datatype == gds_float32) &&
         (Datatype != 0)) {
         return false;
      }
      // check array sizes
      if ((ofs < 0) || (len < 0) || 
         (ofs + len > dat->elNumber()) || (dat->value == 0)) {
         return false;
      }
      // copy real data
      if (dat->datatype == gds_float32) {
         data = (float*) malloc (len * sizeof (float));
         if (data == 0) {
            return false;
         }
         datalength = len;
         memcpy (data, ((float*)dat->value) + ofs, len * sizeof (float));
      }
      // copy complex data
      else {
         int size = ((Datatype <= 1) ? 2 : 1) * len;
         data = (float*) malloc (size * sizeof (float));
         if (data == 0) {
            return false;
         }
         datalength = size;
         if (Datatype <= 1) {
            memcpy (data, ((complex<float>*)dat->value) + ofs, 
                   size * sizeof (float));
         }
         else if (Datatype == 2) {
            complex<float>* y = ((complex<float>*)dat->value) + ofs;
            for (int i = 0; i < len; i++) {
               data[i] = y[i].real();
            }
         }
         else {
            complex<float>* y = ((complex<float>*)dat->value) + ofs;
            for (int i = 0; i < len; i++) {
               data[i] = y[i].imag();
            }
         }
      }
      return true;
   }


   bool diagStorage::putData (const string& Name, int Datatype, int len,
                     int ofs, const float* data, int datalength, 
                     int* newindex)
   {
      // cerr << "PUT DATA CALL for " << Name << " (" << Datatype << ") :" <<
         // ofs << "+" << len << endl;
      if (newindex) {
         *newindex = 0;
      }
      // check data type
      if ((Datatype % 10 < 1) || (Datatype % 10 > 2)) {
         return false;
      }
      gdsDataObject* dat = findData (Name);
      if (dat && (Datatype >= 10)) {
         eraseData (Name);
         dat = 0;
      }
      if (dat == 0) {
         const diagResult* dtype = 0;
         switch (Datatype / 10) {
            case 1:
               dtype = diagResult::self (stObjectTypeTimeSeries);
               break;
            case 2:
               dtype = diagResult::self (stObjectTypeSpectrum);
               break;
            case 3:
               dtype = diagResult::self (stObjectTypeTransferFunction);
               break;
            case 4:
               dtype = diagResult::self (stObjectTypeCoefficients);
               break;
            default:
               return false;
         }
         Datatype = Datatype % 10;
         // create new object
         string n;
         int indx1;
         int indx2;
         if (!analyzeName (Name, n, indx1, indx2)) {
            return false;
         }
         gdsDataType datat = 
            (Datatype % 10 == 1 ? gds_complex32 : gds_float32);
         if ((compareTestNames (n, stReference) == 0) &&
            (indx1 >= 0) && (indx1 < maxReferences) && (indx2 == -1)) {
            dat = dtype->newObject (0, 0, 0, indx1, -1, datat);
            if (dat) dat->name = Name;
         }
         else if ((compareTestNames (n, stResult) == 0) &&
                 (indx1 >= 0) && (indx1 < maxResult) && (indx2 == -1)) {
            dat = dtype->newObject (0, 0, 0, indx1, -1, datat);
         }
         else if (n.size() == 0) {
            indx1 = 0;
            for (gdsDataObjectList::iterator i = Result.begin();
                i != Result.end(); i++, indx1++) {
               if (*i == 0) {
                  break;
               }
            }
            dat = dtype->newObject (0, 0, 0, indx1, -1, datat);
            if (dat && newindex) {
               *newindex = indx1;
            }
         }
         if (dat == 0) {
            return false;
         }
      
         if (!addData (*dat, false)) {
            return false;
         }
      }
   
      // check data type
      if ((dat->datatype != gds_float32) &&
         (dat->datatype != gds_complex32)) {
         return false;
      }
      if ((dat->datatype == gds_float32) &&
         (Datatype % 10 == 1)) {
         return false;
      }
      if ((dat->datatype == gds_complex32) &&
         (Datatype % 10 != 1)) {
         return false;
      }
      // check array sizes
      if ((ofs < 0) || (len < 0)) {
         return false;
      }
      if (len == 0) {
         return true;
      }
      if ((ofs + len > dat->elNumber()) || (dat->value == 0)) {
         // need more space: resize
         int N = len;
         int M = (ofs + len) / len;
         if (N * M < ofs + len) {
            return false;
         }
         if (M <= 1) M = 0;
         if (!dat->resize (N, M)) {
            return false;
         }
      }
      int size = ((Datatype == 1) ? 2 : 1) * len;
      int offset = ((Datatype == 1) ? 2 : 1) * ofs;
      if (datalength < size) {
         size = datalength;
      }
      if (size <= 0) {
         return true;
      }
      // fill data
      memcpy (reinterpret_cast <float*>(dat->value) + offset, data, 
             size * sizeof (float));
      return true;
   }


}

