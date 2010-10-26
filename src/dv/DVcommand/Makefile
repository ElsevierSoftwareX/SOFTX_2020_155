
ifeq ($(shell uname), SunOS)
LIBS = -I../../fb2 -I../Th -L../Lib -L../Lib/UTC_GPS -ltai -lm -lnsl -lpthread -lsocket  -lgrace_np
all: dv_command dv_command_file
else
LIBS = -I../../fb2 -I../Th -L/usr/local/grace/lib -L../Lib/UTC_GPS -ltai -lm -lnsl -lpthread 
all: framer4 frameMemRead 
endif

OBJS = ../Th/datasrv.o ../Th/daqc_access.o

# Explicit setting of SHELL - beware since this gets inherited

# online data display
dv_command: 
	gcc -o dv_command dv_command.c ${OBJS} ${LIBS}

dv_command_file: 
	gcc -o dv_command_file dv_command_file.c ${OBJS} ${LIBS}

clean:
	/bin/rm -f dv_command *.o
