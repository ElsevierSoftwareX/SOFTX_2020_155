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


/* Current motor position readout */
int pos00;
int pos01;
assign pos00 to "{ifo}:{sys}-STEPPER_POS00";
assign pos01 to "{ifo}:{sys}-STEPPER_POS01";
int pos10;
int pos11;
assign pos10 to "{ifo}:{sys}-STEPPER_POS10";
assign pos11 to "{ifo}:{sys}-STEPPER_POS11";
int pos20;
int pos21;
assign pos20 to "{ifo}:{sys}-STEPPER_POS20";
assign pos21 to "{ifo}:{sys}-STEPPER_POS21";
int pos30;
int pos31;
assign pos30 to "{ifo}:{sys}-STEPPER_POS30";
assign pos31 to "{ifo}:{sys}-STEPPER_POS31";

/* Motor selection records (0 or 1) */
int motor0;
int motor1;
int motor2;
int motor3;
assign motor0 to "{ifo}:{sys}-STEPPER_SEL0";
assign motor1 to "{ifo}:{sys}-STEPPER_SEL1";
assign motor2 to "{ifo}:{sys}-STEPPER_SEL2";
assign motor3 to "{ifo}:{sys}-STEPPER_SEL3";

/* Serial port commands */
string command0;
assign command0 to "{ifo}:{sys}-STEPPER_CMD0";
string command1;
assign command1 to "{ifo}:{sys}-STEPPER_CMD1";
string command2;
assign command2 to "{ifo}:{sys}-STEPPER_CMD2";
string command3;
assign command3 to "{ifo}:{sys}-STEPPER_CMD3";

int busy;
assign busy to "{ifo}:{sys}-STEPPER_BUSY";

%%   int open_serial_port(int i) {
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
%%	  cfsetispeed(&options, B9600);
%%	  cfsetospeed(&options, B9600);
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
%%    void exec(int fd, char *cmd, char *resp, int timeout) {
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
%%	  sprintf(cmd_buf, "%s\r", cmnds[i]);
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
%%	char *cmnds[4] = { command0, command1, command2, command3 };
%%	int *motor[4] = { &motor0, &motor1, &motor2, &motor3 };
%%	int *pos[4][2] = { {&pos00, &pos01}, {&pos10, &pos11}, {&pos20, &pos21}, {&pos30, &pos31} };

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

    pos00 = pos01 = pos10 = pos11
    = pos20 = pos21 = pos30 = pos31 = 0;
%% for (i = 0; i < 4; i++) {
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
%%		*(pos[i][j]) = p;
%%	 }
%%  }
%%  close(fd);
%% }
       pvPut(pos00);
       pvPut(pos01);
       pvPut(pos10);
       pvPut(pos11);
       pvPut(pos20);
       pvPut(pos21);
       pvPut(pos30);
       pvPut(pos31);
       motor0 = 0;
       pvPut(motor0);
       motor1 = 0;
       pvPut(motor1);
       motor2 = 0;
       pvPut(motor2);
       motor3 = 0;
       pvPut(motor3);
%%     strcpy(command0, "");
       pvPut(command0);
%%     strcpy(command1, "");
       pvPut(command1);
%%     strcpy(command2, "");
       pvPut(command2);
%%     strcpy(command3, "");
       pvPut(command3);
       busy = 0;
       pvPut(busy);
  }state run
}

state run
{
  when(delay(.1)) {
        pvGet(motor0);
        pvGet(motor1);
        pvGet(motor2);
        pvGet(motor3);
	
	pvGet(command0);
	pvGet(command1);
	pvGet(command2);
	pvGet(command3);
%%      {
%%	int i;
%%   	for (i = 0; i < 4; i++) {
%%	if (strlen(cmnds[i])) {
%%	  int j;
%%	  int fd;
%%	  char cmd_buf[128];
%%	  char buffer[256];
%%
    	  busy = 1;
    	  pvPut(busy);

%%	fd = open_serial_port(i);
%%
%%	/* Select motor first */
%%	  sprintf(cmd_buf, "B%d\r", 1 + (*(motor[i])));
%%  	  exec(fd, cmd_buf, buffer, 1);
%%	/* Execute command */
%%	  sprintf(cmd_buf, "%s\r", cmnds[i]);
%%  	  exec(fd, cmd_buf, buffer, 60);
%%	/* Update current position */
%%  	  exec(fd, "V1\r", buffer, 1);
%%	  if (strlen(buffer) && buffer[0] == 'V') {
%%		int p = atoi(buffer+1);
%%		*(pos[i][*(motor[i])]) = p;
%%	  }
%%  	  close(fd);
%%        strcpy(cmnds[i], "");
%%	  switch (i) {
%%		case 0:
          		pvPut(command0);
          		pvPut(pos00);
          		pvPut(pos01);
%%			break;
%%		case 1:
          		pvPut(command1);
          		pvPut(pos10);
          		pvPut(pos11);
%%			break;
%%		case 2:
          		pvPut(command2);
          		pvPut(pos20);
          		pvPut(pos21);
%%			break;
%%		case 3:
          		pvPut(command3);
          		pvPut(pos30);
          		pvPut(pos31);
%%			break;
%%	  }
    	  busy = 0;
    	  pvPut(busy);

%%      }
%%	}
%%      }
  } state run
}
}
