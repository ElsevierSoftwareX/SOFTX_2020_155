all: 
	@echo Please build individual systems with 'make [system]' command

clean:
	/bin/rm -rf build
realclean:
	/bin/rm -rf build target

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
% :: src/epics/simLink/%.mdl
	(cd src/epics/util; ./feCodeGen.pl ../simLink/$@.mdl $@)
	/bin/rm -rf build/$@epics-medm
	/bin/rm -rf build/$@epics-config
	/bin/mv -f build/$@epics/medm build/$@epics-medm
	/bin/mv -f build/$@epics/config build/$@epics-config
	(/bin/rm -rf target/$@epics build/$@epics; make -f config/Makefile.$@epics)
	/bin/mkdir -p build/$@epics
	/bin/mv -f build/$@epics-medm build/$@epics/medm
	/bin/mv -f build/$@epics-config build/$@epics/config
	(cd src/fe/$@; make clean; make)

% :: config/Makefile.%epics
	(/bin/rm -rf target/$@epics build/$@epics; make -f config/Makefile.$@epics)

# With this rule one can install any system
# By saying 'make install-pde', for example
install-% :: src/epics/simLink/%.mdl
	@system=$(subst install-,,$@); \
	upper_system=`echo $$system | tr a-z A-Z`;\
	hostname=`hostname`;\
	site=`grep site target/$${system}epics/$${system}epics*.cmd | sed 's/.*site=\([a-z]*\).*/\1/g'`; \
	if test $${site}no = no; then echo Please make $$system first; exit 1; fi;\
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | head -1 |sed 's/.*ifo=\([a-zA-Z0-9]*\).*/\1/g'`;\
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%M%S`;\
	/bin/mkdir -p /opt/rtcds/$$site/$${lower_ifo}/chans;\
	echo Installing system=$$system site=$$site ifo=$$ifo,$$lower_ifo;\
	echo Installing /opt/rtcds/$$site/$${lower_ifo}/chans/$${upper_system}.txt;\
	/bin/mkdir -p /opt/rtcds/$$site/$${lower_ifo}/chans/filter_archive/$$system;\
	if test -e /opt/rtcds/$$site/$${lower_ifo}/chans/$${upper_system}.txt; then /bin/mv -f  /opt/rtcds/$$site/$${lower_ifo}/chans/$${upper_system}.txt /opt/rtcds/$$site/$${lower_ifo}/chans/filter_archive/$$system/$${upper_system}_$${cur_date}.txt || exit 1; \
	head -4 build/$${system}epics/config/$${ifo}$${upper_system}.txt > /opt/rtcds/$$site/$${lower_ifo}/chans/$${upper_system}.txt;\
	/bin/grep '^# MODULES' build/$${system}epics/config/$${ifo}$${upper_system}.txt >> /opt/rtcds/$$site/$${lower_ifo}/chans/$${upper_system}.txt;\
	tail -n +4 /opt/rtcds/$$site/$${lower_ifo}/chans/filter_archive/$$system/$${upper_system}_$${cur_date}.txt | grep -v '^# MODULES' >> /opt/rtcds/$$site/$${lower_ifo}/chans/$${upper_system}.txt;\
	else /bin/cp -p build/$${system}epics/config/$${ifo}$${upper_system}.txt  /opt/rtcds/$$site/$${lower_ifo}/chans/$${upper_system}.txt;\
	fi;\
	echo Installing /opt/rtcds/$$site/$${lower_ifo}/target/$${system}epics;\
	if test -e /opt/rtcds/$$site/$${lower_ifo}/target/$${system}; then /bin/mkdir -p /opt/rtcds/$$site/$${lower_ifo}/target_archive/$${system}; /bin/mv -f /opt/rtcds/$$site/$${lower_ifo}/target/$${system} /opt/rtcds/$$site/$${lower_ifo}/target_archive/$${system}/$${system}_$$cur_date || exit 2; fi;\
	/bin/mkdir -p /opt/rtcds/$$site/$${lower_ifo}/target/$${system};\
	/bin/mkdir -p /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/bin;\
	/bin/mkdir -p /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/logs;\
	/bin/mkdir -p /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/scripts;\
	/bin/mkdir -p /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/simLink;\
	/bin/mkdir -p /opt/rtcds/$$site/$${lower_ifo}/target_archive/$${system};\
	/bin/mkdir -p /opt/rtcds/$$site/$${lower_ifo}/target;\
	/bin/cp -pr target/$${system}epics /opt/rtcds/$$site/$${lower_ifo}/target/$${system};\
	if test -e /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/$${system}epics/db/*/autoBurt.req; then /bin/mv -f /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/$${system}epics/db/*/autoBurt.req /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/$${system}epics || exit 3; fi;\
	echo Installing /opt/rtcds/$$site/$${lower_ifo}/target/$${system};\
	if test -e /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/$${system}fe.rtl; then /bin/mv -f /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/$${system}fe.rtl /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/archive/$${system}fe_$${cur_date}.rtl || exit 4; fi;\
	/bin/cp -p src/fe/$${system}/$${system}fe.rtl /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/bin/;\
	/bin/cp -p src/epics/simLink/$${system}.mdl /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/simLink/;\
	echo 'sudo ' /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/bin/$${system}fe.rtl ' >  '/opt/rtcds/$$site/$${lower_ifo}/target/$${system}/logs/log.txt ' 2>& 1 &' > /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/scripts/startup$${ifo}rt;\
	/bin/chmod +x /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/scripts/startup$${ifo}rt;\
	echo Installing start and stop scripts;\
	/bin/mkdir -p /opt/rtcds/$$site/$${lower_ifo}/scripts;\
	echo '#!/bin/bash' > /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	/bin/chmod +x /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo '#!/bin/bash' > /opt/rtcds/$$site/$${lower_ifo}/scripts/kill$${system};\
	/bin/chmod +x /opt/rtcds/$$site/$${lower_ifo}/scripts/kill$${system};\
	echo 'if [ `hostname` != '$${hostname}' ]; then' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo 'echo Cannot run `basename $$0` on `hostname` computer' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo 'exit 1' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo 'fi' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo 'if [ "x`ps h -C ' $${system}epics '`" != x ]; then' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/kill$${system};\
	echo 'cur_date=`date +%y%m%d_%H%M%S`' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/kill$${system};\
	echo 'burtrb -f /opt/rtcds/'$${site}'/'$${lower_ifo}'/target/'$${system}'/'$${system}epics'/autoBurt.req -o /tmp/'$${system}'_burt_$${cur_date}.snap -l /tmp/'$${system}'_burt_$${cur_date}.log -v' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/kill$${system};\
	echo 'sleep 1' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/kill$${system};\
	echo 'fi' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/kill$${system};\
	echo /opt/rtcds/$$site/$${lower_ifo}/scripts/kill$${system} >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo sleep 5 >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo 'sudo killall ' $${system}epics $${system}fe.rtl >> /opt/rtcds/$$site/$${lower_ifo}/scripts/kill$${system};\
	echo 'res=`ps h -C awgtpman | grep ' $${system} '`' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/kill$${system};\
	echo 'if [ "x$${res}" != x ]; then' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/kill$${system};\
	echo 'num=$$(echo $${res} | awk '"'"'{print $$1}'"'"')' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/kill$${system};\
	echo 'sudo kill $$num' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/kill$${system};\
	echo 'fi' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/kill$${system};\
	echo '(cd /opt/rtcds/'$$site'/'$${lower_ifo}'/target/'$${system}'/'$${system}epics' && ./startup'$${ifo}')' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo sleep 5 >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/scripts/startup$${ifo}rt >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo '(cd /opt/rtcds/'$$site'/'$${lower_ifo}'/target/gds && ./startup_'$${system}'.cmd)' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo 'sleep 5; sudo killall -q daqd' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo 'fname=`ls -t /tmp/'$${system}'_burt_*.snap  2>/dev/null | head -1`' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo 'if [ "x$$fname" != x ]; then' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo 'log_fname=$${fname%.*}.log' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo 'burtwb -f $${fname} -l $${log_fname} -v' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo 'fi' >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo touch /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/logs/reboot.log >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	echo chmod 777 /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/logs/reboot.log >> /opt/rtcds/$$site/$${lower_ifo}/scripts/start$${system};\
	/bin/sed 's/caltech/'$$site'/' src/epics/util/daqconfig.tcl > build/$${system}epics/config/daqconfig;\
	/usr/bin/install   build/$${system}epics/config/daqconfig  /opt/rtcds/$$site/$${lower_ifo}/scripts


uninstall-daq-% :: src/epics/simLink/%.mdl
	@system=$(subst uninstall-daq-,,$@); \
	upper_system=`echo $$system | tr a-z A-Z`;\
	site=`grep site target/$${system}epics/$${system}epics*.cmd | sed 's/.*site=\([a-z]*\).*/\1/g'`; \
	if test $${site}no = no; then echo Please make $$system first; exit 1; fi;\
	upper_site=`echo $$site | tr a-z A-Z`;\
	site_letter=M;\
	if test $${site} = llo; then site_letter=L; fi;\
	if test $${site} = lho; then site_letter=H; fi;\
	if test $${site} = geo; then site_letter=G; fi;\
	if test $${site} = caltech; then site_letter=C; fi;\
	if test $${site} = tst; then site_letter=X; fi;\
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | head -1 | sed 's/.*ifo=\([a-zA-Z0-9]*\).*/\1/g'`;\
	gds_node=`grep rmid build/$${system}epics/$${system}.par | head -1| sed 's/[^0-9]*\([0-9]*\)/\1/'`; \
	datarate=`grep datarate build/$${system}epics/$${system}.par | head -1| sed 's/[^0-9]*\([0-9]*\)/\1/'`; \
	gds_file_node=`expr $${gds_node} + 1`; \
	datarate_mult=`expr $${datarate} / 16384 `; \
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%M%S`;\
	echo Removing GDS node $${gds_file_node} configuration file ;\
	echo /opt/rtcds/$${site}/target/gds/param/tpchn_$${system}.par ;\
	/bin/rm -f  /opt/rtcds/$${site}/target/gds/param/tpchn_$${system}.par;\
	echo Removing DAQ configuration file;\
	echo /opt/rtcds/$${site}/$${lower_ifo}/chans/daq/$${upper_system}.ini;\
	/bin/rm -f  /opt/rtcds/$${site}/$${lower_ifo}/chans/daq/$${upper_system}.ini

install-daq-% :: src/epics/simLink/%.mdl
	@system=$(subst install-daq-,,$@); \
	upper_system=`echo $$system | tr a-z A-Z`;\
	site=`grep site target/$${system}epics/$${system}epics*.cmd | sed 's/.*site=\([a-z]*\).*/\1/g'`; \
	if test $${site}no = no; then echo Please make $$system first; exit 1; fi;\
	upper_site=`echo $$site | tr a-z A-Z`;\
	site_letter=M;\
	if test $${site} = llo; then site_letter=L; fi;\
	if test $${site} = lho; then site_letter=H; fi;\
	if test $${site} = geo; then site_letter=G; fi;\
	if test $${site} = caltech; then site_letter=C; fi;\
	if test $${site} = tst; then site_letter=X; fi;\
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | head -1 | sed 's/.*ifo=\([a-zA-Z0-9]*\).*/\1/g'`;\
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	gds_node=`grep rmid build/$${system}epics/$${system}.par | head -1| sed 's/[^0-9]*\([0-9]*\)/\1/'`; \
	datarate=`grep datarate build/$${system}epics/$${system}.par | head -1| sed 's/[^0-9]*\([0-9]*\)/\1/'`; \
	gds_file_node=`expr $${gds_node} + 1`; \
	datarate_mult=`expr $${datarate} / 16384 `; \
	cur_date=`date +%y%m%d_%H%M%S`;\
	echo Installing GDS node $${gds_file_node} configuration file ;\
	echo /opt/rtcds/$${site}/$${lower_ifo}/target/gds/param/tpchn_$${system}.par ;\
	/bin/mkdir -p  /opt/rtcds/$${site}/$${lower_ifo}/target/gds/ ;\
	/bin/mkdir -p  /opt/rtcds/$${site}/$${lower_ifo}/target/gds/param/ ;\
	/bin/mkdir -p  /opt/rtcds/$${site}/$${lower_ifo}/target/gds/param/archive ;\
	if test -e /opt/rtcds/$${site}/$${lower_ifo}/target/gds/param/tpchn_$${system}.par; then /bin/mv -f /opt/rtcds/$${site}/$${lower_ifo}/target/gds/param/tpchn_$${system}.par /opt/rtcds/$${site}/$${lower_ifo}/target/gds/param/archive/tpchn_$${site_letter}$${system}.par || exit 1; fi;\
	/bin/cp -p build/$${system}epics/$${system}.par /opt/rtcds/$${site}/$${lower_ifo}/target/gds/param/tpchn_$${system}.par ;\
	/bin/mkdir -p /opt/rtcds/$$site/$${lower_ifo}/target/$${system}/param;\
	/bin/cp -p build/$${system}epics/$${system}.par /opt/rtcds/$${site}/$${lower_ifo}/target/$${system}/param/tpchn_$${system}.par ;\
	/bin/cp -p build/$${system}epics/$${system}.par /cvs/cds/$${site}/target/gds/param/tpchn_$${system}.par ;\
	if test $${datarate_mult} -gt 1;\
	then \
	  datarate_mult_flag=-$${datarate_mult}; \
	else \
	  datarate_mult_flag=; \
	fi; \
	echo '#!/bin/bash' > /opt/rtcds/$${site}/$${lower_ifo}/target/gds/startup_$${system}.cmd ;\
	echo 'cd /opt/rtcds/'$${site}'/'$${lower_ifo}'/target/gds; sudo /opt/rtcds/'$${site}'/'$${lower_ifo}'/target/gds/bin/awgtpman -s '$${system}' '$${datarate_mult_flag}' > '$${system}'.log 2>& 1 &' >> /opt/rtcds/$${site}/$${lower_ifo}/target/gds/startup_$${system}.cmd ;\
	/bin/chmod +x /opt/rtcds/$${site}/$${lower_ifo}/target/gds/startup_$${system}.cmd ;\
	echo Updating DAQ configuration file ;\
	echo /opt/rtcds/$${site}/$${lower_ifo}/chans/daq/$${upper_system}.ini ;\
	/bin/mkdir -p  /opt/rtcds/$${site}/$${lower_ifo}/chans/daq ;\
	/bin/mkdir -p  /opt/rtcds/$${site}/$${lower_ifo}/chans/daq/archive ;\
	if test -e /opt/rtcds/$${site}/$${lower_ifo}/chans/daq/$${upper_system}.ini;\
	then \
	  /bin/mv -f /opt/rtcds/$${site}/$${lower_ifo}/chans/daq/$${upper_system}.ini /opt/rtcds/$${site}/$${lower_ifo}/chans/daq/archive/$${upper_system}_$${cur_date}.ini || exit 2 ;\
	  echo src/epics/util/updateDaqConfig1.pl -daq_old=/opt/rtcds/$${site}/$${lower_ifo}/chans/daq/archive/$${upper_system}_$${cur_date}.ini -old=/opt/rtcds/$${site}/$${lower_ifo}/target/gds/param/archive/tpchn_$${system}.par -new=build/$${system}epics/$${system}.par -daq=build/$${system}epics/$${system}.ini ;\
	  src/epics/util/updateDaqConfig1.pl -daq_old=/opt/rtcds/$${site}/$${lower_ifo}/chans/daq/archive/$${upper_system}_$${cur_date}.ini -old=/opt/rtcds/$${site}/$${lower_ifo}/target/gds/param/archive/tpchn_$${system}_$${cur_date}.par -new=build/$${system}epics/$${system}.par -daq=build/$${system}epics/$${system}.ini > /opt/rtcds/$${site}/$${lower_ifo}/chans/daq/$${upper_system}.ini ; \
	else \
	  /bin/cp -p build/$${system}epics/$${system}.ini /opt/rtcds/$${site}/$${lower_ifo}/chans/daq/$${upper_system}.ini ;\
	fi;\
	/bin/cp /opt/rtcds/$${site}/$${lower_ifo}/chans/daq/$${upper_system}.ini /opt/rtcds/$${site}/$${lower_ifo}/target/$${system}/param/$${upper_system}.ini;\

install-screens-% :: src/epics/simLink/%.mdl
	@system=$(subst install-screens-,,$@); \
	upper_system=`echo $$system | tr a-z A-Z`;\
	site=`grep site target/$${system}epics/$${system}epics*.cmd | sed 's/.*site=\([a-z]*\).*/\1/g'`; \
	if test $${site}no = no; then echo Please make $$system first; exit 1; fi;\
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | head -1 |sed 's/.*ifo=\([a-zA-Z0-9]*\).*/\1/g'`;\
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%M%S`;\
	echo Installing Epics screens;\
	/bin/mkdir -p /opt/rtcds/$$site/$${lower_ifo}/medm/archive;\
	if test -e /opt/rtcds/$$site/$${lower_ifo}/medm/$${system}; then /bin/cp -a /opt/rtcds/$$site/$${lower_ifo}/medm/$${system} /opt/rtcds/$$site/$${lower_ifo}/medm/archive/$${system}_$${cur_date} || exit 1; fi;\
	/bin/mkdir -p /opt/rtcds/$$site/$${lower_ifo}/medm/$${system};\
	(cd build/$${system}epics/medm; ls | xargs cp -r -t /opt/rtcds/$$site/$${lower_ifo}/medm/$${system};)


# Lighter installation rule, do not reinstall screens and config files
# Install Epics and FE targets only
reinstall-% :: src/epics/simLink/%.mdl
	@system=$(subst reinstall-,,$@); \
	upper_system=`echo $$system | tr a-z A-Z`;\
	site=`grep site target/$${system}epics/$${system}epics*.cmd | sed 's/.*site=\([a-z]*\).*/\1/g'`; \
	if test $${site}no = no; then echo Please make $$system first; exit 1; fi;\
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | head -1 | sed 's/.*ifo=\([a-zA-Z0-9]*\).*/\1/g'`;\
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
	if test -e /opt/rtcds/$$site/target/$${lower_ifo}$${system}/$${system}fe.rtl; then /bin/mv -f /opt/rtcds/$$site/target/$${lower_ifo}$${system}/$${system}fe.rtl /opt/rtcds/$$site/target/$${lower_ifo}$${system}/archive/$${system}fe_$${cur_date}.rtl || exit 3; fi;\
	/bin/cp -pr src/fe/$${system}/$${system}fe.rtl /opt/rtcds/$$site/target/$${lower_ifo}$${system};\
	echo 'sudo ' /opt/rtcds/$$site/target/$${lower_ifo}$${system}/$${system}fe.rtl ' >  '/opt/rtcds/$$site/target/$${lower_ifo}$${system}/log.txt ' 2>& 1 &' > /opt/rtcds/$$site/target/$${lower_ifo}$${system}/startup.cmd;\
	/bin/chmod +x /opt/rtcds/$$site/target/$${lower_ifo}$${system}/startup.cmd

reinstall-fe-% :: src/epics/simLink/%.mdl
	@system=$(subst reinstall-fe-,,$@); \
	upper_system=`echo $$system | tr a-z A-Z`;\
	site=`grep site target/$${system}epics/$${system}epics*.cmd | sed 's/.*site=\([a-z]*\).*/\1/g'`; \
	if test $${site}no = no; then echo Please make $$system first; exit 1; fi;\
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | head -1 | sed 's/.*ifo=\([a-zA-Z0-9]*\).*/\1/g'`;\
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%M%S`;\
	/bin/mkdir -p /opt/rtcds/$$site/chans;\
	echo Installing Front-end Code Only system=$$system site=$$site ifo=$$ifo,$$lower_ifo;\
	/bin/mkdir -p /opt/rtcds/$$site/target/$${lower_ifo}$${system}/archive;\
	if test -e /opt/rtcds/$$site/target/$${lower_ifo}$${system}/$${system}fe.rtl; then /bin/mv -f /opt/rtcds/$$site/target/$${lower_ifo}$${system}/$${system}fe.rtl /opt/rtcds/$$site/target/$${lower_ifo}$${system}/archive/$${system}fe_$${cur_date}.rtl || exit 3; fi;\
	/bin/cp -pr src/fe/$${system}/$${system}fe.rtl /opt/rtcds/$$site/target/$${lower_ifo}$${system};\
	echo 'sudo ' /opt/rtcds/$$site/target/$${lower_ifo}$${system}/$${system}fe.rtl ' >  '/opt/rtcds/$$site/target/$${lower_ifo}$${system}/log.txt ' 2>& 1 &' > /opt/rtcds/$$site/target/$${lower_ifo}$${system}/startup.cmd;\
	/bin/chmod +x /opt/rtcds/$$site/target/$${lower_ifo}$${system}/startup.cmd

# This rule is for epics-only systems, ie. no front-end
install-% :: config/Makefile.%epics
	@system=$(subst install-,,$@); \
	site=`grep SITE $< | sed 's/.*SITE\s*=\s*\([a-z]*\).*/\1/g'`; \
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | head -1 |sed 's/.*ifo=\([a-zA-Z0-9]*\).*/\1/g'`;\
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
	(cd src/nds; test -e configure || autoconf)
	/bin/rm -rf build/nds
	/bin/mkdir -p build/nds
	(cd build/nds; ../../src/nds/configure && make)

# Build frame builder data concentrator program
dc:
	(cd src/daqd; test -e configure || autoconf)
	/bin/rm -rf build/dc
	/bin/mkdir -p build/dc
	(cd build/dc; ../../src/daqd/configure '--enable-symmetricom' '--with-mx' '--enable-debug' '--with-gds=/apps/Linux/gds' '--with-epics=/opt/epics-3.14.9-linux/base' '--with-framecpp=/usr/local' '--with-concentrator' && make)

# Build frame builder NDS or frame writer (broadcast receiver)
rcv:
	(cd src/daqd; test -e configure || autoconf)
	/bin/rm -rf build/rcv
	/bin/mkdir -p build/rcv
	(cd build/rcv; ../../src/daqd/configure '--disable-broadcast' '--enable-debug' '--with-broadcast' '--without-myrinet' '--with-gds=/apps/Linux/gds' '--with-epics=/opt/epics-3.14.9-linux/base' '--with-framecpp=/usr/local' && make)

# build standalone frame builder with IOP timing
stand:
	(cd src/daqd; test -e configure || autoconf)
	/bin/rm -rf build/stand
	/bin/mkdir -p build/stand
	(cdir=`pwd`; cd build/stand; $$cdir/src/daqd/configure '--disable-broadcast' '--enable-debug' '--without-myrinet' '--with-epics=/opt/epics-3.14.9-linux/base' '--with-framecpp=/usr/local' --enable-symmetricom --enable-iop && make)
