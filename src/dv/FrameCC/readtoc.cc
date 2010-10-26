/* 
For dc3: read the input toc file and generate channelset0 file

/ldcg/bin/gcc -o readtoc -I/ldas/ldas-0.0/include -I/ldcg/include readtoc.cc  -L/ldas/ldas-0.0/lib -L/ldcg/lib  -lgeneral -lframecpp -lz
 
*/

#include <framecpp/framereader.hh>
#include <framecpp/frame.hh>
#include <fstream.h>
#include <framecpp/adcdata.hh>
#include <framecpp/tocreader.hh>

using namespace FrameCPP;

int main(int argc, char* argv[])
{
    // Open the frame file and write info to channelset
    cout << "Open file: " << argv[1] << endl;
    int  i, totalch=0;
    INT_4U chan;
    AdcData* adc;

    //ifstream in( "H-653560150.F" );
    //ofstream out("xxx");
    ifstream in(argv[1]);
    ofstream out(argv[2]);
    ofstream outtime(argv[3]);
    TOCReader fin(in);
    Frame* fr;
    string tempst, tempst1;
    int trend = 0, pos, add=1;
 
    int offset = (int)fin.TOC::getFrameNumber();
    cout << "Total frames: " << offset << endl;
    fr = fin.readFrame(0);
    fr->getRawData()->refAdc().rehash();
    chan = fr->getRawData()->refAdc().getSize();
    for (int j=0; j<chan; ++j) {
      adc = fr->getRawData()->refAdc()[j];
      tempst = adc->getName();
      if ( j == 0 ) {
	pos = tempst.find(".n");
        if ( pos > 0) {
	  trend =1;
	  cout << "Trend file" << endl;
	}
      }
      if ( trend ) {
	pos = tempst.find(".min");
	if (pos > 0) {
	  if ( j < 10 ) {
	    if ( adc->getSampleRate() > 0 && adc->getSampleRate() < 1 )
	      trend = 60;
	  }
	  totalch++;
	}
      }
    }

    if ( trend ) {
      cout << "total chan "  << totalch << endl;
      out << totalch << "\n";
    }
    else {
      cout << "total chan "  << chan << endl;
      out << chan << "\n";
    }
    REAL_8 srate;
    for (int j=0; j<chan; ++j) {
      adc = fr->getRawData()->refAdc()[j];
      if ( trend ) { //if suffixes
	tempst = adc->getName();
	pos = tempst.find(".min");
	if (pos > 0) {
	  for (int l=0; l<pos; ++l) {
	    out << tempst[l];
	  }
	  srate = adc->getSampleRate();
	  if (srate > 0 && srate < 1)
	    out << " 60 ";
	  else
	    out << " " << srate << " ";
	  tempst1 = adc->getUnits();
	  if (tempst1.length() == 0)
	    out << "None" << "\n";
	  else
	    out << tempst1 << "\n";
	}
      }
      else { 
	out << adc->getName() << " " << adc->getSampleRate() << " ";
	tempst1 = adc->getUnits();
	if (tempst1.length() == 0)
	  out << "None" << "\n";
	else
	  out << tempst1 << "\n";
      }
   }

    outtime << trend << "\n"; //indicates trend (1 or 60 ) or full data (0)
    outtime << fr->getGTime().getSec() << "\n";
    if (offset > 1) {
      delete fr; 
      fr = fin.readFrame((unsigned int)(offset-1));
    }
    outtime << fr->getGTime().getSec() + (int)(fr->getDt()) << "\n";
    delete fr; 
    
    out.close();
    outtime.close();
    in.close();
    
    cout << "Write channel info to file: " << argv[2] << endl;
}
