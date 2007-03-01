program single_stepper

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


/* Current motor position readout */
int pos00;
int pos01;
assign pos00 to "{ifo}:{sys}-STEPPER_POS{controller}0";
assign pos01 to "{ifo}:{sys}-STEPPER_POS{controller}1";

/* Motor selection records (0 or 1) */
int motor0;
assign motor0 to "{ifo}:{sys}-STEPPER_SEL{controller}";

/* Serial port commands */
string command0;
assign command0 to "{ifo}:{sys}-STEPPER_CMD{controller}";

int busy;
assign busy to "{ifo}:{sys}-STEPPER_BUSY{controller}";

%%   static int open_serial_port(int i) {
%%	  int fd;
%%	  struct termios options;
%%	  char buf[16];
%%	  sprintf(buf, "/dev/ttyS%d", i);
%%  	  if ((fd = open(buf, O_RDWR | O_NOCTTY  /* | O_NDELAY */)) < 0 ) {
%%    		fprintf(stdout, "error opening device %s\n", buf);
%%    		exit(2);
%%	  }
%%
%%	  tcgetattr(fd, &options);
%%
%%	  /* set raw input, 1 second timeout */
%%	  options.c_cflag     |= (CLOCAL | CREAD);
%%	  options.c_lflag     &= ~(ICANON | ECHO | ECHOE | ISIG);
%%	  options.c_oflag     &= ~OPOST;
%%	  options.c_cc[VMIN]  = 0;
%%	  options.c_cc[VTIME] = 10; 
%%
%%	  cfsetispeed(&options, i > 1? B9600: B1200);
%%	  cfsetospeed(&options, i > 1? B9600: B1200);
%%
%%	  /* Odd parity (7O2) */
%%	  options.c_cflag |= PARENB;
%%	  options.c_cflag |= PARODD;
%%	  options.c_cflag |= CSTOPB;
%%	  options.c_cflag &= ~CSIZE;
%%	  options.c_cflag |= CS7;
%%
%%	  /* set the options */
%%	  tcsetattr(fd, TCSANOW, &options);
%%	  return fd;
%%   }
%%
%%    static void exec(int fd, char *cmd, char *resp, int timeout) {
%%	  fd_set readfs;    /* file descriptor set */
%%	  struct timeval Timeout;
%%	  int res;
%%	  int nread = 0;
%%	  char *bufptr;
%%	  char cmd_buf[128];
%%	  char buffer[256];
%%
%%	  /* Select motor */
%%	  /*sprintf(cmd_buf, "B%d\r", 1 + (*(motor[i])));
%%  	  write(fd, cmd_buf, strlen(cmd_buf));
%%	  sprintf(cmd_buf, "%s\r", command0);
%%  	  write(fd, cmd_buf, strlen(cmd_buf));*/
%%	  sprintf(cmd_buf, "%s\r", cmd);
%%  	  write(fd, cmd_buf, strlen(cmd_buf));
%%        FD_ZERO(&readfs);
%%	  FD_SET(fd, &readfs);
%%	  Timeout.tv_usec = 0;  /* milliseconds */
%%	  Timeout.tv_sec  = timeout;  /* seconds */
%%	  res = select(fd+1, &readfs, NULL, NULL, &Timeout);
%%	  if (res==0) {
%%	     /*printf("Response timeout\n");
%%	     fflush(stdout);*/
%%	     strcpy(resp, "");
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
%%	  strcpy(resp, buffer);
%%	  }
%%    }

%% static int controller;

ss stepper{

state init
{
  when() {
%%  int i, j;
%%  int fd;
%%  char cmd_buf[128];
%%  char buffer[256];

    busy = 1;
    pvPut(busy);

    pos00 = pos01  = 0;
    controller = i = atoi(macValueGet("controller"));
%%  fd = open_serial_port(i);
%%  for (j = 0; j < 2; j++) {
%%
%%	 /* Select motor */
%%	 sprintf(cmd_buf, "B%d\r", 1  + j);
%%  	 exec(fd, cmd_buf, buffer, 1);
%%	 /* Update current position */
%%  	 exec(fd, "V1\r", buffer, 1);
%%	 if (strlen(buffer) && buffer[0] == 'V') {
%%		int p = atoi(buffer+1);
%%		if (j == 0) {
%%		  pos00 = p;
%%		} else {
%%		  pos01 = p;
%%		}
%%	 }
%%	 /* Set default speed */
%%  	 exec(fd, "X10,20,10\r", buffer, 1);
%%  }
%%  close(fd);
       pvPut(pos00);
       pvPut(pos01);
       motor0 = 0;
       pvPut(motor0);
%%     strcpy(command0, "");
       pvPut(command0);
       busy = 0;
       pvPut(busy);
  }state run
}

state run
{
  when(delay(.1)) {
        pvGet(motor0);
	pvGet(command0);
%%	if (strlen(command0)) {
%%	  int j;
%%	  int fd;
%%	  char cmd_buf[128];
%%	  char buffer[256];
%%
    	  busy = 1;
    	  pvPut(busy);

%%	fd = open_serial_port(controller);
%%
%%	/* Select motor first */
%%	  sprintf(cmd_buf, "B%d\r", 1 + motor0);
%%  	  exec(fd, cmd_buf, buffer, 1);
%%	/* Execute command */
%%	  sprintf(cmd_buf, "%s\r", command0);
%%  	  exec(fd, cmd_buf, buffer, 60);
%%	/* Update current position */
%%  	  exec(fd, "V1\r", buffer, 1);
%%	  if (strlen(buffer) && buffer[0] == 'V') {
%%		int p = atoi(buffer+1);
%%		if (motor0 == 0) {
%%		  pos00 = p;
%%		} else {
%%		  pos01 = p;
%%		}
%%	  }
%%  	  close(fd);
%%        strcpy(command0, "");
          pvPut(command0);
          pvPut(pos00);
          pvPut(pos01);
    	  busy = 0;
    	  pvPut(busy);

%%      }
  } state run
 }
}
