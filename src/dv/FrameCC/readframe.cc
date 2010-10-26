/* 
For dc3: read the input frame files and generate 
         channelset0 and frametime files
called in the script file: frgenerate

c++ -c readframe -I/usr1/ldas/framecpp-0.4.14Nov12/include -I/ldcg/include readframe.cc

c++ -o readframe -I/usr1/ldas/framecpp-0.4.14Nov12/include -I/ldcg/include readframe.cc -L/usr1/ldas/framecpp-0.4.14Nov12/lib -L/ldcg/lib -lgeneral -lframecpp -lz -lbz2
 
/ldcg/bin/gcc -o readframe -I/usr1/ldas/framecpp-0.4.14Nov12/include -I/ldcg/include readframe.cc  -L/usr1/ldas/framecpp-0.4.14Nov12/lib -L/ldcg/lib  -lgeneral -lframecpp -lz

*/

#include <unistd.h>
#include <iostream>
#include <fstream>
#include <stdio.h>

#include "framecpp/Version6/FrameH.hh"
#include "framecpp/Version6/FrCommon.hh"
#include "framecpp/Version6/Functions.hh"
#include "framecpp/Version6/IFrameStream.hh"
#include "framecpp/Version6/OFrameStream.hh"
#include "framecpp/Version6/Util.hh"

using namespace std;

void
fatal( string msg ) {
  cerr << msg << endl;
  exit (1);
}

void
error_watch (const string& msg)
{
  cerr << "error: " << msg << endl;
}

int main(int argc, char* argv[])
{
    // Open the frame file and write info to channelset
    cout << "Check file: " << argv[1] << endl;
    int  i, totalch=0;
    INT_4U chan;
    FrameCPP::Version_6::FrAdcData *adc;
    //AdcData* adc;
    char inputfile[200], inputfile_t[200];
    FILE *fp;
    INT_4S gps0,gps1;

    ofstream out(argv[2]);
    ofstream outtime(argv[3]);
    //get the file which lists all the frame file names
    fp = fopen(argv[1], "r");
    if ( fp == NULL ) {
      cout << "Cann't open reading file: " <<  argv[1] << endl;
      exit(1);
    }
    
    string tempst, tempst1;
    int trend = 0, pos, add=1;

    fscanf ( fp, "%s", inputfile); 
    fscanf ( fp, "%s", inputfile); //get the first file
    cout << "Open file " << inputfile << " for channel info" << endl;
    ifstream in( inputfile );
    FrameCPP::Version_6::IFrameStream fin (in);
    //fin.setErrorWatch( error_watch );
    strcpy(inputfile_t, inputfile); //get last file for time string
    while ( fscanf ( fp, "%s", inputfile_t) != EOF ) {; }
    fclose(fp);

    FrameCPP::Version_6::FrameH* fr = fin.ReadNextFrame();
    gps0 = fr->getGTime().getSec();
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

    in.close();
    delete fr; 
    outtime << trend << "\n"; //indicates trend (1 or 60 ) or full data (0)
    ifstream in0( inputfile_t );
    FrameCPP::Version_6::IFrameStream fin0 (in0);
    fr = fin0.ReadNextFrame();
    gps1 = fr->getGTime().getSec() + (int)(fr->getDt());
    outtime << gps0 << "\n";
    outtime << gps1 << "\n"; // = last gps + 1
    out.close();
    outtime.close();
    in0.close();
    delete fr; 

    cout << "Write channel info to file: " << argv[2] << endl;
}

