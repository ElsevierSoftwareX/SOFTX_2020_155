#ifndef myImageFrameWriterHH
#define myImageFrameWriterHH

#include <strstream>
#include <framecpp/Version6/OFrameStream.hh>
#include <framecpp/CheckSum.hh>
#include <framecpp/Version6/FrCommon.hh>
#include <framecpp/Version6/FrAdcData.hh>
#include <framecpp/Version6/FrVect.hh>

//: Creates frame image in user supplied output string stream `s'.
// Remembers offsets into `s.str()' for every structure instance
// written out by the means of inheriting TOC writer class and
// ultimately TOC class.
// Provides accessors and mutators for the frame file image.
//
class myImageFrameWriter {
public:
  //: frame file image created in this string stream.
  std::strstream &ost;
  //: offset to the beginning of the Frame Header structure.
  INT_8U frameh_offset;
  //: offset to the beginning of the TOC structure.
  INT_8U toc_offset;
  //: offset to the beginning of the End Of Frame structure.
  INT_8U eof_offset;

  FrameCPP::Version_6::OFrameStream oframestream;

  class myCB_t: public FrameCPP::Version_6::Dictionary::Callback
  {
  public:
    myCB_t( FrameCPP::Version_6::OFrameStream& Stream, myImageFrameWriter& image )
      : m_stream( Stream ),
        m_image( image )
    {
    }
 
    void operator()( FrameCPP::Version_6::FrBase& Base )
    {
      switch( Base.GetClassId( ) )
      {
      case FrameCPP::Version_6::CLASS_FRAME_H:
        m_image.frameh_offset = m_stream.tellp( );
        break;
      case FrameCPP::Version_6::CLASS_FR_END_OF_FRAME:
	m_image.eof_offset = m_stream.tellp( );
        break;
      case FrameCPP::Version_6::CLASS_FR_TOC:
        m_image.toc_offset = m_stream.tellp( );
	break;
      case FrameCPP::Version_6::CLASS_FR_VECT:
	m_image.vectInstanceOffsetMap[Base.GetInstance()].push_back(m_stream.tellp ());
	break;
      case FrameCPP::Version_6::CLASS_FR_ADC_DATA:
	FrameCPP::Version_6::FrAdcData &adc = *((FrameCPP::Version_6::FrAdcData* )&Base);
	std::vector<INT_8U> pos;
	std::vector<INT_4U> chnumgrp;
	pos.push_back(m_stream.tellp());
	chnumgrp.push_back(0);
	chnumgrp.push_back(0);
	m_image.adcNamePositionMap [adc.getName()] =
	  std::pair<std::vector<INT_8U>, std::vector<INT_4U> >(pos, chnumgrp);
#if 0
	cerr << adc.getName() << "\t->\t" << m_stream.tellp() << endl;
#endif
	break;
      }
    }

  private:
    FrameCPP::Version_6::OFrameStream&       m_stream;
    INT_8U              m_toc_pos;
    INT_4U              m_frame_count;
    myImageFrameWriter& m_image;
  };

public:
  friend class FrameCPP::Version_6::OFrameStream;

  //!exc: write_failure - Write failed. 
  myImageFrameWriter (std::strstream &s)
    : ost(s), toc_offset(0), oframestream(s, FrameCPP::CheckSum::NONE, "IGWD")
    {
       myCB_t* mycb(new myCB_t(oframestream, *this));
       oframestream.m_callback_methods.push_back(mycb);
       oframestream.m_active_callbacks.push_back(oframestream.AddCallback(FrameCPP::Version_6::CLASS_FR_VECT, *mycb));
       oframestream.m_active_callbacks.push_back(oframestream.AddCallback(FrameCPP::Version_6::CLASS_FR_ADC_DATA, *mycb));
       oframestream.m_active_callbacks.push_back(oframestream.AddCallback(FrameCPP::Version_6::CLASS_FR_TOC, *mycb));
       oframestream.m_active_callbacks.push_back(oframestream.AddCallback(FrameCPP::Version_6::CLASS_FRAME_H, *mycb));
       oframestream.m_active_callbacks.push_back(oframestream.AddCallback(FrameCPP::Version_6::CLASS_FR_END_OF_FRAME, *mycb));
    }
  //!exc: write_failure - Write failed.    
  ~myImageFrameWriter() {}

public:
  void writeTOC() {
    oframestream.m_toc.FrBase::Write( oframestream );
    oframestream.m_toc.Write( oframestream );

#if 0
    Output::mClosed = false;
    (const_cast<std::vector<INT_8U>&>(getPositionEOF())).clear();
    // Write table of contents
    TOC::write ( *this );
#endif
  }

  //: Write new frame into the string stream
  void writeFrame(FrameCPP::Version_6::FrameH &frame) {
    oframestream.WriteFrame(frame, FrameCPP::CheckSum::NONE);
  };

  //: Finish writing a frame
  void close() {
    oframestream.close();
  };

  //: Vector instance offset map
  // Mapping works like this:
  // instanceId -> vector
  //
  // instanceId is ++ for every newly output Vect structure.
  // Vector has the element for each frame written.
  // Each vector element is an offset to the start of Vect structure
  // in the frame.
  // The offset is measured in `ost->str()'.
  
  std::map<INT_4U, std::vector<INT_4U> > vectInstanceOffsetMap;
  typedef std::map<INT_4U, std::vector<INT_4U> >::const_iterator OMI;

  //: Adc Data
  //
  // Maps an ADC channel name on a std::pair of std::vectors.
  // First std::vector is of FrAdcData structure positions.
  // First std::vectors's length equals to the number of frames in
  // the frame file. If an ADC channel is not present in a frame,
  // corresponding position std::vector element is 0.
  // Second std::vector in a std::pair is a _std::pair_ (a std::vector with 2 elements here)
  // of IDs. First element is channel ID. Second element is channel group ID.
  //
  typedef std::hash_map< std::string, std::pair< std::vector<INT_8U>, std::vector<INT_4U> > > adcNamePositionMap_t;
  typedef adcNamePositionMap_t::const_iterator ANPI;
  typedef adcNamePositionMap_t::iterator ANPINC;
  adcNamePositionMap_t adcNamePositionMap;

public:
  //: Get the table of contents offset.
  inline const INT_8U getTOCOffset() const { return toc_offset; }

  //: Overrides superclass. Called from the superclass.
  //!exc: write_failure - Write failed.    
  inline void startRecord (INT_2U classId, INT_4U length,
			   const FrameCPP::Version_3_4_5::Base* b)
  {
#if 0
      INT_2U instance = getInstance(b);
      FrameWriterTOC::startRecord (classId, length, b); // call the superclass
      if (classId == ID_ENDOFFRAME)
        EOFposition.push_back(ost.pcount ());
      else if (classId == ID_TOC)
        toc_offset = ost.pcount();
      else if (classId == ID_VECT) {
	vectInstanceOffsetMap[instance].push_back(ost.pcount ());
      }
#endif
  }

  /* ADC structure access */
  //: Get ADC name string using the offset.
  inline char *adcName (int offset) {
    return name (offset);
  }
  //: Get ADC comment using offset.
  inline char *adcComment (int offset) {
    return name (offset + nameLength (offset) + 2);
  }

/* skip over the `name' and the `comment' strings */
#define ADC_NAME_COMMENT_OFFSET_DECL(offset) \
  int NL = offset + nameLength (offset) + 2
#define ADC_NAME_COMMENT_OFFSET (NL + nameLength (NL) + 2)

  //: Get ADC channel group number by offset.
  inline INT_4U adcChannelGroup (int offset) {
    ADC_NAME_COMMENT_OFFSET_DECL(offset);
    ost.seekg (ADC_NAME_COMMENT_OFFSET);
    INT_4U crate; ost.read ((char*)&crate, 4);
    return crate;
  }
  //: Get ADC channel number by offset.
  inline INT_4U adcChannelNumber (int offset) {
    ADC_NAME_COMMENT_OFFSET_DECL(offset);
    ost.seekg (ADC_NAME_COMMENT_OFFSET + 4);
    INT_4U cnum; ost.read ((char*)&cnum, 4);
    return cnum;
  }
  //: Get ADC NBits field by offset.
  inline INT_4U adcNBits (int offset) {
    ADC_NAME_COMMENT_OFFSET_DECL(offset);
    ost.seekg (ADC_NAME_COMMENT_OFFSET + 8);
    INT_4U nbits; ost.read ((char*)&nbits, 4);
    return nbits;
  }
  //: Get ADC Bias by offset.
  inline REAL_4 adcBias (int offset) {
    ADC_NAME_COMMENT_OFFSET_DECL(offset);
    ost.seekg (ADC_NAME_COMMENT_OFFSET + 12);
    REAL_4 bias; ost.read ((char*)&bias, 4);
    return bias;
  }
  //: Get ADC Slope by offset.
  inline REAL_4 adcSlope (int offset) {
    ADC_NAME_COMMENT_OFFSET_DECL(offset);
    ost.seekg (ADC_NAME_COMMENT_OFFSET + 16);
    REAL_4 slope; ost.read ((char*)&slope, 4);
    return slope;
  }
  //: Get ADC Units string by offset.
  inline char *adcUnits (int offset) {
    ADC_NAME_COMMENT_OFFSET_DECL(offset);
    return name (ADC_NAME_COMMENT_OFFSET + 20);
  }

#undef ADC_NAME_COMMENT_OFFSET_DECL
#undef ADC_NAME_COMMENT_OFFSET

/* These macros allow backwards movement from the ADC struct end */
#define ADC_OFFSET_HEADER_END_DECL(offset) \
  INT_8U headerLength; \
  ost.seekg (offset); /* move to `header length' */ \
  ost.read ((char*)&headerLength, 8); /* get the length */
#define ADC_OFFSET_HEADER_END (offset + headerLength)

#if 0
  //: Get ADC sampling rate by offset.
  inline REAL_8 adcSampleRate (int offset) {
    ADC_OFFSET_HEADER_END_DECL(offset);
    ost.seekg (ADC_OFFSET_HEADER_END - 12 - 26);
    REAL_8 a; ost.read ((char*)&a, 8);
    return a;
  }
  //: Get ADC Time Offset seconds by offset.
  inline INT_4U adcTimeOffsetS (int offset) {
    ADC_OFFSET_HEADER_END_DECL(offset);
    ost.seekg (ADC_OFFSET_HEADER_END - 12 - 18);
    INT_4U a; ost.read ((char*)&a, 4);
    return a;
  }
  //: Get ADC Time offset nanoseconds by offset.
  inline INT_4U adcTimeOffsetN (int offset) {
    ADC_OFFSET_HEADER_END_DECL(offset);
    ost.seekg (ADC_OFFSET_HEADER_END - 12 - 14);
    INT_4U a; ost.read ((char*)&a, 4);
    return a;
  }
  //: Get ADC Frequency Shift by offset.
  inline REAL_8 adcFShift (int offset) {
    ADC_OFFSET_HEADER_END_DECL(offset);
    ost.seekg (ADC_OFFSET_HEADER_END - 12 - 10);
    REAL_8 fs; ost.read ((char*)&fs, 8);
    return fs;
  }
#endif

  //: Get ADC Data Valid flag by offset.
  inline INT_2U adcDataValid (int offset) {
    ADC_OFFSET_HEADER_END_DECL(offset);
    ost.seekg (ADC_OFFSET_HEADER_END - 18 - 2);
    INT_2U ovr; ost.read ((char*)&ovr, 2);
    return ovr;
  }
  //: Get pointer to the Data Valid flag.
  // Deferencing this pointer could produce SIGBUS.
  inline INT_2U* adcDataValidPtr (int offset) {
    ADC_OFFSET_HEADER_END_DECL(offset);
    return (INT_2U*)(ost.str() + ADC_OFFSET_HEADER_END - 18 - 2);
  }

  //: Given the offset find the ADC data vector ptr.
  inline INT_4U adcDataVectorPtr (int offset) {
    ADC_OFFSET_HEADER_END_DECL(offset);
    // std::cerr << "adc structure length is " << std::hex << headerLength << std::dec << std::endl;
    ost.seekg (ADC_OFFSET_HEADER_END - 18 + 2); // adjust to data vector instance
    INT_4U classInstance;
    ost.read ((char*)&classInstance, 4); // read the vec ptr in
    return classInstance;
  }

  //: Given the offset find the AUX data vector ptr.
  inline INT_4U adcAuxVectorPtr (int offset) {
    ADC_OFFSET_HEADER_END_DECL(offset);
    ost.seekg (ADC_OFFSET_HEADER_END - 12 + 2); // adjust to aux vector instance
    INT_4U classInstance;
    ost.read ((char*)&classInstance, 4); // read the vec ptr in
    return classInstance;
  }
#undef ADC_OFFSET_HEADER_END_DECL
#undef ADC_OFFSET_HEADER_END

  /* Vector structure accessors */
  //: Given the offset to the start of the vector structure,
  //: move the input strstream position to the beginning of the `data'
  inline void seekVectorData (int offset) {
    ost.seekg (offset + nameLength (offset) + 2 + 12);
  }

  //: Get a pointer to the vector's data start by offset.
  inline char *vectorData (int offset) {
    return ost.str () + offset + 14 + nameLength (offset + 14) + 2 + 20;
  }

  /* Frame header mutators */

  //: Set frame file attributes in all frames.
  inline void setFrameFileAttributes(INT_4S run, INT_4U frameNumber,
				     INT_4U dqual, INT_4U gps, INT_2U gpsInc,
				     INT_4U gpsn,
				     INT_2U leapS, INT_4S localTime)
  {
    ost.seekp (frameh_offset + 14 + nameLength (frameh_offset + 14) + 2);
    ost.write ((char*)&run, 4);
    ost.write ((char*)&frameNumber, 4);
    ost.write ((char*)&dqual, 4);
    INT_4U gpsl = 0*gpsInc + gps;
    ost.write ((char*)&gpsl, 4);
    ost.write ((char*)&gpsn, 4);
    ost.write ((char*)&leapS, 2);

    ost.seekp (eof_offset+14);
    ost.write ((char*)&run, 4);
    ost.write ((char*)&frameNumber, 4);

    // Go to the TOC and update the values.
    ost.seekp (toc_offset+14);
    ost.write ((char*)&leapS, 2);
    ost.seekp (toc_offset+14+ 2 + 4);
    ost.write ((char*)&dqual, 4);
    ost.write ((char*)&gpsl, 4);
    ost.write ((char*)&gpsn, 4);
    ost.seekp (toc_offset+14 + 2 + 4 + 4 + 4 + 4 + 8);
    ost.write ((char*)&run, 4);
    ost.write ((char*)&frameNumber, 4);
  }

  /*
   Both nameLength() and name() can be used in all
   structures that had `name' as the third element/
   The list of such strucutres include:
     FrSH, FrSE, Frame, FrADCData, FrDetector, FrHistory,
     FrRawData, FrProcData, FrSerData, FrStatData, FrVect,
     FrEvent, FrSummary.
  */

  inline int nameLength (int offset) {
    ost.seekg (offset);
    INT_2U nameLength;
    ost.read ((char*)&nameLength, 2);
    return nameLength;
  }

  inline char *name (int offset) {
    return ost.str () + offset + 2;
  }

};
#endif
