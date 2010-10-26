/* 
/ldcg/bin/gcc -o maketoc -I/ldas/ldas-0.0/include -I/ldcg/include maketoc.cc  -L/ldas/ldas-0.0/lib -L/ldcg/lib  -lgeneral -lframecpp -lz
 */

#include <framecpp/framereader.hh>
#include <framecpp/frame.hh>
#include <fstream.h>
#include <iostream.h>
#include <stdiostream.h>
#include <framecpp/adcdata.hh>
#include <framecpp/errors.hh>
#include <framecpp/framewritertoc.hh>

#include <unistd.h>
#include <sys/wait.h>
using namespace FrameCPP;

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
    // Open the frame file and initialize the FrameReader
    INT_4U chan;
    AdcData* adc;
    char inputfile[200];
    FILE *fp;

    if (argc < 3) {
      cout << "Need 2 inputs: input file name and output file name. Exit." << endl;
      exit(1);
    }
    //get the file which lists all the frame file names
    fp = fopen(argv[1], "r");
    if ( fp == NULL ) {
      cout << "Cann't open reading file: " <<  argv[1] << endl;
      exit(1);
    }
    
    ofstream out(argv[2]);
    fscanf ( fp, "%s", inputfile); 
    cout << "open file " << inputfile << endl;
    ifstream in0( inputfile );
    FrameReader fin(in0);
    fin.setErrorWatch( error_watch );
    FrameWriterTOC fw( out );


    /* write as a single frame file  */
    try {
      for(;;) {
	Frame* frame = fin.readFrame();
	if (!frame)
	  break;
	fw.writeFrame( *frame );
	delete frame;
      }
    } catch ( read_failure& e ) {
      if ( e.code() != fin.ENDOFFILE )
	{
	  fatal( e.what() );
	}
    }

    int i = 1;
    while ( fscanf ( fp, "%s", inputfile) != EOF ) { 
      if ( strstr (inputfile, "filter_file") == NULL &&
           strstr (inputfile, "outtoc") == NULL) {
	cout << "open file " << inputfile << endl;
	ifstream in( inputfile );
	FrameReader fin(in);
	fin.setErrorWatch( error_watch );
	
	try {
	  for(;;) {
	    Frame* frame = fin.readFrame();
	    if (!frame)
	      break;
	    i++;
	    fw.writeFrame( *frame );
	    delete frame;
	  }
	} catch ( read_failure& e ) {
	  if ( e.code() != fin.ENDOFFILE )
	    {
	      fatal( e.what() );
	    }
	}
      }
    }
    fw.close();   // build TOC
    out.close();
    in0.close();
    fclose(fp);
    cout << "Total " << i << " frames written to " << argv[2]<< ". Finished." << endl;
    
    /* read the file  */
    /*
    ifstream in1( argv[2] );
    FrameReader fin1( in1 );
    int totalfr = 0;
    try {
      for(;;) {
	Frame* fr = fin1.readFrame();
	if (!fr)
	  break;
	totalfr++;
	cout << totalfr << "th frame: " << endl;
	//handle the frame
	fr->getRawData()->refAdc().rehash();
	chan = fr->getRawData()->refAdc().getSize();
	cout << "total channels " << chan << endl;
	cout << "time stamp " << fr->getGTime().getSec() << endl;
	
	delete fr;
      }
    } catch ( read_failure& e ) {
      if ( e.code() != fin1.ENDOFFILE )
	{
	  fatal( e.what() );
	}
    }
    in1.close();
    */
   
}
