#include <stdio.h>
#include "debug.h"
#include "framecpp/Version6/FrameH.hh"
#include "framecpp/Version6/FrTOC.hh"
#include "myframereadplan.hh"
#include "mmstream.hh"

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
  mm_istream in(argv[1]);
  if (! in ) {
    fprintf(stderr,"Failed to mmap file `%s'\n", argv[1]);
    exit(1);	  
  }
  myFrameReadPlan *reader = 0; // frame reader
  try {
    reader = new myFrameReadPlan(in);
  } catch (...) {
    fputs("Failed to create frame reader\n",stderr);
    exit(1);
  }
  //reader->setErrorWatch(error_watch);
  vector<string> names;
  names = split(argv[2]);
  FrameCPP::Version_6::FrameH *frame = 0;
  try {
      frame = &reader->readFrame(0, names);
  } catch (...) {
          fprintf(stderr, "Failed to read first frame");
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
