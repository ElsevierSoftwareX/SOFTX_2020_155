#include <arpa/inet.h>
#include <istream>
#include "debug.h"
#include "plan.hh"
#include "framecpp/Common/FrEndOfFile.hh"

// Constructor reads the table of contents from the
// template frame.
//   
//!exc: std::bad_alloc - Out of memory.
//!exc: read_failure - Read failed.
//      
plan::plan( buffer_type* Stream )
  : IFrameStream(Stream), frame(), raw_data(""), master_plan(0)
{
  // Create required frame structures in the skeleton frame
  // FIXME: frame.setRawData (raw_data);

  // Read last 8 bytes in for future comparison
  //in.seekg( -8, std::ios::end );
  //in.read(toc_offset, 8);
}

plan::plan( buffer_type* Stream, plan *master )
  : IFrameStream(Stream), frame(), raw_data(""), master_plan(master)
{
  // Create required frame structures in the skeleton frame
  // FIXME: frame.setRawData (raw_data);

  // Read last 8 bytes in for future comparison
  //in.seekg( -8, std::ios::end );
  //in.read(toc_offset, 8);
}
//!exc: None.   
plan::~plan() {}

using namespace FrameCPP::Common;

    void IFrameStream::
    load_toc( )
    {
      if ( m_toc_loaded )
      {
        return;
      }
      //-----------------------------------------------------------------
      // One chance to do this
      //-----------------------------------------------------------------
      m_toc_loaded = true;
      //-----------------------------------------------------------------
      // See if TOC is supported by stream frame spec
      //-----------------------------------------------------------------
      try
      {
        if ( frameSpecInfo( ).
             FrameObject( FrameSpec::Info::FSI_FR_TOC )
             == ( FrameSpec::Object*)NULL )
        {
          return;
        }
      }
      catch( const std::range_error& e )
      {
        //---------------------------------------------------------------
        // The frame spec does not support table of contents so just
        // return to the caller.
        //---------------------------------------------------------------
        return;
      }
      //-----------------------------------------------------------------
      // Record the current position within the stream
      //-----------------------------------------------------------------
      const std::streampos here( tellg( ) );

      try
      {
        //---------------------------------------------------------------
        // :TODO: Determine if the stream is rewindable
        //---------------------------------------------------------------
        //---------------------------------------------------------------
        // Get the number of bytes for the FrEndOfFile structure
        //---------------------------------------------------------------
        const std::streampos
          offset( frameSpecInfo( ).FrameObject( FrameSpec::Info::FSI_FR_END_OF_FILE )->Bytes( *this )
                  + frameSpecInfo( ).FrameObject( FrameSpec::Info::FSI_COMMON_ELEMENTS )->Bytes( *this ) );

        seekg( -offset, std::ios_base::end ); // Seek to the beginning of struct
        //---------------------------------------------------------------
        // Read the common elements.
        //---------------------------------------------------------------
        std::auto_ptr< FrameSpec::Object >
          cmn_obj( frameSpecInfo( ).
                   FrameObject(FrameSpec::Info::FSI_COMMON_ELEMENTS )
                   ->Create( *this ) );
        const StreamRefInterface*
          sri( dynamic_cast< const StreamRefInterface* >( cmn_obj.get( ) ) );

        if ( ( sri == NULL )
             || ( sri->GetLength( ) != (StreamRefInterface::length_type)(offset) ) )
        {
          //-------------------------------------------------------------
          // This file appears to be corrupted.
          //-------------------------------------------------------------
          throw std::runtime_error( "Missing FrEndOfFile structure" );
        }
        //---------------------------------------------------------------
        // Create object and place into smart pointer for auto cleanup
        //---------------------------------------------------------------
        std::auto_ptr< FrameSpec::Object >
          eof_obj( frameSpecInfo( ).
                   FrameObject( FrameSpec::Info::FSI_FR_END_OF_FILE )
                   ->Create( *this ) );
        //---------------------------------------------------------------
        // Transform into a form that can be used by generic interface
        //---------------------------------------------------------------
        const FrEndOfFile*
          eof( dynamic_cast< const FrEndOfFile* >( eof_obj.get( ) ) );
        if ( eof )
        {
          if ( eof->SeekTOC( ) )
          {
            seekg( -(eof->SeekTOC( )), std::ios::end );
            //-----------------------------------------------------------
            // Read the reference
            //-----------------------------------------------------------
            std::auto_ptr< FrameSpec::Object >
              ref_obj( frameSpecInfo( ).
                       FrameObject( FrameSpec::Info::FSI_COMMON_ELEMENTS )
                       ->Create( *this ) );
            std::auto_ptr< FrameSpec::Object >
              toc_obj( frameSpecInfo( ).
                       FrameObject( FrameSpec::Info::FSI_FR_TOC )
                       ->Create( *this ) );
            m_toc.reset( dynamic_cast< FrTOC* >( toc_obj.get( ) ) );
            if ( m_toc )
            {
              //---------------------------------------------------------
              // Successful with the dynamic cast so m_toc is now
              // responsible for releasing the memory resouces.
              //---------------------------------------------------------
              toc_obj.release( );
              //---------------------------------------------------------
              // Extract mapping information
              //---------------------------------------------------------
              for ( INT_4U
                      cur = 0,
                      last = m_toc->nSH( );
                    cur != last;
                    ++cur )
              {
                const INT_2U fsi_id = GetClassId( m_toc->SHname( cur ) );
                if ( fsi_id )
                {
                  m_stream_id_to_fsi_id[ m_toc->SHid( cur ) ] = fsi_id;
                }
              }
            }
          }
        }
      }
      catch( ... )
      {
        seekg( here );
        throw;
      }
      //-----------------------------------------------------------------
      // Restore file position
      //-----------------------------------------------------------------
      seekg( here );
    }

bool
plan::can_be_used_for_frame(char *file_name)
{
#if 0
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
#endif
  return false;
}

#if 0
//   
//!exc: std::bad_alloc - Out of memory.
//!exc: read_failure - Read failed.
//!exc: not_found_error - Data is not found.   
//         
void
plan::daqTriggerADC( const std::vector<std::string> &adcNames )
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

#if 0
//   
//!exc: cannot_update 
//!exc: read_failure - Read failed.
//         
FrameCPP::Version::FrameH&
plan::readFrame( INT_4U frameNumber, const std::vector<std::string> &adcNames )

{
#if 0
  FrameCPP::Version::FrRawData::AdcDataContainer& adc( frame.getRawData()->refAdc() );
  //printf("adc.begin()=%0x; adc.end()=%0x\n", adc.begin(), adc.end());
  adc.erase( adc.begin(), adc.end() );

  for ( std::vector<std::string>::const_iterator i = adcNames.begin(); i != adcNames.end(); i++) {
    // Read ADC data structure and append to the skeletal frame
    FrameCPP::Version::FrAdcData *adcptr = ReadFrAdcData(frameNumber, *i );
    if ( adcptr == 0 ) {
	system_log(1,"bad ADC name `%s'", i->c_str());
	throw not_found_error( "bad ADC name" );
    }
    adc.append ( adcptr, false, true ); // Don't allocate copy; Container will own data
  }

  // Only works on the first frame file; for further files the frame header values are not read from disk
  frame.SetDt(GetTOC()->GetDt()[frameNumber]);
  // Dig the kludge
  *(const_cast<FrameCPP::Version::GPSTime *>(&(frame.getGTime()))) =
    FrameCPP::Version::GPSTime(GetTOC()->GetGTimeS()[frameNumber],
				 GetTOC()->GetGTimeN()[frameNumber]);
#endif

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
#endif
