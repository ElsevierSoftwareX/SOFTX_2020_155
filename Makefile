all: 
	@echo Please build individual systems with 'make [system]' command

# Clean system, just say something like 'make clean-pde'
clean-% :: src/epics/simLink/%.mdl
	@system=$(subst clean-,,$@); \
	/bin/rm -rf target/$${system}epics build/$${system}epics; \
	(cd src/fe/$${system}; make -i clean || true); \

clean-% :: config/Makefile.%epics
	@system=$(subst clean-,,$@); \
	/bin/rm -rf target/$${system}epics build/$${system}epics; \

# With this rule one can make any system
# Just say 'make pde', for instance, to make PDE system
# Epics and front-end parts
%:
	(cd src/epics/util; ./feCodeGen.pl $@.mdl $@)
	/bin/rm -rf build/$@epics-medm
	/bin/rm -rf build/$@epics-config
	/bin/mv -f build/$@epics/medm build/$@epics-medm
	/bin/mv -f build/$@epics/config build/$@epics-config
	(/bin/rm -rf target/$@epics build/$@epics; make -f config/Makefile.$@epics RCG_SRC_DIR=`pwd`)
	(cd src/epics/util; ./nameLengthChk.pl $@)
	/bin/mkdir -p build/$@epics
	/bin/mv -f build/$@epics-medm build/$@epics/medm
	/bin/mv -f build/$@epics-config build/$@epics/config
	(cd src/fe/$@; make clean; make)
	@echo
	@echo The following files were used for this build:
	@cat src/epics/util/sources
	@echo
	@echo Successfully compiled

install-%:
	@system=$(subst install-,,$@); ./install $${system} .

uninstall-%:
	@system=$(subst uninstall-,,$@); ./uninstall $${system} .

% :: config/Makefile.%epics
	(/bin/rm -rf target/$@epics build/$@epics; make -f config/Makefile.$@epics)

# Lighter installation rule, do not reinstall screens and config files
# Install Epics and FE targets only
reinstall-% :: src/epics/simLink/%.mdl
	@system=$(subst reinstall-,,$@); \
	upper_system=`echo $$system | tr a-z A-Z`;\
	site=`grep site= target/$${system}epics/$${system}epics*.cmd | sed 's/.*site=\([a-z]*\).*/\1/g'`; \
	if test $${site}no = no; then echo Please make $$system first; exit 1; fi;\
	ifo=`grep ifo= target/$${system}epics/$${system}epics*.cmd | head -1 | sed 's/.*ifo=\([a-zA-Z0-9]*\).*/\1/g'`;\
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%M%S`;\
	/bin/mkdir -p /opt/rtcds/$$site/chans;\
	echo Installing Code Only system=$$system site=$$site ifo=$$ifo,$$lower_ifo;\
	echo Installing /opt/rtcds/$$site/target/$${lower_ifo}$${system}epics;\
	/bin/mkdir -p /opt/rtcds/$$site/target_archive;\
	if test -e /opt/rtcds/$$site/target/$${lower_ifo}$${system}epics; then /bin/mv -f /opt/rtcds/$$site/target/$${lower_ifo}$${system}epics /opt/rtcds/$$site/target_archive/$${lower_ifo}$${system}epics_$$cur_date || exit 1; fi;\
	/bin/mkdir -p /opt/rtcds/$$site/target;\
	/bin/cp -pr target/$${system}epics /opt/rtcds/$$site/target/$${lower_ifo}$${system}epics;\
	if test -e /opt/rtcds/$$site/target/$${lower_ifo}$${system}epics/db/*/autoBurt.req; then /bin/mv -f /opt/rtcds/$$site/target/$${lower_ifo}$${system}epics/db/*/autoBurt.req /opt/rtcds/$$site/target/$${lower_ifo}$${system}epics || exit 2; fi;\
	echo Installing /opt/rtcds/$$site/target/$${lower_ifo}$${system};\
	/bin/mkdir -p /opt/rtcds/$$site/target/$${lower_ifo}$${system}/archive;\
	if test -e /opt/rtcds/$$site/target/$${lower_ifo}$${system}/$${system}fe.ko; then /bin/mv -f /opt/rtcds/$$site/target/$${lower_ifo}$${system}/$${system}fe.ko /opt/rtcds/$$site/target/$${lower_ifo}$${system}/archive/$${system}fe_$${cur_date}.ko || exit 3; fi;\
	/bin/cp -pr src/fe/$${system}/$${system}fe.ko /opt/rtcds/$$site/target/$${lower_ifo}$${system};\
	echo 'sudo ' /opt/rtcds/$$site/target/$${lower_ifo}$${system}/$${system}fe.ko ' >  '/opt/rtcds/$$site/target/$${lower_ifo}$${system}/log.txt ' 2>& 1 &' > /opt/rtcds/$$site/target/$${lower_ifo}$${system}/startup.cmd;\
	/bin/chmod +x /opt/rtcds/$$site/target/$${lower_ifo}$${system}/startup.cmd

reinstall-fe-% :: src/epics/simLink/%.mdl
	@system=$(subst reinstall-fe-,,$@); \
	upper_system=`echo $$system | tr a-z A-Z`;\
	site=`grep site= target/$${system}epics/$${system}epics*.cmd | sed 's/.*site=\([a-z]*\).*/\1/g'`; \
	if test $${site}no = no; then echo Please make $$system first; exit 1; fi;\
	ifo=`grep ifo= target/$${system}epics/$${system}epics*.cmd | head -1 | sed 's/.*ifo=\([a-zA-Z0-9]*\).*/\1/g'`;\
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%M%S`;\
	/bin/mkdir -p /opt/rtcds/$$site/chans;\
	echo Installing Front-end Code Only system=$$system site=$$site ifo=$$ifo,$$lower_ifo;\
	/bin/mkdir -p /opt/rtcds/$$site/target/$${lower_ifo}$${system}/archive;\
	if test -e /opt/rtcds/$$site/target/$${lower_ifo}$${system}/$${system}fe.ko; then /bin/mv -f /opt/rtcds/$$site/target/$${lower_ifo}$${system}/$${system}fe.ko /opt/rtcds/$$site/target/$${lower_ifo}$${system}/archive/$${system}fe_$${cur_date}.ko || exit 3; fi;\
	/bin/cp -pr src/fe/$${system}/$${system}fe.ko /opt/rtcds/$$site/target/$${lower_ifo}$${system};\
	echo 'sudo /sbin/insmod' /opt/rtcds/$$site/target/$${lower_ifo}$${system}/$${system}fe.ko ' >  '/opt/rtcds/$$site/target/$${lower_ifo}$${system}/log.txt ' 2>& 1 &' > /opt/rtcds/$$site/target/$${lower_ifo}$${system}/startup.cmd;\
	/bin/chmod +x /opt/rtcds/$$site/target/$${lower_ifo}$${system}/startup.cmd

# This rule is for epics-only systems, ie. no front-end
install-% :: config/Makefile.%epics
	@system=$(subst install-,,$@); \
	site=`grep SITE $< | head -1 | sed 's/.*SITE\s*=\s*\([a-z]*\).*/\1/g'`; \
	ifo=`grep ifo= target/$${system}epics/$${system}epics*.cmd | head -1 |sed 's/.*ifo=\([a-zA-Z0-9]*\).*/\1/g'`;\
	if test $${ifo}no = no; then echo Please make $$system first; exit 1; fi;\
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%M%S`;\
	echo Installing /opt/rtcds/$$site/target/$${lower_ifo}$${system}epics;\
	/bin/mkdir -p /opt/rtcds/$$site/target_archive;\
	if test -e /opt/rtcds/$$site/target/$${lower_ifo}$${system}epics; then /bin/mv -f /opt/rtcds/$$site/target/$${lower_ifo}$${system}epics /opt/rtcds/$$site/target_archive/$${lower_ifo}$${system}epics_$$cur_date || exit 1; fi;\
	/bin/mkdir -p /opt/rtcds/$$site/target;\
	/bin/cp -pr target/$${system}epics /opt/rtcds/$$site/target/$${lower_ifo}$${system}epics;\
	if test -e /opt/rtcds/$$site/target/$${lower_ifo}$${system}epics/db/*/autoBurt.req; then /bin/mv -f /opt/rtcds/$$site/target/$${lower_ifo}$${system}epics/db/*/autoBurt.req /opt/rtcds/$$site/target/$${lower_ifo}$${system}epics || exit 2; fi;\

#dbbepics: config/Makefile.dbbepics
#	@echo Making dbbepics...
#	@make -f $<  >& dbbepics.makelog || \
#	(echo "FAILED: check dbbepics.makelog"; false)

# Show all predefined preprocessor definitions
dump_predefines:
	gcc -E -dM -x c /dev/null


# Build NDS program
nds:
	(cd src/nds; autoconf)
	/bin/rm -rf build/nds
	/bin/mkdir -p build/nds
	(cd build/nds; ../../src/nds/configure && make)

# Build frame builder data concentrator program
dc:
	(cd src/daqd; autoconf)
	/bin/rm -rf build/dc
	/bin/mkdir -p build/dc
	(cd build/dc; ../../src/daqd/configure '--enable-symmetricom' '--with-mx' '--without-myrinet' '--enable-debug' --with-epics=/opt/rtapps/epics/base '--with-framecpp=/opt/rtapps/framecpp' '--with-concentrator' && make)

# Build frame builder NDS or frame writer (broadcast receiver)
rcv:
	(cd src/daqd; autoconf)
	/bin/rm -rf build/rcv
	/bin/mkdir -p build/rcv
	(cd build/rcv; ../../src/daqd/configure '--disable-broadcast' '--enable-debug' '--with-broadcast' '--without-myrinet' '--with-framecpp=/opt/rtapps/framecpp' --with-epics=/opt/rtapps/epics/base  && make)

# build standalone frame builder
stand:
	(cd src/daqd; autoconf)
	/bin/rm -rf build/stand
	/bin/mkdir -p build/stand
	(cdir=`pwd`; cd build/stand; $$cdir/src/daqd/configure '--disable-broadcast' '--enable-debug' '--without-myrinet' '--with-epics=/opt/epics-3.14.9-linux/base' '--with-framecpp=/usr/local' && make)

# build  mx data receiving daqd
#
mx:
	(cd src/daqd; autoconf)
	/bin/rm -rf build/mx
	/bin/mkdir -p build/mx
	(cdir=`pwd`; cd build/mx; $$cdir/src/daqd/configure '--disable-broadcast' '--enable-debug' '--without-myrinet' '--with-epics=/opt/rtapps/epics-3.14.10/base' '--with-framecpp=/opt/rtapps/framecpp-1.18.2' --with-mx && make)

standiop:
	(cd src/daqd; autoconf)
	/bin/rm -rf build/standiop
	/bin/mkdir -p build/standiop
	(cdir=`pwd`; cd build/standiop; $$cdir/src/daqd/configure '--disable-broadcast' '--enable-debug' '--without-myrinet' '--with-epics=/opt/rtapps/epics/base' '--with-framecpp=/opt/rtapps/framecpp' --enable-iop && make)


#MDL_MODELS = x1cdst1 x1isiham x1isiitmx x1iss x1lsc x1omc1 x1psl x1susetmx x1susetmy x1susitmx x1susitmy x1susquad1 x1susquad2 x1susquad3 x1susquad4 x1x12 x1x13 x1x14 x1x15 x1x16 x1x20 x1x21 x1x22 x1x23

#MDL_MODELS = $(wildcard src/epics/simLink/l1*.mdl)
MDL_MODELS = $(shell cd src/epics/simLink; ls m1*.mdl | sed 's/.mdl//')

World: $(MDL_MODELS)
showWorld:
	@echo $(MDL_MODELS)

cleanWorld: $(patsubst %,clean-%,$(MDL_MODELS))

installWorld: $(patsubst %,install-target-%,$(MDL_MODELS)) $(patsubst %,install-daq-%,$(MDL_MODELS))  $(patsubst %,install-screens-%,$(MDL_MODELS))  


