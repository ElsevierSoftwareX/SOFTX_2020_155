all: 
	@echo Please build individual systems with 'make [system]' command

clean:
	/bin/rm -rf build
realclean:
	/bin/rm -rf build target

# Clean system, jsut say something like 'make clean-pde'
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
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | head -1 |sed 's/.*ifo=\([a-Z0-9]*\).*/\1/g'`;\
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%M%S`;\
	/bin/mkdir -p /cvs/cds/$$site/chans;\
	echo Installing system=$$system site=$$site ifo=$$ifo,$$lower_ifo;\
	echo Installing /cvs/cds/$$site/chans/$${ifo}$${upper_system}.txt;\
	/bin/mkdir -p /cvs/cds/$$site/chans/filter_archive/$$lower_ifo/$$system;\
	if test -e /cvs/cds/$$site/chans/$${ifo}$${upper_system}.txt; then /bin/mv -f  /cvs/cds/$$site/chans/$${ifo}$${upper_system}.txt /cvs/cds/$$site/chans/filter_archive/$$lower_ifo/$$system/$${ifo}$${upper_system}_$${cur_date}.txt || exit 1; \
	head -4 build/$${system}epics/config/$${ifo}$${upper_system}.txt > /cvs/cds/$$site/chans/$${ifo}$${upper_system}.txt;\
	/bin/grep '^# MODULES' build/$${system}epics/config/$${ifo}$${upper_system}.txt >> /cvs/cds/$$site/chans/$${ifo}$${upper_system}.txt;\
	tail -n +4 /cvs/cds/$$site/chans/filter_archive/$$lower_ifo/$$system/$${ifo}$${upper_system}_$${cur_date}.txt | grep -v '^# MODULES' >> /cvs/cds/$$site/chans/$${ifo}$${upper_system}.txt;\
	else /bin/cp -p build/$${system}epics/config/$${ifo}$${upper_system}.txt  /cvs/cds/$$site/chans/$${ifo}$${upper_system}.txt;\
	fi;\
	echo Installing /cvs/cds/$$site/target/$${lower_ifo}$${system}epics;\
	/bin/mkdir -p /cvs/cds/$$site/target_archive;\
	if test -e /cvs/cds/$$site/target/$${lower_ifo}$${system}epics; then /bin/mv -f /cvs/cds/$$site/target/$${lower_ifo}$${system}epics /cvs/cds/$$site/target_archive/$${lower_ifo}$${system}epics_$$cur_date || exit 2; fi;\
	/bin/mkdir -p /cvs/cds/$$site/target;\
	/bin/cp -pr target/$${system}epics /cvs/cds/$$site/target/$${lower_ifo}$${system}epics;\
	if test -e /cvs/cds/$$site/target/$${lower_ifo}$${system}epics/db/*/autoBurt.req; then /bin/mv -f /cvs/cds/$$site/target/$${lower_ifo}$${system}epics/db/*/autoBurt.req /cvs/cds/$$site/target/$${lower_ifo}$${system}epics || exit 3; fi;\
	echo Installing /cvs/cds/$$site/target/$${lower_ifo}$${system};\
	/bin/mkdir -p /cvs/cds/$$site/target/$${lower_ifo}$${system}/archive;\
	if test -e /cvs/cds/$$site/target/$${lower_ifo}$${system}/$${system}fe.rtl; then /bin/mv -f /cvs/cds/$$site/target/$${lower_ifo}$${system}/$${system}fe.rtl /cvs/cds/$$site/target/$${lower_ifo}$${system}/archive/$${system}fe_$${cur_date}.rtl || exit 4; fi;\
	/bin/cp -p src/fe/$${system}/$${system}fe.rtl /cvs/cds/$$site/target/$${lower_ifo}$${system};\
	echo 'sudo ' /cvs/cds/$$site/target/$${lower_ifo}$${system}/$${system}fe.rtl ' >  '/cvs/cds/$$site/target/$${lower_ifo}$${system}/log.txt ' 2>& 1 &' > /cvs/cds/$$site/target/$${lower_ifo}$${system}/startup.cmd;\
	/bin/chmod +x /cvs/cds/$$site/target/$${lower_ifo}$${system}/startup.cmd;\
	echo Installing start and stop scripts;\
	/bin/mkdir -p /cvs/cds/$$site/scripts;\
	echo '#!/bin/bash' > /cvs/cds/$$site/scripts/start$${system};\
	/bin/chmod +x /cvs/cds/$$site/scripts/start$${system};\
	echo '#!/bin/bash' > /cvs/cds/$$site/scripts/kill$${system};\
	/bin/chmod +x /cvs/cds/$$site/scripts/kill$${system};\
	echo 'if [ `hostname` != '$${hostname}' ]; then' >> /cvs/cds/$$site/scripts/start$${system};\
	echo 'echo Cannot run `basename $$0` on `hostname` computer' >> /cvs/cds/$$site/scripts/start$${system};\
	echo 'exit 1' >> /cvs/cds/$$site/scripts/start$${system};\
	echo 'fi' >> /cvs/cds/$$site/scripts/start$${system};\
	echo 'if [ "x`ps h -C ' $${system}epics '`" != x ]; then' >> /cvs/cds/$$site/scripts/kill$${system};\
	echo 'cur_date=`date +%y%m%d_%H%M%S`' >> /cvs/cds/$$site/scripts/kill$${system};\
	echo 'burtrb -f /cvs/cds/'$${site}'/target/'$${lower_ifo}$${system}'epics/autoBurt.req -o /tmp/'$${system}'_burt_$${cur_date}.snap -l /tmp/'$${system}'_burt_$${cur_date}.log -v' >> /cvs/cds/$$site/scripts/kill$${system};\
	echo 'sleep 1' >> /cvs/cds/$$site/scripts/kill$${system};\
	echo 'fi' >> /cvs/cds/$$site/scripts/kill$${system};\
	echo /cvs/cds/$$site/scripts/kill$${system} >> /cvs/cds/$$site/scripts/start$${system};\
	echo sleep 5 >> /cvs/cds/$$site/scripts/start$${system};\
	echo 'sudo killall ' $${system}epics $${system}fe.rtl >> /cvs/cds/$$site/scripts/kill$${system};\
	echo 'res=`ps h -C awgtpman | grep ' $${system} '`' >> /cvs/cds/$$site/scripts/kill$${system};\
	echo 'if [ "x$${res}" != x ]; then' >> /cvs/cds/$$site/scripts/kill$${system};\
	echo 'num=$$(echo $${res} | awk '"'"'{print $$1}'"'"')' >> /cvs/cds/$$site/scripts/kill$${system};\
	echo 'sudo kill $$num' >> /cvs/cds/$$site/scripts/kill$${system};\
	echo 'fi' >> /cvs/cds/$$site/scripts/kill$${system};\
	echo '(cd /cvs/cds/'$$site'/target/'$${lower_ifo}$${system}epics' && ./startup'$${ifo}')' >> /cvs/cds/$$site/scripts/start$${system};\
	echo /cvs/cds/$$site/target/$${lower_ifo}$${system}/startup.cmd >> /cvs/cds/$$site/scripts/start$${system};\
	echo '(cd /cvs/cds/'$$site'/target/gds && ./startup_'$${system}'.cmd)' >> /cvs/cds/$$site/scripts/start$${system};\
	echo 'sleep 5; sudo killall -q daqd' >> /cvs/cds/$$site/scripts/start$${system};\
	echo 'fname=`ls -t /tmp/'$${system}'_burt_*.snap  2>/dev/null | head -1`' >> /cvs/cds/$$site/scripts/start$${system};\
	echo 'if [ "x$$fname" != x ]; then' >> /cvs/cds/$$site/scripts/start$${system};\
	echo 'log_fname=$${fname%.*}.log' >> /cvs/cds/$$site/scripts/start$${system};\
	echo 'burtwb -f $${fname} -l $${log_fname} -v' >> /cvs/cds/$$site/scripts/start$${system};\
	echo 'fi' >> /cvs/cds/$$site/scripts/start$${system};\
	echo touch /cvs/cds/$$site/target/$${lower_ifo}$${system}/reboot.log >> /cvs/cds/$$site/scripts/start$${system};\
	echo chmod 777 /cvs/cds/$$site/target/$${lower_ifo}$${system}/reboot.log >> /cvs/cds/$$site/scripts/start$${system};\
	/bin/sed 's/caltech/'$$site'/' src/epics/util/daqconfig.tcl > build/$${system}epics/config/daqconfig;\
	/usr/bin/install   build/$${system}epics/config/daqconfig  /cvs/cds/$$site/scripts


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
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | head -1 | sed 's/.*ifo=\([a-Z0-9]*\).*/\1/g'`;\
	gds_node=`grep rmid build/$${system}epics/$${system}.par | head -1| sed 's/[^0-9]*\([0-9]*\)/\1/'`; \
	datarate=`grep datarate build/$${system}epics/$${system}.par | head -1| sed 's/[^0-9]*\([0-9]*\)/\1/'`; \
	gds_file_node=`expr $${gds_node} + 1`; \
	datarate_mult=`expr $${datarate} / 16384 `; \
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%M%S`;\
	echo Removing GDS node $${gds_file_node} configuration file ;\
	echo /cvs/cds/$${site}/target/gds/param/tpchn_$${site_letter}$${gds_file_node}.par ;\
	/bin/rm -f  /cvs/cds/$${site}/target/gds/param/tpchn_$${site_letter}$${gds_file_node}.par;\
	echo Removing DAQ configuration file;\
	echo /cvs/cds/$${site}/chans/daq/$${ifo}$${upper_system}.ini;\
	/bin/rm -f  /cvs/cds/$${site}/chans/daq/$${ifo}$${upper_system}.ini


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
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | head -1 | sed 's/.*ifo=\([a-Z0-9]*\).*/\1/g'`;\
	gds_node=`grep rmid build/$${system}epics/$${system}.par | head -1| sed 's/[^0-9]*\([0-9]*\)/\1/'`; \
	datarate=`grep datarate build/$${system}epics/$${system}.par | head -1| sed 's/[^0-9]*\([0-9]*\)/\1/'`; \
	gds_file_node=`expr $${gds_node} + 1`; \
	datarate_mult=`expr $${datarate} / 16384 `; \
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%M%S`;\
	echo Installing GDS node $${gds_file_node} configuration file ;\
	echo /cvs/cds/$${site}/target/gds/param/tpchn_$${site_letter}$${gds_file_node}.par ;\
	/bin/mkdir -p  /cvs/cds/$${site}/target/gds/param/ ;\
	/bin/mkdir -p  /cvs/cds/$${site}/target/gds/param/archive ;\
	if test -e /cvs/cds/$${site}/target/gds/param/tpchn_$${site_letter}$${gds_file_node}.par; then /bin/mv -f /cvs/cds/$${site}/target/gds/param/tpchn_$${site_letter}$${gds_file_node}.par /cvs/cds/$${site}/target/gds/param/archive/tpchn_$${site_letter}$${gds_file_node}_$${cur_date}.par || exit 1; fi;\
	/bin/cp -p build/$${system}epics/$${system}.par /cvs/cds/$${site}/target/gds/param/tpchn_$${site_letter}$${gds_file_node}.par ;\
	if test $${datarate_mult} -gt 1;\
	then \
	  datarate_mult_flag=-$${datarate_mult}; \
	else \
	  datarate_mult_flag=; \
	fi; \
	echo '#!/bin/bash' > /cvs/cds/$${site}/target/gds/startup_$${system}.cmd ;\
	echo 'cd /cvs/cds/'$${site}'/target/gds; sudo /cvs/cds/'$${site}'/target/gds/bin/awgtpman -s '$${system}' '$${datarate_mult_flag}' > '$${system}'.log 2>& 1 &' >> /cvs/cds/$${site}/target/gds/startup_$${system}.cmd ;\
	/bin/chmod +x /cvs/cds/$${site}/target/gds/startup_$${system}.cmd ;\
	echo Updating DAQ configuration file ;\
	echo /cvs/cds/$${site}/chans/daq/$${ifo}$${upper_system}.ini ;\
	/bin/mkdir -p  /cvs/cds/$${site}/chans/daq ;\
	/bin/mkdir -p  /cvs/cds/$${site}/chans/daq/archive ;\
	if test -e /cvs/cds/$${site}/chans/daq/$${ifo}$${upper_system}.ini;\
	then \
	  /bin/mv -f /cvs/cds/$${site}/chans/daq/$${ifo}$${upper_system}.ini /cvs/cds/$${site}/chans/daq/archive/$${ifo}$${upper_system}_$${cur_date}.ini || exit 2 ;\
	  echo src/epics/util/updateDaqConfig1.pl -daq_old=/cvs/cds/$${site}/chans/daq/archive/$${ifo}$${upper_system}_$${cur_date}.ini -old=/cvs/cds/$${site}/target/gds/param/archive/tpchn_$${site_letter}$${gds_file_node}_$${cur_date}.par -new=build/$${system}epics/$${system}.par -daq=build/$${system}epics/$${system}.ini ;\
	  src/epics/util/updateDaqConfig1.pl -daq_old=/cvs/cds/$${site}/chans/daq/archive/$${ifo}$${upper_system}_$${cur_date}.ini -old=/cvs/cds/$${site}/target/gds/param/archive/tpchn_$${site_letter}$${gds_file_node}_$${cur_date}.par -new=build/$${system}epics/$${system}.par -daq=build/$${system}epics/$${system}.ini > /cvs/cds/$${site}/chans/daq/$${ifo}$${upper_system}.ini ; \
	else \
	  /bin/cp -p build/$${system}epics/$${system}.ini /cvs/cds/$${site}/chans/daq/$${ifo}$${upper_system}.ini ;\
	fi

install-screens-% :: src/epics/simLink/%.mdl
	@system=$(subst install-screens-,,$@); \
	upper_system=`echo $$system | tr a-z A-Z`;\
	site=`grep site target/$${system}epics/$${system}epics*.cmd | sed 's/.*site=\([a-z]*\).*/\1/g'`; \
	if test $${site}no = no; then echo Please make $$system first; exit 1; fi;\
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | head -1 |sed 's/.*ifo=\([a-Z0-9]*\).*/\1/g'`;\
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%M%S`;\
	echo Installing Epics screens;\
	/bin/mkdir -p /cvs/cds/$$site/medm/$${lower_ifo}/archive;\
	if test -e /cvs/cds/$$site/medm/$${lower_ifo}/$${system}; then /bin/cp -a /cvs/cds/$$site/medm/$${lower_ifo}/$${system} /cvs/cds/$$site/medm/$${lower_ifo}/archive/$${system}_$${cur_date} || exit 1; fi;\
	/bin/mkdir -p /cvs/cds/$$site/medm/$${lower_ifo}/$${system};\
	(cd build/$${system}epics/medm; ls | xargs cp -r -t /cvs/cds/$$site/medm/$${lower_ifo}/$${system};)


# Lighter installation rule, do not reinstall screens and config files
# Install Epics and FE targets only
reinstall-% :: src/epics/simLink/%.mdl
	@system=$(subst reinstall-,,$@); \
	upper_system=`echo $$system | tr a-z A-Z`;\
	site=`grep site target/$${system}epics/$${system}epics*.cmd | sed 's/.*site=\([a-z]*\).*/\1/g'`; \
	if test $${site}no = no; then echo Please make $$system first; exit 1; fi;\
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | head -1 | sed 's/.*ifo=\([a-Z0-9]*\).*/\1/g'`;\
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%M%S`;\
	/bin/mkdir -p /cvs/cds/$$site/chans;\
	echo Installing Code Only system=$$system site=$$site ifo=$$ifo,$$lower_ifo;\
	echo Installing /cvs/cds/$$site/target/$${lower_ifo}$${system}epics;\
	/bin/mkdir -p /cvs/cds/$$site/target_archive;\
	if test -e /cvs/cds/$$site/target/$${lower_ifo}$${system}epics; then /bin/mv -f /cvs/cds/$$site/target/$${lower_ifo}$${system}epics /cvs/cds/$$site/target_archive/$${lower_ifo}$${system}epics_$$cur_date || exit 1; fi;\
	/bin/mkdir -p /cvs/cds/$$site/target;\
	/bin/cp -pr target/$${system}epics /cvs/cds/$$site/target/$${lower_ifo}$${system}epics;\
	if test -e /cvs/cds/$$site/target/$${lower_ifo}$${system}epics/db/*/autoBurt.req; then /bin/mv -f /cvs/cds/$$site/target/$${lower_ifo}$${system}epics/db/*/autoBurt.req /cvs/cds/$$site/target/$${lower_ifo}$${system}epics || exit 2; fi;\
	echo Installing /cvs/cds/$$site/target/$${lower_ifo}$${system};\
	/bin/mkdir -p /cvs/cds/$$site/target/$${lower_ifo}$${system}/archive;\
	if test -e /cvs/cds/$$site/target/$${lower_ifo}$${system}/$${system}fe.rtl; then /bin/mv -f /cvs/cds/$$site/target/$${lower_ifo}$${system}/$${system}fe.rtl /cvs/cds/$$site/target/$${lower_ifo}$${system}/archive/$${system}fe_$${cur_date}.rtl || exit 3; fi;\
	/bin/cp -pr src/fe/$${system}/$${system}fe.rtl /cvs/cds/$$site/target/$${lower_ifo}$${system};\
	echo 'sudo ' /cvs/cds/$$site/target/$${lower_ifo}$${system}/$${system}fe.rtl ' >  '/cvs/cds/$$site/target/$${lower_ifo}$${system}/log.txt ' 2>& 1 &' > /cvs/cds/$$site/target/$${lower_ifo}$${system}/startup.cmd;\
	/bin/chmod +x /cvs/cds/$$site/target/$${lower_ifo}$${system}/startup.cmd

reinstall-fe-% :: src/epics/simLink/%.mdl
	@system=$(subst reinstall-fe-,,$@); \
	upper_system=`echo $$system | tr a-z A-Z`;\
	site=`grep site target/$${system}epics/$${system}epics*.cmd | sed 's/.*site=\([a-z]*\).*/\1/g'`; \
	if test $${site}no = no; then echo Please make $$system first; exit 1; fi;\
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | head -1 | sed 's/.*ifo=\([a-Z0-9]*\).*/\1/g'`;\
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%M%S`;\
	/bin/mkdir -p /cvs/cds/$$site/chans;\
	echo Installing Front-end Code Only system=$$system site=$$site ifo=$$ifo,$$lower_ifo;\
	/bin/mkdir -p /cvs/cds/$$site/target/$${lower_ifo}$${system}/archive;\
	if test -e /cvs/cds/$$site/target/$${lower_ifo}$${system}/$${system}fe.rtl; then /bin/mv -f /cvs/cds/$$site/target/$${lower_ifo}$${system}/$${system}fe.rtl /cvs/cds/$$site/target/$${lower_ifo}$${system}/archive/$${system}fe_$${cur_date}.rtl || exit 3; fi;\
	/bin/cp -pr src/fe/$${system}/$${system}fe.rtl /cvs/cds/$$site/target/$${lower_ifo}$${system};\
	echo 'sudo ' /cvs/cds/$$site/target/$${lower_ifo}$${system}/$${system}fe.rtl ' >  '/cvs/cds/$$site/target/$${lower_ifo}$${system}/log.txt ' 2>& 1 &' > /cvs/cds/$$site/target/$${lower_ifo}$${system}/startup.cmd;\
	/bin/chmod +x /cvs/cds/$$site/target/$${lower_ifo}$${system}/startup.cmd

# This rule is for epics-only systems, ie. no front-end
install-% :: config/Makefile.%epics
	@system=$(subst install-,,$@); \
	site=`grep SITE $< | sed 's/.*SITE\s*=\s*\([a-z]*\).*/\1/g'`; \
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | head -1 |sed 's/.*ifo=\([a-Z0-9]*\).*/\1/g'`;\
	if test $${ifo}no = no; then echo Please make $$system first; exit 1; fi;\
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%M%S`;\
	echo Installing /cvs/cds/$$site/target/$${lower_ifo}$${system}epics;\
	/bin/mkdir -p /cvs/cds/$$site/target_archive;\
	if test -e /cvs/cds/$$site/target/$${lower_ifo}$${system}epics; then /bin/mv -f /cvs/cds/$$site/target/$${lower_ifo}$${system}epics /cvs/cds/$$site/target_archive/$${lower_ifo}$${system}epics_$$cur_date || exit 1; fi;\
	/bin/mkdir -p /cvs/cds/$$site/target;\
	/bin/cp -pr target/$${system}epics /cvs/cds/$$site/target/$${lower_ifo}$${system}epics;\
	if test -e /cvs/cds/$$site/target/$${lower_ifo}$${system}epics/db/*/autoBurt.req; then /bin/mv -f /cvs/cds/$$site/target/$${lower_ifo}$${system}epics/db/*/autoBurt.req /cvs/cds/$$site/target/$${lower_ifo}$${system}epics || exit 2; fi;\

#dbbepics: config/Makefile.dbbepics
#	@echo Making dbbepics...
#	@make -f $<  >& dbbepics.makelog || \
#	(echo "FAILED: check dbbepics.makelog"; false)

# Show all predefined preprocessor definitions
dump_predefines:
	gcc -E -dM -x c /dev/null

