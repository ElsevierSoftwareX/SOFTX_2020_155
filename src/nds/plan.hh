#ifndef planHH
#define planHH

#include "daqd_net.hh"
#include "mmstream.hh"
#include "framecpp/Common/FrameH.hh"
#include "framecpp/Version8/FrTOCAdcData.hh"
#include "framecpp/Version8/FrTOC.hh"


/// Frame Reading Plan
class plan : public FrameCPP::Common::IFrameStream
{
public:
  plan( buffer_type* Stream );
  plan( buffer_type* Stream, plan *master );
  ~plan();


  typedef typename General::SharedPtr< FrameCPP::Version::FrameH > frame_h_type;
  typedef typename General::SharedPtr< FrameCPP::Version::FrAdcData > fr_adc_data_type;
  typedef typename General::SharedPtr< FrameCPP::Version::FrTOC > fr_toc_data_type;


  frame_h_type ReadFrameH(INT_4U Frame, INT_4U ContainerSet) {
        object_type       retval = readFrameHSubset(Frame, ContainerSet);
        return General::DynamicPointerCast< typename frame_h_type::element_type >( retval );
  }


  fr_adc_data_type ReadFrAdcData( INT_4U Frame, const std::string& Channel ) {
      if (master_plan && !m_toc_loaded) {
	// Will not load the TOC but rather use the master plan TOC 
	// :TODO: need to see if the TOC size is still the same and
	// that the frame sequence is continuous
	// Assign the TOC pointer from the mater plan
	m_toc = master_plan->m_toc;
	// Copy over the hash, neede to resolve structure types
	// :TODO: this needs to come out of here into the contructor
	m_stream_id_to_fsi_id = master_plan->m_stream_id_to_fsi_id;
	m_toc_loaded = true;
	// Populate index entry for the channel we are interested in
	FrameCPP::Version::FrTOC *v8_toc =
		dynamic_cast<FrameCPP::Version::FrTOC *>(m_toc.get());
	FrameCPP::Version::FrTOCAdcData::MapADC_type& adcMap =
		const_cast<FrameCPP::Version::FrTOCAdcData::MapADC_type&>(v8_toc->GetADC());
	FrameCPP::Version::FrTOCAdcData::MapADC_type::iterator i = adcMap.find(Channel);
	std::cerr << "adc seq num: " << i->second.m_index << std::endl;
	std::cerr << "adc toc positions start: " << v8_toc->m_positions_start << std::endl;

	// read new ADC positions at v8_toc->m_positions_start + i->second.m_index * 8
        seekg(-v8_toc->m_positions_start, std::ios_base::end );
        seekg(i->second.m_index * 8, std::ios_base::cur );
	std::cerr << "curent file positions is " << tellg( ) << std::endl;

	// assign new position
	*this >> i->second.m_positionADC[ 0 ]; //i->second.m_positionADC[ 0 ];
	std::cerr << "adc position: " << i->second.m_positionADC[ 0 ] << std::endl;
      } 
      return General::DynamicPointerCast< typename fr_adc_data_type::element_type > ( readFrAdcData( Frame, Channel ) );
  }

  void load_toc();

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

  /// Skeleton frame object
  FrameCPP::Version::FrameH frame;

  /// Skeleton raw data object
  FrameCPP::Version::FrRawData raw_data;

  /// TOC offset variable read from the file, last 8 bytes
  char toc_offset[8];

  plan *master_plan;
}; // class FrameReadPlan

#endif // planHH
