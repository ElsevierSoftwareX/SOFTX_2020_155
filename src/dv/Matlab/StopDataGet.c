/* stop a data transmission
   argv1: server IP
   argv2: process ID

   compile with:
   gcc -o StopDataGet StopDataGet.c Lib/datasrv.o Lib/daqc_access.o -L/home/hding/Cds/Frame/Lib/UTC_GPS -ltai -lsocket -lpthread 
 */

#include "Lib/datasrv.h"

int main(int argc, char *argv[])
{
char  severIP[80];
unsigned long processID = 0;

  printf ( "in StopDataGet:\n");
  strcpy(severIP, argv[1] );
  processID = atoi(argv[2]);
  DataSimpleConnect(severIP, 0);
  DataWriteStop(processID);
  sleep(1);
  DataQuit();
  printf ( "StopDataGet: Process %d terminated.\n", processID );

  return 0;
}
