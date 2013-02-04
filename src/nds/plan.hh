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
  plan( buffer_type* Stream, plan *master = 0 ) : IFrameStream(Stream), master_plan(master) {
	if (master_plan) {
	  // Will not load the TOC but rather use the master plan TOC 
	  // :TODO: need to see if the TOC size is still the same and
	  // that the frame sequence is continuous
	  // Assign the TOC pointer from the mater plan
	  m_toc = master_plan->m_toc;
	  m_toc_loaded = true;
	  // Copy over the hash, needed to resolve structure types
	  m_stream_id_to_fsi_id = master_plan->m_stream_id_to_fsi_id;
	}
  }
  ~plan() {}


  typedef General::SharedPtr< FrameCPP::Version::FrameH > frame_h_type;
  typedef General::SharedPtr< FrameCPP::Version::FrAdcData > fr_adc_data_type;
  typedef General::SharedPtr< FrameCPP::Version::FrTOC > fr_toc_data_type;


  frame_h_type ReadFrameH(INT_4U Frame, INT_4U ContainerSet) {
        object_type       retval = readFrameHSubset(Frame, ContainerSet);
        return General::DynamicPointerCast< frame_h_type::element_type >( retval );
  }

  fr_adc_data_type ReadFrAdcData( INT_4U Frame, const std::string& Channel ) {
      if (master_plan) {
	// Populate index entry for the channel we are interested in
	FrameCPP::Version::FrTOC *v8_toc =
		dynamic_cast<FrameCPP::Version::FrTOC *>(m_toc.get());
	FrameCPP::Version::FrTOCAdcData::MapADC_type& adcMap =
		const_cast<FrameCPP::Version::FrTOCAdcData::MapADC_type&>(v8_toc->GetADC());
	FrameCPP::Version::FrTOCAdcData::MapADC_type::iterator i = adcMap.find(Channel);
	//std::cerr << "adc seq num: " << i->second.m_index << std::endl;
	//std::cerr << "adc toc positions start: " << v8_toc->m_positions_start << std::endl;

	// read new ADC positions at v8_toc->m_positions_start + i->second.m_index * 8
        seekg(-v8_toc->m_positions_start, std::ios_base::end );
        seekg(i->second.m_index * 8, std::ios_base::cur );
	//std::cerr << "curent file positions is " << tellg( ) << std::endl;

	// assign new position
	*this >> i->second.m_positionADC[ 0 ]; //i->second.m_positionADC[ 0 ];
	//std::cerr << "adc position: " << i->second.m_positionADC[ 0 ] << std::endl;
      } 
      return General::DynamicPointerCast< fr_adc_data_type::element_type > ( readFrAdcData( Frame, Channel ) );
  }
private:
  plan *master_plan;
}; // class FrameReadPlan

#endif // planHH
