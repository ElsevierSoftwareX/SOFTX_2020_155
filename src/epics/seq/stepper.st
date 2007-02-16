program stepper

%% #include <stdio.h>
%% #include <string.h>
%% #include <stdlib.h>
%% #include <math.h>
%% #include <sys/types.h>
%% #include <sys/stat.h>
%% #include <fcntl.h>
%% #include <termios.h> /* POSIX terminal control definitions */
%% #include <signal.h>
%% #include <sys/select.h>


/* Motor selection records (1 or 2) */
int motor0;
int motor1;
int motor2;
int motor3;
assign motor0 to "{ifo}:{sys}-MOTOR_SEL0";
assign motor1 to "{ifo}:{sys}-MOTOR_SEL1";
assign motor2 to "{ifo}:{sys}-MOTOR_SEL2";
assign motor3 to "{ifo}:{sys}-MOTOR_SEL3";

/* Serial port commands */
string command0;
assign command0 to "{ifo}:{sys}-PORT_CMD0";
string command1;
assign command1 to "{ifo}:{sys}-PORT_CMD1";
string command2;
assign command2 to "{ifo}:{sys}-PORT_CMD2";
string command3;
assign command3 to "{ifo}:{sys}-PORT_CMD3";

%% const int debug = 1;
ss stepper{

state init
{
  when() {
       motor0 = 1;
       pvPut(motor0);
       motor1 = 1;
       pvPut(motor1);
       motor2 = 1;
       pvPut(motor2);
       motor3 = 1;
       pvPut(motor3);
%%     strcpy(command0, "");
       pvPut(command0);
%%     strcpy(command1, "");
       pvPut(command1);
%%     strcpy(command2, "");
       pvPut(command2);
%%     strcpy(command3, "");
       pvPut(command3);
  }state run
}

state run
{
  when(delay(.1)) {
	pvGet(command0);
	pvGet(command1);
	pvGet(command2);
	pvGet(command3);
%%      {
%%	char *cmnds[4] = { command0, command1, command2, command3 };
%%	int i;
%%   	for (i = 0; i < 4; i++) {
%%	if (strlen(cmnds[i])) {
%%	  int j;
%%	  int fd;
%%	  fd_set readfs;    /* file descriptor set */
%%	  int nread = 0;
%%	  int checksum = 0;
%%	  char char_checksum;
%%	  char buf[16];
%%	  char cmd_buf[128];
%%	  char buffer[256];
%%	  char *bufptr;
%%	  struct termios options;
%%	  int res;
%%	  struct timeval Timeout;
%%
%%	  sprintf(buf, "/dev/ttyS%d", i);
%%	  if (debug) {
%%  	    printf("Writing `%s' to %s\n", cmnds[i], buf);
%%  	    fflush(stdout);
%%	  }
%%  	  if ((fd = open(buf, O_RDWR | O_NOCTTY  /* | O_NDELAY */)) < 0 ) {
%%    		fprintf(stdout, "error opening device %s\n", buf);
%%    		exit(2);
%%	  }
%%	  /* set raw input, 1 second timeout */
%%	  options.c_cflag     |= (CLOCAL | CREAD);
%%	  options.c_lflag     &= ~(ICANON | ECHO | ECHOE | ISIG);
%%	  options.c_oflag     &= ~OPOST;
%%	  options.c_cc[VMIN]  = 0;
%%	  options.c_cc[VTIME] = 10;
%%
%%	  /* set the options */
%%	  /*tcsetattr(fd, TCSANOW, &options);*/
%%
%%	  /* Calculate the checksum */
%%	  for (j = 0; j < strlen(cmnds[i]); j++) {
%%		checksum += (unsigned int)(cmnds[i][j]);
%%	  }
%%	  if (debug) {
%%	    char_checksum = (checksum & 0x7f);
%%  	    printf("Checksum int %d; char `%c'\n", checksum, char_checksum);
%%  	    fflush(stdout);
%%	  }
%%	  sprintf(cmd_buf, "%s%c\r", cmnds[i], char_checksum);
%%  	  write(fd, cmd_buf, strlen(cmd_buf));
%%        FD_ZERO(&readfs);
%%	  FD_SET(fd, &readfs);
%%	  Timeout.tv_usec = 0;  /* milliseconds */
%%	  Timeout.tv_sec  = 5;  /* seconds */
%%	  res = select(fd+1, &readfs, NULL, NULL, &Timeout);
%%	  if (res==0) {
%%	     printf("Response timeout\n");
%%	     fflush(stdout);
%%	  } else {
%%	  /* read characters into our string buffer until we get a CR or NL */
%%	  bufptr = buffer;
%%	  while ((nread = read(fd, bufptr, buffer + sizeof(buffer) - bufptr - 1)) > 0)
%%	   {
%%		if (nread < 0) { printf ("read() timeout\n"); break; }
%%		bufptr += nread;
%%		if (bufptr[-1] == '\n' || bufptr[-1] == '\r') break;
%%	   }
%%	  *bufptr = '\0';
%%	  if (bufptr[-1] == '\n' || bufptr[-1] == '\r') bufptr[-1] = 0;
%%	  if (debug) {
%%	    printf("Response read: `%s'\n", buffer);
%%	    fflush(stdout);
%%	  }
%%	  /* Check responses */
%%	  switch (buffer[0]) {
%%	    case 'V' :
%%	    	/* Received current position counter */
%%		break;
%%	    case 'Y':
%%		/* Command completed OK */
%%		break;
%%	    case 'B':
%%		/* Busy, command could not be processed */
%%		break;
%%	    case 'E':
%%		/* Error processing command */
%%		break;
%%	  }
%%	  }
%%  	  close(fd);
%%        strcpy(cmnds[i], "");
%%	  switch (i) {
%%		case 0:
          		pvPut(command0);
%%			break;
%%		case 1:
          		pvPut(command1);
%%			break;
%%		case 2:
          		pvPut(command2);
%%			break;
%%		case 3:
          		pvPut(command3);
%%			break;
%%	  }
%%      }
%%	}
%%      }
  } state run
}
}
