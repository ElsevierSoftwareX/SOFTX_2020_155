
RPC           = rpcgen
RPCFLAGS      = -C -N -M -D_ADVANCED_LIGO
RPCX = gdsrsched rawgapi rtestpoint 
CFLAGS = -I. -g



SrcPath = 


RPCOBJS=$(RPCX:%=%_xdr.o) $(RPCX:%=%_svc.o) $(RPCX:%=%_clnt.o)
RPCSRC=$(RPCOBJS:%.o=%.c) $(RPCX:%=%.h) 


OBJS = $(RPCOBJS)\
awgapi.o\
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
gdssock.o\
gdsstring.o\
gdstask.o\
gdsxdr_util.o\
gmutex.o\
gpst2gpss.o\
pci.o\
rmapi.o\
rpcinc.o\
sockutil.o\
tconv.o\
testchn.o\
testpointinfo.o\
testpointmgr.o\
testpoint.o\
testpoint_server.o\
timingcard.o\
gdssched_client.o\
gdssched.o\
gdssched_server.o\
gdssched_util.o

awgtpman: $(RPCX:%=$(SrcPath)%.h) ${OBJS}
	c++ ${OBJS} -lpthread -o awgtpman

${RPCOBJS}:	 ${RPCSRC}

clean:
	/bin/rm -f  awgtpman ${OBJS} ${RPCSRC}

$(RPCX:%=$(SrcPath)%.h) : $(SrcPath)%.h: $(SrcPath)%.x
	/bin/rm -f $@
	$(RPC) $(RPCFLAGS) -h -o $@ $(DEFINE) $<
	/bin/rm -f $(@:%.h=%_xdr.c)
	$(RPC) $(RPCFLAGS) -c -o $(@:%.h=%_xdr.c) $(DEFINE) $<
	/bin/rm -f $(@:%.h=%_clnt.c)
	$(RPC) $(RPCFLAGS) -l -o $(@:%.h=%_clnt.c) $(DEFINE) $<
	/bin/rm -f $(@:%.h=%_svc.c)
	$(RPC) $(RPCFLAGS) -m -o $(@:%.h=%_svc.c) $(DEFINE) $<

