/* saveV3.c 
   transfer files saved by version 3 to version 4 */
#include <stdio.h>
#include <stdlib.h>


void get_line(FILE* read_file, char s[]);

int main(int argc, char *argv[])
{
FILE *fin, *fout;
char linetemp[100];
int i, j;

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
    for ( i=0; i<4*16; i++ ) {
      get_line(fin, linetemp);
      fprintf ( fout, "%s", linetemp);
    }
    for ( i=0; i<16; i++ ) { /* add unit indicators */
      fprintf ( fout, "%d\n", 1);
    }
    for ( i=0; i<16; i++ ) { /* add auto indicators */
      fprintf ( fout, "%d\n", 0);
    }
    for ( i=0; i<3*16; i++ ) {
      get_line(fin, linetemp);
      fprintf ( fout, "%s", linetemp);
    }
    for ( i=0; i<5; i++ ) { /* skip old global settings */
      get_line(fin, linetemp);
    }
    for ( i=0; i<9; i++ ) {
      get_line(fin, linetemp);
      fprintf ( fout, "%s", linetemp);
    }
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
