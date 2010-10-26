/* saveV8toV9.c 
   transfer files saved by version 8 to version 9 (XML) */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
FILE *fin, *fout;
char linetemp[100], term1[100], term2[100];
int i, j, windowNum;
char chName[16][80], chUnit[16][80];
int  chint[16];

    if (argc < 3) {
      printf ( "Need 2 inputs: input file name and output file name. Exit.\n" );
      exit(1);
    }
    fin = fopen( argv[1], "r" );
    if ( fin == NULL ) {
      printf ( "Can't open input file %s\n", argv[1] );
      return;
    }
    fout = fopen( argv[2], "w" );
    if ( fout == NULL ) {
      printf ( "Can't open output file %s\n", argv[2] );
      return;
    }
    fprintf ( fout, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>\n\n" );
    fprintf ( fout, "<DATAVIEWER>\n" );
    fprintf ( fout, "<VERSION>9</VERSION>\n" );
    fscanf ( fin, "%s", linetemp );
    fscanf ( fin, "%s", linetemp );
    sscanf(linetemp, "%d", &windowNum );
    fprintf ( fout, "<CHANNO>%d</CHANNO>\n", windowNum  );
    fprintf ( fout, "<!-- channel features -->\n" );
    for ( i=0; i<16; i++ ) {
      fscanf ( fin, "%s %d %s", chName[i], &chint[i], chUnit[i]);
    }
    for ( i=0; i<16; i++ ) 
      fprintf ( fout, "<NAME ch=\"%d\">%s</NAME>\n", i+1, chName[i] );
    for ( i=0; i<16; i++ ) 
      fprintf ( fout, "<RATE ch=\"%d\">%d</RATE>\n", i+1, chint[i] );
    for ( i=0; i<16; i++ ) 
      fprintf ( fout, "<UNIT ch=\"%d\">%s</UNIT>\n", i+1, chUnit[i] );
    for ( i=0; i <16; i++ ) {
      fscanf ( fin, "%s", term1 );
      fprintf ( fout, "<YMIN ch=\"%d\">%s</YMIN>\n", i+1, term1);
    }
    for ( i=0; i <16; i++ ) {
      fscanf ( fin, "%s", term1 );
      fprintf ( fout, "<YMAX ch=\"%d\">%s</YMAX>\n", i+1, term1);
    }
    for ( i=0; i <16; i++ ) {
      fscanf ( fin, "%s", term1 );
      fprintf ( fout, "<XYTYPE ch=\"%d\">%s</XYTYPE>\n", i+1, term1 );
    }
    for ( i=0; i <16; i++ ) {
      fscanf ( fin, "%s", term1 );
      fprintf ( fout, "<EUNIT ch=\"%d\">%s</EUNIT>\n", i+1, term1 );
    }
    for ( i=0; i <16; i++ ) {
      fscanf ( fin, "%s", term1 );
      fprintf ( fout, "<AUTO ch=\"%d\">%s</AUTO>\n", i+1, term1 );
    }
    for ( i=0; i <16; i++ ) {
      fscanf ( fin, "%s", term1 );
      fprintf ( fout, "<XSCALE ch=\"%d\">%s</XSCALE>\n", i+1, term1);
    }
    for ( i=0; i <16; i++ ) {
      fscanf ( fin, "%s", term1 );
      fprintf ( fout, "<XDELAY ch=\"%d\">%s</XDELAY>\n", i+1, term1);
    }
    for ( i=0; i <16; i++ ) {
      fscanf ( fin, "%s", term1 );
      fprintf ( fout, "<COLOR ch=\"%d\">%s</COLOR>\n", i+1, term1 );
    }
    fscanf ( fin, "%s", term1 );
    fprintf ( fout, "<GRAPHOPTION>%s</GRAPHOPTION>\n", term1 );
    fscanf ( fin, "%s", term1 );
    fprintf ( fout, "<GRAPHMODE>%s</GRAPHMODE>\n", term1 );
    fscanf ( fin, "%s", term1 );
    fprintf ( fout, "<CHSELECTED>%s</CHSELECTED>\n", term1 );
    fscanf ( fin, "%s", term1 );
    fprintf ( fout, "<GLOBAL>%s</GLOBAL>\n", term1 );
    fscanf ( fin, "%s", term1 );  /* trigon */
    fscanf ( fin, "%s", term2 );  /* trig chan */
    fprintf ( fout, "<TRIGGER ch=\"%s\">%s</TRIGGER>\n", term2, term1 );
    fscanf ( fin, "%s", term1 );
    fprintf ( fout, "<TRIGGERLEV>%s</TRIGGERLEV>\n", term1 );
    fscanf ( fin, "%s", term1 );
    fprintf ( fout, "<SIGON>%s</SIGON>\n", term1 );
    fscanf ( fin, "%s", term1 );
    fprintf ( fout, "<RESOLUTION>%s</RESOLUTION>\n", term1 );
    fscanf ( fin, "%s", term1 );
    fprintf ( fout, "<REFRESHRATE>%s</REFRESHRATE>\n", term1 );
    fscanf ( fin, "%s", term1 );
    fprintf ( fout, "<FILTER>%s</FILTER>\n", term1 );
    fscanf ( fin, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &chint[0], &chint[1], &chint[2], &chint[3], &chint[4], &chint[5], &chint[6], &chint[7], &chint[8], &chint[9], &chint[10], &chint[11], &chint[12], &chint[13], &chint[14], &chint[15] );
    fprintf ( fout, "<CHMARKED>%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d</CHMARKED>\n", chint[0], chint[1], chint[2], chint[3], chint[4], chint[5], chint[6], chint[7], chint[8], chint[9], chint[10], chint[11], chint[12], chint[13], chint[14], chint[15] );
    fprintf ( fout, "\n</DATAVIEWER>\n" );
    fclose(fin);
    fclose(fout);
    return 0;
}


