#include "debug.h"
#include "myframereadplan.hh"
#include "framecpp/Version6/FrAdcData.hh"

// Constructor reads the table of contents from the
// template frame.
//   
//!exc: std::bad_alloc - Out of memory.
//!exc: read_failure - Read failed.
//      
myFrameReadPlan::myFrameReadPlan( std::istream& in )
  : IFrameStream(in), frame(), raw_data("")
{
  // Create required frame structures in the skeleton frame
  frame.setRawData (raw_data);

  // Read last 8 bytes in for future comparison
  in.seekg( -8, std::ios::end );
  in.read(toc_offset, 8);
}

//!exc: None.   
myFrameReadPlan::~myFrameReadPlan() {}

bool
myFrameReadPlan::can_be_used_for_frame(char *file_name)
{
  mm_istream in(file_name);
  if (! in ) {
     system_log(3, "%s: frame file open failed", file_name);
     return false;
  }

  // Read last 8 bytes in for future comparison
  in.seekg( -8, std::ios::end );
  char new_toc_offset[8];
  in.read(new_toc_offset, 8);
  return !memcmp(toc_offset, new_toc_offset, 8);
}

#if 0
//   
//!exc: std::bad_alloc - Out of memory.
//!exc: read_failure - Read failed.
//!exc: not_found_error - Data is not found.   
//         
void
myFrameReadPlan::daqTriggerADC( const std::vector<std::string> &adcNames )
{
  // Goes through the adcs and adds them into the skeleton frame based on the TOC information.
 
  // Cleanup first
  FrameCPP::Version_6::FrRawData::AdcDataContainer& adc( frame.getRawData()->refAdc() );
  adc.erase( adc.begin(), adc.end() );

  for ( std::vector<std::string>::const_iterator i = adcNames.begin(); i != adcNames.end(); i++) {
    // Read ADC data structure and append to the skeletal frame
    FrameCPP::Version_6::FrAdcData *adcptr = ReadFrAdcData(0, *i );
    if ( adcptr == 0 ) {
	throw not_found_error( "bad ADC name" );
    }
    adc.append ( adcptr, false, true ); // Do not allocate, container will own data
  }
}
#endif

//   
//!exc: cannot_update 
//!exc: read_failure - Read failed.
//         
FrameCPP::Version_6::FrameH&
myFrameReadPlan::readFrame( INT_4U frameNumber, const std::vector<std::string> &adcNames )
{
  FrameCPP::Version_6::FrRawData::AdcDataContainer& adc( frame.getRawData()->refAdc() );
  //printf("adc.begin()=%0x; adc.end()=%0x\n", adc.begin(), adc.end());
  adc.erase( adc.begin(), adc.end() );

  for ( std::vector<std::string>::const_iterator i = adcNames.begin(); i != adcNames.end(); i++) {
    // Read ADC data structure and append to the skeletal frame
    FrameCPP::Version_6::FrAdcData *adcptr = ReadFrAdcData(frameNumber, *i );
    if ( adcptr == 0 ) {
	system_log(1,"bad ADC name `%s'", i->c_str());
	throw not_found_error( "bad ADC name" );
    }
    adc.append ( adcptr, false, true ); // Don't allocate copy; Container will own data
  }

  // Only works on the first frame file; for further files the frame header values are not read from disk
  frame.SetDt(GetTOC()->GetDt()[frameNumber]);
  // Dig the kludge
  *(const_cast<FrameCPP::Version_6::GPSTime *>(&(frame.getGTime()))) =
    FrameCPP::Version_6::GPSTime(GetTOC()->GetGTimeS()[frameNumber],
				 GetTOC()->GetGTimeN()[frameNumber]);

#if 0
  std::istream* mStreamSaved = mStream;
  mStream = &in;
  try {    
    if ( frameNumber >= TOC::frame.size() )
      throw read_failure( "Invalid frame number", BADSTREAM );

    INT_8U frame_offset = TOC::position[ frameNumber ] + 8;

    // update frame header attributes
    mStream->seekg ( frame_offset );
    std::string newFrameName;
    INT_4S newFrameRun;
    INT_4U newFrameNumber;

    (*this) >> newFrameName >> newFrameRun >> newFrameNumber;

    // check run number, it must be the same as in the template frame
    if ( TOC::runs[0] != newFrameRun)
      throw cannot_update ( "Run number changed" );

    frame.mName = newFrameName;
    frame.mFrame = newFrameNumber;
    frame.mRun = newFrameRun;
    if (mFileHeader->getDataFormatVersion() >= 4)
      (*this) >> frame.mDataQuality;
    (*this) >> frame.mGTime >> frame.mULeapS;
    if (mFileHeader->getDataFormatVersion() < 5) {	
      INT_4S localTime;
      (*this) >> localTime;
    }
    (*this) >> frame.mDt;
  
    // update active ADCs -- all the attributes that can change and the data
    //
    FrameCPP::Version_3_4_5::RawData::AdcDataContainer& adc( frame.getRawData () -> refAdc() );
    for ( unsigned i = 0; i < adcDataOffsetVector.size(); i++ ) {
      INT_4U adc_offset = adcDataOffsetVector[ i ][ frameNumber ];
      int NL = (adc_offset + 8) + daqNameLength (adc_offset + 8) + 2;
      mStream->seekg ((NL + daqNameLength (NL) + 2) + 12);
      (*this) >> adc[i]->mBias >> adc[i]->mSlope
	      >> adc[i]->mUnits >> adc[i]->mSampleRate
	      >> adc[i]->mTimeOffset >> adc[i]->mFShift
	      >> adc[i]->mDataValid;

      // Set data byteswap flag to false to avoid swapping bytes in getData()
      // on data that gets written over by next read().
      //
      FrameCPP::Version_3_4_5::Vect &vect = *adc[i]->refData()[0];
      vect.setDataNeedsByteSwap(false);

      // Skip to the start of std::vector
      mStream->seekg ( adc_offset ); // beginning of the ADC structure
      INT_4U length;   // length of structure in bytes
      (*this) >> length; // ADC structure length

      // Skip to the end of AdcData structure
      mStream->seekg ( adc_offset + length ); 

      // Need to skip all SH and SE structures
      for(;;) {
       INT_2U classId;
       INT_4U len;
         // Read next structure header and see if it is a vector
         (*this) >> len >> classId;
       if ( classId == ID_VECT ) {
          // skip two bytes of the instance number 
          // to the beginnig of the vector structure data
            mStream->seekg( 2, std::ios::cur );
          break;
       }
       // skip whole structure minus 6 bytes we just read
         mStream->seekg( len - 6, std::ios::cur );
      }

      (*this) >> vect.mName >> vect.mCompress >> vect.mType >> vect.mNData >> vect.mNBytes;

      // :FIXME: this fails for STRING data type
      mStream->read( (char*) vect.getData(),
		     vect.getNData() * Vect::getTypeSize( vect.getType() ));    

      // Set byteswap flag to whatever reference frame header dictates
      //
      vect.setDataNeedsByteSwap( mFileHeader->byteSwapNeeded() );
    }
  } catch ( ... ) {
    mStream = mStreamSaved;
    throw;
  }
  mStream = mStreamSaved;

#endif // 0


  return frame;
}
