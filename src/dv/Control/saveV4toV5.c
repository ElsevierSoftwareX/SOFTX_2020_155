/* saveV4toV5.c 
   transfer files saved by version 4 to version 5 */
#include <stdio.h>
#include <stdlib.h>


void get_line(FILE* read_file, char s[]);

int main(int argc, char *argv[])
{
FILE *fin, *fout;
char linetemp[100];
int i, j, windowNum;

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
    for ( i=0; i<153; i++ ) {
      get_line(fin, linetemp);
    } /* skip all except the last line */
    get_line(fin, linetemp);
    fprintf ( fout, "%s", linetemp); /* put last line first */
    sscanf(linetemp, "%d", &windowNum );
    fseek(fin, 0, 0); /* reset the reading file stream */
    for ( i=0; i<9; i++ ) {
      for ( j=0; j<windowNum; j++ ) {
	get_line(fin, linetemp);
	fprintf ( fout, "%s", linetemp);
      }
      for ( j=windowNum; j<16; j++ ) {
	get_line(fin, linetemp);
      }
    }
    for ( i=0; i<8; i++ ) {
      get_line(fin, linetemp);
      fprintf ( fout, "%s", linetemp);
    }
    fprintf ( fout, "%d\n", 1); /* add (128th) resolution */
    fprintf ( fout, "%d\n", 1); /* add refreshrate */
    fprintf ( fout, "%d\n", 0); /* add filter indicator */
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
