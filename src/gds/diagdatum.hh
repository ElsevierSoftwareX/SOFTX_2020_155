/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: diagdatum.h						*/
/*                                                         		*/
/* Module Description: Storage Objects for Diagnostics Tests		*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 11Mar99  D. Sigg    	First release		   		*/
/*                                                         		*/
/* Documentation References:						*/
/*	Man Pages: gdsdatum.html					*/
/*	References: none						*/
/*                                                         		*/
/* Author Information:							*/
/* Name          Telephone       Fax             e-mail 		*/
/* Daniel Sigg   (509) 372-8132  (509) 372-2178  sigg_d@ligo.mit.edu	*/
/*                                                         		*/
/* Code Compilation and Runtime Specifications:				*/
/*	Code Compiled on: Ultra-Enterprise, Solaris 5.6			*/
/*	Compiler Used: sun workshop C++ 4.2				*/
/*	Runtime environment: sparc/solaris				*/
/*                                                         		*/
/* Code Standards Conformance:						*/
/*	Code Conforms to: LIGO standards.	OK			*/
/*			  Lint.			TBD			*/
/*			  ANSI			OK			*/
/*			  POSIX			OK			*/
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

#ifndef _DIAG_DATUM_H
#define _DIAG_DATUM_H

/* Header File List: */
#include <inttypes.h>
#include <string>
#include <complex>
#include <vector>
#include <iosfwd>
#include "tconv.h"
#include "dtt/gdstask.h"
#include "gmutex.hh"
#include "dtt/gdsdatum.hh"
#include "dtt/diagnames.h"
#include "dtt/gdsstring.h"

namespace diag {


/** @name Diagnostics storage objects
   
    @memo Objects for storing diagnostics parameters and data
    @author Written March 1999 by Daniel Sigg
    @version 0.1
 ************************************************************************/

//@{

/** @name Constants
    Constants of the diagnostics storage API

    @memo Constants of the diagnostics storage API
 ************************************************************************/

//@{

   /// maximum number of environment objects
   const int		maxEnv = 100;
   /// maximum number of scan objects
   const int		maxScan = 10;
   /// maximum number of channel array indices
   const int		maxChannel = 1000;
   /// maximum number of result objects
   const int		maxResult = 1000;
   /// maximum number of plot objects
   const int		maxPlot = 100;
   /// maximum number of calibration records
   const int		maxCalibration = 1000;
   /// maximum number of reference traces
   const int		maxReferences = 1000; // must not be larger than maxResult
   /// maximum number of traces
   const int		maxTraces = 8;
   /// maximum number of coefficents per columns/rows 
   const int 		maxCoefficients = 100;
   /// maximum number of measurement values per table
   const int		maxMeasurementTableLength = 1000;
   /// maximum number of default array parameters 
   const int		maxDefaultParameters = 5;
   /// maximum number of Lidax servers/UDNs
   const int		maxLidaxServer = 20;
   /// maximum number of channels per Lidax server/UDN
   const int		maxLidaxChannels = 10000;
//@}


/** This class is a generic manager object for accessing diagnostics
    data and parameter objects within a diagnostics storage object.
    It is able to handle multi-dimensional data and indexed names.
    It doesn't contain data by itself, but rather contains information
    about the dimension of the object, its name and its indices. It
    is used to validate names and make sure object stored within the
    diagnostics storage object are of the right format.

    @memo Basic class to manage the name and type of objects in the 
    diagnostics storage.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagObjectName {
   public:
      /** Constructs a object name class.
   	  This object manages the name, indices, type, dimensions and 
   	  default values of a data object or a parameter. MaxIndex1 and
          MaxIndex2 describe if indices are supported ad if yes, what
          their highest value can be. A MaxIndex of 0 means no index,
   	  otherwise MaxIndex must be the largest allowed index + 1.
          If indices are supported the name of the object is composed
          from its short name and the indices as follows: 
          'Name[index1][index2]'. The dimension of the data object or
          parameter are determined as follows: (1) if the MaxDim1 is
          zero, the object has no data asscoiated with it; (2) if 
          MaxDim2 is zero and MaxDim1 is not, the object describes
          a vector; (3) if both MaxDim's are non-zero it describes
          a matrix; (4) if MaxDim is negative any value for the 
          dimension is supported; (5) if MaxDim is positive it 
          represents the maximum value allowed for the dimension.
          Additionally, one can specified whether the object can be
          modified, or whether it is read-only. Names are not
          case sensitive, and spaces and tabs are ignored.
   
          @param Name name of the data object/parameter
   	  @param MaxIndex1 maximum value for first index
   	  @param MaxIndex2 maximum value for second index
   	  @param Datatype data type of object
   	  @param DefValue default value of data object/parameter
          @param MaxDim1 maximum value of first dimension
   	  @param MaxDim2 maximum value of second dimension
   	  @param write if false, data object/parameter is read-only
          @memo Constructor.
          @return void
       ******************************************************************/
      diagObjectName (const string& Name, int MaxIndex1, int MaxIndex2,
                     gdsDataType Datatype, const void* DefValue, 
                     int MaxDim1 = 1, int MaxDim2 = 0, 
                     const string& Unit = "", bool write = true) :
      name (Name), maxIndex1 (MaxIndex1), maxIndex2 (MaxIndex2), 
      datatype (Datatype), maxDim1 (MaxDim1), maxDim2 (MaxDim2), 
      defValue (DefValue), unit (Unit), writeaccess (write) {
      
      }
   
      /** Destructs a object name class.
          @memo Destructor.
       ******************************************************************/
      virtual ~diagObjectName ();
   
      /** This function returns true if the specified name is valid.
          It checks name, indices ans write access. It can also 
          return a normalized name (correct capitalization no
          spaces).
          
          @memo Validates names.
          @param Name name of data object/parameter
   	  @param write if false only read access is requested
   	  @param normName normalized name (return)
          @return true if valid name, false otherwise
       ******************************************************************/
      virtual bool isValid (const string& Name, bool write = true,
                        string* normName = 0) const;
   
      /** Makes a full name out of the short name and the indeices.
          
          @memo Make a name.
          @param Name short name of data object/parameter
   	  @param index1 first index; -1 for none
   	  @param index2 second index; -1 for none
          @return full name
       ******************************************************************/
      static string makeName (const string& Name, int index1 = -1,
                        int index2 = -1);
   
   protected:
      /// name of object
      string		name;
      /// maximum index size, first dimension
      int		maxIndex1;
      /// maximum index size, second dimension
      int		maxIndex2;
      /// data type
      gdsDataType	datatype;
      /// maximum dimension of value, first dimension
      int		maxDim1;
      /// maximum dimension of value, second dimension
      int		maxDim2;
      /// default value(s)
      const void*	defValue;
      /// unit of object
      string		unit;
      /// write access to object?
      bool		writeaccess;
   };



/** This class is a manager object for accessing diagnostics
    data objects within a diagnostics storage object.

    @memo Manager class for data objects in the diagnostics storage.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagObject : public diagObjectName {
   public:
      /** Type describing the flags of the data object.
       ******************************************************************/
      typedef gdsDataObject::objflag objflag;
   
      /** This class is a manager object for accessing diagnostics
          parameters within a diagnostics storage object.
          @memo Manager class for parameter objects in the diagnostcs
          storage.
          @author DS, February 99
          @see Diagnostics storage API
       ******************************************************************/
      class diagParam : public diagObjectName {
         /// Data storage object is a friend.
         friend class diagObject;
      public:
      
         /** Constructs a parameter access object.
      	     For an explanation of the parameters see the parent object
             constructor.
           
             @param Name name of the data object/parameter
             @param MaxIndex1 maximum value for first index
             @param MaxIndex2 maximum value for second index
             @param Datatype data type of object
             @param DefValue default value of data object/parameter
             @param MaxDim1 maximum value of first dimension
             @param write if false, data object/parameter is read-only
             @memo Constructor.
             @return void
          ***************************************************************/
         diagParam (const string& Name, int MaxIndex1, int MaxIndex2,
                   gdsDataType Datatype, const void* DefValue, 
                   int MaxDim = 1, const string& Unit = "",
                   bool write = true) :
         diagObjectName (Name, MaxIndex1, MaxIndex2, Datatype, DefValue,
                        MaxDim, 0, Unit, write) {
         }
      
         /** Returns a new parameter storage object.
           
             @param value parameter value(s)
      	     @param dim dimension of parameter
             @param index1 first index of parameter object name
             @param index2 second index of parameter object name
             @memo New parameter method.
             @return new parameter object
          ***************************************************************/
         virtual gdsParameter* newParam (const void* value, int dim = 1,
                           int index1 = -1, int index2 = -1) const;
      
         /** Returns a new parameter storage object.
           
             @param value parameter value
      	     @param dim dimension of parameter
             @param index1 first index of parameter object name
             @param index2 second index of parameter object name
             @memo New parameter method.
             @return new parameter object
          ***************************************************************/
         virtual gdsParameter* newParam (const gdsDatum& value,
                           int index1 = -1, int index2 = -1) const;
      };
   
      /** Constructs a data object access object.
      	  For an explanation of the parameters see the parent object
          constructor.
   
          @param Objtype type of data object
          @param Name name of the data object/parameter
   	  @param MaxIndex1 maximum value for first index
   	  @param MaxIndex2 maximum value for second index
   	  @param Datatype data type of object
   	  @param DefValue default value of data object/parameter
          @param MaxDim1 maximum value of first dimension
   	  @param MaxDim2 maximum value of second dimension
          @memo Constructor.
          @return void
       ******************************************************************/
      diagObject (objflag Flag, const string& Type,
                 const string& Name, int MaxIndex1, int MaxIndex2,
                 gdsDataType Datatype, const void* DefValue, 
                 int MaxDim1 = 1, int MaxDim2 = 0) :
      diagObjectName (Name, MaxIndex1, MaxIndex2, Datatype, DefValue,
                     MaxDim1, MaxDim2, "", true), 
      flag (Flag), type (Type) {
      }
   
      /** This function returns true if the specified name is valid.
          It checks name, indices and write access. It can also 
          return a normalized name (correct capitalization no
          spaces). If the name is contains a '.' charachter the 
          first string is interpreted as the data object name, 
          whereas the second string is interpreted as the parameter
   	  name. In this case both names a checked.
          
          @memo Validates names.
          @param Name name of data object or dataobject.parameter
   	  @param write if false only read access is requested
   	  @param normName normalized name (return)
          @return true if valid name, false otherwise
       ******************************************************************/
      virtual bool isValid (const string& Name, bool write = true,
                        string* normName = 0) const;
   
      /** Returns a new data storage object including its parameters
          which have a default value.
           
          @param value data value(s)
      	  @param dim1 first dimension of parameter
      	  @param dim2 second dimension of parameter
          @param index1 first index of data object name
          @param index2 second index of data object name
          @memo New data object.
          @return new data object
       ******************************************************************/
      virtual gdsDataObject* newObject (void* value,
                        int dim1 = 0, int dim2 = 0, 
                        int index1 = -1, int index2 = -1, 
                        gdsDataType Datatype = gds_void) const;
   
      /** Clones a data storage object. Copies all valid parameters from 
          the template data object to the cloned data object. Uses
          default values for parameters which are not in the template.
           
          @param obj clone data object
      	  @param templ template data object
      	  @param if true copies the data values as well
          @memo Clone data object.
          @return true if successful
       ******************************************************************/
      virtual bool clone (gdsDataObject& obj, 
                        const gdsDataObject* templ, 
                        bool copydata = true) const;
   
      /** Hook for setting a parameter. This function is called prior of
          setting a parameter to a new value. If it returns true no 
          further action is taken, otherwise the normal set function is 
          executed.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Set parameter hook.
          @return true if successful
       ******************************************************************/
      virtual bool setParamHook (gdsDataObject& obj, const string& pName, 
                        const gdsDatum& value) const;
   
      /** Hook for gettting a parameter. This function is called prior of
          getting a parameter value. If it returns true no 
          further action is taken, otherwise the normal get function is 
          executed.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Set parameter hook.
          @return true if successful
       ******************************************************************/
      virtual bool getParamHook (gdsDataObject& obj, const string& pName, 
                        gdsDatum& value) const;
   
      /** Sets a parameter in the supplied data objects. Makes sure
          the parameter is valid.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Set parameter in data object.
          @return true if successful
       ******************************************************************/
      virtual bool setParam (gdsDataObject& obj, const string& pName, 
                        const gdsDatum& value) const;
   
      /** Sets a parameter in the supplied data objects. Makes sure
          the parameter is valid.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value (string encoded)
          @memo Set parameter in data object.
          @return true if successful
       ******************************************************************/
      virtual bool setParam (gdsDataObject& obj, const string& pName, 
                        const string& value) const;
   
      /** Sets a parameter in the supplied data objects. Makes sure
          the parameter is a valid integer type.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Set parameter in data object.
          @return true if successful
       ******************************************************************/
      virtual bool setParam (gdsDataObject& obj, const string& pName, 
                        char value) const;
   
      /** Sets a parameter in the supplied data objects. Makes sure
          the parameter is a valid integer type.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Set parameter in data object.
          @return true if successful
       ******************************************************************/
      virtual bool setParam (gdsDataObject& obj, const string& pName, 
                        short value) const;
   
      /** Sets a parameter in the supplied data objects. Makes sure
          the parameter is a valid integer type.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Set parameter in data object.
          @return true if successful
       ******************************************************************/
      virtual bool setParam (gdsDataObject& obj, const string& pName, 
                        int value) const;
   
      /** Sets a parameter in the supplied data objects. Makes sure
          the parameter is a valid integer type.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Set parameter in data object.
          @return true if successful
       ******************************************************************/
      virtual bool setParam (gdsDataObject& obj, const string& pName, 
			     int64_t value) const;
   
      /** Sets a parameter in the supplied data objects. Makes sure
          the parameter is a valid floating point number.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Set parameter in data object.
          @return true if successful
       ******************************************************************/
      virtual bool setParam (gdsDataObject& obj, const string& pName, 
			     float value) const;
   
      /** Sets a parameter in the supplied data objects. Makes sure
          the parameter is a valid floating point number.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Set parameter in data object.
          @return true if successful
       ******************************************************************/
      virtual bool setParam (gdsDataObject& obj, const string& pName, 
                        double value) const;
   
      /** Sets a parameter in the supplied data objects. Makes sure
          the parameter is a valid complex number.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Set parameter in data object.
          @return true if successful
       ******************************************************************/
      virtual bool setParam (gdsDataObject& obj, const string& pName, 
                        std::complex<float> value) const;
   
      /** Sets a parameter in the supplied data objects. Makes sure
          the parameter is a valid complex number.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Set parameter in data object.
          @return true if successful
       ******************************************************************/
      virtual bool setParam (gdsDataObject& obj, const string& pName, 
                        std::complex<double> value) const;
   
      /** Sets a parameter in the supplied data objects. Makes sure
          the parameter is a valid boolean.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Set parameter in data object.
          @return true if successful
       ******************************************************************/
      virtual bool setParam (gdsDataObject& obj, const string& pName, 
                        bool value) const;
   
      /** Gets a parameter from the supplied data objects. Makes sure
          the parameter is valid and returns false if the parameter
          doesn't exist.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Get parameter value from data object.
          @return true if successful
       ******************************************************************/
      virtual bool getParam (gdsDataObject& obj, const string& pName,
                        gdsDatum& value) const;
   
      /** Gets a parameter from the supplied data objects. Makes sure
          the parameter is valid and returns false if the parameter
          doesn't exist, or isn't a channel name or a string.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Get string parameter from data object.
          @return true if successful
       ******************************************************************/
      virtual bool getParam (gdsDataObject& obj, const string& pName,
                        string& value) const;
   
      /** Gets a parameter from the supplied data objects. Makes sure
          the parameter is valid and returns false if the parameter
          doesn't exist, or isn't a character.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Get char parameter from data object.
          @return true if successful
       ******************************************************************/
      virtual bool getParam (gdsDataObject& obj, const string& pName,
                        char& value) const;
   
      /** Gets a parameter from the supplied data objects. Makes sure
          the parameter is valid and returns false if the parameter
          doesn't exist, or isn't a short or a character.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Get short parameter from data object.
          @return true if successful
       ******************************************************************/
      virtual bool getParam (gdsDataObject& obj, const string& pName,
                        short& value) const;
      /** Gets a parameter from the supplied data objects. Makes sure
          the parameter is valid and returns false if the parameter
          doesn't exist, or isn't an int, short or a character.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @param max maximum number of parameters
          @memo Get int parameter from data object.
          @return true if successful
       ******************************************************************/
      virtual bool getParam (gdsDataObject& obj, const string& pName,
                        int& value, int max = 1) const;
   
      /** Gets a parameter from the supplied data objects. Makes sure
          the parameter is valid and returns false if the parameter
          doesn't exist, or isn't an int64_t, int, short or a character.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Get int64_t parameter from data object.
          @return true if successful
       ******************************************************************/
      virtual bool getParam (gdsDataObject& obj, const string& pName,
			     int64_t& value) const;
   
      /** Gets a parameter from the supplied data objects. Makes sure
          the parameter is valid and returns false if the parameter
          doesn't exist, or isn't a float.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Get float parameter from data object.
          @return true if successful
       ******************************************************************/
      virtual bool getParam (gdsDataObject& obj, const string& pName,
                        float& value) const;
   
      /** Gets a parameter from the supplied data objects. Makes sure
          the parameter is valid and returns false if the parameter
          doesn't exist, or isn't a float or a double.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @param max maximum number of parameters
          @memo Get double parameter from data object.
          @return true if successful
       ******************************************************************/
      virtual bool getParam (gdsDataObject& obj, const string& pName,
                        double& value, int max = 1) const;
   
      /** Gets a parameter from the supplied data objects. Makes sure
          the parameter is valid and returns false if the parameter
          doesn't exist, or isn't a complex float, float or a double.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Get complex<float> parameter from data object.
          @return true if successful
       ******************************************************************/
      virtual bool getParam (gdsDataObject& obj, const string& pName,
                        std::complex<float>& value) const;
   
      /** Gets a parameter from the supplied data objects. Makes sure
          the parameter is valid and returns false if the parameter
          doesn't exist, or isn't a complex float, complex double,
          float or a double.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Get complex<double> parameter from data object.
          @return true if successful
       ******************************************************************/
      virtual bool getParam (gdsDataObject& obj, const string& pName,
                        std::complex<double>& value) const;
   
      /** Gets a parameter from the supplied data objects. Makes sure
          the parameter is valid and returns false if the parameter
          doesn't exist, or isn't a bool.
           
          @param obj data object
      	  @param pName name of parameter
          @param value parameter value
          @memo Get bool parameter from data object.
          @return true if successful
       ******************************************************************/
      virtual bool getParam (gdsDataObject& obj, const string& pName,
                        bool& value) const;
   
      /** Sets the value(s) of a data object. Makes sure the data is
          of the correct format and with the correct dimensions.
           
          @param obj data object
          @param value parameter value (string encoded)
          @param dim1 first dimension of parameter
      	  @param dim2 second dimension of parameter
   	  @param Datatype data type (void for default)
          @memo Set value(s) in data object.
          @return true if successful
       ******************************************************************/
      virtual bool setData (gdsDataObject& obj, const void* value,
                        int dim1 = 0, int dim2 = 0, 
                        gdsDataType Datatype = gds_void) const;
   
      /** Gets the object type.
          @memo Get type.
          @return object type
       ******************************************************************/
      string getType () const {
         return type; }
      /** Gets the object flag.
          @memo Get flag.
          @return object flag
       ******************************************************************/
      objflag getFlag () const {
         return flag; }
   
   protected:
      /// type for list of parameter manager classes
      typedef std::vector <diagParam> diagParamList;
      /// list of parameter manager classes
      diagParamList	dParams;
      /// object flag
      objflag		flag;
      /// object type
      string		type;
   };


/** This class manages the access to global parameters.

    @memo Manager class for global parameters.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagGlobal : public diagObject {
   public:
      /** Constructs an access object for global parameters.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagGlobal ();
   
      /** Returns a const reference to an object of itself.
   
          @memo Reference object.
          @return reference object
       ******************************************************************/
      static const diagGlobal& self () {
         return myself;
      }
   
      /** This function returns true if the specified global parameter
          name is valid.
          
          @memo Validates names.
          @param Name name of data object or dataobject.parameter
   	  @param write if false only read access is requested
   	  @param normName normalized name (return)
          @return true if valid name, false otherwise
       ******************************************************************/
      virtual bool isValid (const string& Name, bool write = true,
                        string* normName = 0) const;
   
   private:
      static const diagGlobal myself;
   };


/** This class manages the access to common parameters.

    @memo Manager class for common parameters.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagDef : public diagObject {
   public:
      /** Constructs an access object for common parameters.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagDef ();
   
      /** Returns a const reference to an object of itself.
   
          @memo Reference object.
          @return reference object
       ******************************************************************/
      static const diagDef& self () {
         return myself;
      }
   
   private:
      static const diagDef myself;
   };


/** This class manages the access to lidax parameters.

    @memo Manager class for lidax parameters.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagLidax : public diagObject {
   public:
      /** Constructs an access object for common parameters.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagLidax ();
   
      /** Returns a const reference to an object of itself.
   
          @memo Reference object.
          @return reference object
       ******************************************************************/
      static const diagLidax& self () {
         return myself;
      }
   
   private:
      static const diagLidax myself;
   };


/** This class manages the access to synchronization settings.

    @memo Manager class for synchronization parameters.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagSync : public diagObject {
   public:
   
      /** Constructs an access object for synchronization parameters.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagSync ();
   
      /** Returns a const reference to an object of itself.
   
          @memo Reference object.
          @return reference object
       ******************************************************************/
      static const diagSync& self () {
         return myself;
      }
   
   private:
      static const diagSync myself;
   };


/** This class manages the access to environment parameters.

    @memo Manager class for environment parameters.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagEnv : public diagObject {
   public:
   
      /** Constructs an access object for environment parameters.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagEnv ();
   
      /** Returns a const reference to an object of itself.
   
          @memo Reference object.
          @return reference object
       ******************************************************************/
      static const diagEnv& self () {
         return myself;
      }
   
   private:
      static const diagEnv myself;
   };


/** This class manages the access to scan parameters.

    @memo Manager class for scan parameters.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagScan : public diagObject {
   public:
   
      /** Constructs an access object for scan parameters.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagScan ();
   
      /** Returns a const reference to an object of itself.
   
          @memo Reference object.
          @return reference object
       ******************************************************************/
      static const diagScan& self () {
         return myself;
      }
   
   private:
      static const diagScan myself;
   };


/** This class manages the access to optimization parameters.

    @memo Manager class for find parameters.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagFind : public diagObject {
   public:
   
      /** Constructs an access object for find parameters.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagFind ();
   
      /** Returns a const reference to an object of itself.
   
          @memo Reference object.
          @return reference object
       ******************************************************************/
      static const diagFind& self () {
         return myself;
      }
   
   private:
      static const diagFind myself;
   };


/** This class manages the access to plot settings.

    @memo Manager class for plot parameters.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagPlot : public diagObject {
   public:
   
      /** Constructs an access object for plot parameters.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagPlot ();
   
      /** Returns a const reference to an object of itself.
   
          @memo Reference object.
          @return reference object
       ******************************************************************/
      static const diagPlot& self () {
         return myself;
      }
   
   private:
      static const diagPlot myself;
   };


/** This class manages the access to calibration records.

    @memo Manager class for calibration records.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagCalibration : public diagObject {
   public:
   
      /** Constructs an access object for calibration records.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagCalibration ();
   
      /** Returns a const reference to an object of itself.
   
          @memo Reference object.
          @return reference object
       ******************************************************************/
      static const diagCalibration& self () {
         return myself;
      }
   
   private:
      static const diagCalibration myself;
   };



/** This class manages the access to the index.

    @memo Manager class for the index.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagIndex : public diagObject {
   public:
   
      /// indent for index entries
      static const string indexIndent;
      /// category separator
      static const string indexCat;
      /// index entry equal sign
      static const string indexEqual;
      /// index entry delimiter
      static const string indexEnd;
   
      /// master index type
      typedef std::map<string, int> masterindex;
   
      /** Constructs an access object for the index.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagIndex ();
   
      /** Returns a const reference to an object of itself.
   
          @memo Reference object.
          @return reference object
       ******************************************************************/
      static const diagIndex& self () {
         return myself;
      }
   
      /** Checks if index category is valid.
   
          @memo Chack index category method.
          @param cat category name
   	  @param step step number of category (-1 = none)
          @param catname canonical category name with step index (return)
          @return true if valid
       ******************************************************************/
      bool isCategory (const string& cat, int step, 
                      string* catname = 0) const;
   
      /** Get the master index.
   
          @memo Get master index method.
          @param obj data object
          @param index master index (return)
          @return true if successful
       ******************************************************************/
      bool getMasterIndex (gdsDataObject& obj,
                        masterindex& index) const;
   
      /** Set the master index.
   
          @memo Set master index method.
          @param obj data object
          @param index master index
          @return true if successful
       ******************************************************************/
      bool setMasterIndex (gdsDataObject& obj,
                        const masterindex& index) const;
   
      /** Sets a new entry of specified category.
   
          @memo Set entry method.
          @param obj data object
   	  @param category index entry type
   	  @param step step number of category (-1 = none)
          @param entry index entry string
          @return true if successful
       ******************************************************************/
      bool setEntry (gdsDataObject& obj, const string& category,
                    int step, const string& entry) const;
   
      /** Gets the entry of the specified category.
   
          @memo Get entry method.
          @param obj data object
   	  @param category index entry type
   	  @param step step number of category (-1 = none)
          @param entry index entry string (return)
          @return true if successful
       ******************************************************************/
      bool getEntry (gdsDataObject& obj, const string& category,
                    int step, string& entry) const;
   
      /** Deletes the entry of the specified category.
   
          @memo Delete entry method.
          @param obj data object
   	  @param category index entry type
   	  @param step step number of category (-1 = none)
          @return true if successful
       ******************************************************************/
      bool delEntry (gdsDataObject& obj, const string& category,
                    int step) const;
   
      /** Writes a channel entry to the stream.
   
          @memo Write channel entry method.
          @param os output stream
   	  @param num channel number
          @param name channel name
   	  @param type type of channel: 'A', 'B' or ' '
          @return output stream
       ******************************************************************/
      static std::ostream& channelEntry (std::ostream& os, int num, 
                        const string& name, char type = ' ');
   
      /** Writes a result entry to the stream.
   
          @memo Write result entry method.
          @param os output stream
   	  @param res index of result object
   	  @param ofs data offset
   	  @param len data length
   	  @param i1 first index of entry
   	  @param i1 second index of entry
          @return output stream
       ******************************************************************/
      static std::ostream& resultEntry (std::ostream& os, int res, 
                        int ofs, int len, int i1, int i2 = -1);
   
   private:
      /// const object of itself
      static const diagIndex myself;
      /// mutex to protect index
      static thread::recursivemutex indexmux;
   };


/** This is a basic class for manageing access objects with multiple
    configurations.

    @memo Manager class for multiple configuration objects.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagMultiObject : public diagObject {
   public:
   
      /** Constructs a multiple configuration access object.
      	  For an explanation of the parameters see the parent object
          constructor.
   
          @param ID configuration name
          @param Flag flag of data object
          @param Type type of data object
          @param Name name of the data object/parameter
   	  @param MaxIndex1 maximum value for first index
   	  @param MaxIndex2 maximum value for second index
   	  @param Datatype data type of object
   	  @param DefValue default value of data object/parameter
          @param MaxDim1 maximum value of first dimension
   	  @param MaxDim2 maximum value of second dimension
          @memo Constructor.
          @return void
       ******************************************************************/
      diagMultiObject (const string& ID, objflag Flag,
                      const string& Type, const string& Name, 
                      int MaxIndex1, int MaxIndex2,
                      gdsDataType Datatype, const void* DefValue, 
                      int MaxDim1 = 1, int MaxDim2 = 0) 
      : diagObject (Flag, Type, Name, MaxIndex1, MaxIndex2, 
                   Datatype, DefValue, MaxDim1, MaxDim2), myname (ID) {
         thread::semlock	lockit (submux);
      }
   
      /** Returns the name of the configuration represented by the 
          this object.
   
          @memo configuration identification.
          @return configuration name
       ******************************************************************/
      const string& ID () const {
         return myname;
      }
   
   protected:
      /// type name of object
      string		myname;
      /// subscribe function
      virtual bool subscribe (const string& ID) = 0;
   private:
      thread::mutex		submux;
   };


/** This is an access class for result data objects. This objects 
    manages the list of all possible configurations of result objects.

    @memo Manager class for result objects.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagResult : public diagMultiObject {
   public:
   
      /** Constructs an access object for test results.
   
          @param ID configuration name
          @param MaxDim1 maximum value of first dimension
          @param MaxDim2 maximum value of second dimension
          @memo Constructor.
          @return void
       ******************************************************************/
      diagResult (const string& ID, 
                 int MaxDim1 = -1, int MaxDim2 = 0);
   
      /** Returns a pointer to an access object of the specified
          configuration. Returns 0, if the configuration name is
          invalid.
   
          @memo Reference objects.
          @return pointer to reference object
       ******************************************************************/
      static const diagResult* self (const string& Type);
   
   protected:
      virtual bool subscribe (const string& ID);
   
   private:
      typedef std::vector<const diagResult*> diagResultList;
      static diagResultList myself;
   };


/** This is an access class for time series results.

    @memo Manager class for time series results.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagTimeSeries : public diagResult {
   public:
   
      /** Constructs an access object for time series results.
          Automatically adds the configuration to the result access
          object.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagTimeSeries (bool subscribe = true);
   
   };


/** This is an access class for FFT spectrum results.

    @memo Manager class for FFT spectrum results.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagSpectrum : public diagResult {
   public:
   
      /** Constructs an access object for FFT spectrum results.
          Automatically adds the configuration to the result access
          object.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagSpectrum ();
   
   };


/** This is an access class for transfer function results.

    @memo Manager class for transfer function results.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagTransferFunction : public diagResult {
   public:
   
      /** Constructs an access object for transfer function results.
          Automatically adds the configuration to the result access
          object.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagTransferFunction ();
   
   };


/** This is an access class for coefficients list results.

    @memo Manager class for coefficients list results.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagCoefficients : public diagResult {
   public:
   
      /** Constructs an access object for coefficients list results.
          Automatically adds the configuration to the result access
          object.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagCoefficients ();
   
   };


/** This is an access class for measurement table results.

    @memo Manager class for measurement table results.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagMeasurementTable : public diagResult {
   public:
   
      /** Constructs an access object for measurement table results.
          Automatically adds the configuration to the result access
          object.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagMeasurementTable ();
   
   };


/** This is an access class for channel data.

    @memo Manager class for channel data.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagChn : public diagTimeSeries {
   public:
   
      /** Constructs an access object for channel data.
          Automatically adds the configuration to the result access
          object.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      diagChn ();
   
      /** Returns a const reference to an object of itself.
   
          @memo Reference object.
          @return reference object
       ******************************************************************/
      static const diagChn& self () {
         return myself;
      }
   
      /** This function returns true if the specified channel name
          is valid.
          
          @memo Validates names.
          @param Name name of data object or dataobject.parameter
   	  @param write if false only read access is requested
   	  @param normName normalized name (return)
          @return true if valid name, false otherwise
       ******************************************************************/
      virtual bool isValid (const string& Name, bool write = true,
                        string* normName = 0) const;
   
   private:
      static const diagChn myself;
   };


/** This is an access class for test data objects. This objects 
    manages the list of all possible configurations of test objects.

    @memo Manager class for test objects.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class diagTest : public diagMultiObject {
   public:
   
      /** Constructs an access object for diagnostics tests access 
          objects.
   
          @param ID configuration name
          @memo Constructor.
          @return void
       ******************************************************************/
      diagTest (const string& ID);
   
      /** Returns a pointer to an access object of the specified
          configuration. Returns 0, if the configuration name is
          invalid.
   
          @memo Reference objects.
          @return pointer to reference object
       ******************************************************************/
      static const diagTest* self (const string& Type);
   
   protected:
      virtual bool subscribe (const string& ID);
   
   private:
      typedef std::vector<const diagTest*> diagTestList;
      static diagTestList myself;
   };


/** This is an access class for sine response tests.

    @memo Manager class for sine response tests.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class testSineResponse : public diagTest {
   public:
   
      /** Constructs an access object for sine response tests.
          Automatically adds the configuration to the test access
          object.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      testSineResponse ();
   
   };


/** This is an access class for swept sine response tests.

    @memo Manager class for swept sine response tests.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class testSweptSine : public diagTest {
   public:
   
      /** Constructs an access object for swept sine response tests.
          Automatically adds the configuration to the test access
          object.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      testSweptSine ();
   
   };


/** This is an access class for FFT tests.

    @memo Manager class for FFT tests.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class testFFT : public diagTest {
   public:
   
      /** Constructs an access object for FFT tests.
          Automatically adds the configuration to the test access
          object.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      testFFT ();
   
   };


/** This is an access class for time series tests.

    @memo Manager class for time series tests.
    @author DS, February 99
    @see Diagnostics storage API
 ************************************************************************/
   class testTimeSeries : public diagTest {
   public:
   
      /** Constructs an access object for time series tests.
          Automatically adds the configuration to the test access
          object.
   
          @memo Default constructor.
          @return void
       ******************************************************************/
      testTimeSeries ();
   
   };



/** This class is used to store diagnostics data. A storage object 
    contains a list of data objects which have both data and 
    parameters associated with them. Additionally, a parameter can
    be stored in global context. Every data object is characterized by
    its name which must be unique. Every parameter is characterized
    by its name and an associated data object (if it isn't in global
    context). The paramter name has to be unique within its context.
   
    MT safe: All public methods of the storage object are multi-thread
    safe and can be called from independently running tasks. Direct
    access to its public data member should be avoided and should 
    always guared by the mutex of the storage object.
   
    @memo Class to store a diagnostics data.
    @author DS, November 98
    @see Diagnostics storage API
   ************************************************************************/
   class diagStorage : public gdsStorage {
   
   public:
   
      /// list of data objects
      typedef std::vector<gdsDataObject*> gdsDataObjectList;
   
      /// Test class
      gdsParameter*	TestType;
      /// Test name 
      gdsParameter*	TestName;
      /// Test supervisory
      gdsParameter*	Supervisory;
      /// Test iterator
      gdsParameter*	TestIterator;
      /// Test time GPS
      gdsParameter*	TestTime;
      /// Test time UTC
      gdsParameter*	TestTimeUTC;
      /// defaults data object
      gdsDataObject*	Def;
      /// lidax data object
      gdsDataObject*	Lidax;
      /// synchronization data object
      gdsDataObject*	Sync;
      /// environment objects
      gdsDataObjectList	Env;
      /// scan objects
      gdsDataObjectList	Scan;
      /// find object
      gdsDataObject*	Find;
      /// test object
      gdsDataObject*	Test;
      /// Channel objects
      gdsDataObjectList	Channel;
      /// Index objects
      gdsDataObject*	Index;
      /// Result objects
      gdsDataObjectList	Result;
      /// Plot objects
      gdsDataObjectList	Plot;
      /// Calibration records
      gdsDataObjectList	Calibration;
      /// Reference trace list
      gdsDataObjectList	ReferenceTraces;
   
      /** Constructs a diagnostics storage object.
          @memo Deafult constructor.
          @param test name of test class
          @return void
       ******************************************************************/
      explicit diagStorage (const string& test = fftname);
   
      /** Destructs the storage object.
          @memo Destructor.
          @return void
       ******************************************************************/
      virtual ~diagStorage ();
   
      /** Determines ifdata object is auxiliary or part of the main.
          All result objects which are not listed in the index are
          considered auxiliarly.
          @memo Is auxiliary function.
          @param obj Data object
          @return true if auxiliary
       ******************************************************************/
      bool isAuxiliaryResult (gdsDataObject& obj);
   
      /** Saves a storage object to a file.
          @memo File save function.
          @param filename name of the output file
          @param saveflags specifies what to save
          @param filetype specifies the filetype
          @return true if successful
       ******************************************************************/
      virtual bool fsave (string filename, 
                        ioflags saveflags = ioStandard, 
                        filetype FileType = LigoLW_XML);
   
      /** Reads in data objects and parameters from a file and appends
          them to the storage object. Use the corresponding constructor
          if teh storage object should be created newly.)
          @memo File restore function.
          @param filename name of the input file
          @param restoreflags specifies what to restore
          @param filetype specifies the filetype
          @return true if successful
       ******************************************************************/
      virtual bool frestore (string filename, 
                        ioflags restoreflags = ioExtended, 
                        filetype FileType = LigoLW_XML);
   
      /** Updates the diagnostics test. If a new test is selected the 
          parameters of the test object have to be updated.
   
          @memo update test method.
          @param newtest name of new test
          @return true if successful
       ******************************************************************/
      virtual bool updateTest (const string& newtest);
   
      /** Adds a data object. The data object will be copied if copy is 
          true and stored in global context.
          @memo Add a data object.
          @param dat data object
          @param copy copy object if true, otherwise transfer ownership
          @return true if successful
       ******************************************************************/
      virtual bool addData (gdsDataObject& dat, bool copy = true);
   
      /** Removes a data object. The specified data object is removed
   	  from its global context.
          @memo Remove a data object.
          @param objname name of data object
          @return true if successful
       ******************************************************************/
      virtual bool eraseData (const string& objname);
   
      /** Erases all result objects.
   
          @memo erase result method.
          @return true if successful
       ******************************************************************/
      virtual bool eraseResults ();
   
      /** Erases all reference traces.
   
          @memo erase reference traces method.
          @return true if successful
       ******************************************************************/
      virtual bool eraseReferenceTraces ();
   
      /** Erases all plot settings objects.
   
          @memo erase plot settings method.
          @return true if successful
       ******************************************************************/
      virtual bool erasePlotSettings ();
   
      /** Erases all calibration record objects.
   
          @memo erase calibration record method.
          @return true if successful
       ******************************************************************/
      virtual bool eraseCalibration ();
   
      /** Purges channel data objects. This method deletes the oldest
          channel data object until no more than the specified amount
          are left. If a non-negative step and first index is specified,
          only prior channel objects are considered to be deleted.
   
          @memo purge channel data method.
          @param left number of channel data objects kept
          @param step Step of first object not to be deleted
          @param firstindex Index of first object not to be deleted
          @return true if successful
       ******************************************************************/
      virtual bool purgeChannelData (int left = 0, 
                        int step = -1, int firstindex = -1);
   
      /** Returns channel object names. This function returns the names
          of the currently available raw data objects describing channel
          data (i.e., a time series object).
   
          @memo Channel names method.
          @param names List of channel names
          @return true if successful
       ******************************************************************/
      virtual bool getChannelNames (std::vector<string>& names);
   
      /** Returns the names of the auxiliary result obejcts.
   
          @memo Auxiliary names method.
          @param names List of auxiliary results
          @return true if successful
       ******************************************************************/
      virtual bool getAuxiliaryResultNames (std::vector<string>& names);
   
      /** Returns reference trace names. This function returns the names
          of the currently available reference traces.
   
          @memo Reference names method.
          @param names List of reference traces
          @return true if successful
       ******************************************************************/
      virtual bool getReferenceTraceNames (std::vector<string>& names);
   
      /** Separates a name into its short name and its indices.
   
          @memo analyze name method.
          @param name name of data object or parameter
   	  @param n short name (return)
   	  @param index1 first index (return)
   	  @param index2 second index (return)
          @return true if successful
       ******************************************************************/
      static bool analyzeName (const string& name,
                        string& n, int& index1, int& index2);
   
      /** Separates a hierarchical name first into its two parts and the
          into the corresponding short names and indices.
   
          @memo analyze name method.
          @param name name of data object.parameter
   	  @param nameA short name of data object (return)
   	  @param indexA1 first index of data object (return)
   	  @param indexA2 second index of data object (return)
   	  @param nameB short name of parameter (return)
   	  @param indexB1 first index of parameter (return)
   	  @param indexB2 second index of parameter (return)
          @return true if successful
       ******************************************************************/
      static bool analyzeName (const string& name,
                        string& nameA, int& indexA1, int& indexA2,
                        string& nameB, int& indexB1, int& indexB2);
   
      /** Sets a parameter or a data object value(s). The supplied name
          either describes a data object name, a global parameter or a 
          hierarchical name of a data object with a parameter.
   
          @memo set method.
          @param var name of data object or one of its parameter
   	  @param val value to be set (string encoded)
          @return true if successful
       ******************************************************************/
      virtual bool set (const string& var, const string& val);
   
      /** Erases a parameter or a data object. The supplied name
          either describes a data object name, a global parameter or a 
          hierarchical name of a data object with a parameter.
   
          @memo erase method.
          @param var name of data object or one of its parameter
   	  @param norm normalized name
          @return true if successful
       ******************************************************************/
      virtual bool erase (const string& var, string* norm = 0);
   
      /** Gets a parameter or a data object value(s). The supplied name
          either describes a data object name, a global parameter or a 
          hierarchical name of a data object with a parameter.
   
          @memo get method.
          @param var name of data object or one of its parameter
   	  @param dat datum (return)
   	  @param norm normalized name
          @return true if successful
       ******************************************************************/
      virtual bool get (const string& var, gdsDatum& dat, 
                       string* norm = 0) const;
   
      /** Gets a parameter or a data object value(s). The supplied name
          either describes a data object name, a global parameter or a 
          hierarchical name of a data object with a parameter.
   
          @memo get method.
          @param var name of data object or one of its parameter
   	  @param dat datum (string encoded) (return)
   	  @param norm normalized name
          @return true if successful
       ******************************************************************/
      virtual bool get (const string& var, string& val, 
                       string* norm = 0) const;
   
      /** Gets a multiple parameter or a data object value(s). The 
          supplied name either describes a data object name, a global 
          parameter or a hierarchical name of a data object with a 
          parameter. The wild card character '*' can be used to search
          all parameters and data objects which match the supplied
          name fragment. The wild card has to be the last character
          in the supplied name fragment.
   
          @memo get method.
          @param var name of data object or one of its parameter
   	  @param info parameter and data object names with value(s) 
                 (return)
   	  @param brief if true, each value is limited to one line
   	  @param nameonly if true, only names are returned
          @return true if successful
       ******************************************************************/
      virtual bool getMultiple (const string& var, string& info,
                        bool brief = true, bool nameonly = false) const;
   
      /** Gets data from a data object in binary form. The supplied
          name must describes a data object name. The data type is  
   	  either: 0 - asis, 1 - complex, 2 - real part, and 
          3 - imaginary part. Additionally, the data length and the 
          offset into the data object have to be specified in number 
          of points (i.e. 1 complex number point = 2 floating point 
          numbers). This method will allocate a new data array 
          and the caller is responsible to free it. The data block
          is allocated with malloc! 
   
          @memo get data method.
          @param datatype type of data
   	  @param len Number of data points
   	  @param ofs Offset into data object
          @param data pointer to data array (return)
          @param datalen Number of float values in data array (return)
          @return true if successful
       ******************************************************************/
      virtual bool getData (const string& name, int datatype, int len,
                        int ofs, float*& data, int& datalength) const;
   
      /** Writes data to a data object in binary form. The supplied
          name must describes a data object name of the form 
          "Result[#]" or "Reference[#]"; or it can be empty, in which
          case a new "Result[#]" is chosen. The data type is  
   	  either: 1 - complex, 2 - real. When a new data object is 
          requested the following has to be added: 10 for time series, 
          20 - for spectrum, 30 - for transfer function, 40 - for 
          list of coefficient. Additionally, the data length and 
          the offset into the data object have to be specified in number 
          of points (i.e. 1 complex number point = 2 floating point 
          numbers). When successful, this method will add the data 
          to the storage object.
   
          @memo put data method.
          @param datatype type of data
   	  @param len Number of data points
   	  @param ofs Offset into data object
          @param data pointer to data array
          @param datalen Number of float values in data array
          @param newindex new Result index if empty name was supplied
          @return true if successful
       ******************************************************************/
      virtual bool putData (const string& name, int datatype, int len,
                        int ofs, const float* data, int datalength,
                        int* newindex = 0);
   
   
   protected:
      /** Initializes the diagnostics test object.
          @memo init method.
          @param test name of test
       ******************************************************************/
      virtual void init (const string& test);
   
   private:
      diagStorage (const diagStorage&);
      diagStorage& operator= (const diagStorage&);
      void parameterInfo (const gdsDataObject& obj, 
                        std::ostringstream& os,
                        const string& Name, 
                        bool brief, bool nameonly) const;
   };


//@}
}

#endif /* _DIAG_DATUM_H */
