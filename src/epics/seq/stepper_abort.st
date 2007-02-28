program stepper_abort

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


/* Serial port abort_commands */
string abort_command0;
assign abort_command0 to "{ifo}:{sys}-STEPPER_ABORT0";
string abort_command1;
assign abort_command1 to "{ifo}:{sys}-STEPPER_ABORT1";
string abort_command2;
assign abort_command2 to "{ifo}:{sys}-STEPPER_ABORT2";
string abort_command3;
assign abort_command3 to "{ifo}:{sys}-STEPPER_ABORT3";

ss stepper{

state init
{
  when() {
%%     strcpy(abort_command0, "");
       pvPut(abort_command0);
%%     strcpy(abort_command1, "");
       pvPut(abort_command1);
%%     strcpy(abort_command2, "");
       pvPut(abort_command2);
%%     strcpy(abort_command3, "");
       pvPut(abort_command3);
  }state run
}

state run
{
  when(delay(.1)) {
	pvGet(abort_command0);
	pvGet(abort_command1);
	pvGet(abort_command2);
	pvGet(abort_command3);
%%      {
%%	char *cmnds[4] = { abort_command0, abort_command1, abort_command2, abort_command3 };
%%	int i;
%%   	for (i = 0; i < 4; i++) {
%%	if (strlen(cmnds[i])) {
%%	  int j;
%%	  int fd;
%%	  fd_set readfs;    /* file descriptor set */
%%	  int nread = 0;
%%	  char buf[16];
%%	  char cmd_buf[128];
%%	  char buffer[256];
%%	  char *bufptr;
%%	  struct termios options;
%%	  int res;
%%	  struct timeval Timeout;
%%
%%	  sprintf(buf, "/dev/ttyS%d", i);
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
%%	  strcpy(cmd_buf, "K\r");
%%  	  write(fd, cmd_buf, strlen(cmd_buf));
%%	#if 0
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
%%	#endif
%%  	  close(fd);
%%        strcpy(cmnds[i], "");
%%	  switch (i) {
%%		case 0:
          		pvPut(abort_command0);
%%			break;
%%		case 1:
          		pvPut(abort_command1);
%%			break;
%%		case 2:
          		pvPut(abort_command2);
%%			break;
%%		case 3:
          		pvPut(abort_command3);
%%			break;
%%	  }
%%      }
%%	}
%%      }
  } state run
}
}
