/* saveV5-7toV8.c 
   transfer files saved by version 5 - 7 to version 8 */
#include <stdio.h>
#include <stdlib.h>


void get_line(FILE* read_file, char s[]);

int main(int argc, char *argv[])
{
FILE *fin, *fout;
char linetemp[100];
int i, j, windowNum;
int chMarked[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

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
    get_line(fin, linetemp);
    fprintf ( fout, "%s", "V8.0\n"); 
    fprintf ( fout, "%s", linetemp); 
    sscanf(linetemp, "%d", &windowNum );
    for ( i=0; i<9; i++ ) {
      for ( j=0; j<windowNum; j++ ) {
	get_line(fin, linetemp);
	fprintf ( fout, "%s", linetemp);
      }
      if (i == 0) /* name, rate, unit  */
	for ( j=windowNum; j<16; j++ ) {
	  fprintf ( fout, "%s %d %s\n", "None", 0, "None");
	}
      else if (i == 1) /* ymin */
	for ( j=windowNum; j<16; j++ ) {
	  fprintf ( fout, "%d\n", -5);
	}
      else if (i == 2) /* ymax */
	for ( j=windowNum; j<16; j++ ) {
	  fprintf ( fout, "%d\n", 5);
	}
      else if (i == 6) /* xscale */
	for ( j=windowNum; j<16; j++ ) {
	  fprintf ( fout, "%d\n", 8);
	}
      else
	for ( j=windowNum; j<16; j++ ) {
	  fprintf ( fout, "%d\n", 0);
	}
    }
    for ( i=0; i<11; i++ ) {
      get_line(fin, linetemp);
      fprintf ( fout, "%s", linetemp);
    }
    for ( i=0; i<windowNum; i++ ) 
      chMarked[i] = 1;
    /* add marked channels */
    fprintf ( fout, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", chMarked[0], chMarked[1], chMarked[2], chMarked[3], chMarked[4], chMarked[5], chMarked[6], chMarked[7], chMarked[8], chMarked[9], chMarked[10], chMarked[11], chMarked[12], chMarked[13], chMarked[14], chMarked[15] );
    fclose(fin);
    fclose(fout);
    return 0;
}


void get_line(FILE* read_file, char s[])
{ int j;
  fgets(s, 100, read_file);
  j = strlen(s);
  s[j] = '\0';
  return;
}
