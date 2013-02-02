#include <stdio.h>
#include <arpa/inet.h>
#include "debug.h"
#include "plan.hh"
#include "mmstream.hh"


using namespace std;

static void error_watch (const string& msg) {
  fprintf(stderr,"framecpp error: %s", msg.c_str());
}

int nds_log_level=4; // Controls volume of log messages

const std::vector<std::string>
split(std::string value) throw()
{
  std::vector<std::string> res;
  for (;;) {
    int idx = value.find_first_of (" ");
    if (idx == std::string::npos) {
      if (value.size() > 0)
        res.push_back(value);
      break;
    }
    std::string sname = value.substr(0, idx);
    if (sname != " " && sname.size() > 0)
      res.push_back(sname);
    value = value.substr(idx+1);
  }
  return res;
}


main(int argc, char* argv[])
{
  if (argc < 3) {
    fputs("Usage: fracfg <frame file name> <adc name>\n", stderr);
    exit(1);
  }

  vector<string> frames;
  frames = split(argv[1]);

  vector<string> names;
  names = split(argv[2]);
  unsigned int num_signals = names.size();

  General::SharedPtr< FrameCPP::Version::FrRawData > rawData = General::SharedPtr< FrameCPP::Version::FrRawData > (new FrameCPP::Version::FrRawData);
  General::SharedPtr<FrameCPP::Version::FrameH> first_frame;
  plan *first_reader = 0;

  for (int fidx = 0; fidx < frames.size(); fidx++) {
    FrameCPP::Common::FrameBuffer<filebuf>* ibuf = new FrameCPP::Common::FrameBuffer<std::filebuf>(std::ios::in);
    ibuf -> open(frames[fidx], std::ios::in | std::ios::binary);

    plan *reader = 0; // frame reader
    try {
      reader = new plan(ibuf, first_reader);
      //reader = new plan(ibuf);
    } catch (...) {
      fputs("Failed to create frame reader\n",stderr);
      exit(1);
    }

    //reader->setErrorWatch(error_watch);
    try {
      if (fidx == 0) {
        first_frame = reader->ReadFrameH(0, 0);
        first_frame.get()->SetRawData(rawData);
	first_reader = reader;
      }

      for (int i = 0; i < num_signals; i++) {
        FrameCPP::Version::FrAdcData *adc = reader->ReadFrAdcData(0, names[i]).get();
        adc->RefData()[0]->Uncompress();
        first_frame.get()->GetRawData () -> RefFirstAdc ().erase(0);
        first_frame.get()->GetRawData () -> RefFirstAdc ().append (*adc);
        printf("Added %s\n", adc->GetName().c_str());
      }
    } catch (exception &e) {
            fprintf(stderr, "Failed to read first frame; %s\n", e.what());
    }
  }
}

/*
const INT_2U FR_VECT_C = 0;
const INT_2U FR_VECT_2S = 1;
const INT_2U FR_VECT_8R = 2;
const INT_2U FR_VECT_4R = 3;
const INT_2U FR_VECT_4S = 4;
const INT_2U FR_VECT_8S = 5;
const INT_2U FR_VECT_8C = 6;
const INT_2U FR_VECT_16C = 7;
const INT_2U FR_VECT_STR = 8;
const INT_2U FR_VECT_2U = 9;
const INT_2U FR_VECT_4U = 10;
const INT_2U FR_VECT_8U = 11;
const INT_2U FR_VECT_1U = 12;
*/
