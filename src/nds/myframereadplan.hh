#ifndef myFrameReadPlanHH
#define myFrameReadPlanHH

#include "framecpp/Version6/FrameH.hh"
#include "framecpp/Version6/IFrameStream.hh"
#include "mmstream.hh"

//-----------------------------------------------------------------------------
//
//: Frame Reading Plan
//
class myFrameReadPlan : public FrameCPP::Version_6::IFrameStream
{
public:
  //
  //: Constructor
  //   
  //!exc: std::bad_alloc - Out of memory.
  //!exc: read_failure - Read failed.
  //   
  myFrameReadPlan( std::istream& in );

  //: Destructor
  //
  //!exc: None.
  ~myFrameReadPlan();

  //: Read frame data
  //   
  //!exc: read_failure - Read failed.
  //!exc: cannot_update    
  //      
  FrameCPP::Version_6::FrameH& readFrame( INT_4U frameNumber, const std::vector<std::string> &adcNames );

#if 0  
  //: Include ADCs in the frame
  //   
  //!exc: std::bad_alloc - Out of memory.
  //!exc: read_failure - Read failed.
  //!exc: not_found_error - Data is not found.   
  //      
  void daqTriggerADC( const std::vector<std::string> &adcNames );
#endif

  bool can_be_used_for_frame(char *fname);
private:

  //: Skeleton frame object
  FrameCPP::Version_6::FrameH frame;

  //: Skeleton raw data object
  FrameCPP::Version_6::FrRawData raw_data;

  //: TOC offset variable read from the file, last 8 bytes
  char toc_offset[8];
}; // class FrameReadPlan

#endif // myFrameReadPlanHH
