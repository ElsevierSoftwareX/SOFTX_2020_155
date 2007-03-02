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

int speed0;
int speed1;
assign speed0 to "{ifo}:{sys}-STEPPER_SPEED{controller}0";
assign speed1 to "{ifo}:{sys}-STEPPER_SPEED{controller}1";

int max_speed;
assign max_speed to "{ifo}:{sys}-STEPPER_MAX_SPEED{controller}";

int min_speed;
assign min_speed to "{ifo}:{sys}-STEPPER_MIN_SPEED{controller}";

/* The limit of how many steps we can go in one shot */
int max_steps;
assign max_steps to "{ifo}:{sys}-STEPPER_MAX_STEPS{controller}";

int sign0;
int sign1;
assign sign0 to "{ifo}:{sys}-STEPPER_SIGN{controller}0";
assign sign1 to "{ifo}:{sys}-STEPPER_SIGN{controller}1";

int backlash0;
int backlash1;
assign backlash0 to "{ifo}:{sys}-STEPPER_BACKLASH{controller}0";
assign backlash1 to "{ifo}:{sys}-STEPPER_BACKLASH{controller}1";



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
%% static int old_v = 0;

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
    sign0 = atoi(macValueGet("sign0"));
    pvPut(sign0);
    sign1 = atoi(macValueGet("sign1"));
    pvPut(sign1);
    backlash0 = atoi(macValueGet("backlash0"));
%%  if (backlash0 > 50) backlash0 = 50;
    pvPut(backlash0);
    backlash1 = atoi(macValueGet("backlash1"));
%%  if (backlash1 > 50) backlash1 = 50;
    pvPut(backlash1);
%%
    min_speed = atoi(macValueGet("min_speed"));
%%  if (min_speed < 10) {
%%	printf("Warning, controller %d minimum speed set to 10\n", controller);
%%	min_speed = 10;
%%  }
    pvPut(min_speed);
    max_speed = atoi(macValueGet("max_speed"));
%%  if (max_speed < 10) {
%%	printf("Warning, controller %d maximum speed set to 10\n", controller);
%%	max_speed = 10;
%%  }
%%  if (max_speed <= min_speed) {
%%	printf("Warning, controller %d maximum speed set to %d\n", controller, min_speed + 1);
%%	max_speed = min_speed + 1;
%%  }
    pvPut(max_speed);
    speed0 = max_speed;
    speed1 = max_speed;
    pvPut(speed0);
    pvPut(speed1);
    max_steps = atoi(macValueGet("max_steps"));
    pvPut(max_steps);
%%
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
%%	 /* Set maximum speed */
%%	 sprintf(cmd_buf, "X10,%d,10\r", max_speed);
%%  	 exec(fd, cmd_buf, buffer, 1);
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
	pvGet(speed0);
%%	if (speed0 > max_speed) {
	  speed0 = max_speed;
	  pvPut(speed0);
%%	} else if (speed0 < min_speed) {
	  speed0 = min_speed;
	  pvPut(speed0);
%% 	}
	pvGet(speed1);
%%	if (speed1 > max_speed) {
	  speed1 = max_speed;
	  pvPut(speed1);
%%	} else if (speed1 < min_speed) {
	  speed1 = min_speed;
	  pvPut(speed1);
%% 	}
        pvGet(motor0);
	pvGet(command0);
%%	if (strlen(command0)) {
%%	  int j;
%%	  int fd;
%%	  char cmd_buf[128];
%%	  char buffer[256];
%%
%%	  /* Verify command */
%%	  int v = atoi(command0);
%%	  if (v != 0 && v >= -max_steps && v <= max_steps) {
    	     busy = 1;
    	     pvPut(busy);
%%	     fd = open_serial_port(controller);
%%	     /* Select motor */
%%	     sprintf(cmd_buf, "B%d\r", 1 + motor0);
%%  	     exec(fd, cmd_buf, buffer, 1);
%%	     /* Set speed */
%%	     sprintf(cmd_buf, "T%d\r", motor0? speed1: speed0);
%%  	     exec(fd, cmd_buf, buffer, 1);
%%	     /* Apply sign to movement count */
%%	     if (motor0 == 0) {
%%		if (sign0 < 0) v *= -1;
%%	     } else {
%%		if (sign1 < 0) v *= -1;
%%	     }
%%
%%	     /* See if we are reversing direction, apply backlash comp */
%%	     if ((old_v * v) < 0) {
%%	       if (motor0 == 0 && backlash0 != 0) {
%%		v += v > 0? backlash0: -backlash0;
%%	       } else if (motor0 == 1 && backlash1 != 0) {
%%		v += v > 0? backlash1: -backlash1;
%%	       }
%%	     }
%%	     old_v = v;
%%
%%	     /* Execute movement command */
%%	     sprintf(cmd_buf, "%+d\r", v);
%%  	     exec(fd, cmd_buf, buffer, 60);
%%	     /* Update current position */
%%  	     exec(fd, "V1\r", buffer, 1);
%%	     if (strlen(buffer) && buffer[0] == 'V') {
%%		int p = atoi(buffer+1);
%%		if (motor0 == 0) {
%%		  pos00 = p;
%%		} else {
%%		  pos01 = p;
%%		}
%%	     }
%%  	     close(fd);
%%	  }
%%        strcpy(command0, "");
          pvPut(command0);
          pvPut(pos00);
          pvPut(pos01);
    	  busy = 0;
    	  pvPut(busy);
%%      }
    	pvPut(sign0);
    	pvPut(sign1);
    	pvPut(backlash0);
    	pvPut(backlash1);
    	pvPut(max_speed);
    	pvPut(max_steps);
  } state run
 }
}
