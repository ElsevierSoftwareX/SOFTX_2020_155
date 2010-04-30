#include <config.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <fstream>

using namespace std;

#include "circ.hh"
#include "daqd.hh"
#include "sing_list.hh"

#ifdef USE_FRAMECPP
#if FRAMECPP_DATAFORMAT_VERSION > 4

#include "framecpp/Version6/FrameH.hh"
#include "framecpp/Version6/FrCommon.hh"
#include "framecpp/Version6/Functions.hh"
#include "framecpp/Version6/IFrameStream.hh"
#include "framecpp/Version6/OFrameStream.hh"
#include "framecpp/Version6/Util.hh"
#include "myimageframewriter.hh"
#else

#include "framecpp/frame.hh"
#include "framecpp/detector.hh"
#include "framecpp/adcdata.hh"
#include "framecpp/framereader.hh"
#include "framecpp/framewriter.hh"
#include "framecpp/framewritertoc.hh"
#include "framecpp/imageframewriter.hh"

#endif
#endif

extern daqd_c daqd;


// saves striped data 
void *
trender_c::raw_minute_saver ()
{

#ifndef VMICRFM_PRODUCER
  // Put this thread into the realtime scheduling class with half the priority
  daqd_c::realtime ("raw minute trend saver", 2);
#endif

  long minute_put_cntr;
  unsigned int rmp = raw_minute_trend_saving_period;

  for (minute_put_cntr = 0;; minute_put_cntr++) {
    int eof_flag = 0;
    circ_buffer_block_prop_t cur_prop[rmp];
    trend_block_t cur_blk [rmp][max_trend_output_channels];

    int nb = mtb -> get (raw_msaver_cnum);
    DEBUG(3, cerr << "minute trender saver; block " << nb << endl);
    TNF_PROBE_1(raw_minute_trender_c_saver_start, "minute_trender_c::raw_saver",
		"got one block",
		tnf_long,   block_number,    nb);
    {
      cur_prop[minute_put_cntr % rmp]
	= mtb -> block_prop (nb) -> prop;
      if (! mtb -> block_prop (nb) -> bytes)
	{
	  mtb -> unlock (raw_msaver_cnum);
	  eof_flag = 1;
	  DEBUG1(cerr << "minute trender framer EOF" << endl);
	  break; // out of the for() loop
	}
      memcpy (cur_blk[minute_put_cntr % rmp],
	      mtb -> block_ptr (nb), num_channels * sizeof (trend_block_t));
    }
    TNF_PROBE_0(raw_minute_trender_c_saver_end, "minute_trender_c::raw_saver", "end of block processing");
    mtb -> unlock (raw_msaver_cnum);

  if (minute_put_cntr % rmp == (rmp-1)) {
    // write to raw files. one for each channel
    // open/create file, lock file ???, write a record, close the file

    time_t t = time(0);
    DEBUG(1, cerr << "Begin raw minute trend writing" << endl);

    for (int j = 0; j < num_channels; j++)
      {
	// Calculate combined status
	int status = 0;
	int k;
	for (k=0;k<rmp;k++) status|=cur_blk[k][j].n;
	if ( status ) { // don't write bad data points
	  char tmpf [filesys_c::filename_max + 10];
	  strcat (strcat (strcpy (tmpf, raw_minute_fsd.get_path ()), "/"),
		  channels [j].name);
	  int fd = open (tmpf, O_CREAT|O_WRONLY|O_APPEND, 0644);
	  if (fd < 0) {
	    system_log(1, "Couldn't open raw minute trend file `%s' for writing; errno %d", tmpf, errno);
	    daqd.set_fault ();
	  } else {
	    struct stat fst;
	    int check_close = 0;
	    if (fstat (fd, &fst)) {
	      system_log(1, "can't stat raw minute trend file `%s'; errno %d", tmpf, errno);
	      daqd.set_fault ();	    
	    } else {
	      // Will not try writing to improperly sized file
	      if (fst.st_size % sizeof(raw_trend_record_struct) != 0) {
		system_log(1, "failed to write raw minute trend file `%s' out; errno %d", tmpf, errno);
		daqd.set_fault ();
	      } else {
	        raw_trend_record_struct rmtr[rmp];
	        for (k=0;k<rmp;k++) {
		  rmtr[k].gps = cur_prop[k].gps;
		  rmtr[k].tb = cur_blk[k][j];
	        }

	        int wsize = rmp*sizeof (raw_trend_record_struct);
	        int nw = write (fd, &rmtr, wsize);
	        if (nw != wsize) {
		  ftruncate (fd, fst.st_size); // to keep data integrity
		  system_log(1, "failed to write raw minute trend file `%s' out; errno %d", tmpf, errno);
		  daqd.set_fault ();
		}
		check_close = 1; // verify success of close()
	      }
	    }
            if (check_close) {
	      if (daqd.do_fsync) {
	        if (fsync (fd) != 0) {
                  // No space left in the file system?
                  ftruncate (fd, fst.st_size); // to keep data integrity
                  system_log(1, "failed to write raw minute trend file `%s' out; in fsync() errno %d", tmpf, errno);
                  daqd.set_fault ();
		}
	      }
	    }
  	    close(fd);
	  }
	}
      } // for
      t = time(0) - t;
      DEBUG(1, cerr << "Finished raw minute trend writing in " << t << " seconds" << endl);
  }

    if (eof_flag)
      break; // out of the for() loop
  }
  
  // signal to the producer that we are done
  this -> shutdown_minute_trender ();
  return NULL;
}


void *
trender_c::minute_framer ()
{
#if defined(USE_FRAMECPP) || FRAMECPP_DATAFORMAT_VERSION >= 6

#ifndef VMICRFM_PRODUCER
  // Put this thread into the realtime scheduling class with half the priority
  daqd_c::realtime ("minute trend framer", 2);
#endif

#if FRAMECPP_DATAFORMAT_VERSION >= 6
  FrameCPP::Version::FrameH *frame = 0;
  FrameCPP::Version::FrRawData raw_data ("");
#elif FRAMECPP_DATAFORMAT_VERSION > 4
  FrameCPP::Version_6::FrameH* frame = 0;
  FrameCPP::Version_6::FrRawData raw_data ("");
  myImageFrameWriter *fw = 0;
#else
  FrameCPP::Frame *frame = 0;
  FrameCPP::RawData raw_data ("");
  FrameCPP::ImageFrameWriter *fw = 0;
#endif

#if FRAMECPP_DATAFORMAT_VERSION < 6
  strstream *ost;
#endif
  int frame_length_seconds;
  int frame_length_blocks;

  /* Circular buffer length determines minute trend frame length */
  frame_length_blocks = mtb -> blocks ();

  /* Length in seconds is the product of lengths of trend and minute trend buffers */
  frame_length_seconds = frame_length_blocks * tb -> blocks ();

#if FRAMECPP_DATAFORMAT_VERSION > 4
  FrameCPP::Version::FrDetector detector = daqd.getDetector1();
#else  
  /* Detector data */
  FrameCPP::Detector detector (daqd.detector_name,
			       FrameCPP::Location (daqd.detector_longitude_degrees,
						   daqd.detector_longitude_minutes,
						   daqd.detector_longitude_seconds,
						   daqd.detector_latitude_degrees,
						   daqd.detector_latitude_minutes,
						   daqd.detector_latitude_seconds),
			       daqd.detector_elevation,
			       daqd.detector_arm_x_azimuth,
			       daqd.detector_arm_y_azimuth);
#endif

  // Create minute trend frame
  //
#if FRAMECPP_DATAFORMAT_VERSION > 4
  try {
    frame = new FrameCPP::Version::FrameH ("LIGO",
					     0, // run number ??? buffptr -> block_prop (nb) -> prop.run;
					     1, // frame number
					     FrameCPP::Version_6::GPSTime (0, 0),
					     0, // leap seconds
					     frame_length_seconds // dt
					     );
#if FRAMECPP_DATAFORMAT_VERSION >= 6
    frame -> SetRawData (raw_data);
    frame -> RefDetectProc ().append (detector);

#else
    frame -> setRawData (raw_data);
    frame -> refDetectorProc ().append (detector);
#endif

    // Append second detector if it is defined
    if (daqd.detector_name1.length() > 0) {
#if FRAMECPP_DATAFORMAT_VERSION >= 6
      FrameCPP::Version::FrDetector detector1 = daqd.getDetector2();
      frame -> RefDetectProc ().append (detector1);
#else
      FrameCPP::Version_6::FrDetector detector1 = daqd.getDetector2();
      frame -> refDetectorProc ().append (detector1);
#endif
    }
  } catch (...) {
    system_log(1, "Couldn't create minute trend frame");
    this -> shutdown_minute_trender ();
    return NULL;
  }
#else
  try {
    frame = new FrameCPP::Frame ("LIGO",
				 0, // run number ??? buffptr -> block_prop (nb) -> prop.run;
				 1, // frame number
				 FrameCPP::Time (0, 0),
				 0,
				 0, // localTime
				 frame_length_blocks // dt
				 );

    frame -> setRawData (raw_data);
    frame -> setDetectProc (detector);
  } catch (...) {
    system_log(1, "Couldn't create minute trend frame");
    this -> shutdown_minute_trender ();
    return NULL;
  }
#endif

  // Keep pointers to the data samples for each data channel
#if FRAMECPP_DATAFORMAT_VERSION >= 6
  unsigned
#endif
        char *adc_ptr [num_trend_channels];
  //INT_2U data_valid [num_trend_channels];
  INT_2U *data_valid_ptr [num_trend_channels];

  // Create ADCs
  try {
    REAL_8 junk [frame_length_blocks];
    memset(junk, 0, sizeof(REAL_8)*frame_length_blocks);
    for (int i = 0; i < num_trend_channels; i++) {
#if FRAMECPP_DATAFORMAT_VERSION >= 6
      FrameCPP::Version::FrAdcData adc
        = FrameCPP::Version::FrAdcData (std::string(trend_channels [i].name),
                                          trend_channels [i].group_num,
                                          i, // channel ???
                                          CHAR_BIT * trend_channels [i].bps,
					  1./frame_length_blocks,
                                          trend_channels [i].signal_offset,
                                          trend_channels [i].signal_slope,
                                          std::string(trend_channels [i].signal_units),
                                          .0,
                                          0,
                                          0,
                                          .0);
      frame -> GetRawData () -> RefFirstAdc ().append (adc);
      FrameCPP::Version::Dimension  dims [1] = { FrameCPP::Version::Dimension (frame_length_blocks, frame_length_blocks, "") };     
      switch (trend_channels [i].data_type) {
      case _64bit_double:
        {
          FrameCPP::Version::FrVect vect("", 1, dims, (REAL_8 *) junk, "");
          frame -> GetRawData () -> RefFirstAdc () [i] -> RefData().append (vect);
          bool owns;
          adc_ptr[i] = frame -> GetRawData () -> RefFirstAdc () [i] -> RefData()[0] -> SteelData(owns);
          break;
        }
      case _32bit_float: 
        { 
          FrameCPP::Version::FrVect vect("", 1, dims, (REAL_4 *) junk, "");
          frame -> GetRawData () -> RefFirstAdc () [i] -> RefData().append (vect);
          bool owns;
          adc_ptr[i] = frame -> GetRawData () -> RefFirstAdc () [i] -> RefData()[0] -> SteelData(owns);
          break;
        }
      case _32bit_integer:
        {
          FrameCPP::Version::FrVect vect("", 1, dims, (INT_4S *) junk, "");
          frame -> GetRawData () -> RefFirstAdc () [i] -> RefData().append (vect);
          bool owns;
          adc_ptr[i] = frame -> GetRawData () -> RefFirstAdc () [i] -> RefData()[0] -> SteelData(owns);
          break;
        }
      default:
        {
          abort();
          FrameCPP::Version::FrVect vect("", 1, dims, (INT_2S *) junk, "");
          frame -> GetRawData () -> RefFirstAdc () [i] -> RefData().append (vect);
          bool owns;
          adc_ptr[i] = frame -> GetRawData () -> RefFirstAdc () [i] -> RefData()[0] -> SteelData(owns);
          break;
        }
      }
#elif FRAMECPP_DATAFORMAT_VERSION > 4
      FrameCPP::Version_6::FrAdcData adc
	= FrameCPP::Version_6::FrAdcData (trend_channels [i].name,
					  trend_channels [i].group_num,
					  i, // channel ???
					  CHAR_BIT * trend_channels [i].bps,
					  1./frame_length_blocks,
					  trend_channels [i].signal_offset,
					  trend_channels [i].signal_slope,
					  trend_channels [i].signal_units,
					  .0,
					  FrameCPP::Version_6::GPSTime( 0, 0 ),
					  0,
					  .0);

      frame -> getRawData () -> refAdc ().append (adc);

      FrameCPP::Version_6::Dimension  dims [1] = { FrameCPP::Version_6::Dimension (frame_length_blocks, frame_length_blocks, "") };
      switch (trend_channels [i].data_type) { 
      case _64bit_double:
	{
	  FrameCPP::Version_6::FrVect vect("", 1, dims, (REAL_8 *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      case _32bit_float: 
	{
	  FrameCPP::Version_6::FrVect vect("", 1, dims, (REAL_4 *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      case _32bit_integer:
	{
	  FrameCPP::Version_6::FrVect vect("", 1, dims, (INT_4S *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      default:
	{
	  abort();
	  FrameCPP::Version_6::FrVect vect("", 1, dims, (INT_2S *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      }
#else
      FrameCPP::AdcData adc 
	= FrameCPP::AdcData (trend_channels [i].name, // channel name
			     trend_channels [i].group_num,
			     i, // channel ???
			     CHAR_BIT * trend_channels [i].bps, // nBits
			     1./frame_length_blocks,
			     trend_channels [i].signal_offset,
			     trend_channels [i].signal_slope,
			     trend_channels [i].signal_units);

      frame -> getRawData () -> refAdc ().append (adc);

      FrameCPP::Dimension  dims [1] = { FrameCPP::Dimension (frame_length_blocks, frame_length_blocks, "") };
      switch (trend_channels [i].data_type) { 
      case _64bit_double:
	{
	  FrameCPP::Vect vect("", 1, dims, (REAL_8 *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      case _32bit_float: 
	{
	  FrameCPP::Vect vect("", 1, dims, (REAL_4 *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      case _32bit_integer:
	{
	  FrameCPP::Vect vect("", 1, dims, (INT_4S *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      default:
	{
	  abort();
	  FrameCPP::Vect vect("", 1, dims, (INT_2S *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      }
#endif
    }
  } catch (...) {
    system_log(1, "Couldn't create one or several adcs");

    abort();

    this -> shutdown_trender ();
    return NULL;
  }
  system_log(1, "Done creating ADC structures");

#if FRAMECPP_DATAFORMAT_VERSION < 6
  // Create frame image
  try {
#if FRAMECPP_DATAFORMAT_VERSION > 4
    fw = new myImageFrameWriter (*(ost = new strstream ()));
#else
    fw = new FrameCPP::ImageFrameWriter (*(ost = new strstream ()));
#endif
    fw -> writeFrame (*frame);
    fw -> close ();
  } catch (write_failure) {
    system_log(1, "Couldn't create minute trend image frame writer; write_failure");
    
    abort();
    
    this -> shutdown_minute_trender ();
    return NULL;
  }
  system_log(1, "Done writing frame image");

  int image_size = ost -> pcount ();

  // Frame itself is not needed any more and may be freed
  delete frame;
  frame = 0;

#if 0
  // Finally, prepare an array of pointers to the ADC data
  char *adc_ptr [num_trend_channels];
  INT_2U *data_valid_ptr [num_trend_channels];
#endif

  {
#if FRAMECPP_DATAFORMAT_VERSION > 4
    typedef myImageFrameWriter::OMI OMI;
#else
    typedef FrameCPP::ImageFrameWriter::OMI OMI;
#endif
    for (int i = 0; i < num_trend_channels; i++) {
      INT_8U offs = 
	fw -> adcNamePositionMap [trend_channels [i].name].first[0];
      INT_4U classInstance = fw -> adcDataVectorPtr (offs);
      INT_2U instance = classInstance  & 0xffff;
      OMI v = fw -> vectInstanceOffsetMap.find (instance);
#if 0
#ifndef NDEBUG
      cout << "trend vector " << (classInstance >> 16) << "\t" << (classInstance&0xffff) << endl;
      if (v == fw -> vectInstanceOffsetMap.end()) {
	cout << "trend vector " << instance << " not found!" << endl;
	exit(1);
      } else {
       cout << "data vector offset is " << v -> second[0] << endl;
      }
#endif
#endif // 0

      adc_ptr [i] = fw->vectorData (v -> second[0]);
      data_valid_ptr [i] = fw->adcDataValidPtr (offs);
    }
  }
  system_log(1, "Done creating ADC pointers");
#endif

  sem_post (&minute_frame_saver_sem);
  
  long frame_cntr;
  for (frame_cntr = 0;; frame_cntr++)
    {
      int eof_flag = 0;
      circ_buffer_block_prop_t file_prop;
      trend_block_t cur_blk [max_trend_output_channels];

      // Accumulate frame adc data
      for (int i = 0; i < frame_length_blocks; i++)
	{
	  int nb = mtb -> get (msaver_cnum);
	  DEBUG(3, cerr << "minute trender saver; block " << nb << endl);
	  TNF_PROBE_1(minute_trender_c_framer_start, "trender_c::framer",
		      "got one block",
		      tnf_long,   block_number,    nb);
	  {
	    if (! mtb -> block_prop (nb) -> bytes)
	      {
		mtb -> unlock (msaver_cnum);
		eof_flag = 1;
		DEBUG1(cerr << "trender framer EOF" << endl);
		break; // out of the for() loop
	      }
	    if (! i)
	      file_prop = mtb -> block_prop (nb) -> prop;
	      
	    memcpy (cur_blk, mtb -> block_ptr (nb), num_channels * sizeof (trend_block_t));
	  }
	  TNF_PROBE_0(minute_trender_c_framer_end, "trender_c::framer", "end of block processing");
	  mtb -> unlock (msaver_cnum);

	  // Check if the gps time isn't aligned on an hour boundary
	  if (! i) {
	    unsigned long gps_mod = file_prop.gps % 3600;
	    if ( gps_mod ) {
	      // adjust time to make files aligned on gps mod 3600
	      file_prop.gps -= gps_mod;
	      if (file_prop.cycle > gps_mod*16)
	      	file_prop.cycle -= gps_mod*16;
	      else
		file_prop.cycle = 0;
	      i += gps_mod/60;
	    }

#if 0
	    // zero out some data that's missing
	    for (int j = 0; j < num_trend_channels; j++)
	      memset ( adc_ptr [j], 0, (gps_mod/60) * trend_channels [j].bps );
#endif
	  }

	  
	  for (int j = 0; j < num_channels; j++)
	    {
	      if (channels [j].data_type == _64bit_double) {
		memcpy(adc_ptr [5*j] + i*sizeof(REAL_8), &cur_blk [j].min.D, sizeof (REAL_8));
		memcpy(adc_ptr [5*j+1] + i*sizeof(REAL_8), &cur_blk [j].max.D, sizeof (REAL_8));
	      } else if (channels [j].data_type == _32bit_float) {
		memcpy(adc_ptr [5*j] + i*sizeof(REAL_4), &cur_blk [j].min.F, sizeof (REAL_4));
		memcpy(adc_ptr [5*j+1] + i*sizeof(REAL_4), &cur_blk [j].max.F, sizeof (REAL_4));
	      } else {
		memcpy(adc_ptr [5*j] + i*sizeof(INT_4S), &cur_blk [j].min.I, sizeof (INT_4S));
		memcpy(adc_ptr [5*j+1] + i*sizeof(INT_4S), &cur_blk [j].max.I, sizeof (INT_4S));
	      }
	      //cerr << j << "\t" << i << hex << (void*)(adc_ptr[5*j+2]) << endl;
	      memcpy(adc_ptr [5*j+2] + i*sizeof(INT_4S), &cur_blk [j].n, sizeof (INT_4S));
	      //adc_ptr [5*j+2][4*i]=0;
#ifdef not_def
	      REAL_8 rms = sqrt(cur_blk [j].rms);
	      memcpy(adc_ptr [5*j+3] + i*sizeof(REAL_8), &rms, sizeof (REAL_8));
#endif
	      memcpy(adc_ptr [5*j+3] + i*sizeof(REAL_8), &cur_blk [j].rms, sizeof (REAL_8));
	      memcpy(adc_ptr [5*j+4] + i*sizeof(REAL_8), &cur_blk [j].mean, sizeof (REAL_8));
	    }
	}

      if (eof_flag)
	break; // out of the for() loop

#if FRAMECPP_DATAFORMAT_VERSION < 6
      fw -> setFrameFileAttributes (file_prop.run, file_prop.cycle / 16 / 3600, 0,
				    file_prop.gps, 3600, file_prop.gps_n,
				    file_prop.leap_seconds, file_prop.altzone);
#else
      frame -> SetGTime(FrameCPP::Version::GPSTime (file_prop.gps, file_prop.gps_n));
#endif

      time_t file_gps;
      time_t file_gps_n;
      int dir_num = 0;

      char _tmpf [filesys_c::filename_max + 10];
      char tmpf [filesys_c::filename_max + 10];

      file_gps = file_prop.gps;
      file_gps_n = file_prop.gps_n;
      dir_num = minute_fsd.getDirFileNames (file_gps, _tmpf, tmpf, 1, 3600 );

      int fd = creat (_tmpf, 0644);
      if (fd < 0) {
	system_log(1, "Couldn't open minute trend frame file `%s' for writing; errno %d", _tmpf, errno);
	minute_fsd.report_lost_frame ();
	daqd.set_fault ();
      } else {
	DEBUG(3, cerr << "`" << _tmpf << "' opened" << endl);
#if defined(DIRECTIO_ON) && defined(DIRECTIO_OFF)
	if (daqd.do_directio) directio (fd, DIRECTIO_ON);
#endif
	TNF_PROBE_1(minute_trender_c_framer_frame_write_start, "trender_c::framer",
		    "frame write",
		    tnf_long,   frame_number,   frame_cntr);
	  
#if FRAMECPP_DATAFORMAT_VERSION < 6
	if (daqd.cksum_file != "") {
	  daqd_c::fr_cksum(daqd.cksum_file, tmpf, (unsigned char *)(ost -> str ()), image_size);
	}
#endif

#if FRAMECPP_DATAFORMAT_VERSION >= 6
	close (fd);
        FrameCPP::Common::FrameBuffer<filebuf>* obuf
            = new FrameCPP::Common::FrameBuffer<std::filebuf>(std::ios::out);
        obuf -> open(_tmpf, std::ios::out | std::ios::binary);
        FrameCPP::Common::OFrameStream  ofs(obuf);
        ofs.SetCheckSumFile(FrameCPP::Common::CheckSum::CRC);
        DEBUG(1, cerr << "Begin minute trend WriteFrame()" << endl);
        time_t t = time(0);
        ofs.WriteFrame(*(frame), //FrameCPP::Common::CheckSum::NONE,
                        daqd.no_compression? FrameCPP::FrVect::RAW:
				 FrameCPP::FrVect::ZERO_SUPPRESS_OTHERWISE_GZIP, 1,
                        FrameCPP::Common::CheckSum::CRC);
        t = time(0) - t;
        DEBUG(1, cerr << "Done in " << t << " seconds" << endl);
        ofs.Close();
        obuf->close();
	if (1)
#else
	/* Write out a frame */
	int nwritten = write (fd, ost -> str (), image_size);
        if (nwritten == image_size) 
#endif
	{
	  if (rename(_tmpf, tmpf)) {
		system_log(1, "minute_framer(): failed to rename file; errno %d", errno);
		minute_fsd.report_lost_frame ();
		daqd.set_fault ();
	  } else {
	  	DEBUG(3, cerr << "minute trend frame " << frame_cntr << " is written out" << endl);
	  	// Successful frame write
	  	minute_fsd.update_dir (file_gps, file_gps_n, frame_length_blocks, dir_num);
	  }
	} else {
	  system_log(1, "minute_framer(): failed to write trend frame out; errno %d", errno);
	  minute_fsd.report_lost_frame ();
	  daqd.set_fault ();
	}
#if FRAMECPP_DATAFORMAT_VERSION < 6
	close (fd);
#endif
	TNF_PROBE_0(minute_trender_c_framer_frame_write_end, "trender_c::framer", "frame write");
      }


#if EPICS_EDCU == 1
      /* Epics display: minute trend data look back size in seconds */
      extern unsigned int pvValue[1000];
      pvValue[11] = minute_fsd.get_max() - minute_fsd.get_min();

      /* Epics display: current trend frame directory */
      extern unsigned int pvValue[1000];
      pvValue[12] = minute_fsd.get_cur_dir();
#endif
    }

  // signal to the producer that we are done
  this -> shutdown_minute_trender ();
#endif
  return NULL;
}

#if 0
// :TODO:
// change minute framer to use framecpp

void *
trender_c::minute_framer ()
{

#ifndef VMICRFM_PRODUCER
  // Put this thread into the realtime scheduling class with half the priority
  daqd_c::realtime ("minute trend framer", 2);
#endif

  long minute_frame_cntr;
  struct FrameH *minute_frame;
  struct FrAdcData *minute_min_adc [max_trend_channels];
  struct FrAdcData *minute_max_adc [max_trend_channels];
  struct FrAdcData *minute_rms_adc [max_trend_channels];
  struct FrAdcData *minute_mean_adc [max_trend_channels];
  struct FrAdcData *minute_n_adc [max_trend_channels];
  struct FrFile *mFile; // minute long trend frame file

  int minute_frame_length_seconds;
  int minute_frame_length_blocks;

  /* Create minute long trend data frame */
  pthread_mutex_lock (&framelib_lock);
  minute_frame = FrameHNew ("LIGO");
  pthread_mutex_unlock (&framelib_lock);

  if (! minute_frame)
    {
      system_log(1, "framer(): couldn't create minute trend frame");
      // shutdown_server ();
      this -> shutdown_minute_trender ();
      return NULL;
    }

  // Create detector
  struct FrDetector *frdetect 
    = minute_frame -> detectProc = FrDetectorNew (const_cast<char *>(daqd.detector_name.c_str()));

  frdetect->longitudeD   = daqd.detector_longitude_degrees;
  frdetect->longitudeM   = daqd.detector_longitude_minutes;
  frdetect->longitudeS   = daqd.detector_longitude_seconds;
  
  frdetect->latitudeD    = daqd.detector_latitude_degrees;
  frdetect->latitudeM    = daqd.detector_latitude_minutes;
  frdetect->latitudeS    = daqd.detector_latitude_seconds;
  
  frdetect->elevation    = daqd.detector_elevation;
  
  frdetect->armXazimuth  = daqd.detector_arm_x_azimuth;
  frdetect->armYazimuth  = daqd.detector_arm_y_azimuth;

  /* Circular buffer length determines minute trend frame length */
  minute_frame_length_blocks = mtb -> blocks ();

  /* Length in seconds is the product of lengths of trend and minute trend buffers */
  minute_frame_length_seconds = minute_frame_length_blocks * tb -> blocks ();

  /*
    Create ADC data channels.
    There will be three channels created for each configured
    minute trend channel, naming it with the appropriate suffix.
  */
  for (int i = 0; i < num_channels; i++)
    {
      char chname [channel_t::channel_name_max_len + 5];

      pthread_mutex_lock (&framelib_lock);

      int minmax_data_type;

      if (channels [i].data_type == _64bit_double)
	minmax_data_type = -64;
      else if (channels [i].data_type == _32bit_float)
	minmax_data_type = -32;
      else
	minmax_data_type = 32;

      minute_min_adc [i] = FrAdcDataNew (minute_frame, strcat (strcpy (chname, channels [i].name), ".min"),
					 1./minute_frame_length_blocks, /* sample rate */
					 minute_frame_length_blocks, minmax_data_type);
	
      minute_max_adc [i] = FrAdcDataNew (minute_frame, strcat (strcpy (chname, channels [i].name), ".max"),
					 1./minute_frame_length_blocks, /* sample rate */
					 minute_frame_length_blocks, minmax_data_type);
      minute_rms_adc [i] = FrAdcDataNew (minute_frame, strcat (strcpy (chname, channels [i].name), ".rms"),
					 1./minute_frame_length_blocks, /* sample rate */
					 minute_frame_length_blocks, -64);
      minute_mean_adc [i] = FrAdcDataNew (minute_frame, strcat (strcpy (chname, channels [i].name), ".mean"),
					  1./minute_frame_length_blocks, minute_frame_length_blocks, -64);
      minute_n_adc [i] = FrAdcDataNew (minute_frame, strcat (strcpy (chname, channels [i].name), ".n"),
				       1./minute_frame_length_blocks, minute_frame_length_blocks, 32);
      pthread_mutex_unlock (&framelib_lock);

      if (!(((int) minute_min_adc [i] && (int) minute_max_adc [i])
	    && (int) minute_rms_adc [i]
	    && (int) minute_mean_adc [i] && (int) minute_n_adc [i]
	     ))
	{
	  system_log(1, "Couldn't create one or several adcs");
	  // Have to free all already allocated ADC structures at this point
	  // to avoid memory leaks
	  // Or (even better) just exit
	  //	  shutdown_server ();
	  abort ();
	  //exit (1);
	  this -> shutdown_minute_trender ();
	  return NULL;
	}

      // Assign calibration values
      minute_min_adc [i] -> bias =
	minute_max_adc [i] -> bias =
	minute_rms_adc [i] -> bias =
	minute_mean_adc [i] -> bias =
	channels [i].signal_offset;
      minute_min_adc [i] -> slope =
	minute_max_adc [i] -> slope =
	minute_rms_adc [i] -> slope =
	minute_mean_adc [i] -> slope =
	channels [i].signal_slope;
      minute_min_adc [i] -> units =
	minute_max_adc [i] -> units =
	minute_rms_adc [i] -> units =
	minute_mean_adc [i] -> units =
	channels [i].signal_units;
    }

  for (minute_frame_cntr = 0;; minute_frame_cntr++)
    {
      int eof_flag = 0;
      circ_buffer_block_prop_t minute_file_prop;
      trend_block_t cur_blk [max_trend_output_channels];

      // Accumulate minute trend data into the frame
      for (int i = 0; i < minute_frame_length_blocks; i++)
	{
	  int nb = mtb -> get (msaver_cnum);
	  DEBUG(3, cerr << "minute trender saver; block " << nb << endl);
	  TNF_PROBE_1(minute_trender_c_framer_start, "minute_trender_c::framer",
		      "got one block",
		      tnf_long,   block_number,    nb);
	  {
	    if (! mtb -> block_prop (nb) -> bytes)
	      {
		mtb -> unlock (msaver_cnum);
		eof_flag = 1;
		DEBUG1(cerr << "minute trender framer EOF" << endl);
		break; // out of the for() loop
	      }
	    if (! i)
	      minute_file_prop = mtb -> block_prop (nb) -> prop;

	    memcpy (cur_blk, mtb -> block_ptr (nb), num_channels * sizeof (trend_block_t));
	  }
	  TNF_PROBE_0(minute_trender_c_framer_end, "minute_trender_c::framer", "end of block processing");
	  mtb -> unlock (msaver_cnum);

	  // Check if the gps time isn't aligned on an hour bounday
	  if (! i) {
	    unsigned long gps_mod = minute_file_prop.gps % 3600;
	    if ( gps_mod ) {
	      // adjust time to make files aligned on gps mod 3600
	      minute_file_prop.gps -= gps_mod;
	      i += gps_mod/60;

	      // zero out some data that's missing
	      for (int j = 0; j < num_channels; j++)
		for (int k = 0; k < gps_mod/60; k++)
		  {
		    if (channels [j].data_type == _64bit_double) {
		      minute_min_adc [j] -> data -> dataD [k] = .0;
		      minute_max_adc [j] -> data -> dataD [k] = .0;
		    } else if (channels [j].data_type == _32bit_float) {
		      minute_min_adc [j] -> data -> dataF [k] = .0;
		      minute_max_adc [j] -> data -> dataF [k] = .0;
		    } else {
		      minute_min_adc [j] -> data -> dataI [k] = 0;
		      minute_max_adc [j] -> data -> dataI [k] = 0;
		    }
		    minute_rms_adc [j] -> data -> dataD [k] = .0;
		    minute_mean_adc [j] -> data -> dataD [k] = .0;
		    minute_n_adc [j] -> data -> dataI [k] = 0;
		  }
	    }
	  }

	  for (int j = 0; j < num_channels; j++)
	    {
	      if (channels [j].data_type == _64bit_double) {
		minute_min_adc [j] -> data -> dataD [i] = cur_blk [j].min.D;
		minute_max_adc [j] -> data -> dataD [i] = cur_blk [j].max.D;
	      } else if (channels [j].data_type == _32bit_float) {
		minute_min_adc [j] -> data -> dataF [i] = cur_blk [j].min.F;
		minute_max_adc [j] -> data -> dataF [i] = cur_blk [j].max.F;
	      } else {
		minute_min_adc [j] -> data -> dataI [i] = cur_blk [j].min.I;
		minute_max_adc [j] -> data -> dataI [i] = cur_blk [j].max.I;
	      }
	      minute_rms_adc [j] -> data -> dataD [i] = cur_blk [j].rms;
	      minute_mean_adc [j] -> data -> dataD [i] = cur_blk [j].mean;
	      minute_n_adc [j] -> data -> dataI [i] = cur_blk [j].n;
	    }
	}

      if (eof_flag)
	break; // out of the for() loop

      minute_frame -> run = minute_file_prop.run;
      minute_frame -> GTimeS = minute_file_prop.gps;
      minute_frame -> GTimeN = minute_file_prop.gps_n;
      minute_frame -> ULeapS = minute_file_prop.leap_seconds;
      minute_frame -> localTime = minute_file_prop.altzone;

      minute_frame -> dt = minute_frame_length_seconds;
      minute_frame -> frame = minute_frame_cntr;

      time_t file_gps;
      time_t file_gps_n;
      int dir_num = 0;

      if (! (minute_frame_cntr % frames_per_file))
	{
	  char tmpf [filesys_c::filename_max + 10];

	  file_gps = minute_file_prop.gps;
	  file_gps_n = minute_file_prop.gps_n;
	  dir_num = minute_fsd.dir_fname (file_gps, tmpf, 1, 3600);

	  // FIXME: using the save `frames_lib_buffer' as in framer() can brake FrameL.c , I think...
	  // Well, maybe not...

	  pthread_mutex_lock (&framelib_lock);
#if FRAMELIB_VERSION >= 400
	  mFile = FrFileONew (tmpf, 0);
#else
	  mFile = FrFileONew (tmpf, 0, frames_lib_buffer, daqd_c::frame_buf_size);
#endif
	  pthread_mutex_unlock (&framelib_lock);

	  if (! mFile)
	    {
	      system_log(1, "minute_framer(): couldn't open trend file `%s'", tmpf);
	      minute_fsd.report_lost_frame ();
	      daqd.set_fault ();
	    }
	  DEBUG(3, cerr << "`" << tmpf << "' opened" << endl);
	}

      if (mFile) {
	TNF_PROBE_1(trender_c_minute_framer_frame_write_start, "minute_trender_c::framer",
		    "frame write",
		    tnf_long,   frame_number,   minute_frame_cntr);

	/* Write out a frame */
	pthread_mutex_lock (&framelib_lock);
	int fr_write_res = FrameWrite (minute_frame, mFile);
	pthread_mutex_unlock (&framelib_lock);

	if (fr_write_res != FR_OK) {
	  system_log(1, "minute_framer(): failed to write trend frame out");
	  minute_fsd.report_lost_frame ();
	  daqd.set_fault ();
	} else {
	  DEBUG(3, cerr << "minute frame " << minute_frame_cntr << " is written out" << endl);
	}
	TNF_PROBE_0(trender_c_minute_framer_frame_write_end, "minute_trender_c::framer", "frame write");
      }

      // Close trend frame file, if all frames are written out
      if (minute_frame_cntr % frames_per_file == frames_per_file - 1) {
	if (mFile)
	  {
	    DEBUG(3, cerr << "`" << mFile -> fileName << "' closed" << endl);

	    pthread_mutex_lock (&framelib_lock);
	    FrFileOEnd (mFile);
	    pthread_mutex_unlock (&framelib_lock);

	    minute_fsd.update_dir (file_gps, file_gps_n, minute_frame_length_seconds, dir_num);

	    mFile = (struct FrFile *)0;
	  }
      }
    }

  pthread_mutex_lock (&framelib_lock);
  FrameFree (minute_frame);
  pthread_mutex_unlock (&framelib_lock);

  // signal to the producer that we are done
  this -> shutdown_minute_trender ();
  return NULL;
}

#endif // 0



// Minute trender accumulates trend samples into the local storage
// until the number of samples equal to the length of trend buffer
// is accumulated. At this point minute trender puts a block into
// the minute trend buffer.

// FIXME: shutdown is messed up

void *
trender_c::minute_trend ()
{

#ifndef VMICRFM_PRODUCER
  // Put this thread into the realtime scheduling class with half the priority
  daqd_c::realtime ("minute trender", 2);
#endif

  int tblen = tb -> blocks (); // trend buffer length
  int nc; // number of trend samples accumulated
  int nb; // trend buffer block number
  trend_block_t ttb [num_channels]; // minute trend local storage
  unsigned long npoints [num_channels]; // number of data points processed

  for (nc = 0;;)
    {
      circ_buffer_block_prop_t prop;

      // Request to shut us down received from consumer
      // 
      if (shutdown_minute_now)
	{
	  // FIXME -- synchronize with the demise of the trender
	  //	  shutdown_buffer ();
	  continue;
	}
      
      nb = tb -> get (mcnum);
      TNF_PROBE_1(trender_c_minute_trend_start, "trender_c::minute_trend",
		  "got one block",
		  tnf_long,   block_no,    nb);
      {
	trend_block_t *btr = (trend_block_t *) tb -> block_ptr (nb);
	int bytes;

	// Shutdown request from trend circ buffer
	if (! (bytes = tb -> block_prop (nb) -> bytes))
	  {
	    char pbuf [1];
	    mtb -> put (pbuf, 0);
	    return NULL;
	  }

	// Start of new minute trend block
	if (! nc) {
	  prop = tb -> block_prop (nb) -> prop; // remember initial properties
	  memset (ttb, 0, sizeof(ttb[0]) * num_channels);
	  memset (npoints, 0, sizeof(npoints[0]) * num_channels);
	  // Check GPS time; it must be aligned on 60 secs boundary
	  // correct if wrong
	  if (prop.gps%tblen) {
	    nc = prop.gps%tblen;
    	    system_log(1, "Minute trender made GPS time correction; gps=%d; gps%%%d=%d", prop.gps, tblen, prop.gps%tblen);
	    prop.gps -= prop.gps%tblen; // This minute trend point has skips (gap in the beginning)
	  }
	}

	for (register int j = 0; j < num_channels; j++)
	  {
	    if ( btr [j].n ) {
	      if ( !npoints [j] ) { // first good data point for this channel
		ttb [j] = btr [j]; // set initial trend for this channel
		ttb [j].rms *= ttb [j].rms;  // prepare squares for RMS channel
	      } else { // not the first data point for this channel
#ifndef NO_RMS
		ttb [j].rms +=  (double) (btr[j].rms * btr[j].rms);
#endif
		ttb [j].mean += btr [j].mean;
		ttb [j].n += btr [j].n;
		if (channels [j].data_type == _64bit_double) {
		  if (btr [j].min.D < ttb [j].min.D)
		    ttb [j].min.D = btr [j].min.D;
		  if (btr [j].max.D > ttb [j].max.D)
		    ttb [j].max.D = btr[j].max.D;
		} else if (channels [j].data_type == _32bit_float) {
		  if (btr [j].min.F < ttb [j].min.F)
		    ttb [j].min.F = btr [j].min.F;
		  if (btr [j].max.F > ttb [j].max.F)
		    ttb [j].max.F = btr [j].max.F;
		} else {
		  if (btr [j].min.I < ttb [j].min.I)
		    ttb [j].min.I = btr [j].min.I;
		  if (btr [j].max.I > ttb [j].max.I)
		    ttb [j].max.I = btr [j].max.I;
		}
	      }
	      npoints [j]++; // count this data point in, there is good data available
	    }
	  }
      }
      TNF_PROBE_1(trender_c_minute_trend_end, "minute_trender_c::trender", "end of block processing",
		  tnf_long,   block_timestamp,    prop.gps);
      tb -> unlock (mcnum);

      nc++;
      if (nc == tblen) {
#ifndef NO_RMS
	for (register int j = 0; j < num_channels; j++) {
	  if ( npoints [j] ) {
	    ttb [j].rms = sqrt (ttb [j].rms / (double) npoints [j]);
	    ttb [j].mean = ttb [j].mean / (double)npoints [j];
	  }
	}
#endif
	nc = 0;
	DEBUG(3, cerr << "minute trender consumer " << prop.gps << endl);
	mtb -> put ((char *) ttb, block_size, &prop);
      }
    }
  return NULL;
}


void *
trender_c::saver ()
{
  int nb;

  for (;;)
    {
      nb = tb -> get (saver_cnum);
      {
	if (! tb -> block_prop (nb) -> bytes)
	  break;

	trend_block_t *blk;

	blk = (trend_block_t *) tb -> block_ptr (nb);

	*fout << "block " << nb << "gps=" << tb -> block_prop (nb) -> prop.gps << endl;
	for (int j = 0; j < num_channels; j++)
	  {
	    *fout << channels [j].name << endl;
	    *fout << "min=" << blk [j].min.I << endl;
	    *fout << "max=" << blk [j].max.I << endl;
#ifndef NO_RMS
	    *fout << "rms=" << blk [j].rms << endl;
#endif
	  }
      }
      tb -> unlock (saver_cnum);
    } 
  DEBUG1(cerr << "trender saver EOF" << endl);
  this -> shutdown_trender ();
  return NULL;
}

void *
trender_c::framer ()
{

#if defined(USE_FRAMECPP) || FRAMECPP_DATAFORMAT_VERSION >= 6

#ifndef VMICRFM_PRODUCER
  // Put this thread into the realtime scheduling class with half the priority
  daqd_c::realtime ("trend framer", 2);
#endif

#if FRAMECPP_DATAFORMAT_VERSION >= 6
  FrameCPP::Version::FrameH *frame = 0;
  FrameCPP::Version::FrRawData raw_data ("");
#elif FRAMECPP_DATAFORMAT_VERSION > 4
  FrameCPP::Version_6::FrameH* frame = 0;
  FrameCPP::Version_6::FrRawData raw_data ("");
  myImageFrameWriter *fw = 0;
#else  
  FrameCPP::Frame *frame = 0;
  FrameCPP::RawData raw_data ("");
  FrameCPP::ImageFrameWriter *fw = 0;
#endif

#if FRAMECPP_DATAFORMAT_VERSION < 6
  strstream *ost;
#endif
  int frame_length_blocks;
  
  /* Circular buffer length determines trend frame length */
  frame_length_blocks = tb -> blocks ();

  /* Detector data */
#if FRAMECPP_DATAFORMAT_VERSION > 4
  FrameCPP::Version::FrDetector detector = daqd.getDetector1();
#else
  FrameCPP::Detector detector (daqd.detector_name,
			       FrameCPP::Location (daqd.detector_longitude_degrees,
						   daqd.detector_longitude_minutes,
						   daqd.detector_longitude_seconds,
						   daqd.detector_latitude_degrees,
						   daqd.detector_latitude_minutes,
						   daqd.detector_latitude_seconds),
			       daqd.detector_elevation,
			       daqd.detector_arm_x_azimuth,
			       daqd.detector_arm_y_azimuth);
#endif

  // Create trend frame
  //
#if FRAMECPP_DATAFORMAT_VERSION > 4
  try {
    frame = new FrameCPP::Version::FrameH ("LIGO",
					     0, // run number ??? buffptr -> block_prop (nb) -> prop.run;
					     1, // frame number
					     FrameCPP::Version_6::GPSTime (0, 0),
					     0, // localTime
					     frame_length_blocks // dt
					     );
#if FRAMECPP_DATAFORMAT_VERSION >= 6
    frame -> SetRawData (raw_data);
    frame -> RefDetectProc ().append (detector);

#else
    frame -> setRawData (raw_data);
    frame -> refDetectorProc ().append (detector);
#endif
    // Append second detector if it is defined
    if (daqd.detector_name1.length() > 0) {
#if FRAMECPP_DATAFORMAT_VERSION >= 6
      FrameCPP::Version::FrDetector detector1 = daqd.getDetector2();
      frame -> RefDetectProc ().append (detector1);
#else
      FrameCPP::Version_6::FrDetector detector1 = daqd.getDetector2();
      frame -> refDetectorProc ().append (detector1);
#endif
    }
  } catch (...) {
    system_log(1, "Couldn't create trend frame");
    this -> shutdown_trender ();
    return NULL;
  }
#else
  try {
    frame = new FrameCPP::Frame ("LIGO",
				 0, // run number ??? buffptr -> block_prop (nb) -> prop.run;
				 1, // frame number
				 FrameCPP::Time (0, 0),
				 0,
				 0, // localTime
				 frame_length_blocks // dt
				 );
    frame -> setRawData (raw_data);
    frame -> setDetectProc (detector);
  } catch (...) {
    system_log(1, "Couldn't create trend frame");
    this -> shutdown_trender ();
    return NULL;
  }
#endif  

  // Keep pointers to the data samples for each data channel
#if FRAMECPP_DATAFORMAT_VERSION >= 6
  unsigned
#endif
	char *adc_ptr [num_trend_channels];
  //INT_2U data_valid [num_trend_channels];
  INT_2U *data_valid_ptr [num_trend_channels];

  // Create  ADCs
  try {
    for (int i = 0; i < num_trend_channels; i++) {
      REAL_8 junk [16 * 1024];
#if FRAMECPP_DATAFORMAT_VERSION >= 6
      FrameCPP::Version::FrAdcData adc
	= FrameCPP::Version::FrAdcData (std::string(trend_channels [i].name),
					  trend_channels [i].group_num,
					  i, // channel ???
					  CHAR_BIT * trend_channels [i].bps,
					  trend_channels [i].sample_rate,
					  trend_channels [i].signal_offset,
					  trend_channels [i].signal_slope,
					  std::string(trend_channels [i].signal_units),
					  .0,
					  0,
					  0,
					  .0);
      frame -> GetRawData () -> RefFirstAdc ().append (adc);

      FrameCPP::Version::Dimension  dims [1] = { FrameCPP::Version::Dimension (frame_length_blocks, 1. / trend_channels [i].sample_rate, "") };
      switch (trend_channels [i].data_type) { 
      case _64bit_double:
	{
	  FrameCPP::Version::FrVect vect("", 1, dims, (REAL_8 *) junk, "");
	  frame -> GetRawData () -> RefFirstAdc () [i] -> RefData().append (vect);
          bool owns;
          adc_ptr[i] = frame -> GetRawData () -> RefFirstAdc () [i] -> RefData()[0] -> SteelData(owns);
	  break;
	}
      case _32bit_float: 
	{
	  FrameCPP::Version::FrVect vect("", 1, dims, (REAL_4 *) junk, "");
	  frame -> GetRawData () -> RefFirstAdc () [i] -> RefData().append (vect);
          bool owns;
          adc_ptr[i] = frame -> GetRawData () -> RefFirstAdc () [i] -> RefData()[0] -> SteelData(owns);
	  break;
	}
      case _32bit_integer:
	{
	  FrameCPP::Version::FrVect vect("", 1, dims, (INT_4S *) junk, "");
	  frame -> GetRawData () -> RefFirstAdc () [i] -> RefData().append (vect);
          bool owns;
          adc_ptr[i] = frame -> GetRawData () -> RefFirstAdc () [i] -> RefData()[0] -> SteelData(owns);
	  break;
	}
      default:
	{
	  abort();
	  FrameCPP::Version::FrVect vect("", 1, dims, (INT_2S *) junk, "");
	  frame -> GetRawData () -> RefFirstAdc () [i] -> RefData().append (vect);
          bool owns;
          adc_ptr[i] = frame -> GetRawData () -> RefFirstAdc () [i] -> RefData()[0] -> SteelData(owns);
	  break;
	}
      }
#elif FRAMECPP_DATAFORMAT_VERSION > 4
      FrameCPP::Version_6::FrAdcData adc

	= FrameCPP::Version_6::FrAdcData (trend_channels [i].name,
					  trend_channels [i].group_num,
					  i, // channel ???
					  CHAR_BIT * trend_channels [i].bps,
					  trend_channels [i].sample_rate,
					  trend_channels [i].signal_offset,
					  trend_channels [i].signal_slope,
					  trend_channels [i].signal_units,
					  .0,
					  FrameCPP::Version_6::GPSTime( 0, 0 ),
					  0,
					  .0);

      frame -> getRawData () -> refAdc ().append (adc);

      FrameCPP::Version_6::Dimension  dims [1] = { FrameCPP::Version_6::Dimension (frame_length_blocks, 1. / trend_channels [i].sample_rate, "") };
      switch (trend_channels [i].data_type) { 
      case _64bit_double:
	{
	  FrameCPP::Version_6::FrVect vect("", 1, dims, (REAL_8 *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      case _32bit_float: 
	{
	  FrameCPP::Version_6::FrVect vect("", 1, dims, (REAL_4 *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      case _32bit_integer:
	{
	  FrameCPP::Version_6::FrVect vect("", 1, dims, (INT_4S *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      default:
	{
	  abort();
	  FrameCPP::Version_6::FrVect vect("", 1, dims, (INT_2S *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      }
#else
      FrameCPP::AdcData adc 
	= FrameCPP::AdcData (trend_channels [i].name, // channel name
			     trend_channels [i].group_num,
			     i, // channel ???
			     CHAR_BIT * trend_channels [i].bps, // nBits
			     trend_channels [i].sample_rate,
			     trend_channels [i].signal_offset,
			     trend_channels [i].signal_slope,
			     trend_channels [i].signal_units);

      frame -> getRawData () -> refAdc ().append (adc);

      FrameCPP::Dimension  dims [1] = { FrameCPP::Dimension (frame_length_blocks, 1. / trend_channels [i].sample_rate, "") };
      switch (trend_channels [i].data_type) { 
      case _64bit_double:
	{
	  FrameCPP::Vect vect("", 1, dims, (REAL_8 *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      case _32bit_float: 
	{
	  FrameCPP::Vect vect("", 1, dims, (REAL_4 *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      case _32bit_integer:
	{
	  FrameCPP::Vect vect("", 1, dims, (INT_4S *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      default:
	{
	  abort();
	  FrameCPP::Vect vect("", 1, dims, (INT_2S *) junk, "");
	  frame -> getRawData () -> refAdc () [i] -> refData().append (vect);
	  break;
	}
      }
#endif
    }
  } catch (...) {
    system_log(1, "Couldn't create one or several adcs");

    abort();

    this -> shutdown_trender ();
    return NULL;
  }

#if FRAMECPP_DATAFORMAT_VERSION >= 6
  
  sem_post (&frame_saver_sem);

  long frame_cntr;
  for (frame_cntr = 0;; frame_cntr++)
    {
      int eof_flag = 0;
      circ_buffer_block_prop_t file_prop;
      trend_block_t cur_blk [max_trend_output_channels];

      // Accumulate frame adc data
      for (int i = 0; i < frame_length_blocks; i++)
	{
	  int nb = tb -> get (saver_cnum);
	  DEBUG(3, cerr << "trender saver; block " << nb << endl);
	  TNF_PROBE_1(trender_c_framer_start, "trender_c::framer",
		      "got one block",
		      tnf_long,   block_number,    nb);
	  {
	    if (! tb -> block_prop (nb) -> bytes)
	      {
		tb -> unlock (saver_cnum);
		eof_flag = 1;
		DEBUG1(cerr << "trender framer EOF" << endl);
		break; // out of the for() loop
	      }
	    if (! i)
	      file_prop = tb -> block_prop (nb) -> prop;
	      
	    memcpy (cur_blk, tb -> block_ptr (nb), num_channels * sizeof (trend_block_t));
	  }
	  TNF_PROBE_0(trender_c_framer_end, "trender_c::framer", "end of block processing");
	  tb -> unlock (saver_cnum);
	  
	  // Check if the gps time isn't aligned on a minute boundary
	  if (! i) {
	    unsigned long gps_mod = file_prop.gps % 60;
	    if ( gps_mod ) {
	      // adjust time to make files aligned on gps mod 60
	      file_prop.gps -= gps_mod;
	      i += gps_mod;

              if (file_prop.cycle > gps_mod*16)
                file_prop.cycle -= gps_mod*16;
              else
                file_prop.cycle = 0;


#if 1
	      // zero out some data that's missing
	      for (int j = 0; j < num_trend_channels; j++)
		memset ( adc_ptr [j], 0, gps_mod * trend_channels [j].bps );
#endif
	    }

	  }

#if 1
	  for (int j = 0; j < num_channels; j++)
	    {
	      if (channels [j].data_type == _64bit_double) {
		memcpy(adc_ptr [5*j] + i*sizeof(REAL_8), &cur_blk [j].min.D, sizeof (REAL_8));
		memcpy(adc_ptr [5*j+1] + i*sizeof(REAL_8), &cur_blk [j].max.D, sizeof (REAL_8));
	      } else if (channels [j].data_type == _32bit_float) {
		memcpy(adc_ptr [5*j] + i*sizeof(REAL_4), &cur_blk [j].min.F, sizeof (REAL_4));
		memcpy(adc_ptr [5*j+1] + i*sizeof(REAL_4), &cur_blk [j].max.F, sizeof (REAL_4));
	      } else {
		memcpy(adc_ptr [5*j] + i*sizeof(INT_4S), &cur_blk [j].min.I, sizeof (INT_4S));
		memcpy(adc_ptr [5*j+1] + i*sizeof(INT_4S), &cur_blk [j].max.I, sizeof (INT_4S));
	      }
	      //cerr << j << "\t" << i << hex << (void*)(adc_ptr[5*j+2]) << endl;
	      memcpy(adc_ptr [5*j+2] + i*sizeof(INT_4S), &cur_blk [j].n, sizeof (INT_4S));
	      //adc_ptr [5*j+2][4*i]=0;
#ifdef not_def
	      REAL_8 rms = sqrt(cur_blk [j].rms);
	      memcpy(adc_ptr [5*j+3] + i*sizeof(REAL_8), &rms, sizeof (REAL_8));
#endif
	      memcpy(adc_ptr [5*j+3] + i*sizeof(REAL_8), &cur_blk [j].rms, sizeof (REAL_8));
	      memcpy(adc_ptr [5*j+4] + i*sizeof(REAL_8), &cur_blk [j].mean, sizeof (REAL_8));
	    }
#endif
	}

      if (eof_flag)
	break; // out of the for() loop

      //fw -> setFrameFileAttributes (file_prop.run, file_prop.cycle / 16 / 60, 0,
				    //file_prop.gps, 60, file_prop.gps_n,
				    //file_prop.leap_seconds, file_prop.altzone);

      frame -> SetGTime(FrameCPP::Version::GPSTime (file_prop.gps, file_prop.gps_n));

      DEBUG(1, cerr << "about to write second trend frame @ " << file_prop.gps << endl);

      time_t file_gps;
      time_t file_gps_n;
      int dir_num = 0;

      char tmpf [filesys_c::filename_max + 10];
      char _tmpf [filesys_c::filename_max + 10];

      file_gps = file_prop.gps;
      file_gps_n = file_prop.gps_n;
      dir_num = fsd.getDirFileNames (file_gps, _tmpf, tmpf, 1, 60);

      int fd = creat (_tmpf, 0644);
      if (fd < 0) {
	system_log(1, "Couldn't open full trend frame file `%s' for writing; errno %d", tmpf, errno);
	fsd.report_lost_frame ();
	daqd.set_fault ();
      } else {
	DEBUG(3, cerr << "`" << _tmpf << "' opened" << endl);
#if 0
#if defined(DIRECTIO_ON) && defined(DIRECTIO_OFF)
	if (daqd.do_directio) directio (fd, DIRECTIO_ON);
#endif
#endif
	TNF_PROBE_1(trender_c_framer_frame_write_start, "trender_c::framer",
		    "frame write",
		    tnf_long,   frame_number,   frame_cntr);

#if 0
	if (daqd.cksum_file != "") {
	  daqd_c::fr_cksum(daqd.cksum_file, tmpf, (unsigned char *)(ost -> str ()), image_size);
	}
#endif
	  
	  close (fd);
          FrameCPP::Common::FrameBuffer<filebuf>* obuf
            = new FrameCPP::Common::FrameBuffer<std::filebuf>(std::ios::out);
          obuf -> open(_tmpf, std::ios::out | std::ios::binary);
          FrameCPP::Common::OFrameStream  ofs(obuf);
          ofs.SetCheckSumFile(FrameCPP::Common::CheckSum::CRC);
          DEBUG(1, cerr << "Begin second trend WriteFrame()" << endl);
          time_t t = time(0);
          ofs.WriteFrame(*(frame), //FrameCPP::Common::CheckSum::NONE,
                        daqd.no_compression? FrameCPP::FrVect::RAW:
				 FrameCPP::FrVect::ZERO_SUPPRESS_OTHERWISE_GZIP, 1,
                        FrameCPP::Common::CheckSum::CRC);
          t = time(0) - t;
          DEBUG(1, cerr << "Done in " << t << " seconds" << endl);
          ofs.Close();
          obuf->close();

	  if (rename(_tmpf, tmpf)) {
		system_log(1, "framer(): failed to rename file; errno %d", errno);
		fsd.report_lost_frame ();
		daqd.set_fault ();
	  } else {
	  	DEBUG(3, cerr << "trend frame " << frame_cntr << " is written out" << endl);
	  	// Successful frame write
	  	fsd.update_dir (file_gps, file_gps_n, frame_length_blocks, dir_num);
	  }
	TNF_PROBE_0(trender_c_framer_frame_write_end, "trender_c::framer", "frame write");
      }

#if EPICS_EDCU == 1
      /* Epics display: second trend data look back size in seconds */
      extern unsigned int pvValue[1000];
      pvValue[9] = fsd.get_max() - fsd.get_min();

      /* Epics display: current trend frame directory */
      extern unsigned int pvValue[1000];
      pvValue[10] = fsd.get_cur_dir();
#endif
    }
#else
  try {
#if FRAMECPP_DATAFORMAT_VERSION > 4
    fw = new myImageFrameWriter (*(ost = new strstream ()));
#else
    fw = new FrameCPP::ImageFrameWriter (*(ost = new strstream ()));
#endif
    fw -> writeFrame (*frame);
    fw -> close ();
  } catch (write_failure) {
    system_log(1, "Couldn't create image frame writer; write_failure");
    
    abort();
    
    this -> shutdown_trender ();
    return NULL;
  }
  
  int image_size = ost -> pcount ();

  // Frame itself is not needed any more and may be freed
  delete frame;
  frame = 0;


#if 0
  // Finally, prepare an array of pointers to the ADC data
  char *adc_ptr [num_trend_channels];
  INT_2U *data_valid_ptr [num_trend_channels];
#endif

  {
#if FRAMECPP_DATAFORMAT_VERSION > 4
    typedef myImageFrameWriter::OMI OMI;
#else
    typedef FrameCPP::ImageFrameWriter::OMI OMI;
#endif
    for (int i = 0; i < num_trend_channels; i++) {
      INT_8U offs = 
	fw -> adcNamePositionMap [trend_channels [i].name].first[0];
      INT_4U classInstance = fw -> adcDataVectorPtr (offs);
      INT_2U instance = classInstance  & 0xffff;
      OMI v = fw -> vectInstanceOffsetMap.find (instance);
#if 0
#ifndef NDEBUG
      cout << "trend vector " << (classInstance >> 16) << "\t" << (classInstance&0xffff) << endl;
      if (v == fw -> vectInstanceOffsetMap.end()) {
	cout << "trend vector " << instance << " not found!" << endl;
	exit(1);
      } else {
       cout << "data vector offset is " << v -> second[0] << endl;
      }
#endif
#endif
      adc_ptr [i] = fw->vectorData (v -> second[0]);
      data_valid_ptr [i] = fw->adcDataValidPtr (offs);
    }
  }
  
  sem_post (&frame_saver_sem);

  long frame_cntr;
  for (frame_cntr = 0;; frame_cntr++)
    {
      int eof_flag = 0;
      circ_buffer_block_prop_t file_prop;
      trend_block_t cur_blk [max_trend_output_channels];

      // Accumulate frame adc data
      for (int i = 0; i < frame_length_blocks; i++)
	{
	  int nb = tb -> get (saver_cnum);
	  DEBUG(3, cerr << "trender saver; block " << nb << endl);
	  TNF_PROBE_1(trender_c_framer_start, "trender_c::framer",
		      "got one block",
		      tnf_long,   block_number,    nb);
	  {
	    if (! tb -> block_prop (nb) -> bytes)
	      {
		tb -> unlock (saver_cnum);
		eof_flag = 1;
		DEBUG1(cerr << "trender framer EOF" << endl);
		break; // out of the for() loop
	      }
	    if (! i)
	      file_prop = tb -> block_prop (nb) -> prop;
	      
	    memcpy (cur_blk, tb -> block_ptr (nb), num_channels * sizeof (trend_block_t));
	  }
	  TNF_PROBE_0(trender_c_framer_end, "trender_c::framer", "end of block processing");
	  tb -> unlock (saver_cnum);
	  
	  // Check if the gps time isn't aligned on a minute boundary
	  if (! i) {
	    unsigned long gps_mod = file_prop.gps % 60;
	    if ( gps_mod ) {
	      // adjust time to make files aligned on gps mod 60
	      file_prop.gps -= gps_mod;
	      i += gps_mod;

              if (file_prop.cycle > gps_mod*16)
                file_prop.cycle -= gps_mod*16;
              else
                file_prop.cycle = 0;


	      // zero out some data that's missing
	      for (int j = 0; j < num_trend_channels; j++)
		memset ( adc_ptr [j], 0, gps_mod * trend_channels [j].bps );
	    }

	  }

	  for (int j = 0; j < num_channels; j++)
	    {
	      if (channels [j].data_type == _64bit_double) {
		memcpy(adc_ptr [5*j] + i*sizeof(REAL_8), &cur_blk [j].min.D, sizeof (REAL_8));
		memcpy(adc_ptr [5*j+1] + i*sizeof(REAL_8), &cur_blk [j].max.D, sizeof (REAL_8));
	      } else if (channels [j].data_type == _32bit_float) {
		memcpy(adc_ptr [5*j] + i*sizeof(REAL_4), &cur_blk [j].min.F, sizeof (REAL_4));
		memcpy(adc_ptr [5*j+1] + i*sizeof(REAL_4), &cur_blk [j].max.F, sizeof (REAL_4));
	      } else {
		memcpy(adc_ptr [5*j] + i*sizeof(INT_4S), &cur_blk [j].min.I, sizeof (INT_4S));
		memcpy(adc_ptr [5*j+1] + i*sizeof(INT_4S), &cur_blk [j].max.I, sizeof (INT_4S));
	      }
	      //cerr << j << "\t" << i << hex << (void*)(adc_ptr[5*j+2]) << endl;
	      memcpy(adc_ptr [5*j+2] + i*sizeof(INT_4S), &cur_blk [j].n, sizeof (INT_4S));
	      //adc_ptr [5*j+2][4*i]=0;
#ifdef not_def
	      REAL_8 rms = sqrt(cur_blk [j].rms);
	      memcpy(adc_ptr [5*j+3] + i*sizeof(REAL_8), &rms, sizeof (REAL_8));
#endif
	      memcpy(adc_ptr [5*j+3] + i*sizeof(REAL_8), &cur_blk [j].rms, sizeof (REAL_8));
	      memcpy(adc_ptr [5*j+4] + i*sizeof(REAL_8), &cur_blk [j].mean, sizeof (REAL_8));
	    }
	}

      if (eof_flag)
	break; // out of the for() loop

      fw -> setFrameFileAttributes (file_prop.run, file_prop.cycle / 16 / 60, 0,
				    file_prop.gps, 60, file_prop.gps_n,
				    file_prop.leap_seconds, file_prop.altzone);

      time_t file_gps;
      time_t file_gps_n;
      int dir_num = 0;

      char tmpf [filesys_c::filename_max + 10];
      char _tmpf [filesys_c::filename_max + 10];

      file_gps = file_prop.gps;
      file_gps_n = file_prop.gps_n;
      dir_num = fsd.getDirFileNames (file_gps, _tmpf, tmpf, 1, 60);

      int fd = creat (_tmpf, 0644);
      if (fd < 0) {
	system_log(1, "Couldn't open full trend frame file `%s' for writing; errno %d", tmpf, errno);
	fsd.report_lost_frame ();
	daqd.set_fault ();
      } else {
	DEBUG(3, cerr << "`" << _tmpf << "' opened" << endl);
#if 0
#if defined(DIRECTIO_ON) && defined(DIRECTIO_OFF)
	if (daqd.do_directio) directio (fd, DIRECTIO_ON);
#endif
#endif
	TNF_PROBE_1(trender_c_framer_frame_write_start, "trender_c::framer",
		    "frame write",
		    tnf_long,   frame_number,   frame_cntr);

	if (daqd.cksum_file != "") {
	  daqd_c::fr_cksum(daqd.cksum_file, tmpf, (unsigned char *)(ost -> str ()), image_size);
	}
	  
	/* Write out a frame */
	int nwritten = write (fd, ost -> str (), image_size);
        if (nwritten == image_size) {
	  if (rename(_tmpf, tmpf)) {
		system_log(1, "framer(): failed to rename file; errno %d", errno);
		fsd.report_lost_frame ();
		daqd.set_fault ();
	  } else {
	  	DEBUG(3, cerr << "trend frame " << frame_cntr << " is written out" << endl);
	  	// Successful frame write
	  	fsd.update_dir (file_gps, file_gps_n, frame_length_blocks, dir_num);
	  }
	} else {
	  system_log(1, "framer(): failed to write trend frame out; errno %d", errno);
	  fsd.report_lost_frame ();
	  daqd.set_fault ();
	}
	close (fd);
	TNF_PROBE_0(trender_c_framer_frame_write_end, "trender_c::framer", "frame write");
      }

#if EPICS_EDCU == 1
      /* Epics display: second trend data look back size in seconds */
      extern unsigned int pvValue[1000];
      pvValue[9] = fsd.get_max() - fsd.get_min();

      /* Epics display: current trend frame directory */
      extern unsigned int pvValue[1000];
      pvValue[10] = fsd.get_cur_dir();
#endif
    }
#endif

  // signal to the producer that we are done
  this -> shutdown_trender ();
#endif
  return NULL;
}

inline void
trender_c::trend_loop_func(int j, int* status_ptr, char* block_ptr)  {
  int bad = *(status_ptr + channels [j].seq_num * 17);
  if ( bad && daqd.zero_bad_data) {
    // No data is available for this channels for this second.
    // Mark trend for this channels as bad setting number of points available to zero.
    // Reset all the rest of the data also.
    memset (&ttb [j], 0, sizeof(trend_block_t));
  } else {
    ttb [j].n = channels [j].sample_rate;
#ifndef NO_SLOW_CHANNELS
    if (channels [j].slow) {
#ifndef NO_RMS
      ttb [j].rms = 
#endif
	ttb [j].mean =
	ttb [j].min.F = ttb [j].max.F = *(float *) (block_ptr + channels [j].offset);
    } else
#endif // NO_SLOW_CHANNELS
      {
	int rate = channels [j].sample_rate;
	if (channels [j].data_type == _64bit_double) {
	  register double *data = (double *) (block_ptr + channels [j].offset);
	  
#ifndef NO_RMS
	  register double rms = (double) (*data * *data);
#endif
	  register double mean = *data;
	  register double min = *data;
	  register double max = *data;
	  
	  for (register int i = 1; i < rate; i++)
	    {
	      register double test = data [i];
#ifndef NO_RMS
	      rms += (test * test);
#endif
	      mean += test;
	      if (test < min)
		min = test;
	      if (test > max)
		max = test;
	    }
#ifndef NO_RMS
	   ttb [j].rms = sqrt (rms / (double)rate);
#endif
	   ttb [j].mean = mean / (double)rate;
	   ttb [j].min.D = min;
	   ttb [j].max.D = max;
	} else if (channels [j].data_type == _32bit_float && channels [j].bps == 4) {
	  int *data = (int *) (block_ptr + channels [j].offset);
	  float d;

	  // is not compiled correctly with the new compiler
	  //*((int *)&d) = *data;
	  // Replacing the above with the float assignment fixes the problem!
	  d = *((float *)(block_ptr + channels [j].offset));

#ifndef NO_RMS
	  double rms = (double) (d * d) ;
#endif
	  double mean = d;
 	  float min = d;
	  float max = d;
	  
	  for (int i = 1; i < rate; i++)
	    {
	      float test;
	      //*((int *)&test) = data [i];
	      test = *((float *)(data + i));
#ifndef NO_RMS
	      rms += (test * test);
#endif
	      mean += test;
	      if (test < min)
		min = test;
	      if (test > max)
		max = test;
	    }
#ifndef NO_RMS
	   ttb [j].rms = sqrt (rms / (double)rate);
#endif
	   ttb [j].mean = mean / (double)rate;
	   ttb [j].min.F = min;
	   ttb [j].max.F = max;
	} else if (channels [j].data_type == _32bit_float && channels [j].bps == 8) { /* Complex data */
	  register float *data = (float *) (block_ptr + channels [j].offset);
	  double test = sqrt(data[0]*data[0] + data[1]*data[1]);
	  data += 2;
#ifndef NO_RMS
	  register double rms = test * test;
#endif
	  register double mean = test;
	  register float min = test;
	  register float max = test;
	  
	  /* Complex data trend are calculated on magnitude */
	  for (register int i = 1; i < rate; i++)
	    {
	      test = data[0]*data[0] + data[1]*data[1]; /* Magnitude squared */
	      data += 2;
#ifndef NO_RMS
	      rms += test;
#endif
	      test = sqrt(test); /* Magnitude */
	      mean += test;
	      if (test < min)
		min = test;
	      if (test > max)
		max = test;
	    }
#ifndef NO_RMS
	   ttb [j].rms = sqrt (rms / (double)rate);
#endif
	   ttb [j].mean = mean / (double)rate;
	   ttb [j].min.F = min;
	   ttb [j].max.F = max;
	} else {
	  register short *data = (short *) (block_ptr + channels [j].offset);
	  
#ifndef NO_RMS
	  register double rms = (double) (*data * *data);
#endif
	  register double mean = *data;
	  register short min = *data;
	  register short max = *data;
	  
	  for (register int i = 1; i < rate; i++)
	    {
	      register short test = data [i];
#ifndef NO_RMS
	      rms += (double) (test * test);
#endif
	      mean += test;
	      if (test < min)
		min = test;
	      if (test > max)
		max = test;
	    }
#ifndef NO_RMS
	   ttb [j].rms = sqrt (rms / (double)rate);
#endif
	   ttb [j].mean = mean / (double)rate;
	   ttb [j].min.I = min;
	   ttb [j].max.I = max;
	}
      }
  }
}

#define WORKPILE_INC 10

void *
trender_c::trend ()
{
#ifndef VMICRFM_PRODUCER
  // Put this thread into the realtime scheduling class with half the priority
  daqd_c::realtime ("trender", 2);
#endif

  int nb;
  circ_buffer *ltb = this -> tb;
  sem_post (&trender_sem);

#ifdef not_def
  // Find out how to split the work with worker thread
  {
    unsigned long trend_load_size = 0;
    // Calculate summary trender data load
    for (unsigned int  i = 0; i < num_channels; i++)
      trend_load_size += channels[i].bytes;

    system_log(1, "Trend's work load is %d bytes", trend_load_size);
    unsigned long bsize = trend_load_size / 2;
    unsigned long load_size = 0;
    for (worker_first_channel = 0; worker_first_channel < num_channels; worker_first_channel++) {
      load_size += channels[worker_first_channel].bytes;
      if (load_size > bsize)
	break;
    }
    system_log(1, "Trend Worker's first channel is #%d (%s)", worker_first_channel, channels[worker_first_channel].name);
  }
#endif

  for (;;)
    {
      circ_buffer_block_prop_t prop;

#ifdef not_def
      pthread_mutex_lock (&lock);
      ltb = this -> tb;
      pthread_mutex_unlock (&lock);

      if (ltb) /* There is a trend buffer, so we need to process data */
#endif

	{
	  // Request to shut us down received from consumer
	  // 
	  if (shutdown_now)
	    {
	      // FIXME -- synchronize with the demise of the minute trender
	      shutdown_buffer ();
	      continue;
	    }

	  nb = daqd.b1 -> get (cnum);
	  TNF_PROBE_1(trender_c_trend_start, "trender_c::trend",
		      "got one block",
		      tnf_long,   block_no,    nb);
	  {
	    char *block_ptr = daqd.b1 -> block_ptr (nb);
	    unsigned long block_bytes = daqd.b1 -> block_prop (nb) -> bytes;

	    // Shutdown request from the main circ buffer
	    if (! block_bytes )
	      {
		char pbuf [1];
		ltb -> put (pbuf, 0);
		return NULL;
	      }

	    pthread_mutex_lock (&worker_lock);
	    trend_worker_nb = nb;
	    next_trender_block = 0;
	    next_worker_block = num_channels;
	    worker_busy = 1;
	    pthread_mutex_unlock (&worker_lock);
	    pthread_cond_signal (&worker_notempty);

	    // This points to the start of status word area
	    int *status_ptr = (int *) (block_ptr + daqd.block_size)
			- 17*daqd.num_channels;

	    TNF_PROBE_1(trender_c_trend_start, "trender_c::trend",
			"start of trender thread channel processing",
			tnf_long,   block_no,    nb);
	    
	    for (bool finished = false;;) {
	      pthread_mutex_lock (&worker_lock);
	      int first_block = next_trender_block;
	      if (next_trender_block >= next_worker_block) {
		finished = true;
	      } else  {
		next_trender_block += WORKPILE_INC;
                if (next_trender_block > num_channels)
		  next_trender_block = num_channels;

	        if (next_trender_block > next_worker_block) 
		  next_trender_block = next_worker_block;
	      }
	      pthread_mutex_unlock (&worker_lock);	    

	      if (finished) {
		break;
	      }

	      // Calculate MIN, MAX and RMS for all channels
	      for (register int j = first_block; j < next_trender_block; j++)
		trend_loop_func(j, status_ptr, block_ptr);
	    }

	    TNF_PROBE_1(trender_c_trend_end, "trender_c::trender", "end of trender thread channel processing",
			tnf_long,   block_timestamp,    prop.gps);

            prop = daqd.b1 -> block_prop (nb) -> prop;

	    // wait for worker thread
	    pthread_mutex_lock (&worker_lock);
	    while (worker_busy)
	      pthread_cond_wait (&worker_done, &worker_lock);
	    pthread_mutex_unlock (&worker_lock);

	    DEBUG(3, cerr << "trender finished; next_trender_block=" << next_trender_block << "; next_worker_block=" << next_worker_block << endl);
	  }
	  TNF_PROBE_1(trender_c_trend_end, "trender_c::trender", "end of block processing",
		      tnf_long,   block_timestamp,    prop.gps);
	  daqd.b1 -> unlock (cnum);

	  DEBUG(3, cerr << "trender consumer " << prop.gps << endl);

#ifdef not_def
	  for (register int j = 0; j < num_channels; j++) {
	    ttb [j].min.F = ttb [j].max.F = j;
	    ttb [j].rms = j;
	  }
#endif
	  ltb -> put ((char *) ttb, block_size, &prop);

	}
#ifdef not_def
      else /* Do stuff required for the synchronization */
	daqd.b1 -> noop (cnum);
#endif
    }

  return NULL;
}


void *
trender_c::trend_worker ()
{
  for (;;)
    {
      // get control from the trender thread
      pthread_mutex_lock (&worker_lock);
      while (!worker_busy)
	pthread_cond_wait (&worker_notempty, &worker_lock);
      int nb = trend_worker_nb;
      pthread_mutex_unlock (&worker_lock);

      char *block_ptr = daqd.b1 -> block_ptr (nb);
      
      // This points to the start of status word area
      int *status_ptr = (int *) (block_ptr + daqd.block_size) - 17*daqd.num_channels;

      TNF_PROBE_1(trender_c_trend_start, "trender_c::trend",
		  "start of trend worker thread channel processing",
		  tnf_long,   block_no,    nb);

      // Calculate MIN, MAX and RMS for all channels
      for (bool finished = false;;) {
	pthread_mutex_lock (&worker_lock);
	int last_block = next_worker_block;
	if (next_trender_block >= next_worker_block)
	  finished = true;
	else {
	  if (next_worker_block < WORKPILE_INC)
	    next_worker_block = 0;
	  else
	    next_worker_block -= WORKPILE_INC;
	  if (next_worker_block < next_trender_block) 
            next_worker_block=next_trender_block;
	}
	pthread_mutex_unlock (&worker_lock);	    
	
	if (finished)
	  break;
	
	// Calculate MIN, MAX and RMS for all channels
	for (register int j = next_worker_block; j < last_block; j++)
	  trend_loop_func(j, status_ptr, block_ptr);

	if (next_worker_block == 0) // finished
		break;
      }

      TNF_PROBE_0(trender_c_trend_end, "trender_c::trender", "end of trend worker thread channel processing");

      // signal trender thread that we are finished
      pthread_mutex_lock (&worker_lock);
      worker_busy = 0;
      pthread_mutex_unlock (&worker_lock);
      pthread_cond_signal (&worker_done);
    }

  return NULL;
}


int 
trender_c::start_trend (ostream *yyout, int pframes_per_file, int pminute_frames_per_file,
			int ptrend_buffer_blocks, int pminute_trend_buffer_blocks, int pnum_threads = 1)
{
  if (this -> tb) {
    *yyout << "trend is already running" << endl;
    return 1;
  }

  /*
     Set trender operational parameters.
  */
  frames_per_file = pframes_per_file;
  minute_frames_per_file = pminute_frames_per_file;
  trend_buffer_blocks = ptrend_buffer_blocks;
  minute_trend_buffer_blocks = pminute_trend_buffer_blocks;
  num_threads = pnum_threads;

  // Allocate trend circular buffer
  {
    void *mptr = malloc (sizeof(circ_buffer));
    if (!mptr) {
      *yyout << "couldn't construct trend circular buffer, memory exhausted" << endl;
      return 1;
    }

#ifdef not_def
    if (! (tb = new circ_buffer (0, trend_buffer_blocks, block_size))) {
      *yyout << "couldn't construct trender circular buffer, memory exhausted" << endl;
      return 1;
    }
#endif

    tb = new (mptr) circ_buffer (0, trend_buffer_blocks, block_size);
    if (! (tb -> buffer_ptr ())) {
      tb -> ~circ_buffer();
      free ((void *) tb);
      tb = 0;
      *yyout << "couldn't allocate trender buffer data blocks, memory exhausted" << endl;
      return 1;
    }
  }

  // Allocate minute trend cicrular buffer
  {
    void *mptr = malloc (sizeof(circ_buffer));
    if (!mptr) {
      *yyout << "couldn't construct minute trend circular buffer, memory exhausted" << endl;
      return 1;
    }

    // `trend_buffer_blocks' gives minute trend buffer data block period
    mtb = new (mptr) circ_buffer (0, minute_trend_buffer_blocks, block_size, trend_buffer_blocks);
    if (! (mtb -> buffer_ptr ())) {
      tb -> ~circ_buffer();
      free ((void *) tb);
      tb = 0;
      *yyout << "couldn't allocate trender buffer data blocks, memory exhausted" << endl;
      return 1;
    }
  }

  // Start minute trender
  if ((mcnum = tb -> add_consumer ()) >= 0)
    {
      pthread_attr_t attr;
      pthread_attr_init (&attr);
      pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
      pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
      pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
      int err_no;
      if (err_no = pthread_create (&mconsumer, &attr, (void *(*)(void *))minute_trend_static, (void *) this)) {
	system_log(1, "couldn't create minute trender thread; pthread_create() err=%d", err_no);
	tb -> delete_consumer (mcnum);
	mcnum = 0;
	tb -> ~circ_buffer();
	free ((void *) tb);
	tb = 0;
	mtb -> ~circ_buffer();
	free ((void *) mtb);
	mtb = 0;
	pthread_attr_destroy (&attr);
	return 1;
      }
      pthread_attr_destroy (&attr);
      DEBUG(2, cerr << "minute trender created; tid=" << consumer << endl);
    }
  else
    {
      *yyout << "couldn't add minute trend consumer" << endl;
      tb -> ~circ_buffer();
      free ((void *) tb);
      tb = 0;
      mtb -> ~circ_buffer();
      free ((void *) mtb);
      mtb = 0;
      return 1;
    }

  // Start trender
  if ((cnum = daqd.b1 -> add_consumer ()) >= 0)
    {
      sem_wait (&trender_sem);

      pthread_attr_t attr;
      pthread_attr_init (&attr);
      pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
      pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
      pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
      int err_no;

      if (err_no = pthread_create (&worker_tid, &attr, (void *(*)(void *))trend_worker_static, (void *) this)) {
	system_log(1, "couldn't create trender worker thread; pthread_create() err=%d", err_no);
	abort();
      }

      if (err_no = pthread_create (&consumer, &attr, (void *(*)(void *))trend_static, (void *) this)) {
	system_log(1, "couldn't create trender thread; pthread_create() err=%d", err_no);

	// FIXME: have to cancel minute trend thread here first !!!
	abort();

	daqd.b1 -> delete_consumer (cnum);
	cnum = 0;
	tb -> ~circ_buffer();
	free ((void *) tb);
	tb = 0;
	mtb -> ~circ_buffer();
	free ((void *) mtb);
	mtb = 0;
	pthread_attr_destroy (&attr);
	return 1;
      }
      pthread_attr_destroy (&attr);
      DEBUG(2, cerr << "trender created; tid=" << consumer << endl);
    }
  else
    {
      *yyout << "couldn't add trend consumer" << endl;

      // FIXME: have to cancel minute trend thread here first !!!
      abort();

      tb -> ~circ_buffer();
      free ((void *) tb);
      tb = 0;
      mtb -> ~circ_buffer();
      free ((void *) mtb);
      mtb = 0;
      return 1;
    }

  return 0;
}

int
trender_c::start_raw_minute_trend_saver (ostream *yyout)
{
  if (!this -> tb) {
    *yyout << "please start trend first" << endl;
    return 1;
  }

  // Start raw minute trend saver
  assert (mtb);
  assert (tb);
  if ((raw_msaver_cnum = mtb -> add_consumer ()) < 0)
    {
      *yyout << "start_raw_trend_saver: too many minute trend consumers, saver was not started" << endl;
      return 1;
    }

  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

    /* Start minute trend frame saver thread */
  int err_no;
  if (err_no = pthread_create (&tsaver, &attr, (void *(*)(void *))daqd.trender.raw_minute_trend_saver_static,
			       (void *) this)) {
    system_log(1, "couldn't create raw minute trend framer thread; pthread_create() err=%d", err_no);

      // FIXME: have to cancel frame saver thread here
    abort();

    mtb -> delete_consumer (raw_msaver_cnum);
    raw_msaver_cnum = 0;
    pthread_attr_destroy (&attr);
    return 1;
  }
  pthread_attr_destroy (&attr);
  DEBUG(2, cerr << "raw minute trend saver thread started; tid=" << tsaver << endl);
  return 0;
}


int
trender_c::start_minute_trend_saver (ostream *yyout)
{
  if (!this -> tb) {
    *yyout << "please start trend first" << endl;
    return 1;
  }

  // Start minute trend saver
  {
    assert (mtb);
    assert (tb);
    if ((msaver_cnum = mtb -> add_consumer ()) < 0)
      {
	*yyout << "start_trend_saver: too many minute trend consumers, saver was not started" << endl;
	return 1;
      }

    sem_wait (&minute_frame_saver_sem);
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

    /* Start minute trend frame saver thread */
    int err_no;
    if (err_no = pthread_create (&tsaver, &attr, (void *(*)(void *))daqd.trender.minute_trend_framer_static,
				     (void *) this)) {
      system_log(1, "couldn't create minute trend framer thread; pthread_create() err=%d", err_no);

      // FIXME: have to cancel frame saver thread here
      abort();

      mtb -> delete_consumer (msaver_cnum);
      msaver_cnum = 0;
      pthread_attr_destroy (&attr);
      return 1;
    }
    pthread_attr_destroy (&attr);
    DEBUG(2, cerr << "minute trend saver thread started; tid=" << tsaver << endl);
  }

  return 0;
}


int
trender_c::start_trend_saver (ostream *yyout)
{
  if (!this -> tb) {
    *yyout << "please start trend first" << endl;
    return 1;
  }

  // Start trend saver
  assert (tb);
  if ((saver_cnum = tb -> add_consumer ()) < 0)
    {
      *yyout << "start_trend_saver: too many trend consumers, saver was not started" << endl;
      return 1;
    }

  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setstacksize (&attr, daqd.thread_stack_size);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

  if (ascii_output)
    {
      fout = new ofstream ("trend.data", ios::out);
      if (!fout) {
	*yyout << "failed to create file `trend.data'" <<  endl;
	return 1;
      }

      /* Start trend saver consumer thread */
      int err_no;
      if (err_no = pthread_create (&tsaver, &attr, (void *(*)(void *))daqd.trender.saver_static, (void *) this)) {
	system_log(1, "couldn't create ascii trend saver thread; pthread_create() err=%d", err_no);
	tb -> delete_consumer (saver_cnum);
	pthread_attr_destroy (&attr);
	return 1;
      }
      pthread_attr_destroy (&attr);
      DEBUG(2, cerr << "ASCII trend saver thread started; tid=" << tsaver << endl);
    }
  else
    {
      sem_wait (&frame_saver_sem);

      /* Start trend frame saver thread */
      int err_no;
      if (err_no = pthread_create (&tsaver, &attr, (void *(*)(void *))daqd.trender.trend_framer_static, (void *) this)) {
	system_log(1, "couldn't create trend framer thread; pthread_create() err=%d", err_no);
	tb -> delete_consumer (saver_cnum);
	pthread_attr_destroy (&attr);
	return 1;
      }
      pthread_attr_destroy (&attr);
      DEBUG(2, cerr << "trend saver thread started; tid=" << tsaver << endl);
    }

  return 0;
}



int
trender_c::kill_trend (ostream *yyout)
{
  char buf [1];
  circ_buffer *oldb;

#ifdef not_def

  if (! tb) 
    {
      *yyout << "trend is not running" << endl;
      return 1;
    }

  /* Put zero-length block in the queue
     and wait till the saver is done */
  tb -> put (buf, 0);
  pthread_join (tsaver, NULL);
  return 0;

#endif

  *yyout << "Not implemented" << endl;
  return 1;
}
