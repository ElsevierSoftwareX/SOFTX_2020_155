
ifeq ($(shell uname), SunOS)
LIBS = -I../../daqd -I../Th -L../Lib -L../Lib/UTC_GPS -ltai -lm -lnsl -lpthread -lsocket
all: framer3 framer4 frameMemRead frameMemReadXMGR tocLongRead frameMemTrig
else
LIBS = -I../../daqd -I../Th -L../Lib/UTC_GPS -ltai -lm -lnsl -lpthread 
all: framer4 frameMemRead 
endif

OBJS = ../Th/datasrv.o ../Th/daqc_access.o

# Explicit setting of SHELL - beware since this gets inherited

# online data display
framer4: framer4.c ${OBJS}
	gcc -g -o framer4 framer4.c ${OBJS} ${LIBS} -lgrace_np 

framer3: framer3.c ${OBJS}
	gcc -g -o framer3 framer3.c ${OBJS} ${LIBS} -lacegr_np_my

# playback
frameMemRead: frameMemRead.c ${OBJS}
	gcc -g -o frameMemRead frameMemRead.c ${OBJS} ${LIBS} -lgrace_np 

frameMemReadXMGR: frameMemReadXMGR.c ${OBJS}
	gcc -g -o frameMemReadXMGR frameMemReadXMGR.c ${OBJS} ${LIBS} -lacegr_np_my

# playback w/trigger
frameMemTrig: frameMemTrig.c ${OBJS}
	gcc -g -o frameMemTrig frameMemTrig.c ${OBJS} ${LIBS} -lgrace_np 

# frame file playback
tocLongRead: tocLongRead.c ${OBJS}
	gcc -g -o tocLongRead tocLongRead.c ${OBJS} ${LIBS} -lgrace_np


clean:
	/bin/rm -f framer3 framer4 frameMemRead tocLongRead *.o
