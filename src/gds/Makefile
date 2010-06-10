
CFLAGS = -I.

OBJS = awgapi.o\
awgfunc.o\
awg.o\
awg_server.o\
awgtpman.o\
caldate_mjd.o\
cobox.o\
confserver.o\
ds340.o\
gdschannel.o\
gdsdac.o\
gdserr.o\
gdsheartbeat.o\
gdsics115.o\
gdsprm.o\
gdsrand.o\
gdsrsched_clnt.o\
gdsrsched_svc.o\
gdsrsched_xdr.o\
gdssched_client.o\
gdssched.o\
gdssched_server.o\
gdssched_util.o\
gdssock.o\
gdsstring.o\
gdstask.o\
gdsxdr_util.o\
gmutex.o\
gpst2gpss.o\
pci.o\
rawgapi_clnt.o\
rawgapi_svc.o\
rawgapi_xdr.o\
rmapi.o\
rpcinc.o\
rtestpoint_clnt.o\
rtestpoint_svc.o\
rtestpoint_xdr.o\
sockutil.o\
tconv.o\
testchn.o\
testpointinfo.o\
testpointmgr.o\
testpoint.o\
testpoint_server.o\
timingcard.o

all: ${OBJS}
	c++ ${OBJS} -lpthread -o awgtpman

clean:
	/bin/rm -f  awgtpman ${OBJS}

