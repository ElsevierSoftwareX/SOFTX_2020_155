static const char *versionId = "Version $Id$" ;
/*----------------------------------------------------------------------*/
/*                                                         		*/
/* Module Name: gdsdatum						*/
/*                                                         		*/
/* Module Description: implements a storage object to store data and	*/
/* parameters, results and settings of a diagnostics test		*/
/*                                                         		*/
/*----------------------------------------------------------------------*/

/* Header File List: */

#include <strings.h>
#include <complex>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <xmlparse.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include "dtt/gdsdatum.hh"
#include "dtt/gdsstring.h"
#include "dtt/diagnames.h"

namespace diag {
   using namespace std;
   using namespace thread;

   const char xmlVersion[] = "<?xml version=\"1.0\"?>";
   //const char xmlDocType[] = "<!DOCTYPE LIGO_LW SYSTEM "
   //"\"http://www.cacr.caltech.edu/projects/ligo_lw.dtd\">";
   const char xmlDocType[] = 
   "<!DOCTYPE LIGO_LW [\n"
   "<!ELEMENT LIGO_LW ((LIGO_LW|Comment|Param|Time|Table|Array|Stream)*)>\n"
   "<!ATTLIST LIGO_LW Name CDATA #IMPLIED Type CDATA #IMPLIED>\n"
   "<!ELEMENT Comment (#PCDATA)>\n"
   "<!ELEMENT Param (#PCDATA)>\n"
   "<!ATTLIST Param Name CDATA #IMPLIED Type CDATA #IMPLIED Dim CDATA #IMPLIED\n"
   "                Unit CDATA #IMPLIED>\n"
   "<!ELEMENT Table (Comment?,Column*,Stream?)>\n"
   "<!ATTLIST Table Name CDATA #IMPLIED Type CDATA #IMPLIED>\n"
   "<!ELEMENT Column EMPTY>\n"
   "<!ATTLIST Column Name CDATA #IMPLIED Type CDATA #IMPLIED Unit CDATA #IMPLIED>\n"
   "<!ELEMENT Array (Dim*,Stream?)>\n"
   "<!ATTLIST Array Name CDATA #IMPLIED Type CDATA #IMPLIED>\n"
   "<!ELEMENT Dim (#PCDATA)>\n"
   "<!ATTLIST Dim Name CDATA #IMPLIED>\n"
   "<!ELEMENT Stream (#PCDATA)>\n"
   "<!ATTLIST Stream Name CDATA #IMPLIED Type (Remote|Local) \"Local\"\n"
   "          Delimiter CDATA \",\" Encoding CDATA #IMPLIED Content CDATA #IMPLIED>\n"
   "<!ELEMENT Time (#PCDATA)>\n"
   "<!ATTLIST Time Name CDATA #IMPLIED Type (GPS|Unix|ISO-8601) \"ISO-8601\">\n"
   "]>";

   const char xmlLigoLW[] = "LIGO_LW";
   const char xmlContainer[] = "LIGO_LW";
   const char xmlHeader[] = "Header";
   const char xmlCreator[] = "Creator";
   const char xmlTime[] = "Time";
   const char xmlComment[] = "Comment";
   const char xmlParameter[] = "Param";
   const char xmlName[] = "Name";
   const char xmlType[] = "Type";
   const char xmlFlag[] = "Flag";
   const char xmlUnit[] = "Unit";
   const char xmlLength[] = "Dim";
   const char xmlArray[] = "Array";
   const char xmlDimension[] = "Dim";
   const char xmlData[] = "Stream";
   const char xmlDelimiter[] = "Delimiter";
   const char xmlLink[] = "Link";
   const char xmlOffset[] = "Offset";
   const char xmlRef[] = "Ref";
   const char xmlEncoding[] = "Encoding";
   const char xmlByteOrderWord[2][16] = {"BigEndian", "LittleEndian"};
   const int XML_BinaryAlign = 16;

   const char xmlObjTypeTestParameters[] = "TestParameters";
   const char xmlObjTypeTimeSeries[] = "TimeSeries";
   const char xmlObjTypeSettings[] = "Settings";
   const char xmlObjTypeImage[] = "Image";
   const char xmlObjTypeResult[] = "Result";


//______________________________________________________________________________
   bool littleEndian ()
   {
      int test = 0;
      *(char*) &test = 1;
      return (test == 1);
   }

//______________________________________________________________________________
   inline void swap64 (uint64_t* ww)
   {
      uint32_t temp;
      temp = 
         (((((uint32_t*)ww)[0])&0xff)<<24) |
         (((((uint32_t*)ww)[0])&0xff00)<<8) |
         (((((uint32_t*)ww)[0])&0xff0000)>>8) |
         (((((uint32_t*)ww)[0])&0xff000000)>>24); 
      ((uint32_t*)ww)[0] = 
         (((((uint32_t*)ww)[1])&0xff)<<24) |
         (((((uint32_t*)ww)[1])&0xff00)<<8) |
         (((((uint32_t*)ww)[1])&0xff0000)>>8) |
         (((((uint32_t*)ww)[1])&0xff000000)>>24); 
      ((uint32_t*)ww)[1] = temp;
   }

//______________________________________________________________________________
   inline void swap32 (uint32_t* w)
   {
      *((uint32_t*)w) = 
         ((((*(uint32_t*)w))&0xff)<<24) |
         (((*((uint32_t*)w))&0xff00)<<8) |
         (((*((uint32_t*)w))&0xff0000)>>8) |
         (((*((uint32_t*)w))&0xff000000)>>24);   
   }

//______________________________________________________________________________
   inline void swap16 (uint16_t* w)
   {
      *((uint16_t*)w) = 
         ((((*(uint16_t*)w))&0xff)<<8) |
         (((*((uint16_t*)w))&0xff00)>>8);   
   }

//______________________________________________________________________________
   void swapByteOrder (char* p, int num, int elsize) 
   {
      switch (elsize) {
         default:
         case 1:
            {
               break;
            }
         case 2:
            {
               uint16_t* x = (uint16_t*) p;
               for (int i = 0; i < num; i++, x++) {
                  swap16 (x);
               }
               break;
            }
         case 4:
            {
               uint32_t* x = (uint32_t*) p;
               for (int i = 0; i < num; i++, x++) {
                  swap32 (x);
               }
               break;
            }
         case 8:
            {
               uint64_t* x = (uint64_t*) p;
               for (int i = 0; i < num; i++, x++) {
                  swap64 (x);
               }
               break;
            }
      }
   }

//______________________________________________________________________________
   string xmlByteOrder (void) 
   {
      int	test = 0;
      *(char*) &test = 1;
      return (test == 1) ? xmlByteOrderWord[1] : xmlByteOrderWord [0];
   }

   const char* const xmlKeys[] = 
   {xmlLigoLW, xmlContainer, xmlComment, xmlParameter, xmlTime, 
   xmlArray, xmlDimension, xmlData};

   const char table_uuencode[65] = 
   "`!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";
   const char table_base64[65] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

   const char itable_uuencode[257] =
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017"
   "\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037"
   "\040\041\042\043\044\045\046\047\050\051\052\053\054\055\056\057"
   "\060\061\062\063\064\065\066\067\070\071\072\073\074\075\076\077"
   "\000\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377";
   const char itable_base64[257] =
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\076\377\377\377\077"
   "\064\065\066\067\070\071\072\073\074\075\377\377\377\377\377\377"
   "\377\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016"
   "\017\020\021\022\023\024\025\026\027\030\031\377\377\377\377\377"
   "\377\032\033\034\035\036\037\040\041\042\043\044\045\046\047\050"
   "\051\052\053\054\055\056\057\060\061\062\063\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
   "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377";



   string gdsDataTypeName (gdsDataType datatype)
   {
      switch (datatype) {
         case gds_int8:
            return "byte";
         case gds_int16:
            return "short";
         case gds_int32:
            return "int";
         case gds_float32:
            return "float";
         case gds_int64:
            return "long";
         case gds_float64:
            return "double";
         case gds_complex32:
            return "floatComplex";
         case gds_complex64:
            return "doubleComplex";
         case gds_string:
         case gds_channel:
            return "string";
         case gds_bool:
            return "boolean";
         default: 
            return "void";
      }
   }


   gdsDataType gdsNameDataType (string name) 
   {
      if ((gds_strcasecmp (name.c_str(), "byte") == 0) ||
         (strcasecmp (name.c_str(), "char") == 0) || // ldas rubbish
         (strcasecmp (name.c_str(), "char_u") == 0)) {
         return gds_int8;
      } 
      else if ((gds_strcasecmp (name.c_str(), "short") == 0) ||
              (strcasecmp (name.c_str(), "int_2s") == 0) || // ldas rubbish
              (strcasecmp (name.c_str(), "int_2u") == 0)) {
         return gds_int16;
      } 
      else if ((gds_strcasecmp (name.c_str(), "int") == 0) ||
              (strcasecmp (name.c_str(), "int_4s") == 0) || // ldas rubbish
              (strcasecmp (name.c_str(), "int_4u") == 0)) {
         return gds_int32;
      } 
      else if ((gds_strcasecmp (name.c_str(), "long") == 0) ||
              (strcasecmp (name.c_str(), "int_8s") == 0) || // ldas rubbish
              (strcasecmp (name.c_str(), "int_8u") == 0)) {
         return gds_int64;
      } 
      else if ((gds_strcasecmp (name.c_str(), "float") == 0) ||
              (strcasecmp (name.c_str(), "real_4") == 0)) { // ldas rubbish
         return gds_float32;
      } 
      else if ((gds_strcasecmp (name.c_str(), "double") == 0) ||
              (strcasecmp (name.c_str(), "real_8") == 0)) { // ldas rubbish
      
         return gds_float64;
      } 
      else if ((gds_strcasecmp (name.c_str(), "floatComplex") == 0) ||
              (gds_strcasecmp (name.c_str(), "complex_8") == 0)) {
         return gds_complex32;
      } 
      else if ((gds_strcasecmp (name.c_str(), "doubleComplex") == 0) ||
              (gds_strcasecmp (name.c_str(), "complex_16") == 0)) {
         return gds_complex64;
      } 
      else if ((gds_strcasecmp (name.c_str(), "string") == 0) ||
              (strcasecmp (name.c_str(), "lstring") == 0)) { // ldas rubbish
         return gds_string;
      } 
      else if (gds_strcasecmp (name.c_str(), "channel") == 0) {
         return gds_channel;
      } 
      else if (gds_strcasecmp (name.c_str(), "boolean") == 0) {
         return gds_bool;
      } 
      else {
         return gds_void;
      }
   }


//______________________________________________________________________________
   static std::string xsilStringEscape (const char* s)
   {
      string p;
      for (; *s; ++s) {
         if (*s == '<') {
            p += "&lt;";
         }
         else if (*s == '>') {
            p += "&gt;";
         }
         else if (*s == '&') {
            p += "&amp;";
         }
         else if (*s == '"') {
            p += "&quot;";
         }
         else if (*s == '\'') {
            p += "&apos;";
         }
         else {
            p += *s;
         }
      }
      return p;
   }


//______________________________________________________________________________
   string gdsStrDataType (gdsDataType datatype, const void* value,
                     bool xmlescape)
   {
      static ostringstream	oss;
      ostringstream	os;
   
      if (value == 0) {
         return "";
      }
   
      switch (datatype) {
         case gds_int8:
            os << (isgraph (*((char*) value)) ? *((char*) value) : ' ');
            break;
         case gds_int16:
            os << *((short*) value);
            break;
         case gds_int32:
            os << *((int*) value);
            break;
         case gds_float32:
            os << *((float*) value);
            break;
         case gds_int64:
            os << *((int64_t*) value);
            break;
         case gds_float64:
            os << *((double*) value);
            break;
         case gds_complex32:
            os << ((complex<float>*) value)->real() << " " << 
               ((complex<float>*) value)->imag();
            break;
         case gds_complex64:
            os << ((complex<double>*) value)->real() << " " << 
               ((complex<double>*) value)->imag();
            break;
         case gds_string:
         case gds_channel:
            if (xmlescape) {
               return xsilStringEscape ((char*) value);
            }
            else {
               return string ((char*) value);
            }
         case gds_bool:
            if (*((bool*) value)) {
               os << "true";
            }
            else {
               os << "false";
            }
            break;
         default: 
            break;
      }
   
      return os.str();
   }


   bool gdsValueDataType (void* value, gdsDataType datatype, 
                     const string& datum)
   {
      istringstream	is (datum.c_str());
      if (value == 0) {
         return false;
      }
      switch (datatype) {
         case gds_int8:
            is >> *((char*) value);
            if (!is || !isgraph (*((char*) value))) {
               *((char*) value) = ' ';
            }
            break;
         case gds_int16:
            is >> *((short*) value);
            break;
         case gds_int32:
            is >> *((int*) value);
            break;
         case gds_float32:
            is >> *((float*) value);
            break;
         case gds_int64:
            is >> *((int64_t*) value);
            break;
         case gds_float64:
            is >> *((double*) value);
            break;
         case gds_complex32:
            float	re, im;
            is >> re >> im;
            *(complex<float>*) value = complex<float> (re, im);
            break;
         case gds_complex64:
            double	dre, dim;
            is >> dre >> dim;
            *(complex<double>*) value = complex<double> (dre, dim);
            break;
         case gds_string:
         case gds_channel:
            is >> *((char*) value);
            break;
         case gds_bool:
            *((bool*) value) = (datum.size() > 0) && 
               ((datum[0] == 't') || (datum[0] == 'y') || 
               (datum[0] == 'T') || (datum[0] == 'Y') || 
               (datum[0] == '1'));
            break;
         default: 
            break;
      }
      return !!is;
   
   }


   class indent {
      friend ostream& operator << (ostream&, const indent&);
   public:
      indent (int num) : space (num) {
      }
   private:
      int 	space;
   };


   ostream& operator << (ostream& os, const indent& dat)          
   {
      if (dat.space > 0) {
         os << setw (2 * dat.space) << ' ';
      }
      return os;
   }

   //ofstream log ("memory.log");

   gdsDatum::gdsDatum (gdsDataType DataType, const void* Value,
                     int dim1, int dim2, int dim3, int dim4) 
   : datatype (DataType), value (0), encoding (ascii), swapit (false)
   {
      if ((datatype == gds_string) || (datatype == gds_channel)) {
         dimension.push_back (1);
         if (Value == 0) {
            value = 0;
         }
         else {
            int		len = strlen ((char*) Value);
            value = new (nothrow) char [len + 1];
            if (value != 0) {
               ((char*) value)[len] = '\0';
               strncpy ((char*) value, (char*) Value, len);
            }
         }
      }
      
      else {
         if (dim1 != 0) {
            dimension.push_back (dim1);
            if (dim2 != 0) {
               dimension.push_back (dim2);
               if (dim3 != 0) {
                  dimension.push_back (dim3);
                  if (dim4 != 0) {
                     dimension.push_back (dim4);
                  }
               }
            }
         }
      
         int		valsize	= size();
         if (valsize > 0) {
            value = new (std::nothrow) char [valsize];
            // if (valsize > 1000) 
               // log << "Allocate(-) " << valsize << " @ " << (void*)value << endl;
            if (value != 0) {
               if (Value != 0) {
                  memcpy (value, Value, valsize);
               }
               else {
                  memset (value, 0, valsize);
               }
            }
         }
         else {
            value = 0;
         }
      }
   }


   gdsDatum::~gdsDatum (void) 
   {
      // if (size() > 1000) 
         // log << "Deallocate(~)" << size() << " @ " << (void*)value << endl;
      delete [] (char*)value;
   }


   gdsDatum::gdsDatum (const gdsDatum& dat) 
   : value (0) 
   {
      assignDatum (dat);
   }


   bool gdsDatum::resize (int dim1, int dim2, int dim3, int dim4)
   {
      if ((datatype == gds_string) || (datatype == gds_channel)) {
         return true;
      }
      // set new dimensions
      int oldSize = size();
      dimension_t olddim = dimension;
      dimension.clear();
      if (dim1 != 0) {
         dimension.push_back (dim1);
         if (dim2 != 0) {
            dimension.push_back (dim2);
            if (dim3 != 0) {
               dimension.push_back (dim3);
               if (dim4 != 0) {
                  dimension.push_back (dim4);
               }
            }
         }
      }
      int valsize = size();
      if (oldSize == valsize) {
         return true;
      }
      if (valsize <= 0) {
         // if (size() > 1000) 
            // log << "Deallocate(resize) " << oldSize << " @ " << (void*)value << endl;
         delete [] (char*)value;
         value = 0;
      }
      else {
         void* tmp = new (std::nothrow) char [valsize];
         // if (valsize > 1000) 
            // log << "Allocate(resize) " << valsize << " @ " << (void*)tmp << endl;
         if (tmp == 0) {
            dimension = olddim;
            return false;
         }
         int cpy = valsize < oldSize ? valsize : oldSize;
         memcpy (tmp, value, cpy);
         if (cpy < valsize) {
            memset ((char*)tmp + cpy, 0, valsize - cpy);
         }
         // if (size() > 1000) 
            // log << "Deallocate(resize) " << oldSize << " @ " << (void*)value << endl;
         delete [] (char*)value;
         value = tmp;
      }
      return true;
   }


   bool gdsDatum::assignDatum (const gdsDatum& dat)
   {
      lock (true);
      dat.lock ();
      // if (size() > 1000) 
         // log << "Deallocate(assign) " << size() << " @ " << (void*)value << endl;
      datatype = dat.datatype;
      dimension = dat.dimension;
      encoding = dat.encoding;
      swapit = dat.swapit;
      if (value) delete [] (char*)value;
      if (dat.value == 0) {
         value = 0;
      }
      else {
         if ((datatype == gds_string) || (datatype == gds_channel)) {
            int		len = strlen ((char*) dat.value);
            value = new (nothrow) char [len + 1];
            if (value == 0) {
               unlock ();
               dat.unlock();
               return false;
            }
            ((char*) value)[len] = '\0';
            strncpy ((char*) value, (char*) dat.value, len);
         }
         else {
            value = new (nothrow) char [dat.size()];
            // if (dat.size() > 1000) 
               // log << "Allocate(assign) " << dat.size() << " @ " << (void*)value << endl;
            if (value == 0) {
               unlock ();
               dat.unlock();
               return false;
            }
            memcpy (value, dat.value, dat.size());
         }
      }
      unlock ();
      dat.unlock();
      return true;
   }


   string gdsDatum::codeName (encodingtype ctype) 
   {
      switch (ctype) {
         case ascii:
            {
               return "Text";
            }
         case binary:
            {
               return "Binary";
            }
         case uuencode:
            {
               return "uuencode";
            }
         case base64:
            {
               return "base64";
            }
         default:
            {
               return "unknown";
            }
      }
   }


   gdsDatum::encodingtype gdsDatum::code (string name) 
   {
      if (gds_strcasecmp (name.c_str(), "text") == 0) {
         return ascii;
      }
      else if (gds_strcasecmp (name.c_str(), "binary") == 0) {
         return binary;
      }
      else if (gds_strcasecmp (name.c_str(), "uuencode") == 0) {
         return uuencode;
      }
      else if (gds_strcasecmp (name.c_str(), "base64") == 0) {
         return base64;
      }
      else {
         return ascii;
      }
   }


   bool gdsDatum::encode (ostream& os, const char p[], int len, 
                     encodingtype ctype, int indent)
   {
      const char* 	table =
         ((ctype == uuencode) ? table_uuencode : table_base64);
      int		i = 0;
   
      if (p == 0) {
         return len == 0;
      }
   
      // convert
      while (i < len) {
         if ((indent > 0) && (i % 48 == 0)) {
            os << setw (indent) << ' ';
         }
         os.put(table[(p[i++] >> 2) & 0x3F]);
         if (i >= len) {
            break;
         }
         os.put(table[((p[i-1] << 4) | ((p[i] >> 4) & 0x0F)) & 0x3F]);
         i++;
         if (i >= len) {
            break;
         }
         os.put(table[((p[i-1] << 2) | ((p[i] >> 6) & 0x03)) & 0x3F]);
         os.put(table[p[i] & 0x3F]);
         i++;
         if (i % 48 == 0) {
            os << endl;
         }
      }
   
      // patch end
      switch (i % 3) {
         case 0: 
            {
               break;
            }
         case 1: 
            {
               os.put(table[(p[i-1] & 0x03) << 4]);
               if (ctype == base64) {
                  os << "==";
               }
               break;
            }
         case 2: 
            {
               os.put(table[(p[i-1] & 0x0F) << 2]);
               if (ctype == base64) {
                  os << "=";
               }
               break;
            }
      }
      return !!os;
   }


   bool gdsDatum::decode (istream& is, char* p, int len, 
                     encodingtype ctype)
   {
      const char* 	table =
         ((ctype == uuencode) ? itable_uuencode : itable_base64);
      int		i = 0;
      int		bits = 0;
      int		tmp = 0;
      char		c;
   
      while (i < len) {
         while (is.get (c) && (table[c] == '\377')) {};
         if (!is) {
            return false;
         }
         tmp = (tmp << 6) | (table[c] & 0x3F);
         bits += 6;
         if (bits >= 8) {
            p[i++] = (tmp >> (bits - 8)) & 0xFF;
            bits -= 8;
         }
      }
      return true;
   }


   bool gdsDatum::decode (const char* code, int codelen,
                     char* p, int len, 
                     encodingtype ctype)
   {
      const char* 	table =
         ((ctype == uuencode) ? itable_uuencode : itable_base64);
      int		i = 0;
      int		bits = 0;
      int		tmp = 0;
      int		pos = 0;
      int 		decod;
   
      while (i < len) {
         if ((pos >= codelen) || 
            ((decod = table[code[pos++]]) == '\377')) {
            return false;
         }
         tmp = (tmp << 6) | (decod & 0x3F);
         bits += 6;
         if (bits >= 8) {
            p[i++] = (tmp >> (bits - 8)) & 0xFF;
            bits -= 8;
         }
      }
      return true;
   }


   gdsDatum& gdsDatum::operator= (const gdsDatum& dat) 
   {
      if (this != &dat) {
         gdsDatum::assignDatum (dat);
      }
      return *this;
   }


   int gdsDatum::elSize () const
   {
      switch (datatype) {
         case gds_int8:
            return 1;
         case gds_int16:
            return 2;
         case gds_int32:
         case gds_float32:
            return 4;
         case gds_int64:
         case gds_float64:
         case gds_complex32:
            return 8;
         case gds_complex64:
            return 16;
         case gds_string:
         case gds_channel:
            return 1;
         case gds_bool:
            return sizeof (bool);
         default: 
            return 0;
      }
   }

   int gdsDatum::elNumber () const
   {
      int		s = 1;
   
      for (dimension_t::const_iterator iter = dimension.begin(); 
          iter != dimension.end(); iter++) {
         s *= *iter;
      }
      return s;
   }

   int gdsDatum::size () const {
      return elSize() * elNumber();
   };

   bool gdsDatum::isComplex () const
   {
      return ((datatype == gds_complex32) || (datatype == gds_complex64));
   }


   istream& operator >> (istream& is, gdsDatum&)
   {
      return is;
   }


   int gdsDatum::readValues (const string& txt) 
   {
      lock (true);
   
      // free memory
      if (value != 0) {
         // if (size() > 1000) 
            // log << "Deallocate(read) " << size() << " @ " << (void*)value << endl;
         delete [] (char*)value;
         value = 0;
      }
      // read string
      if ((datatype == gds_string) || (datatype == gds_channel)) {
         if (!txt.empty()) {
            value = new (nothrow) char [txt.size() + 1];
            if (value != 0) {
               ((char*) value)[txt.size()] = '\0';
               strncpy ((char*) value, txt.c_str(), txt.size());
            }
         }
         else {
            // value = new (nothrow) char [1];
            // if (value != 0) {
               // ((char*) value)[0] = '\0';
            // }
            value = 0;
         }
         unlock ();
         return 1;
      }
      // read numbers
      else {
         // allocate memory
         value = new (nothrow) char [size()];
         // if (size() > 1000) 
            // log << "Allocate(read) " << size() << " @ " << (void*)value << endl;
         if (value == 0) {
            unlock ();
            return -1;
         }
         memset (value, 0, size());
         // set temporary vars
         istringstream 	is (txt.c_str());
         char* 		val = (char*) value;
         int		max = elNumber();
         string 	datum;
         // read in values
         for (int num = 0; num < max; num++, val += elSize()) {
            is >> datum;
            // since space chars are automatically removed; add them here
            if ((datatype == gds_int8) && (!is)) {
               datum = " ";
               is.clear();
            }
            // special case: complex numbers
            if ((datatype == gds_complex32) ||
               (datatype == gds_complex64)) {
               string		datum2;
               is >> datum2;
               datum += " " + datum2;
            }
            if ((!is) || 
               (!gdsValueDataType (val, datatype, datum))) {
               unlock ();
               return num;
            }
         }
         unlock ();
         return max;
      }
   }


   ostream& operator << (ostream& os, const gdsDatum& dat)
   {
      int		max = dat.elNumber();
      char*		val = (char*) dat.value;
      int		ofs = dat.elSize();
      int		size = dat.size();
   
      switch (dat.encoding) {
         case gdsDatum::binary:
            {
               os.write (val, size);
               break;
            }
         case gdsDatum::uuencode:
         case gdsDatum::base64:
            {
               gdsDatum::encode (os, val, size, dat.encoding, 0);
               break;
            }
         case gdsDatum::ascii:
         default: 
            {
               for (int num = 0; num < max; num++, val+=ofs) {
                  if ((dat.dimension.size() > 1) && 
                     (num %  dat.dimension.back() == 0)) {
                     os << indent (3);
                  }
                  os << gdsStrDataType (dat.datatype, val, true);
                  if (num + 1 < max) {
                     if ((dat.dimension.size() > 1) && 
                        ((num + 1) % dat.dimension.back() == 0)) {
                        os << endl;
                     }
                     else if (num % 10 == 9) {
                        os << endl << "          ";
                     }
                     else {
                        os << '\t';
                     }
                  }
               }
               break;
            }
      }
      return os;
   }


   int compareObjectNames (const string& ss1, const string& ss2) 
   {
      const char*	s1 = ss1.c_str();
      const char*	s2 = ss2.c_str();
      int 		d;
   
      while (*s1 || *s2) {
         // skip blanks
         if ((*s1 == ' ') || (*s1 == '\t')) {
            s1++;
            continue;
         }
         if ((*s2 == ' ') || (*s2 == '\t')) {
            s2++;
            continue;
         }
      	 // compare with ignoring case
         if ((d = tolower (*s1) - tolower (*s2))) {
            return d;
         }
      	 // check if array indices are used
         if (*s1 == '[') {
            int index1 = atoi (s1 + 1);
            int index2 = atoi (s2 + 1);
            // compare indices
            if ((d = index1 - index2)) {
               return d;
            }
            // skip until closing bracket
            while (*s1 && (*s1 != ']')) {
               s1++;
            }
            while (*s2 && (*s2 != ']')) {
               s2++;
            }
         }
         else {
            s1++;
            s2++;
         }
      }
      return 0;
   }


   bool gdsNamedStorage::operator == (const gdsNamedStorage& x) const 
   {
      return (compareObjectNames (name, x.name) == 0);
   }


   bool gdsNamedStorage::operator == (const string& x) const 
   {
      return (compareObjectNames (name, x) == 0);
   }


   bool gdsNamedStorage::operator != (const gdsNamedStorage& x) const 
   {
      return !(*this == x);
   }


   bool gdsNamedStorage::operator != (const string& x) const 
   {
      return (compareObjectNames (name, x) != 0);
   }


   bool gdsNamedStorage::operator <= (const gdsNamedStorage& x) const 
   {
      return (compareObjectNames (name, x.name) <= 0);
   }


   bool gdsNamedStorage::operator < (const gdsNamedStorage& x) const 
   {
      return (compareObjectNames (name, x.name) < 0);
   }


   bool gdsNamedStorage::operator < (const string& x) const 
   {
      return (compareObjectNames (name, x) < 0);
   }


   bool gdsNamedStorage::operator >= (const gdsNamedStorage& x) const 
   {
      return !(*this < x);
   }


   bool gdsNamedStorage::operator > (const gdsNamedStorage& x) const 
   {
      return !(*this <= x);
   }


   istream& operator >> (istream& is, gdsParameter&)
   {
      return is;
   }


   ostream& operator << (ostream& os, const gdsParameter& prm)
   {
      if ((prm.datatype == gds_int64) && (prm.elNumber() == 1) &&
         (prm.unit == "ns")) {
         // time parameter
         string t = gdsStrDataType (gds_int64, prm.value);
         if (t.size() > 9) {
            t.insert (t.size() - 9, 1, '.');
         }
         else {
            while (t.size() < 9) t.insert ((string::size_type)0, 1, '0');
            t.insert (0, "0.");
         }
         while (t[t.size()-1] == '0') {
            t.erase (t.size()-1, 1);
         }
         if (t[t.size()-1] == '.') {
               //t.erase (t.size()-1, 1);
            t += '0'; // for ldas
         }
         os << indent(1+prm.level) << "<" << xmlTime <<
            " " << xmlName << "=\"" << prm.name << "\"" <<
            " Type=\"GPS\">" << t << "</" << xmlTime << ">" << endl;
      }
      else if ((prm.datatype == gds_string) && (prm.elNumber() == 1) &&
              (prm.unit == "ISO-8601")) {
         // time parameter
         os << indent(1+prm.level) << "<" << xmlTime <<
            " " << xmlName << "=\"" << prm.name << "\"" <<
            " Type=\"ISO-8601\">" << (gdsDatum&) prm << "</" << 
            xmlTime << ">" << endl;
      }
      else {
         // normal parameter
         os << indent(1+prm.level) << "<" << xmlParameter;
         os << " " << xmlName << "=\"" << prm.name << "\"";
         if (prm.datatype != gds_void) {
            os << " " << xmlType << "=\"" << 
               gdsDataTypeName (prm.datatype) << "\"";
         }
         if (prm.datatype == gds_channel) {
            os << " " << xmlUnit << "=\"channel\"";
         }
         else if (prm.unit.size() > 0) {
            os << " " << xmlUnit << "=\"" << prm.unit << "\"";
         }
         if (prm.elNumber() > 1) {
            os << " " << xmlLength << "=\"" << prm.elNumber() << "\"";
         }
         if (prm.comment.size() > 0) {
            os  << " " << xmlComment << "=\"" << prm.comment << "\"";
         }
         os << ">";
         if (prm.datatype != gds_void) {
            os << *(gdsDatum*) &prm;
         }
         os << "</" << xmlParameter << ">" << endl;
      }
      return os;
   }


   ostream& operator << (ostream& os, const gdsDataReference& link)
   {
      os << indent(2) << "<" << xmlLink << ">" << endl;
      os << indent(3) << "<" << xmlOffset << ">" << setw (10) <<
         (link.selfref ? link.selfofs : link.offset) << 
         "</" << xmlOffset << ">" << endl;
      os << indent(3) << "<" << xmlLength << ">" << 
         link.length << "</" << xmlLength << ">" << endl;
      os << indent(3) << "<" << xmlEncoding << ">" << 
         xmlByteOrder() << "</" << xmlEncoding << ">" << endl;
      if (link.selfref || (link.fileref == "")) {
         os << indent(3) << "<" << xmlRef << "/>" << endl;
      }
      else {
         os << indent(3) << "<" << xmlLink << ">" << 
            link.fileref << "</" << xmlLink << ">" << endl;
      }
      os << indent(2) << "</" << xmlLink << ">" << endl;
      return os;
   }


   gdsDataReference& gdsDataReference::operator= (const gdsDataReference& ref)
   {
      if (&ref != this) {
         if (maddr != 0) {
            munmap ((void*) maddr, mlen);
            maddr = 0;
            mlen = 0;
         }
         if (reference && (fileref != "") &&
            gdsStorage::isTempFile (fileref)) {
            gdsStorage::unregisterTempFile (fileref);
         }
         reference = ref.reference;
         selfref = ref.selfref;
         fileref = ref.fileref;
         offset = ref.offset;
         selfofs = ref.selfofs;
         length = ref.length;
         encoding = ref.encoding;
         if (reference && (fileref != "") &&
            gdsStorage::isTempFile (fileref)) {
            gdsStorage::registerTempFile (fileref);
         }
      }
      return *this;
   }


   gdsDataReference::~gdsDataReference ()
   {
      if (maddr != 0) {
         munmap ((void*) maddr, mlen);
         maddr = 0;
         mlen = 0;
      }
      if (reference && (fileref != "") &&
         gdsStorage::isTempFile (fileref)) {
         gdsStorage::unregisterTempFile (fileref);
      }
   }


   bool gdsDataReference::setMapping (gdsDataObject& dat)
   {
      /* nothing to do if not a valid link */
      if (!reference) {
         return true;
      }
      /* mapped file must be at least as long as size of data object */
      if (length < dat.size()) {
         return false;
      };
      /* test if something changed */
      if ((maddr != 0) && (length == mlen)) {
         return true;
      }
      /* remove old mapping */
      if (maddr != 0) {
         munmap ((void*) maddr, mlen);
      }
      /* open file */
      int fd = ::open (fileref.c_str(), O_RDWR);
      if (fd == -1) {
         dat.value = maddr = 0;
         return false;
      }
      /* get its size */
      struct stat info;
      int ret = ::fstat (fd, &info);
      if (ret || (info.st_size < offset + length)) {
         dat.value = maddr = 0;
         return false;
      }
      /* map */
      mlen = length + offset;
      maddr = ::mmap ((void*) maddr, mlen, PROT_READ | PROT_WRITE, 
                     MAP_SHARED, fd, 0);
      // QFS requires the exec flag to be set, so try again      
      if (maddr == MAP_FAILED) {      
         maddr = ::mmap ((void*) maddr, mlen, PROT_READ | PROT_WRITE | 
                        PROT_EXEC, MAP_SHARED, fd, 0);
      }      
      ::close (fd);
      if (maddr == MAP_FAILED) {
         dat.value = maddr = 0;
         return false;
      }
      dat.value = (char*) maddr + offset;
      return true;
   }



   gdsDataObject::~gdsDataObject () 
   {
      if (link.reference) {
         value = 0;
      }
   }


   gdsDataObject& gdsDataObject::operator= (const gdsDataObject& dat) 
   {
      if (this != &dat) {
         gdsNamedDatum::operator= (dat);
         flag = dat.flag;
         xmltype = dat.xmltype;
         link = dat.link;
         error = dat.error;
         for (gdsParameterList::const_iterator iter = dat.parameters.begin();
             iter != dat.parameters.end(); iter++) {
            if (iter->get() != 0) {
               parameters.push_back (gdsParameterPtr (**iter));
            }
         }
      }
      return *this;
   }


   bool gdsDataObject::assignDatum (const gdsDatum& dat)
   {
      // delete link if necessary
      if (isRef()) {
         link = gdsDataReference ();
      }
      encodingtype oldencoding = encoding;
      bool ret = gdsNamedDatum::assignDatum (dat);
      encoding = oldencoding;
      return ret;
   }


   gdsDataObject::objflag gdsDataObject::gdsObjectFlag (const string& ot)
   {
      stringcase otype (ot.c_str());
      string::size_type pos;
      while ((pos = otype.find (" ")) != string::npos) {
         otype.erase (pos, 1);
      }
      if (otype == xmlObjTypeTestParameters) {
         return gdsDataObject::parameterObj;
      }
      else if (otype == xmlObjTypeSettings) {
         return gdsDataObject::settingsObj;
      }
      else if (otype == xmlObjTypeTimeSeries) {
         return gdsDataObject::rawdataObj;
      }
      else if (otype == xmlObjTypeImage) {
         return gdsDataObject::imageObj;
      }
      else {
         return gdsDataObject::resultObj;
      }
   }


   string gdsDataObject::gdsObjectFlagName (gdsDataObject::objflag oflag)
   {
      switch (oflag) {
         case gdsDataObject::parameterObj: 
            {
               return xmlObjTypeTestParameters;
            }
         case gdsDataObject::settingsObj: 
            {
               return xmlObjTypeSettings;
            }
         case gdsDataObject::rawdataObj: 
            {
               return xmlObjTypeTimeSeries;
            }
         case gdsDataObject::imageObj: 
            {
               return xmlObjTypeImage;
            }
         case gdsDataObject::resultObj: 
         default:
            {
               return xmlObjTypeResult;
            }
      }
   }



   istream& operator >> (istream& is, gdsDataObject&)
   {
      return is;
   }


   ostream& operator << (ostream& os, const gdsDataObject& dat)
   {
      // start tag
      os << indent(dat.level) << "<" << xmlContainer;
      os << " " << xmlName << "=\"" << dat.name << "\"";
      if (!dat.xmltype.empty()) {
         os << " " << xmlType << "=\"" << dat.xmltype << "\"";
      }
      os << ">" << endl;
      // flag
      os << indent(1+dat.level) << "<" << xmlParameter << " Name=\"" << 
         xmlFlag << "\" Type=\"" << gdsDataTypeName (gds_string) <<
         "\">" << gdsDataObject::gdsObjectFlagName (dat.getFlag()) << 
         "</" << xmlParameter << ">" << endl;
      // comment
      if (dat.comment.size() > 0) {
         os << indent(1+dat.level) << "<" << xmlComment << ">" << 
            dat.comment << "</" << xmlComment << ">" << endl;
      }
      // list of parameters
      for (gdsDataObject::gdsParameterList::const_iterator 
          iter2 = dat.parameters.begin(); 
          iter2 != dat.parameters.end(); iter2++) {
         if (strcasecmp ((*iter2)->name.c_str(), stObjectType) != 0) {
            os << *(*iter2);
         }
      }
      // data
      if ((dat.elNumber() > 0) && (dat.datatype != gds_void)) {
         os << indent(1+dat.level) << "<" << xmlArray;
         os << " " << xmlType << "=\"" << 
            gdsDataTypeName (dat.datatype) << "\"";
         if (dat.datatype == gds_channel) {
            os << " " << xmlUnit << "=\"channel\"";
         }
         else if (dat.unit.size() > 0) {
            os << " " << xmlUnit << "=\"" << dat.unit << "\"";
         }
         os << ">" << endl;
         for (gdsDatum::dimension_t::const_iterator iter = dat.dimension.begin(); 
             iter != dat.dimension.end(); iter++) {
            os << indent(2+dat.level) << "<" << xmlDimension << ">" <<
               *iter << "</" << xmlDimension << ">" << endl;
         }
         if ((dat.encoding == gdsDatum::binary) && (dat.isRef())) {
            os << dat.link;
         }
         else {
            os << indent(2+dat.level) << "<" << xmlData;
            os << " " << xmlEncoding << "=\"" << xmlByteOrder() << "," <<
               gdsDatum::codeName (dat.encoding) << "\"" ;
            if (dat.encoding == gdsDatum::ascii) {
               os << " " << xmlDelimiter << "=\" \"";
            }
            os << ">" << endl;
            os << (gdsDatum&) dat << endl;
            os << indent(2+dat.level) << "</" << xmlData << ">" << endl;
         }
         os << indent(1+dat.level) << "</" << xmlArray << ">" << endl; 
      }
      // end tag
      os << indent(dat.level) << "</" << xmlContainer << ">" << endl;
      return os;
   }


   const gdsStorage::objflag gdsStorage::ioAll[5] = 
   {parameterObj, settingsObj, resultObj, rawdataObj, imageObj};
   const gdsStorage::ioflags gdsStorage::ioEverything(ioAll, ioAll+5);
   const gdsStorage::ioflags gdsStorage::ioExtended(ioAll, ioAll+4);
   const gdsStorage::ioflags gdsStorage::ioStandard(ioAll, ioAll+3);
   const gdsStorage::ioflags gdsStorage::ioParamOnly(ioAll, ioAll+2);


   gdsStorage::tempnames 	gdsStorage::tempfiles;
   mutex			gdsStorage::tempfilemux;

   gdsStorage::gdsStorage ()
   : gdsDataObject (xmlLigoLW, gds_void, 0, 0),
   initialized (false), activeFlags (ioExtended), activeFiletype (LigoLW_XML)
   
   {
   }


   gdsStorage::~gdsStorage () 
   {      
      // mux.lock();
      // objects.clear();
      // parameters.clear();
      // XML_text.reset();
   }


   gdsStorage::gdsStorage (const string& Creator, const string& Date,
                     const string& Comment)
   : gdsDataObject (xmlLigoLW, gds_void, 0, 0, "", Comment),
   creator (Creator), date (Date), initialized (true),
   activeFlags (ioExtended), activeFiletype (LigoLW_XML)
   
   {
   }


   gdsStorage::gdsStorage (string filename, 
                     ioflags restoreflags, 
                     filetype FileType)
   : gdsDataObject (xmlLigoLW, gds_void, 0, 0),
   initialized (false), activeFlags (ioExtended), activeFiletype (LigoLW_XML)
   {      
      semlock		lockit (mux);
      initialized = frestore (filename, restoreflags, FileType);
   }


   bool gdsStorage::operator ! () const 
   {
      return !initialized;
   }


   istream& operator >> (istream& is, gdsStorage& dat) 
   {
      semlock		lockit (dat.mux);
   
      tempFilename	filename;
      ofstream		out (filename.c_str());
   
      dat.registerTempFile (filename);
      out << is.rdbuf();
      out.close();
      dat.frestore (filename);
      dat.unregisterTempFile (filename);
      return is;
   }


   ostream& operator << (ostream& os, gdsStorage& dat)          
   {
      semlock		lockit (dat.mux);
   
      tempFilename	filename;
   
      if (!dat.fsave (filename)) {
         os.setstate (ios::failbit);
         return os;
      }
   
      ifstream		inp (filename.c_str());
      os << inp.rdbuf();
   
      return os;
   }


   void gdsStorage::fwriteXML (ostream& os)
   {
      // intro
      os << xmlVersion << endl;
      os << xmlDocType << endl;
      os << "<" << xmlContainer <<  
         " Name=\"Diagnostics Test\">"<< endl;
      // header info
      os << indent(1) << "<" << xmlContainer << 
         " Name=\"" << xmlHeader << "\" Type=\"" << getType() << "\">" << endl;
      // object flag
      os << indent(2) << "<" << xmlParameter << " Name=\"" << 
         xmlFlag << "\" Type=\"" << gdsDataTypeName (gds_string) <<
         "\">" << gdsObjectFlagName (getFlag()) << "</" << 
         xmlParameter << ">" << endl;
      // creator
      if (creator.size() > 0) {
         os << indent(2) << "<" << xmlParameter << " Name=\"" << 
            xmlCreator << "\" Type=\"" << gdsDataTypeName (gds_string) <<
            "\">" << creator << "</" << xmlParameter << ">" << endl;
      }
      // date/time
      if (date.size() > 0) {
         os << indent(2) << "<" << xmlTime << " Type=\"ISO-8601\">" << 
            date << "</" << xmlTime << ">" << endl;
      }
      // comment
      if (comment.size() > 0) {
         os << indent(2) << "<" << xmlComment << ">" << comment << 
            "</" << xmlComment << ">" << endl;
      }
      // all other parameters
      if (activeFlags.count (gdsDataObject::parameterObj) > 0) {
         for (gdsDataObject::gdsParameterList::const_iterator iter = 
             parameters.begin(); 
             iter != parameters.end(); iter++) {
            if (strcasecmp ((*iter)->name.c_str(), stObjectType) != 0) {
               os << **iter;
            }
         }
      }
      os << indent(1) << "</" << xmlContainer << ">" << endl;
      /* data objects */
      for (int i = 0; i < 5; i++) {
         objflag type;
         switch (i) {
            case 0: 
               type = parameterObj;
               break;
            case 1: 
               type = settingsObj;
               break;
            case 2: 
               type = resultObj;
               break;
            case 3: 
               type = rawdataObj;
               break;
            case 4: 
               type = imageObj;
               break;
         }
         if (activeFlags.count (type) == 0) {
            continue;
         }
         for (gdsStorage::gdsObjectList::const_iterator 
             iter2 = objects.begin(); 
             iter2 != objects.end(); iter2++) {
            if ((*iter2)->getFlag() == type) {
               os << **iter2;
            }
         }
      }
      /* trailer */
      os << "</" << name << ">" << endl;
   }


   bool gdsStorage::fwriteBinary (ostream& out)       
   {
      bool		err = false;
   
      for (gdsObjectList::iterator iter = objects.begin();
          iter != objects.end(); iter++) {
         if ((!(*iter)->isRef()) || (!(*iter)->link.selfref) ||
            ((*iter)->encoding != binary)) {
            continue;
         }
         out.seekp ((*iter)->link.selfofs);
         if (out.bad()) {
            out.clear();
            out.seekp (0, ios::end);
            int diff = (*iter)->link.selfofs - out.tellp();
            if (diff > 0) {
               out << setw (diff) << ' ';
            }
         }
         if ((*iter)->value == 0) {
            XML_Error = "Binary data unavailable";
            err = true;
            continue;
         }
      
         out.write ((char*) (*iter)->value, (*iter)->link.length);
      	 /* stop if failed */
         if (!out) {
            XML_Error = "Failure while writing binary data";
            return false;
         }
      }
      return !err;
   }


   int gdsStorage::ffixRef (int XML_Length)       
   {
      int		pos = XML_Length;
   
      for (gdsObjectList::iterator iter = objects.begin();
          iter != objects.end(); iter++) {
         if ((*iter)->isRef() && (*iter)->link.selfref &&
            ((*iter)->encoding == binary)) {
            (*iter)->link.selfofs = pos;
            pos += (((*iter)->link.length + XML_BinaryAlign - 1) / 
                   XML_BinaryAlign) * XML_BinaryAlign;
         }
      }
   
      return pos - XML_Length;
   }


   void gdsStorage::startElement (const string& elName, 
                     const attrtype& atts)
   {
      // cerr << endl << "start = " << elName << endl;
      // for (attrtype::const_iterator i = atts.begin(); 
          // i != atts.end(); i++) {
         // cerr << "attribute: " << i->first << " = " << 
            // i->second.c_str() << endl;
      // }
   
      /* skip if nested too deep */
      if ((XML_Skip != 0) || (XML_Key != "")) {
         XML_Skip++;
         return;
      }
      /* skip if before start tag */
      if (!XML_init) {
         if (elName == xmlLigoLW) {
            XML_init = true;
         }
         return;
      }
   
      // Container
      if (elName == xmlContainer) {
         attrtype::const_iterator niter = atts.find (xmlName);
         // header first
         if ((niter != atts.end()) && (niter->second == xmlHeader)) {
            // not on top level: skip
            if ((XML_Obj != 0) || (XML_Param != 0)) {
               XML_Skip++;
               return;
            }
            XML_Obj = this;
         }
         // data object
         else if (niter != atts.end()) {
            // nested too deep: skip
            if ((XML_Obj != 0) || (XML_Param != 0)) {
               XML_Skip++;
               return;
            }
            XML_Obj = new (std::nothrow) 
               gdsDataObject (niter->second.c_str(), gds_void, 0, 0);
            if (XML_Obj == 0) {
               // creation failed: skip rest
               XML_Skip++;
               return;
            }
            attrtype::const_iterator titer = atts.find (xmlType);
            attrtype::const_iterator fiter = atts.find (xmlFlag);
            // compatibility with old style (with flag attribute)
            if (fiter != atts.end()) {
               XML_Obj->setFlag (gdsObjectFlag (fiter->second.c_str()));
               if (titer != atts.end()) {
                  XML_Obj->setType (titer->second.c_str());
                  // gdsParameter* prm = new (std::nothrow) gdsParameter (
                                       // stObjectType, gds_string, 
                                       // titer->second.c_str());  
                  // if (prm) 
                     // XML_Obj->parameters.push_back (gdsParameterPtr (prm));
               }
            }
            // new style (no flag attribute)
            else {
               if (titer != atts.end()) {
                  XML_Obj->setType (titer->second.c_str());
               }
            }
         }
         // skip containers without name
         else {
            XML_Skip++;
            return;
         }
      }
      
      // Parameter & Time
      else if ((elName == xmlParameter) || (elName == xmlTime)) {
         attrtype::const_iterator niter = atts.find (xmlName);
         // nested too deep, placed on top level, no name or ObjectType
         if ((XML_Param != 0) || (XML_Obj == 0) || 
            ((niter == atts.end()) && (elName != xmlTime))) {
            XML_Skip++;
            return;
         }
         attrtype::const_iterator titer = atts.find (xmlType);
         attrtype::const_iterator uiter = atts.find (xmlUnit);
         attrtype::const_iterator citer = atts.find (xmlComment);
         attrtype::const_iterator liter = atts.find (xmlLength);
         gdsDataType 	dtype;	// parameter data type
         string 	u;	// unit string
         int		dim = 1;// parameter length
         string		n;	// name string
      
         if (elName == xmlTime) {
            dtype = ((titer != atts.end()) && (titer->second == "GPS")) ? 
               gds_int64 : gds_string;
            u =  (titer->second == "GPS") ? "ns" : "ISO-8601";
            dim = 1;
            n = (niter == atts.end()) ? xmlTime : niter->second.c_str();
         }
         else {
            dtype = (titer != atts.end()) ?
               gdsNameDataType (titer->second.c_str()) : gds_void;
            u = (uiter != atts.end()) ? uiter->second.c_str() : "";
            if ((dtype != gds_string) && (dtype != gds_void) &&
               (liter != atts.end())) {
               dim = atoi (liter->second.c_str());
               if (dim < 1) {
                  dim = 1;
               }
            }
            n = niter->second.c_str();
         }
         // check for channel name
         if ((dtype == gds_string) && (u == "channel")) {
            dtype = gds_channel;
            u = "";
         }
         string com ((citer != atts.end()) ? 
                    citer->second.c_str() : "");
         XML_Param = new (std::nothrow) gdsParameter (
                              n, dtype, 0, dim, u, com);
         if (XML_Param == 0) {
            // creation failed: skip rest
            XML_Skip++;
            return;
         }
         // add parameter
         XML_Obj->parameters.push_back (gdsParameterPtr (XML_Param));
      }
      
      // Comments
      else if (elName == xmlComment) {
         // no comments in parameters
         if (XML_Param != 0) {
            XML_Skip++;
            return;
         }
         XML_Key = xmlComment;
      }
      
      // parameter specific stuff
      else if (XML_Param != 0) {
         // skip tags within parameters
         XML_Skip++;
      }
      
      // data object specific stuff
      else if (XML_Obj != 0) {
         // Handle array
         if (elName == xmlArray) {
            // skip array in array
            if (XML_Key2 != "") {
               XML_Skip++;
               return;
            }
            // set data type & unit of data object
            attrtype::const_iterator titer = atts.find (xmlType);
            attrtype::const_iterator uiter = atts.find (xmlUnit);
            XML_Obj->datatype = (titer != atts.end()) ?
               gdsNameDataType (titer->second.c_str()) : gds_void;
            XML_Obj->unit = (uiter != atts.end()) ? 
               uiter->second.c_str() : "";
            // check for channel name
            if ((XML_Obj->datatype == gds_string) && 
               (XML_Obj->unit == "channel")) {
               XML_Obj->datatype = gds_channel;
               XML_Obj->unit = "";
            }
            XML_Key2 = xmlArray;
         }
         // handle size of array
         else if (elName == xmlDimension) {
            // skip if not within an array
            if (XML_Key2 != xmlArray) {
               XML_Skip++;
               return;
            }
            XML_Key = xmlDimension;
         }
         // handle data
         else if (elName == xmlData) {
            // skip if not within an array
            if (XML_Key2 != xmlArray) {
               XML_Skip++;
               return;
            }
            // get stream type, encoding & delimiter
            attrtype::const_iterator titer = atts.find (xmlType);
            attrtype::const_iterator eiter = atts.find (xmlEncoding);
            //attrtype::const_iterator diter = atts.find (xmlDelimiter);
            // remote streams not supported
            if ((titer != atts.end()) && (titer->second != "Local")) {
               XML_Skip++;
               return;
            }
            // determine encoding
            if (eiter == atts.end()) {
               XML_Obj->encoding = ascii;
            }
            else {
               if (eiter->second.find ("Binary") != string::npos) {
                  XML_Obj->encoding = binary;
               }
               else if (eiter->second.find ("uuencode") != string::npos) {
                  XML_Obj->encoding = uuencode;
               }
               else if (eiter->second.find ("base64") != string::npos) {
                  XML_Obj->encoding = base64;
               }
               else {
                  XML_Obj->encoding = ascii;
               }
               // skip wrong endian
               if ((eiter->second.find (xmlByteOrderWord[0]) == string::npos) &&
                  (eiter->second.find (xmlByteOrderWord[1]) == string::npos)) {
                  XML_Skip++;
                  return;
               }
               // determine if we need to swap
               if ((littleEndian() && 
                   (eiter->second.find (xmlByteOrderWord[0]) != string::npos)) ||
                  (!littleEndian() && 
                  (eiter->second.find (xmlByteOrderWord[1]) != string::npos))) {
                  XML_Obj->swapit = true;
               }
               else {
                  XML_Obj->swapit = false;
               }
            }
            // only space delimiter supported
            XML_Key = xmlData;
         }
      }
      /* skip all others */
      else {
         XML_Skip++;
      }
   }


   void gdsStorage::endElement (const string& elName)
   {
      //cerr << "stop = " << elName << " skip = " << XML_Skip << endl;
   
      // handle termination & initialization
      if ((elName == xmlLigoLW) && (XML_Obj == 0)) {
         XML_fini = true;
         return;
      }
      if ((!XML_init) || (XML_fini)) {
         return;
      }
   
      // skip unrecognized tags
      if (XML_Skip != 0) {
         XML_Skip--;
         return;
      }
   
      // data object
      if (elName == xmlContainer) {
         if (XML_Obj != this) {
            // make sure predefined objects are of the correct type
            stringcase n = XML_Obj->name.c_str();
            stringcase::size_type pos = n.find ("[");
            if (pos != stringcase::npos) {
               n.erase (pos);
            }
            if (n == stDef) {
               XML_Obj->setFlag (gdsDataObject::parameterObj);
               XML_Obj->setType (stObjectTypeDef);
            }
            else if (n == stSync) {
               XML_Obj->setFlag (gdsDataObject::parameterObj);
               XML_Obj->setType (stObjectTypeSync);
            }
            else if (n == stEnv) {
               XML_Obj->setFlag (gdsDataObject::parameterObj);
               XML_Obj->setType (stObjectTypeEnv);
            }
            else if (n == stScan) {
               XML_Obj->setFlag (gdsDataObject::parameterObj);
               XML_Obj->setType (stObjectTypeScan);
            }
            else if (n == stFind) {
               XML_Obj->setFlag (gdsDataObject::parameterObj);
               XML_Obj->setType (stObjectTypeFind);
            }
            else if (n == stPlot) {
               XML_Obj->setFlag (gdsDataObject::settingsObj);
               XML_Obj->setType (stObjectTypePlot);
            }
            else if (n == stCal) {
               XML_Obj->setFlag (gdsDataObject::settingsObj);
               XML_Obj->setType (stObjectTypeCalibration);
            }
            else if (n == stIndex) {
               XML_Obj->setFlag (gdsDataObject::resultObj);
               XML_Obj->setType (stObjectTypeIndex);
            }
            else if (n == stTestParameter) {
               XML_Obj->setFlag (gdsDataObject::parameterObj);
               XML_Obj->setType (stObjectTypeTestParameter);
            }
            else if (n == stCal) {
               XML_Obj->setFlag (gdsDataObject::settingsObj);
               XML_Obj->setType (stObjectTypeCalibration);
            }
            else if (n == stCal) {
               XML_Obj->setFlag (gdsDataObject::settingsObj);
               XML_Obj->setType (stObjectTypeCalibration);
            }
            // now add object to collection
            if (!addData (*XML_Obj, false)) {
            }
         }
         XML_Obj = 0;
      }
      
      // parameter & time
      else if ((elName == xmlParameter) || (elName == xmlTime)) {
         // treat creator of header specially
         if ((XML_Obj == this) && (XML_Param != 0) && 
            (XML_Param->name == xmlCreator) && 
            (XML_Param->datatype == gds_string) &&
            (XML_Param->value != 0)) {
            creator = (char*) XML_Param->value;
            parameters.pop_back();
         }
         // treat time of header specially
         else if ((XML_Obj == this) && (XML_Param != 0) && 
                 (XML_Param->name == xmlTime) && 
                 (XML_Param->datatype == gds_string) &&
                 (XML_Param->value != 0)) {
            date = (char*) XML_Param->value;
            parameters.pop_back();
         }
         // name is required!
         else if ((XML_Param != 0) && (XML_Param->name == "")) {
            XML_Obj->parameters.pop_back();
         }
         // remove object type
         else if ((XML_Param != 0) && (XML_Param->name == stObjectType)) {
            if ((XML_Param->datatype == gds_string) && 
               (XML_Param->value != 0)) {
               XML_Obj->setType ((const char*)XML_Param->value);
            }
            XML_Obj->parameters.pop_back();
         }
         // remove flag type
         else if ((XML_Param != 0) && (XML_Param->name == xmlFlag)) {
            if ((XML_Param->datatype == gds_string) && 
               (XML_Param->value != 0)) {
               XML_Obj->setFlag ((const char*)XML_Param->value);
            }
            XML_Obj->parameters.pop_back();
         }
         XML_Param = 0;
      }
      else if ((elName == xmlComment) || (elName == xmlDimension) || 
              (elName == xmlData)) {
         XML_Key = "";
      }
      else if (elName == xmlArray) {
         XML_Key2 = "";
         XML_Key = "";
      }
      else {
      }
   }


   void gdsStorage::textHandler (stringstream& text)
   {
      // if (XML_Key != xmlData) {
         // cerr << "text = " << text.str() << endl;
      // }
   
      /* skip if uninteresting */
      if (XML_Skip > 0) {
         return;
      }
   
      // handle parameter
      if (XML_Param != 0) {
         // treat GPS time special
         if ((XML_Param->datatype == gds_int64) && (XML_Param->unit == "ns")) {
            string t = text.str();
            string::size_type pos = t.find ('.');
            if (pos != string::npos) {
               if (t.size() - pos < 10) {
                  t.insert (t.size(), pos + 10 - t.size(), '0');
               }
               else if (t.size() - pos > 10) {
                  t.erase (pos + 10);
               }
               t.erase (pos, 1);
            }
            else if (t.size() < 12) {
               t += "000000000";
            }
            if (XML_Param->readValues (t) < 0) {
               XML_Error = "error reading time value(s)";
            }
         }
         // read in parameter values
         else if (XML_Param->readValues (text.str()) < 0) {
            XML_Error = "error reading parameter value(s)";
         }
      }
      
      // handle data object
      else if (XML_Obj != 0) {
         // comment
         if (XML_Key == xmlComment) {
            XML_Obj->comment = text.str();
         }
         // array dimension
         else if (XML_Key == xmlDimension) {
            int			dim = 0;
            if (text >> dim) {
               XML_Obj->dimension.push_back (dim);
            }
         }
         // read data
         else if (XML_Key == xmlData) {
            switch (XML_Obj->encoding) {
               case ascii:
                  {
                     if (XML_Obj->readValues (text.str()) < 0) {
                        XML_Error = "error reading data value(s)";
                     }
                     break;
                  }
               case uuencode:
               case base64:
                  {
                     if (XML_fast) {
                        // already done
                        break;
                     }
                     // slow decode
                     if (XML_Obj->value != 0) {
                        delete [] (char*)XML_Obj->value;
                     }
                     XML_Obj->value =
                        new (nothrow) char [XML_Obj->size()];
                     if (XML_Obj->value == 0) {
                        XML_Error = "error reading data value(s)";
                     }
                     else {
                        if (!gdsDatum::decode 
                           (text, (char*) XML_Obj->value, 
                           XML_Obj->size(), XML_Obj->encoding)) {
                           XML_Error = "error reading data value(s)";
                        }
                        else if (XML_Obj->swapit) {
                           if (XML_Obj->isComplex()) {
                              swapByteOrder ((char*)XML_Obj->value, 
                                            2*XML_Obj->elNumber(), 
                                            XML_Obj->elSize()/2);
                           }
                           else {
                              swapByteOrder ((char*)XML_Obj->value, 
                                            XML_Obj->elNumber(), 
                                            XML_Obj->elSize());
                           }
                           XML_Obj->swapit = false;
                        }
                     }
                     // try the next one using fast decoding
                     XML_fast = true;
                     break;
                  }
               case binary:
               default:
                  {
                     break;
                  }
            }
         }
      }
   }


   void gdsStorage::startelement (gdsStorage* dat, const char* name, 
                     const char** attributes)
   {
      // empty text buffer
      dat->XML_text.reset (0);
   
      /* reformat attributes */
      attrtype 		atts;
      const char**	a = attributes;
      while ((*a != 0) && (*(a+1) != 0)) {
         atts.insert (attrtype::value_type 
                     (string (*a), stringcase (*(a+1))));
         a += 2;
      }
   
      // call start element handler
      dat->startElement (name, atts);
   }


   void gdsStorage::endelement (gdsStorage* dat, const char* name)
   {
   
      if (dat->XML_text.get() != 0) {
         if (dat->XML_text->good()) {
            dat->textHandler (*dat->XML_text);
         }
         dat->XML_text.reset (0);
      }
      dat->endElement (name);
   }


   void gdsStorage::texthandler (gdsStorage* dat, const char* text,
                     int len)
   {
      //cerr << "TXT = " << text << " l = " << len << endl;
      if (dat->XML_text.get() == 0) {
         dat->XML_text.reset (new (nothrow) stringstream);
         if (dat->XML_text.get() == 0) {
            return;
         }
      }
      if ((dat->XML_text->tellp() > 0) && dat->XML_newline) {
         *(dat->XML_text) << endl;
      }
      dat->XML_text->write (text, len);
      dat->XML_newline = false;
   }


   bool gdsStorage::fsave (string filename, ioflags saveflags, 
                     filetype FileType)
   {
      semlock		lockit (mux);
      int 		XML_Length;
      ofstream 		out (filename.c_str());
   
      XML_Error = "";
      if (!out) {
         XML_Error = "Unable to open output file";
         return false;
      }
   
      /* set active flags */
      activeFlags = saveflags;
      activeFiletype = FileType;
   
      /* first run */
      fwriteXML (out);
      if (!out) {
         out.close();
         remove (filename.c_str());
         XML_Error = "Unable to write XML file";
         return false;
      }
   
      for (int i = 0; i < 5; i++) {
         XML_Length = out.tellp();
         XML_Length = XML_BinaryAlign *
            ((XML_Length + 5 * XML_BinaryAlign) / XML_BinaryAlign);
      
         /* fix self references and make a second run if necessary */
         int XML_BinaryNeed = ffixRef (XML_Length);
         if (XML_BinaryNeed <= 0) {
            return true;
         }
         out.seekp (0);
         fwriteXML (out);
         if (!out) {
            out.close();
            remove (filename.c_str());
            XML_Error = "Unable to write XML file";
            return false;
         }
         if (XML_Length >= out.tellp()) {
            break;
         }
      }
      /* five trials failed */
      if (XML_Length < out.tellp()) {
         out.close();
         remove (filename.c_str());
         XML_Error = "Unable to write XML file";
         return false;
      }
      /* succesfully written XML; now write binaries */
      return fwriteBinary (out);
   }


   bool gdsStorage::frestore (string filename, ioflags restoreflags, 
                     filetype FileType)
   {
      semlock		lockit (mux);
      ifstream 		inp (filename.c_str());
      XML_Error = "";
      if (!inp) {
         XML_Error = "Unable to open input file";
         return false;
      }
   
      const int		max_line = 1024;
      char		line[max_line];
      string 		line2;

      // set active flags
      activeFlags = restoreflags;
      activeFiletype = FileType;
   
      // setup XML parser
      XML_Parser parser = XML_ParserCreate (NULL);
      XML_SetUserData (parser, this);
      XML_SetElementHandler 
         (parser, 
         (XML_StartElementHandler) gdsStorage::startelement, 
         (XML_EndElementHandler) gdsStorage::endelement);
      XML_SetCharacterDataHandler 
         (parser, 
         (XML_CharacterDataHandler) gdsStorage::texthandler);
      XML_init = false;
      XML_fini = false;
      XML_Skip = 0;
      XML_Key = string ("");
      XML_Param = 0;
      XML_Obj = 0;
      XML_fast = true;
      // read through file: line by line
      //while (!XML_fini && (inp.getline (line, max_line))) {
      while (!XML_fini && getline (inp, line2)) {
         // call parser
         //int count = inp.gcount();
         //for (char* p = line + count; p > line; ) 
            //if (!*(--p)) *p = ' ';
         //cerr << "XML = " << line << " count = " << count << endl;
         XML_newline = true;
         //XML_Parse (parser, line, count, false);
         XML_Parse (parser, line2.c_str(), line2.size(), false);
      
         // fast optimization for data objects which are
         // base64 or uu encoded (bypass parser)
         if (XML_fast && (XML_Obj != 0) && (XML_Key == xmlData) &&
            ((XML_Obj->encoding == base64) || 
            (XML_Obj->encoding == uuencode))) {
            //cerr << "XML_Obj->size() = " << XML_Obj->size() << endl;
            int maxsize = 4 * XML_Obj->size() / 3 + 100 + max_line;
            char* code = new (nothrow) char [maxsize]; // stores coded data
            int cur = 0; 		// current position
            int orig = inp.tellg ();	// original file position
            int pos;			// current file position
            int len;			// line length
            const char* table = (XML_Obj->encoding == base64) ?
               itable_base64 : itable_uuencode;
            // read encoded data
            do {
               pos = inp.tellg();
               inp.getline (line, max_line);
               len = strlen (line);
               if ((len > 0) && (line[len-1] == '\r')) { // DOS
                  line[len-1] = 0;
                  --len;
               }
               if (cur + len < maxsize) {
                  strcpy (code + cur, line);
                  cur += len;
               }
               //cerr << "current = " << cur << endl;
            } while (inp && table[line[0]] != '\377');
            // protect against a superficial empty line at the end
            if (inp && (strlen (line) == 0)) {
               pos = inp.tellg();
               inp.getline (line, max_line);
            }
            // now check if this is </stream>
            if (!inp || (strstr (line, xmlData) == 0)) {
               //cerr << "line = " << line << endl;
               //inp.getline (line, max_line);
               //cerr << "next = " << line << endl;
               //inp.getline (line, max_line);
               //cerr << "next = " << line << endl;
               inp.seekg (orig);
               XML_fast = false;
               delete [] code;
               cerr << "FAST FAST FAST FAST READ FAILED FAILED FAILED " << endl;
               continue;
            }
            inp.seekg (pos);
            // new value
            if (XML_Obj->value != 0) {
               delete [] (char*)XML_Obj->value;
            }
            XML_Obj->value =
               new (nothrow) char [XML_Obj->size()];
            if ((XML_Obj->value == 0) || 
               !gdsDatum::decode (code, cur, (char*) XML_Obj->value, 
                                 XML_Obj->size(), XML_Obj->encoding)) {
               // error: do it the slow way
               inp.seekg (orig);
               XML_fast = false;
               cerr << "FAST FAST FAST FAST READ FAILED FAILED FAILED 2" << endl;
            }
            else if (XML_Obj->swapit) {
               if (XML_Obj->isComplex()) {
                  swapByteOrder ((char*)XML_Obj->value, 2*XML_Obj->elNumber(), 
                                XML_Obj->elSize()/2);
               }
               else {
                  swapByteOrder ((char*)XML_Obj->value, XML_Obj->elNumber(), 
                                XML_Obj->elSize());
               }
               XML_Obj->swapit = false;
            }
            delete [] code;
         }
      }
      // free parser
      XML_newline = true;
      XML_Parse (parser, line, 0, true);
      XML_ParserFree (parser);
      if ((XML_Obj != 0) && (XML_Obj != this)) {
         delete XML_Obj;
         XML_Obj = 0;
      }
   
      // update links
      for (gdsObjectList::iterator iter = objects.begin();
          iter != objects.end(); iter++) {
         if (!(*iter)->link.setMapping (**iter)) {
            XML_Error = "Invalid links to binary files";
         }
      }
   
      return true;
   }


   gdsStorage::tempnames::~tempnames ()
   {
      while (size() > 0) {
         unregisterTempFile (front());
      }
   }


   void gdsStorage::registerTempFile (const string& filename)
   {
      semlock		lockit (tempfilemux);
      if (filename != "") {
         tempfiles.push_back (filename);
      }
   }


   void gdsStorage::unregisterTempFile (const string& filename)
   {
      semlock		lockit (tempfilemux);
      if (filename != "") {
      #if !defined(__GNUG__) || __GNUC__ < 3
         int		num = 0;
         count (tempfiles.begin(), tempfiles.end(), filename, num);
      #else
         int num = count (tempfiles.begin(), tempfiles.end(), filename);
      #endif
         if (num == 0) {
            return;
         }
         if (num == 1) {
            /* remove file if last reference */
            remove (filename.c_str());
         }
      	 /* remove entry */
         tempnames::iterator iter 
            (find (tempfiles.begin(), tempfiles.end(), filename));
         if (iter != tempfiles.end()) {
            tempfiles.erase (iter);
         }
      }
   }


   bool gdsStorage::isTempFile (const string& filename)
   {
      semlock		lockit (tempfilemux);
      tempnames::const_iterator iter = 
         find (tempfiles.begin(), tempfiles.end(), filename);
      return (iter != tempfiles.end());
   }


   bool gdsStorage::addParameter (const string& objname, 
                     gdsParameter& prm, bool copy)
   {
      semlock		lockit (mux);
      gdsDataObject* obj = findData (objname);
      if (obj == 0) {
         return 0;
      }
      gdsParameterPtr	ptr (0);
      if (copy) {
         ptr = gdsParameterPtr (prm);
      }
      else {
         ptr = gdsParameterPtr (&prm);
      }
      if (ptr.get() == 0) {
         return false;
      }
      obj->parameters.push_back (ptr);
      return true;
   }


   bool gdsStorage::addParameter (gdsParameter& prm, bool copy)
   {
      semlock		lockit (mux);
      return addParameter ("", prm, copy);
   }


   bool gdsStorage::addData (gdsDataObject& dat, bool copy)
   {
      semlock		lockit (mux);
   
      gdsDataObjectPtr	pdata = 
         copy ? gdsDataObjectPtr (dat) : gdsDataObjectPtr (&dat);
      data_iterator 	iter = 
         lower_bound (objects.begin(), objects.end(), pdata);
      objects.insert (iter, pdata);
      return true;
   }


   bool gdsStorage::eraseParameter (const string& objname, 
                     const string& prmname) 
   {
      semlock		lockit (mux);
      gdsDataObject* obj = findData (objname);
      if (obj == 0) {
         return 0;
      }
      prm_iterator iter2 = 
         find (obj->parameters.begin(), obj->parameters.end(), prmname);
      if (iter2 == obj->parameters.end()) {
         return false;
      }
      else {
         obj->parameters.erase (iter2);
         return true;
      }
   }


   bool gdsStorage::eraseParameter (const string& prmname)
   {
      semlock		lockit (mux);
      return eraseParameter ("", prmname);
   }


   bool gdsStorage::eraseData (const string& objname)
   {
      semlock		lockit (mux);
   
      //log << "eraseDataA " << objname << endl;
      if (objname == "") {
         return false;
      }
      data_iterator 	iter = 
         lower_bound (objects.begin(), objects.end(), objname);
      if ((iter == objects.end()) || !(*iter == objname)) {
         return false;
      }
      else {
         //log << "ERASE OBJS " << objname << " " <<
            //(*iter)->size() << " @ " << (*iter)->value << endl;
         objects.erase (iter);
         //log << "ERASE OBJS 2 " << endl;
         return true;
      }
   }


   gdsParameter* gdsStorage::findParameter (const string& objname, 
                     const string& prmname) const
   {
      semlock		lockit (mux);
      const gdsDataObject* obj = findData (objname);
      if (obj == 0) {
         return 0;
      }
      const_prm_iterator iter2 = 
         find (obj->parameters.begin(), obj->parameters.end(), prmname);
      if (iter2 == obj->parameters.end()) {
         return 0;
      }
      else {
         return &(**iter2);
      }
   }


   gdsParameter* gdsStorage::findParameter (const string& prmname) const
   {
      semlock		lockit (mux);
      return findParameter ("", prmname);
   }


   gdsDataObject* gdsStorage::findData (const string& objname) const
   {
      semlock		lockit (mux);
   
      if (objname == "") {
         return (gdsDataObject*) this;
      }
      const_data_iterator iter = 
         lower_bound (objects.begin(), objects.end(), objname);
      if ((iter == objects.end()) || !(*iter == objname)) {
         return 0;
      }
      else {
         return &(**iter);
      }
   }


   gdsDataObject* gdsStorage::newChannel (const string& objname, 
                     tainsec_t start, double dt, bool cmplx,
                     bool memmap)
   {
      if (findData (objname) != 0) {
         return 0;
      }
      /* create data object */
      gdsDataObject dat (objname, cmplx ? gds_complex32 : gds_float32,
                        0, "", "raw time series", rawdataObj);
      dat.dimension[0] = 0;
   
      /* add parameters */
      dat.setType ("TimeSeries");
      gdsParameter 	prm;
      // prm = gdsParameter ("ObjectType", "TimeSeries");
      // dat.parameters.push_back (gdsParameterPtr (prm));
      prm = gdsParameter ("Subtype", cmplx ? (int) 1 : (int) 0);
      dat.parameters.push_back (gdsParameterPtr (prm));
      prm = gdsParameter ("t0", start, "ns");
      dat.parameters.push_back (gdsParameterPtr (prm));
      prm = gdsParameter ("dt", dt, "s");
      dat.parameters.push_back (gdsParameterPtr (prm));
   
      /* set temp file reference */
      tempFilename 	filename;
      if (memmap) {
         ofstream 	    	out (filename.c_str());
         if (!out) {
            return 0;
         }
         out << '\0';
         out.close();
         registerTempFile (filename);
         gdsDataReference 	ref (filename);
         dat.link = ref;
      }
   
      /* add data object to storage list */
      if (!addData (dat)) {
         if (memmap) {
            unregisterTempFile (filename);
         }
         return 0;
      }
      return findData (objname);
   }


   float* gdsStorage::allocateChannelMem (const string& objname, 
                     int length)
   {
      // lock data object for write
      gdsDataObject*	dat = lockData (objname, true);
      if (dat == 0) {
         return 0;
      }
      // make checks
      if ((dat->getFlag() != rawdataObj) || (dat->error) ||
         (dat->dimension.size() != 1) || (length < 0)) {
         unlockData (dat);
         return 0;
      }
   
      // check if memory mapped files are used
      int len = length * dat->elSize();
      bool memmap = dat->isRef();
      if (memmap) {
         ofstream	out (dat->link.fileref.c_str(), ios::app);
         if (!out) {
            unlockData (dat);
            return 0;
         }
         // allocate memory
         char*	buf = new (nothrow) char[len];
         if (buf == 0) {
            unlockData (dat);
            return 0;
         }
         out.write (buf, len);
         if (!out) {
            delete [] buf;
            unlockData (dat);
            return 0;
         } 
         out.close();
         delete [] buf;
      
         // set memory mapping
         dat->link.length += len;
         dat->dimension[0] += length;
         if (!dat->link.setMapping (*dat)) {
            unlockData (dat);
            return 0;
         }
      }
      else { // !memmap
         char* buf = new (nothrow) char[dat->size() + len];
         // log << "data buffer length = " << dat->size() + len << 
            // " @ " << (void*)buf << endl;
         if (buf == 0) {
            unlockData (dat);
            return 0;
         }
         if (dat->value != 0) {
            memcpy (buf, dat->value, dat->size());
            // log << "free buffer length = " << dat->size() << 
               // " @ " << (void*)dat->value << endl;
            delete [] (char*)dat->value;
         }
         dat->value = buf;
         dat->dimension[0] += length;
      }
   
      // return without unlock!
      return (float*) ((char*) dat->value + dat->size() - len);
   }


   void gdsStorage::notifyChannelMem (const string& objname, bool Error)
   {
      /* set error flag */
      gdsDataObject*	dat = findData (objname);
      if (dat == 0) {
         return;
      }
      if (Error) {
         dat->error = true;
      }
   
      /* unlock data object */
      unlockData (dat);
   }


   gdsDataObject* gdsStorage::lockData (const string& objname, bool write)
   {
      gdsDataObject* 	dat = findData (objname);
      if (dat == 0) {
         return 0;
      }
      dat->lock (write);
      return dat;
   }


   gdsDataObject* gdsStorage::trylockData (const string& objname, 
                     bool write)
   {
      gdsDataObject* 	dat = findData (objname);
      if (dat == 0) {
         return 0;
      }
      if (dat->trylock (write)) {
         return dat;
      }
      else {
         return 0;
      }
   }


   void gdsStorage::unlockData (gdsDataObject* dat)
   {
      if (dat != 0) {
         dat->unlock();
      }
   }

}
