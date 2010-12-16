/* Version: $Id$ */
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsdatum.h						*/
/*                                                         		*/
/* Module Description: Storage Objects for Diagnostics Tests		*/
/*									*/
/*                                                         		*/
/* Module Arguments: none				   		*/
/*                                                         		*/
/* Revision History:					   		*/
/* Rel   Date     Programmer  	Comments				*/
/* 0.1	 4Nov98   D. Sigg    	First release		   		*/
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

#ifndef _GDS_DATUM_H
#define _GDS_DATUM_H

/* Header File List: */
#include <string>
#include <complex>
#include <vector>
#include <memory>
#include <set>
#include <iosfwd>
#include <map>
#include "tconv.h"
#include "dtt/gdstask.h"
#include "gmutex.hh"
#include "dtt/gdsstring.h"


namespace diag {

/** @name Generic storage API
    
   
    @memo Objects for storing diagnostics parameters and data
    @author Written November 1998 by Daniel Sigg
    @version 0.1
 ************************************************************************/

//@{

/** @name Data types
    * Data types of the diagnostics storage API

    @memo Data types of the diagnostics storage API
 ************************************************************************/

//@{

/** Represents the data type of a storage object.
 ************************************************************************/
   enum gdsDataType {
   /// void, unknown
   gds_void = 0,
   /// 8 bit integer
   gds_int8 = 1, 
   /// 16 bit integer
   gds_int16 = 2, 
   /// 32 bit integer
   gds_int32 = 3, 
   /// 64 bit integer
   gds_int64 = 4, 
   /// single precision floating point
   gds_float32 = 5, 
   /// double precision floating point
   gds_float64 = 6,
   /// complex number with single precision floating point
   gds_complex32 = 7, 
   /// complex number with double precision floating point
   gds_complex64 = 8, 
   /// string
   gds_string = 9,
   /// channel name
   gds_channel = 10,
   /// bool
   gds_bool = 11
   };
   typedef enum gdsDataType gdsDataType;

//@}



/** @name Functions
    * Functions of the diagnostics storage API

    @memo Functions of the diagnostics storage API
 ************************************************************************/

//@{

/** Returns the name of a data type.

    @param datatype identifier of a data type
    @return string representing the data type
    @author DS, November 98
    @see Diagnostics storage API
 ************************************************************************/
   string gdsDataTypeName (gdsDataType datatype);

/** Returns the data type of the specified name.

    @param string representing the data type
    @return datatype identifier of a data type
    @author DS, November 98
    @see Diagnostics storage API
 ************************************************************************/
   gdsDataType gdsNameDataType (string name);

/** Returns a string representing the value specified by the value
    pointer and the data type.

    @param datatype identifier of a data type
    @param value pointer to data value
    @return string representing the data value
    @author DS, November 98
    @see Diagnostics storage API
 ************************************************************************/
   string gdsStrDataType (gdsDataType datatype, const void* value,
                     bool xmlescape = false);

/** Converts a string into a data value given the specified data type.
    Returns false on error.

    @param value pointer to data value
    @param datatype identifier of a data type
    @param datum string representing the data value
    @return true if successful
    @author DS, November 98
    @see Diagnostics storage API
 ************************************************************************/
   bool gdsValueDataType (void* value, gdsDataType datatype, 
                     const string& datum);
//@}



/** This class is used to store data fields. It is self describing and
    contains the data type, the number of elements (array dimensions)
    and a pointer to the data.
    @memo Class to store a basic data field.
    @author DS, November 98
    @see Diagnostics storage API
 ************************************************************************/
   class gdsDatum {
   public:
   
      /** This function reads the data field from an input stream.
          @memo Input operator.
          @param istream input stream
          @param gdsDatum class
          @return input stream
       ******************************************************************/
      friend std::istream& operator >> (std::istream&, gdsDatum&);
   
      /** This function writes the data field to an output stream
          following the LigoLW XML specification.
   	  @memo Output operator.
          @param ostream output stream
          @param gdsDatum class
          @return input stream
       ******************************************************************/
      friend std::ostream& operator << (std::ostream&, const gdsDatum&);
   
      /** Type describing the encoding scheme used to put and get the
           data value(s) to and from a stream.
       ******************************************************************/
      enum encodingtype {
      /// ascii
      ascii = 0,
      /// binary
      binary = 1,
      /// uuencoding
      uuencode = 2,
      /// base64 encoding
      base64 = 3
      };
   
      /** Type describing the dimensions of an array. If the list 
          contains two integers, say dim1 and dim2, the corresponding
          array has the dimension [dim1][dim2].
       ******************************************************************/
      typedef std::vector<int> dimension_t;
   
      /// Describes the type of the data field.
      gdsDataType 	datatype;
      /// Contains the information of the dimensions of the data field.
      dimension_t	dimension;
      /// Pointer to the data field.
      void*		value;
      /// encoding type for stream operations
      encodingtype	encoding;
      /// swap required
      bool		swapit;
   
      /** Constructs a data field which contains no data.
          @memo Default constructor.
          @return void
       ******************************************************************/
      gdsDatum ()
      : datatype (gds_void), dimension (), value (0), encoding (ascii) {
      }
      /** Constructs a data field. Data is copied.
   	  @memo Constructor.
          @param DataType data type of field
          @param Value pointer to data field
          @param dim1 first dimension
          @param dim2 second dimension
          @param dim3 third dimension
          @param dim4 fourth dimension
          @return void
       ******************************************************************/
      gdsDatum (gdsDataType DataType, const void* Value,
               int dim1 = 1, int dim2 = 0, int dim3 = 0, int dim4 = 0);
   
      /** Destructs a data field.
          @memo Destructor.
          @return void
       ******************************************************************/
      virtual ~gdsDatum ();
   
      /** Constructs and copies a data field.
          @memo Copy constructor.
          @param gdsDatum data field which will be copied
          @return void
       ******************************************************************/
      gdsDatum (const gdsDatum& dat);
   
      /** Copies a data field.
          @memo Assignment.
          @param gdsDatum data field which will be copied
          @return this data field
       ******************************************************************/
      virtual bool assignDatum (const gdsDatum& dat);
   
      /** Encodes value(s) of a datum object onto a stream.
          Works only with UU and base64 encoding.
          @memo Encode method.
          @param os output stream
          @param val pointer to value(s)
          @param len length of value array (in bytes)
   	  @param ctype encoding type
   	  @param indent indent use at each newline
          @return true if succesful
       ******************************************************************/
      static bool encode (std::ostream& os, const char* val, int len, 
                        encodingtype ctype = base64, int indent = 0);
   
      /** Decodes value(s) of a datum object from a stream.
          Works only with UU and base64 encoding.
          @memo Decode method.
          @param is input stream
          @param val pointer to value(s) (return)
          @param len length of value array (in bytes)
   	  @param ctype encoding type
          @return true if succesful
       ******************************************************************/
      static bool decode (std::istream& is, char* val, int len, 
                        encodingtype ctype = base64);
   
      /** Decodes value(s) of a datum object from a char buffer.
          Works only with UU and base64 encoding.
          @memo Decode method.
          @param code input array
          @param codelen Length of input array
          @param val pointer to value(s) (return)
          @param len length of value array (in bytes)
   	  @param ctype encoding type
          @return true if succesful
       ******************************************************************/
      static bool decode (const char* code, int codelen,
                        char* val, int len, 
                        encodingtype ctype = base64);
   
      /** Returns the name of the specified encodeing scheme.
          @memo Code name method.
          @param ctype encoding type
          @return name of encoding scheme
       ******************************************************************/
      static std::string codeName (encodingtype ctype);
   
      /** Returns the name of the specified encodeing scheme.
          @memo Code name method.
          @param ctype encoding type
          @return name of encoding scheme
       ******************************************************************/
      static encodingtype code (std::string name);
   
      /** Copies a data field.
          @memo Assignment oparator.
          @param gdsDatum data field which will be copied
          @return this data field
       ******************************************************************/
      gdsDatum& operator= (const gdsDatum& dat);
   
      /** Returns the size of an element of the data field.
          @memo Returns the size of an element (in bytes).
          @return size of an element (in bytes)
       ******************************************************************/
      bool resize (int dim1, int dim2 = 0, int dim3 = 0, int dim4 = 0);
   
      /** Returns the size of an element of the data field.
          @memo Returns the size of an element (in bytes).
          @return size of an element (in bytes)
       ******************************************************************/
      int elSize () const;
   
      /** Returns the number of elements in the data field.
          @memo Returns the number of elements.
          @return number of elements
       ******************************************************************/
      int elNumber () const;
   
      /** Returns the total size of the data field.
          @memo Returns the total size (in bytes) of the data field.
          @return size of the data field (in bytes)
       ******************************************************************/
      int size () const;
   
      /** Returns true if data is complex.
          @memo Returns true if data is complex.
          @return True if data is complex
       ******************************************************************/
      bool isComplex () const;
   
      /** Reads value(s) from an input stream. Returns the number of
          read values if successful. If value is not 0 the associated 
          memory will be released before allocating new memory. The 
          dimension of the value array has to be set before calling
          this function.
          @memo Reads value(s) from input stream.
          @param is input stream
          @return number of elements read; <0 on error
       ******************************************************************/
      int readValues (const string& txt);
   
      /** Applies a read-write lock to make sure that the object isn't 
          changed while accessing it.
          @memo Lock object.
          @param write if true locks for exclusive use
          @return void
       ******************************************************************/
      void lock (bool writeaccess = false) const {
         if (writeaccess) {
            rwlock.writelock ();
         }
         else {
            rwlock.readlock ();
         }
      }
   
      /** Tries to apply a read-write lock. Returns true if successfully 
          locked, false otherwise. 
          @memo Try lock object.
          @param write if true locks for exclusive use
          @return void
       ******************************************************************/              
      bool trylock (bool writeaccess = false) const {
         if (writeaccess) {
            return rwlock.trylock (thread::abstractsemaphore::wrlock);
         } 
         else {
            return rwlock.trylock (thread::abstractsemaphore::rdlock);
         }
      }
   
      /** Removes the read-write lock.
          @memo Unlock object.
          @return void
       ******************************************************************/
      void unlock () const {
         rwlock.unlock ();
      }
   
   protected:
      /// read wrote lock for datum
      mutable thread::readwritelock		rwlock;
   
   };


/** This class is used to give a storage object a name. One can also
    add a comment string.
    @memo Class to store a name and a comment.
    @author DS, November 98
    @see Diagnostics storage API
 ************************************************************************/
   class gdsNamedStorage {
   public:
   
      /// Describes the name of a storage object.
      string		name;
      /// Contains a comment string.
      string		comment;
   
      /** Constructs a named storage object with name and a comment 
          string. Default values for name and comment are empty 
          strings.
          @memo Default constructor.
          @param Name name of the storage object (default "")
          @param Comment comment string (default "")
          @return void
       ******************************************************************/
      explicit gdsNamedStorage (const string& Name = "", 
                        const string& Comment = "")
      : name (Name), comment (Comment) {
      };
   
      /** Compares the name of two named storage objects (equal).
          The comparison is not case sensitive.
          @memo Equality operator.
          @param x named storage object
          @return true if equal, false otherwise
       ******************************************************************/
      bool operator == (const gdsNamedStorage& x) const;
   
      /** Compares the name of a named storage objects to a string 
          (equal). The comparison is not case sensitive.
          @memo Equality operator.
          @param x string
          @return true if equal, false otherwise
       ******************************************************************/
      bool operator == (const string& x) const;
   
      /** Compares the name of a named storage objects to a string 
          (equal). The comparison is not case sensitive.
          @memo Inquality operator.
          @param x string
          @return true if equal, false otherwise
       ******************************************************************/
      bool operator != (const string& x) const;
   
      /** Compares the name of a named storage object to a string 
          (unequal).
          The comparison is not case sensitive.
          @memo Inequality operator.
          @param x named storage object
          @return true if unequal, false otherwise
       ******************************************************************/
      bool operator != (const gdsNamedStorage& x) const;
   
      /** Compares the name of two named storage objects (less or equal).
          The comparison is not case sensitive.
          @memo Less or equal operator.
          @param x named storage object
          @return true if less or equal, false otherwise
       ******************************************************************/
      bool operator <= (const gdsNamedStorage& x) const;
   
      /** Compares the name of a named storage object to a string (less).
          The comparison is not case sensitive.
          @memo Less than operator.
          @param x named storage object
          @return true if less, false otherwise
       ******************************************************************/
      bool operator < (const gdsNamedStorage& x) const;
   
      /** Compares the name of two named storage objects (less).
          The comparison is not case sensitive.
          @memo Less than operator.
          @param x named storage object
          @return true if less, false otherwise
       ******************************************************************/
      bool operator < (const string& x) const;
   
      /** Compares the name of two named storage objects (greater of 
          equal).
          The comparison is not case sensitive.
          @memo Greater or equal operator.
          @param x named storage object
          @return true if greater or equal, false otherwise
       ******************************************************************/
      bool operator >= (const gdsNamedStorage& x) const;
   
      /** Compares the name of two named storage objects (greater).
          The comparison is not case sensitive.
          @memo Greater than operator.
          @param x named storage object
          @return true if greater, false otherwise
       ******************************************************************/
      bool operator > (const gdsNamedStorage& x) const;
   };


/** This class is used to store a named data field. It is self describing 
    and contains the data type, the number of elements (array dimensions),
    the name of the data field and its unit.
    @memo Class to store a named data field.
    @author DS, November 98
    @see Diagnostics storage API
 ************************************************************************/
   class gdsNamedDatum : public gdsNamedStorage, public gdsDatum {
   public:
      /// mutex to protect object
      mutable thread::recursivemutex	mux;
      /// Describes the unit of a data field.
      string		unit;
      /// Nesting level
      int		level;
   
      /** Constructs a named data field with no name and empty 
          values.
          @memo Default constructor.
          @return void
       ******************************************************************/
      gdsNamedDatum () 
      : gdsNamedStorage (), gdsDatum (), unit (""), level (1) {
      }
   
      /** Constructs a named data field. The supplied data field
          is copied (the caller is responsible to free the supplied
          data field if necessary.
          @memo Constructor.
          @param Name name of data field
          @param DataType type of data field
          @param Value pointer to data field
   	  @param dim1 first dimension of data field (default 1)
   	  @param dim2 second dimension of data field (default 0)
   	  @param dim3 third dimension of data field (default 0)
   	  @param dim4 fourth dimension of data field (default 0)
   	  @param Unit unit of data field
          @param Comment comment string for describing data field
          @return void
       ******************************************************************/
      gdsNamedDatum (const string& Name, gdsDataType DataType, 
                    const void* Value, int dim1 = 1, int dim2 = 0, 
                    int dim3 = 0, int dim4 = 0,
                    const string& Unit = "", const string& Comment = "")
      : gdsNamedStorage (Name, Comment),
      gdsDatum (DataType, (void*) Value, dim1, dim2, dim3, dim4),
      unit (Unit), level (1) {
      }
   
      /** Constructs a named data field. The supplied data field
          object is copied (the caller is responsible to free the 
          supplied data field if necessary.
          @param Name name of data field
          @param Value data field object
          @param Unit unit of data field
          @param Comment comment string for describing data field
          @memo Constructor.
          @return void
       ******************************************************************/
      gdsNamedDatum (const string& Name, const gdsDatum& Value,
                    const string& Unit = "", const string& Comment = "")
      : gdsNamedStorage (Name, Comment), gdsDatum (Value),
      unit (Unit), level (1) {
      }
   };



/** This class is used to store a named data field. It is self describing 
    and contains the data type, the number of elements (array dimensions),
    the name of the data field and its unit.
    @memo Class to store a parameter.
    @author DS, November 98
    @see Diagnostics storage API
 ************************************************************************/
   class gdsParameter : public gdsNamedDatum {
   public:
      /** This function reads a parameter from an input stream.
          @memo Input operator.
          @param istream input stream
          @param gdsDatum class
          @return input stream
       ******************************************************************/
      friend std::istream& operator >> (std::istream&, gdsParameter&);
   
      /** This function writes the parameter to an output stream
          following the LigoLW XML specification.
   	  @memo Output operator.
          @param ostream output stream
          @param gdsDatum class
          @return input stream
       ******************************************************************/
      friend std::ostream& operator << (std::ostream&, 
                        const gdsParameter&);
   
      /** Constructs a parameter object with no name and empty 
          value.
          @memo Default constructor.
          @return void
       ******************************************************************/
      gdsParameter () : gdsNamedDatum () {
      }
   
      /** Constructs a parameter object.
          @memo Copy constructor.
          @return void
       ******************************************************************/
      gdsParameter (const gdsParameter& prm) {
         *this = prm;
      }
   
      /** Constructs a parameter object. The supplied data field
          is copied (the caller is responsible to free the supplied
          data field if necessary.
          @memo Constructor.
          @param Name name of parameter
          @param DataType type of parameter
          @param Value pointer to a parameter value field
   	  @param dim1 number of parameter values
          @param Unit unit of parametr value
          @param Comment comment string for describing parameter
          @return void
       ******************************************************************/
      gdsParameter (const string& Name, gdsDataType DataType, 
                   const void* Value,
                   const string& Unit = "", const string& Comment = "")
      : gdsNamedDatum (Name, DataType, Value, 1, 0, 0, 0, 
                      Unit, Comment) {
      }
   
      /** Constructs a parameter object. The supplied data field
          is copied (the caller is responsible to free the supplied
          data field if necessary.
          @memo Constructor.
          @param Name name of parameter
          @param DataType type of parameter
          @param Value pointer to a parameter value field
   	  @param dim1 number of parameter values
          @param Unit unit of parametr value
          @param Comment comment string for describing parameter
          @return void
       ******************************************************************/
      gdsParameter (const string& Name, gdsDataType DataType, 
                   const void* Value,
                   int dim1, const string& Unit = "", 
                   const string& Comment = "")
      : gdsNamedDatum (Name, DataType, Value, dim1, 0, 0, 0, 
                      Unit, Comment) {
      }
   
      /** Constructs a parameter object. The supplied data object
          is copied (the caller is responsible to free the supplied
          data field if necessary.
          @memo Constructor.
          @param Name name of parameter
          @param Value data field object
   	  @param Unit unit of parametr value
          @param Comment comment string for describing parameter
          @return void
       ******************************************************************/
      gdsParameter (const string& Name, const gdsDatum& Value,
                   const string& Unit = "", const string& Comment = "")
      : gdsNamedDatum (Name, Value, Unit, Comment) {
      }
   
      /** Constructs a parameter object from a character.
          @memo Constructor.
          @param Name name of parameter
          @param Value parameter value
   	  @param Unit unit of parametr value
          @param Comment comment string for describing parameter
          @return void
       ******************************************************************/
      gdsParameter (const string& Name, char Value,
                   const string& Unit = "", const string& Comment = "")
      : gdsNamedDatum (Name, gds_int8, (void*) &Value, 
                      1, 0, 0, 0, Unit, Comment) {
      }
   
      /** Constructs a parameter object from a short integer.
          @memo Constructor.
          @param Name name of parameter
          @param Value parameter value
   	  @param Unit unit of parametr value
          @param Comment comment string for describing parameter
          @return void
       ******************************************************************/
      gdsParameter (const string& Name, short Value,
                   const string& Unit = "", const string& Comment = "")
      : gdsNamedDatum (Name, gds_int16, (void*) &Value, 
                      1, 0, 0, 0, Unit, Comment) {
      }
   
      /** Constructs a parameter object from an 32 bit integer.
          @memo Constructor.
          @param Name name of parameter
          @param Value parameter value
   	  @param Unit unit of parametr value
          @param Comment comment string for describing parameter
          @return void
       ******************************************************************/
      gdsParameter (const string& Name, int Value,
                   const string& Unit = "", const string& Comment = "")
      : gdsNamedDatum (Name, gds_int32, (void*) &Value, 
                      1, 0, 0, 0, Unit, Comment) {
      }
   
      /** Constructs a parameter object from a 64 bit integer.
          @memo Constructor.
          @param Name name of parameter
          @param Value parameter value
   	  @param Unit unit of parametr value
          @param Comment comment string for describing parameter
          @return void
       ******************************************************************/
      gdsParameter (const string& Name, int64_t Value,
                   const string& Unit = "", const string& Comment = "")
      : gdsNamedDatum (Name, gds_int64, (void*) &Value, 
                      1, 0, 0, 0, Unit, Comment) {
      }
   
      /** Constructs a parameter object from a single precision
          floating point number.
          @memo Constructor.
          @param Name name of parameter
          @param Value parameter value
   	  @param Unit unit of parametr value
          @param Comment comment string for describing parameter
          @return void
       ******************************************************************/
      gdsParameter (const string& Name, float Value,
                   const string& Unit = "", const string& Comment = "")
      : gdsNamedDatum (Name, gds_float32, (void*) &Value, 
                      1, 0, 0, 0, Unit, Comment) {
      }
   
      /** Constructs a parameter object from a double precision
          floating point number.
          @memo Constructor.
          @param Name name of parameter
          @param Value parameter value
   	  @param Unit unit of parametr value
          @param Comment comment string for describing parameter
          @return void
       ******************************************************************/
      gdsParameter (const string& Name, double Value,
                   const string& Unit = "", const string& Comment = "")
      : gdsNamedDatum (Name, gds_float64, (void*) &Value, 
                      1, 0, 0, 0, Unit, Comment) {
      }
   
      /** Constructs a parameter object from a single precision
          floating point complex number.
          @memo Constructor.
          @param Name name of parameter
          @param Value parameter value
   	  @param Unit unit of parametr value
          @param Comment comment string for describing parameter
          @return void
       ******************************************************************/
      gdsParameter (const string& Name, std::complex<float> Value,
                   const string& Unit = "", const string& Comment = "")
      : gdsNamedDatum (Name, gds_complex32, (void*) &Value, 
                      1, 0, 0, 0, Unit, Comment) {
      }
   
      /** Constructs a parameter object from a double precision
          floating point complex number.
          @memo Constructor.
          @param Name name of parameter
          @param Value parameter value
   	  @param Unit unit of parametr value
          @param Comment comment string for describing parameter
          @return void
       ******************************************************************/
      gdsParameter (const string& Name, std::complex<double> Value,
                   const string& Unit = "", const string& Comment = "")
      : gdsNamedDatum (Name, gds_complex64, (void*) &Value, 
                      1, 0, 0, 0, Unit, Comment) {
      }
   
      /** Constructs a parameter object from a string.
          @memo Constructor.
          @param Name name of parameter
          @param Value parameter value
   	  @param Unit unit of parametr value
          @param Comment comment string for describing parameter
          @return void
       ******************************************************************/
      gdsParameter (const string& Name, const string& Value,
                   const string& Unit = "", const string& Comment = "")
      : gdsNamedDatum (Name, gds_string, (void*) Value.c_str(), 
                      1, 0, 0, 0, Unit, Comment) {
      }
   };


   class gdsDataObject;


/** This class is used to store a data reference.
    @memo Class to store a data reference.
    @author DS, November 98
    @see Diagnostics storage API
 ************************************************************************/
   class gdsDataReference {
   public:
   
      /** This function writes a data reference to an output stream
          following the LigoLW XML specification.
   	  @memo Output operator.
          @param ostream output stream
          @param gdsDataReference class
          @return input stream
       ******************************************************************/
      friend std::ostream& operator << (std::ostream&, 
                        const gdsDataReference&);
   
      /** Byte order of binary data (enumerated type).
   	  @memo Byte order of binary data.
       ******************************************************************/
      enum byteorder {
      /// big endian
      BE = 0, 
      /// little endian
      LE = 1};
   
      /// byte order type
      typedef enum byteorder byteorder;
   
      /// valid reference if true
      bool		reference;
      /// self reference (needed for attaching data to the save file)
      bool		selfref;
      /// referenced filename
      string		fileref;
      /// offset of binary data within the file (in bytes)
      int		offset;
      /// offset for self reference */
      int		selfofs;
      /// length of binary data (in bytes)
      int		length;
      /// byte order of binary data 
      byteorder		encoding;
   
      /** Constructs a data reference object with no actual 
          reference.
          @memo Default constructor.
          @return void
       ******************************************************************/
      gdsDataReference () 
      : reference (false), selfref (false), fileref (""), 
      offset (0), length (0), encoding (LE), maddr (0) {
      }
   
      /** Destructs the data reference.
          @memo Destructor.
          @return void
       ******************************************************************/
      ~gdsDataReference ();
   
      /** Constructs a data reference object with a refrence to a file. 
          @memo Constructor.
          @return void
       ******************************************************************/
      explicit gdsDataReference (const string& Filename, 
                        bool Self = true, int Offset = 0, int Length = 0)
      : reference (true), selfref (Self), fileref (Filename), 
      offset (Offset), length (Length), encoding (LE), maddr (0) {
      }
   
      /** Constructs a data reference from another one.
          @memo Copy constructor.
          @return void
       ******************************************************************/
      gdsDataReference (const gdsDataReference& dref) {
         *this = dref;
      }
   
      /** Copies a data reference. This function does not establish a
          new mapping! Call setMapping to point the data value to the new
          reference if necessary.
          @memo Assignment oparator.
          @param ref data reference which will be copied
          @return this data reference
       ******************************************************************/
      gdsDataReference& operator= (const gdsDataReference& ref);
   
      /** Sets the value pointer to the file data if the data object
          is a reference to a file. Returns true if successful. This
          function always returns true, if the data object has no 
          external link.
          @memo Sets mapping for to binary data.
          @return true if mapping successful or not needed
       ******************************************************************/
      bool setMapping (gdsDataObject& dat);
   
   private:
      /// mapping address (from mmap)
      void*		maddr;
      /// mapping length (from mmap)
      int		mlen;
   };


#if 0
/** This template class is used to implement an auto pointer of a 
    storage object. It is derived from the auto_ptr and implements 
    a special set of constructors and comparision operators.
    @memo Template class for implementing a storage auto pointer.
    @author DS, November 98
    @see Diagnostics storage API
 ************************************************************************/
template <class T>
   class storage_ptr : public std::auto_ptr<T> {
   public:
      /** Constructs a storage auto pointer from an object
          by copying it into a newly allocated object. The caller is
          responsible for destroying the argument object. 
   	  @param x reference to object
          @memo Constructor.
          @return void
       ******************************************************************/
      explicit storage_ptr (const T& x) throw() : 
      std::auto_ptr<T> (new (std::nothrow) T (x)) {
      }
   
      /** Constructs a storage auto pointer from an object
          pointer by insertion (no copy). The ownership is
          transferred to the storage auto pointer. The object had
          to be dynamically allocated with the new operator; and
          must not be destoyed by the caller.
          @memo Constructor.
   	  @param x pointer to object
          @return void
       ******************************************************************/
      explicit storage_ptr (T* x = 0) throw() : 
      std::auto_ptr<T> (x) {
      }
   
      /** Constructs a storage auto pointer from a storage auto pointer
          The ownership is transferred to the storage auto pointer. 
          @memo Copy constructor.
   	  @param st storage pointer
          @return void
       ******************************************************************/
      storage_ptr (const storage_ptr<T>& st) throw() :
      std::auto_ptr<T> (const_cast<storage_ptr<T>&>(st)) {
      }
   
      /** Destructs a storage auto pointer. The storage pointer object
          is released.. 
          @memo Destructor.
          @return void
       ******************************************************************/
      ~storage_ptr () throw() {
         reset ();
      }   
   
      /** Copies a storage auto pointer from a storage auto pointer
          The ownership is transferred to the storage auto pointer. 
          @memo Copy operator.
   	  @param st storage pointer
          @return void
       ******************************************************************/
      storage_ptr<T>& operator= (const storage_ptr<T>& st) throw () {
         reset (const_cast<storage_ptr<T>&>(st).release());
         return *this;
      }
   
      /** Compares the value of an auto pointer with a string.
          @memo Equality operator.
          @param s string
          @return true if equal
       ******************************************************************/
      bool operator== (const string& s) const throw (){
         return (**this == s);
      }
   
      /** Compares the value of an auto pointer with a string.
          @memo Unequality operator.
          @param s string
          @return true if unequal
       ******************************************************************/
      bool operator!= (const string& s) const throw (){
         return (**this != s);
      }
   
      /** Compares the value of an auto pointer with a string.
          @memo Smaller operator.
          @param s string
          @return true if smaller
       ******************************************************************/
      bool operator< (const string& s) const throw () {
         return (**this < s);
      }
   
      /** Compares the value of two auto pointers.
          @memo Equality operator.
          @param s string
          @return true if equal
       ******************************************************************/
      bool operator== (const storage_ptr<T>& st) const throw (){
         return (**this == *st);
      }
   
      /** Compares the value of two auto pointers.
          @memo Smaller operator.
          @param s string
          @return true if smaller
       ******************************************************************/
      bool operator< (const storage_ptr<T>& st) const throw () {
         return (**this < *st);
      }
   };
#endif
/** This template class is used to implement an auto pointer of a 
    storage object. It is derived from the auto_ptr and implements 
    a special set of constructors and comparision operators.
    @memo Template class for implementing a storage auto pointer.
    @author DS, November 98
    @see Diagnostics storage API
 ************************************************************************/
   class prm_storage_ptr {
   protected:
      mutable gdsParameter* ptr;
   public:
      /** Constructs a storage auto pointer from an object
          by copying it into a newly allocated object. The caller is
          responsible for destroying the argument object. 
   	  @param x reference to object
          @memo Constructor.
          @return void
       ******************************************************************/
      explicit prm_storage_ptr (const gdsParameter& x) {
         ptr = new gdsParameter (x); }
   
      /** Constructs a storage auto pointer from an object
          pointer by insertion (no copy). The ownership is
          transferred to the storage auto pointer. The object had
          to be dynamically allocated with the new operator; and
          must not be destoyed by the caller.
          @memo Constructor.
   	  @param x pointer to object
          @return void
       ******************************************************************/
      explicit prm_storage_ptr (gdsParameter* x = 0) {
         ptr = x; }
   
      /** Constructs a storage auto pointer from a storage auto pointer
          The ownership is transferred to the storage auto pointer. 
          @memo Copy constructor.
   	  @param st storage pointer
          @return void
       ******************************************************************/
      prm_storage_ptr (const prm_storage_ptr& st) {
         ptr = st.ptr; st.ptr = 0; }
   
      /** Destructs a storage auto pointer. The storage pointer object
          is released.. 
          @memo Destructor.
          @return void
       ******************************************************************/
      ~prm_storage_ptr () {
         reset ();
      }   
   
      gdsParameter& operator* () const {
         return *ptr; }
      gdsParameter* operator-> () const {
         return ptr; }
      gdsParameter* get () const {
         return ptr; }
      gdsParameter* release () {
         gdsParameter* tmp = ptr; ptr = 0; 
         return tmp; }
      void reset (gdsParameter* p = 0) {
         if (ptr != p) {
            delete ptr;
            ptr = p;
         } }
   
      /** Copies a storage auto pointer from a storage auto pointer
          The ownership is transferred to the storage auto pointer. 
          @memo Copy operator.
   	  @param st storage pointer
          @return void
       ******************************************************************/
      prm_storage_ptr& operator= (const prm_storage_ptr& st) {
         reset (st.ptr); st.ptr = 0; 
         return *this; }
   
      /** Compares the value of an auto pointer with a string.
          @memo Equality operator.
          @param s string
          @return true if equal
       ******************************************************************/
      bool operator== (const string& s) const {
         return (**this == s); }
   
      /** Compares the value of an auto pointer with a string.
          @memo Unequality operator.
          @param s string
          @return true if unequal
       ******************************************************************/
      bool operator!= (const string& s) const {
         return (**this != s); }
   
      /** Compares the value of an auto pointer with a string.
          @memo Smaller operator.
          @param s string
          @return true if smaller
       ******************************************************************/
      bool operator< (const string& s) const {
         return (**this < s); }
   
      /** Compares the value of two auto pointers.
          @memo Equality operator.
          @param s string
          @return true if equal
       ******************************************************************/
      bool operator== (const prm_storage_ptr& st) const {
         return (**this == *st); }
   
      /** Compares the value of two auto pointers.
          @memo Smaller operator.
          @param s string
          @return true if smaller
       ******************************************************************/
      bool operator< (const prm_storage_ptr& st) const {
         return (**this < *st); }
   };


/** This class is used to store a data object.
    @memo Class to store a data object.
    @author DS, November 98
    @see Diagnostics storage API
 ************************************************************************/
   class gdsDataObject : public gdsNamedDatum {
   public:
   
      /** This function reads the data object from an input stream.
          @memo Input operator.
          @param istream input stream
          @param gdsDataObject class
          @return input stream
       ******************************************************************/
      friend std::istream& operator >> (std::istream&, gdsDataObject&);
   
      /** This function writes a data object to an output stream
          following the LigoLW XML specification.
   	  @memo Output operator.
          @param ostream output stream
          @param gdsDataObject class
          @return input stream
       ******************************************************************/
      friend std::ostream& operator << (std::ostream&, 
                        const gdsDataObject&);
   
      typedef prm_storage_ptr gdsParameterPtr;
      /// list of pointers to parameter objects
      typedef std::vector<gdsParameterPtr> gdsParameterList;
      /// flags for describing a data object; used for save/restore
      enum objflag {
      /// global diganostics test parameters
      parameterObj = 0,
      /// the user settings of a diagnostics test
      settingsObj = 1,
      /// results of a diganotics test
      resultObj = 2,
      /// the raw data of a diagnostics test
      rawdataObj = 3,
      /// the plots of a diagnostics test
      imageObj = 4
      };
   
       /// list of parameters asscoaited with the data object
      gdsParameterList	parameters;
      /// link object for referencing binary data objects
      gdsDataReference	link;
      /// Error flag
      bool error;
   
   private:
      /// flag of data object
      objflag		flag;
      /// XML type of object
      string		xmltype;
   
   public:
      /** Constructs a data object with no name and empty 
          value.
          @memo Default constructor.
          @return void
       ******************************************************************/
      gdsDataObject () 
      : gdsNamedDatum ("", gds_void, 0, 0, 0, 0, 0, "", ""),
      error (false), flag (resultObj) {
         encoding = base64;
      }
   
      /** Constructs a data object from an other one.
          @memo Copy constructor.
          @return void
       ******************************************************************/
      gdsDataObject (const gdsDataObject& dat) {
         *this = dat;
      }
   
      /** Constructs a data object. The supplied data field
          is copied (the caller is responsible to free the supplied
          data field if necessary.
          @memo Constructor.
          @param Name name of data object
          @param DataType type of data object
          @param Value pointer to a data value field
          @param Unit unit of data value
          @param Comment comment string for describing data object
   	  @param flag type of data object
          @return void
       ******************************************************************/
      gdsDataObject (const string& Name, gdsDataType DataType, 
                    const void* Value,
                    const string& Unit = "", const string& Comment = "",
                    objflag Flag = resultObj)
      : gdsNamedDatum (Name, DataType, Value, 1, 0, 0, 0, 
                      Unit, Comment), error (false), flag (Flag) {
         encoding = base64;
      }
   
      /** Constructs a data object. The supplied data field
          is copied (the caller is responsible to free the supplied
          data field if necessary.
          @memo Constructor.
          @param Name name of data object
          @param DataType type of data object
          @param Value pointer to a data value field
   	  @param dim1 number of data values
          @param Unit unit of data value
          @param Comment comment string for describing data object
          @param flag type of data object
          @return void
       ******************************************************************/
      gdsDataObject (const string& Name, gdsDataType DataType, 
                    const void* Value, int dim1,
                    const string& Unit = "", const string& Comment = "",
                    objflag Flag = resultObj)
      : gdsNamedDatum (Name, DataType, Value, dim1, 0, 0, 0, 
                      Unit, Comment), error (false), flag (Flag) {       
         encoding = base64;
      }
   
      /** Constructs a data object. The supplied data field
          is copied (the caller is responsible to free the supplied
          data field if necessary.
          @memo Constructor.
          @param Name name of data object
          @param DataType type of data object
          @param Value pointer to a data value field
   	  @param dim1 first dimension of data array
   	  @param dim2 second dimension of data array
          @param Unit unit of data value
          @param Comment comment string for describing data object
          @param flag type of data object
          @return void
       ******************************************************************/
      gdsDataObject (const string& Name, gdsDataType DataType, 
                    const void* Value, int dim1, int dim2,
                    const string& Unit = "", const string& Comment = "",
                    objflag Flag = resultObj)
      : gdsNamedDatum (Name, DataType, Value, dim1, dim2, 0, 0, 
                      Unit, Comment), error (false), flag (Flag) {       
         encoding = base64;
      }
   
      /** Constructs a data object. The supplied data field
          is copied (the caller is responsible to free the supplied
          data field if necessary.
          @memo Constructor.
          @param Name name of data object
          @param DataType type of data object
          @param Value pointer to a data value field
   	  @param dim1 first dimension of data array
   	  @param dim2 second dimension of data array
   	  @param dim3 third dimension of data array
          @param Unit unit of data value
          @param Comment comment string for describing data object
          @param flag type of data object
          @return void
       ******************************************************************/
      gdsDataObject (const string& Name, gdsDataType DataType, const void* Value,
                    int dim1, int dim2, int dim3,
                    const string& Unit = "", const string& Comment = "",
                    objflag Flag = resultObj)
      : gdsNamedDatum (Name, DataType, Value, dim1, dim2, dim3, 0, 
                      Unit, Comment), error (false), flag (Flag) {       
         encoding = base64;
      }
   
      /** Constructs a data object. The supplied data field
          is copied (the caller is responsible to free the supplied
          data field if necessary.
          @memo Constructor.
          @param Name name of data object
          @param DataType type of data object
          @param Value pointer to a data value field
   	  @param dim1 first dimension of data array
   	  @param dim2 second dimension of data array
   	  @param dim3 third dimension of data array
   	  @param dim4 fourth dimension of data array
          @param Unit unit of data value
          @param Comment comment string for describing data object
          @param flag type of data object
          @return void
       ******************************************************************/
      gdsDataObject (const string& Name, gdsDataType DataType, const void* Value,
                    int dim1, int dim2, int dim3, int dim4,
                    const string& Unit = "", const string& Comment = "",
                    objflag Flag = resultObj)
      : gdsNamedDatum (Name, DataType, Value, dim1, dim2, dim3, dim4, 
                      Unit, Comment), error (false), flag (Flag) {
         encoding = base64;
      }
   
      /** Denstructs the data object.
          @memo Destructor.
          @return void
       ******************************************************************/
      virtual ~gdsDataObject ();
   
      /** Copies a data object from an other one.
          @memo Copy operator.
          @return data object
       ******************************************************************/
      gdsDataObject& operator= (const gdsDataObject& dat);
   
      /** Copies a data field.
          @memo Assignment.
          @param gdsDatum data field which will be copied
          @return this data field
       ******************************************************************/
      virtual bool assignDatum (const gdsDatum& dat);
   
      /** Returns the object flag from its name.
          @memo Object flag from name.
          @param oflag object name
          @return object flag
       ******************************************************************/
      static objflag gdsObjectFlag (const string& oflag);
   
      /** Returns the object name from its flag.
          @memo Object flag name
          @param oflag object flag
          @return object flag name
       ******************************************************************/
      static string gdsObjectFlagName (objflag oflag);
   
      /** Gets the object type.
          @memo Get type.
          @return object type
       ******************************************************************/
      string getType () const {
         return xmltype; }
      /** Gets the object flag.
          @memo Get flag.
          @return object flag
       ******************************************************************/
      objflag getFlag () const {
         return flag; }
      /** Sets the object type.
          @memo Set type.
          @param object type
          @return void
       ******************************************************************/
      void setType (const string& otype) {
         xmltype = otype; }
      /** Sets the object flag.
          @memo Set flag.
          @param object flag
          @return void
       ******************************************************************/
      void setFlag (objflag oflag) {
         flag = oflag; }
      /** Sets the object flag.
          @memo Set flag.
          @param object flag name
          @return void
       ******************************************************************/
      void setFlag (const string& oflag) {
         setFlag (gdsObjectFlag (oflag)); }
   
      /** Returns true if the data object maintains a link to a
          file containing binary data.
          @memo Checks if reference to binary data.
          @return true if binary data is referenced, false otherwise
       ******************************************************************/
      bool isRef () const {
         return link.reference;
      }
   };


/** This template class is used to implement an auto pointer of a 
    storage object. It is derived from the auto_ptr and implements 
    a special set of constructors and comparision operators.
    @memo Template class for implementing a storage auto pointer.
    @author DS, November 98
    @see Diagnostics storage API
 ************************************************************************/
   class data_storage_ptr {
   protected:
      mutable gdsDataObject* ptr;
   
   public:
      /** Constructs a storage auto pointer from an object
          by copying it into a newly allocated object. The caller is
          responsible for destroying the argument object. 
   	  @param x reference to object
          @memo Constructor.
          @return void
       ******************************************************************/
      explicit data_storage_ptr (const gdsDataObject& x) {
         ptr = new gdsDataObject (x); }
   
      /** Constructs a storage auto pointer from an object
          pointer by insertion (no copy). The ownership is
          transferred to the storage auto pointer. The object had
          to be dynamically allocated with the new operator; and
          must not be destoyed by the caller.
          @memo Constructor.
   	  @param x pointer to object
          @return void
       ******************************************************************/
      explicit data_storage_ptr (gdsDataObject* x = 0) {
         ptr = x; }
   
      /** Constructs a storage auto pointer from a storage auto pointer
          The ownership is transferred to the storage auto pointer. 
          @memo Copy constructor.
   	  @param st storage pointer
          @return void
       ******************************************************************/
      data_storage_ptr (const data_storage_ptr& st) {
         ptr = st.ptr; st.ptr = 0;
      }
   
      /** Destructs a storage auto pointer. The storage pointer object
          is released.. 
          @memo Destructor.
          @return void
       ******************************************************************/
      ~data_storage_ptr () {
         reset ();
      }   
      gdsDataObject& operator* () const {
         return *ptr; }
      gdsDataObject* operator-> () const {
         return ptr; }
      gdsDataObject* get () const {
         return ptr; }
      gdsDataObject* release () {
         gdsDataObject* tmp = ptr; ptr = 0; 
         return tmp; }
      void reset (gdsDataObject* p = 0) {
         if (ptr != p) {
            delete ptr;
            ptr = p;
         } }
   
      /** Copies a storage auto pointer from a storage auto pointer
          The ownership is transferred to the storage auto pointer. 
          @memo Copy operator.
   	  @param st storage pointer
          @return void
       ******************************************************************/
      data_storage_ptr& operator= (const data_storage_ptr& st) {
         reset (st.ptr); st.ptr = 0;
         return *this;
      }
   
      /** Compares the value of an auto pointer with a string.
          @memo Equality operator.
          @param s string
          @return true if equal
       ******************************************************************/
      bool operator== (const string& s) const {
         return (**this == s); }
   
      /** Compares the value of an auto pointer with a string.
          @memo Unequality operator.
          @param s string
          @return true if unequal
       ******************************************************************/
      bool operator!= (const string& s) const {
         return (**this != s); }
   
      /** Compares the value of an auto pointer with a string.
          @memo Smaller operator.
          @param s string
          @return true if smaller
       ******************************************************************/
      bool operator< (const string& s) const {
         return (**this < s); }
   
      /** Compares the value of two auto pointers.
          @memo Equality operator.
          @param s string
          @return true if equal
       ******************************************************************/
      bool operator== (const data_storage_ptr& st) {
         return (**this == *st); }
   
      /** Compares the value of two auto pointers.
          @memo Smaller operator.
          @param s string
          @return true if smaller
       ******************************************************************/
      bool operator< (const data_storage_ptr& st) const {
         return (**this < *st); }
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
   class gdsStorage : public gdsDataObject {
   public:
   
      /** This function reads the storage object from an input stream.
          The data objects and parameters which are read in are 
          appended to the storage object.
          @memo Input operator.
          @param istream input stream
          @param gdsStorage class
          @return input stream
       ******************************************************************/
      friend std::istream& operator >> (std::istream&, gdsStorage&);
   
      /** This function writes a storage object to an output stream
          following the LigoLW XML specification.
   	  @memo Output operator.
          @param ostream output stream
          @param gdsStorage class
          @return input stream
       ******************************************************************/
      friend std::ostream& operator << (std::ostream&, gdsStorage&);
   
      /// (auto) pointer to a data object object
      typedef data_storage_ptr gdsDataObjectPtr;
      /// list of pointers to data objects
      typedef std::vector<gdsDataObjectPtr> gdsObjectList;
      /// iterator for paramters
      typedef gdsDataObject::gdsParameterList::iterator prm_iterator;
      /// const iterator for paramters
      typedef gdsDataObject::gdsParameterList::const_iterator 
      const_prm_iterator;
      /// iterator for data objects
      typedef gdsObjectList::iterator data_iterator;
      /// const iterator for data objects
      typedef gdsObjectList::const_iterator const_data_iterator;
      /// file type  for save and restore
      enum filetype {
      /// Ligo light-weight data format (XML based)
      LigoLW_XML = 1,
      /// Straight ASCII format
      ASCII = 2
      };
      /// set for describing the save/restore flags
      typedef std::set<objflag, std::less<objflag> > ioflags;
   
      /// set of all save/restore flags
      static const ioflags ioEverything;
      /// set of all save/restore flags, except io_images
      static const ioflags ioExtended;
      /** standard set of save/restore flags: io_parameters, 
          io_results, io_settings */
      static const ioflags ioStandard;
      /// set of save/restore flags only including io_parameters
      static const ioflags ioParamOnly;
   
      /// Mutex to protect storage object in MT environment */
      mutable thread::recursivemutex	mux;
      /// Name of the great maker
      string 		creator;
      /// date of object creation 
      string		date;
      /// list of pointers to data objects
      gdsObjectList	objects;
   
      /** Constructs an empty storage object.
          @memo Default constructor.
          @return void
       ******************************************************************/
      gdsStorage ();
   
      /** Destructs the storage object.
          @memo Destructor.
          @return void
       ******************************************************************/
      virtual ~gdsStorage ();
   
      /** Constructs an empty storage object.
          @memo Constructor.
          @param Creator name of the great maker
          @param Date data/time string
          @param Comment comment string for describing storage object
          @return void
       ******************************************************************/
      gdsStorage (const string& Creator, const string& Date,
                 const string& Comment = "");
   
      /** Constructs a storage object and initializes it from a file.
          @memo Constructor.
          @param filename name of the input file
          @param restoreflags specifies what to restore
          @param filetype specifies the filetype
          @return void
       ******************************************************************/
      explicit gdsStorage (string filename, 
                        ioflags restoreflags = ioExtended, 
                        filetype FileType = LigoLW_XML);
   
      /** Returns true if storage object isn't yet initialized. This
          function can be used to test whether the construction from
          an input file was successful.
          @memo Not operator.
          @return true if not initialized
       ******************************************************************/
      bool operator ! () const;
   
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
   
      /** Returns the error message if either fsave or frestore failed.
          @memo File error function.
          @return error message or empty string
       ******************************************************************/
      string errmsg () const {
         return XML_Error; }
   
      /** Tests wheather the given file name is a registred temporary
          file name.
          @memo Temporary file query function.
          @param filename name of file
          @return true if filename is a temporary file
       ******************************************************************/
      static bool isTempFile (const string& filename);
   
      /** Registers a temporary file name.
          @memo Temporary file register function.
          @param filename name of file
          @return void
       ******************************************************************/
      static void registerTempFile (const string& filename);
   
      /** Unregisters a temporary file name. The temporary file is 
          deleted if this was the last reference to it.
          @memo Temporary file unregister function.
          @param filename name of file
          @return void
       ******************************************************************/
      static void unregisterTempFile (const string& filename);
   
      /** Adds a parameter to the storage object. The parameter
          will be copied if copy is true, and stored with the associated 
          data object
          (specified by its name). If the data object name is empty,
          i.e. equal to "", the parameter is stored in global context.
          @memo Add a parameter.
          @param objname name of data object
          @param prm parameter object
          @param copy copy object if true, otherwise transfer ownership
          @return true if successful
       ******************************************************************/
      virtual bool addParameter (const string& objname, gdsParameter& prm, 
                        bool copy = true);
   
      /** Adds a global parameter. The parameter will be copied if copy
          is true, and stored in global context.
          @memo Add a parameter.
          @param prm parameter object
          @param copy copy object if true, otherwise transfer ownership
          @return true if successful
       ******************************************************************/
      virtual bool addParameter (gdsParameter& prm, bool copy = true);
   
      /** Adds a data object. The data object will be copied if copy is 
          true and stored in global context.
          @memo Add a data object.
          @param dat data object
          @param copy copy object if true, otherwise transfer ownership
          @return true if successful
       ******************************************************************/
      virtual bool addData (gdsDataObject& dat, bool copy = true);
   
      /** Removes a parameter object from its asscoiated data object.
          If the data object name is empty, i.e. equal to "", 
          the parameter is removed from global context. 
          @memo Remove a parameter.
          @param objname name of data object
          @param prmname name of parameter
          @return true if successful
       ******************************************************************/
      virtual bool eraseParameter (const string& objname, 
                        const string& prmname);
   
      /** Removes a global parameter object. The specified parameter is 
          removed from its global context.
          @memo Remove a parameter.
          @param prmname name of parameter
          @return true if successful
       ******************************************************************/
      virtual bool eraseParameter (const string& prmname);
   
      /** Removes a data object. The specified data object is removed
   	  from its global context.
          @memo Remove a data object.
          @param objname name of data object
          @return true if successful
       ******************************************************************/
      virtual bool eraseData (const string& objname);
   
      /** Finds a parameter by its name and the name of its associated
          data object.
          @memo Find a parameter.
          @param prmname name of parameter
          @param objname name of data object
          @return pointer to parameter, or 0 if not found
       ******************************************************************/	
      virtual gdsParameter* findParameter (const string& objname, 
                        const string& prmname) const;
   
      /** Finds a globale parameter by its name.
          @memo Find a parameter.
          @param prmname name of parameter
          @return pointer to parameter, or 0 if not found
       ******************************************************************/					
      virtual gdsParameter* findParameter (const string& prmname) const;
   
      /** Finds a data object by its name.
          @memo Find a data object.
          @param objname name of data object
          @return pointer to data object, or 0 if not found
       ******************************************************************/
      virtual gdsDataObject* findData (const string& objname) const;
   
      /** Sets up a new data object for channel data. The newly created
          data object is either of type float or complex<float>. It
          contains no data, it uses a memory mapped file by default.
          @memo Sets up a channel data object.
          @param name name of channel data object
          @param start time of first data point (GPS nsec)
          @param dt spacing of data points (sec)
          @param cmplx true if time series is down-converted
          @return pointer to data object, or 0 if not failed
       ******************************************************************/
      virtual gdsDataObject* newChannel (const string& objname, 
                        tainsec_t start, double dt, bool cmplx = false,
                        bool memmap = false);
   
      /** Allocates memory for new channel data. The allocated memory has 
          to filled by the caller; and if finished, the caller has to
          call the notifyChannelMem method to inform the data object to 
          make the new data available for read. A second allocation of
          memory for the same channel returns only after the notification
          of the first allocation was seen.
          @memo Allocates memory for channel data.
          @param name name of channel data object
          @param len number of data points
          @return pointer to newly allocated data array
       ******************************************************************/
      virtual float* allocateChannelMem (const string& objname, 
                        int length);
   
      /** Notifies the data object that the newly allocated memory for
          channel data is initialized.
          @memo Notification of memory initialization.
          @param name name of channel data object
          @param error can be used to set the error flag
          @return void
       ******************************************************************/
      virtual void notifyChannelMem (const string& objname, 
                        bool error = false);
   
      /** Locks the data object to make sure that it isn't changed
          while accessing.
          @memo Lock data object.
          @param name name of data object
          @param write if true locks for exclusive use
          @return pointer to data object, 0 if failed
       ******************************************************************/
      gdsDataObject* lockData (const string& objname, bool write = false);
   
      /** Tries to lock a data object for exclusive use (write==true),
          or for shared use (write==false).
          @memo Trylock data object.
          @param name name of data object
          @param write if true locks for exclusive use
          @return pointer to data object, 0 if failed
       ******************************************************************/
      gdsDataObject* trylockData (const string& objname, 
                        bool write = false);
   
      /** Unlocks the data object after use.
          @memo Unlock data object.
          @param dat pointer to data object
          @return void
       ******************************************************************/
      void unlockData (gdsDataObject* dat);
   
   protected:
      /// storage of temprary file names
      class tempnames : public std::vector<string> {
      public:
         ~tempnames ();
      };
      /// attribute list of an XML tag
      typedef std::map<string, stringcase> attrtype;
   
      /// mutex to access temporary file storage
      static thread::mutex	tempfilemux;
      /// temporary file storage
      static tempnames	tempfiles;
   
      /// XML termination flag
      bool		XML_fini;
      /// XML initialization flag;
      bool		XML_init;
      /// XML new line flag
      bool		XML_newline;
      /// skip level: if >0 XML elements are skipped
      int		XML_Skip;
      /// XML key
      string		XML_Key;
      /// XML 2nd key
      string		XML_Key2;
      /// pointer to current parameter object for XML read
      gdsParameter*	XML_Param;
      /// pointer to current data object for XML read
      gdsDataObject*	XML_Obj;
      /// determines if fast decoding of is present
      bool		XML_fast;
      /// XML error string
      string		XML_Error;
   
      /// write XML header
      void fwriteXML (std::ostream& os);
      /// write binary data
      bool fwriteBinary (std::ostream& os);
      /// fix self references
      int ffixRef (int XML_Length);
   
      /// XML start element handler
      virtual void startElement (const string& elName, 
                        const attrtype& atts);
      /// XML end element handler
      virtual void endElement (const string& elName);
      /// XML text element handler
      virtual void textHandler (std::stringstream& text);
   
   private:
      /// holds a list of all io flags
      static const objflag ioAll[5];
      /// true of storage object is initialized
      bool		initialized;
      /// currently active flags
      ioflags 		activeFlags;
      /// currently active file type
      filetype 		activeFiletype;
      /// XML text buffer for text handler
      std::auto_ptr<std::stringstream> XML_text;
      /// XML start element handler callback
      static void startelement (gdsStorage* dat, const char* name, 
                        const char** attributes);
      /// XML end element handler callback
      static void endelement (gdsStorage* dat, const char* name);
      /// XML text element handler callback
      static void texthandler (gdsStorage* dat, const char* text, int len);
   
      /// prevent copy
      gdsStorage (const gdsStorage&);
      gdsStorage& operator= (const gdsStorage&);
   };

//@}
}

#endif /* _GDS_DATUM_H */
