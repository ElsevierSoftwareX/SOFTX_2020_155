max_chnname_size=-DMAX_CHNNAME_SIZE=60

RPC           = rpcgen
RPCFLAGS      = -C -N -M -DPORTMAP -DLIGO_GDS -DGDS_NO_EPICS $(max_chnname_size)

RPCX	      = gdsrsched rawgapi rtestpoint
CFLAGS	      = -std=gnu99 -I. -I../include -g -DPORTMAP -DLIGO_GDS -DGDS_NO_EPICS $(max_chnname_size)
CXXFLAGS      = -g -DPORTMAP -DLIGO_GDS -DGDS_NO_EPICS $(max_chnname_size)

# Add this to build on RTLInux
#CFLAGS += -DRTLINUX=1

SrcPath	      = 


RPCOBJS=$(RPCX:%=%_xdr.o) $(RPCX:%=%_svc.o) $(RPCX:%=%_clnt.o)
RPCSRC=$(RPCOBJS:%.o=%.c) $(RPCX:%=%.h) 

OBJS = $(RPCOBJS)\
awgapi.o\
awgfunc.o\
awg.o\
awg_server.o\
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
gdssched_util.o\

awgtpman: $(RPCX:%=$(SrcPath)%.h) awgtpman.o ${OBJS} 
	c++ ${OBJS} awgtpman.o -lpthread -o awgtpman

${RPCOBJS}:	 ${RPCSRC}

clean:
	/bin/rm -f  awgtpman awgtpman.o ${OBJS} ${RPCSRC} 

$(RPCX:%=$(SrcPath)%.h) : $(SrcPath)%.h: $(SrcPath)%.x
	/bin/rm -f $@
	$(RPC) $(RPCFLAGS) -h -o $@ $(DEFINE) $<
	/bin/rm -f $(@:%.h=%_xdr.c)
	$(RPC) $(RPCFLAGS) -c -o $(@:%.h=%_xdr.c) $(DEFINE) $<
	/bin/rm -f $(@:%.h=%_clnt.c)
	$(RPC) $(RPCFLAGS) -l -o $(@:%.h=%_clnt.c) $(DEFINE) $<
	/bin/rm -f $(@:%.h=%_svc.c)
	$(RPC) $(RPCFLAGS) -m -o $(@:%.h=%_svc.c) $(DEFINE) $<

