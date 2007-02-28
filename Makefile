all: pde omc dbb sus hepi

clean:
	/bin/rm -rf build
realclean:
	/bin/rm -rf build target

# Clean system, jsut say something like 'make clean-pde'
clean-% :: src/epics/simLink/%.mdl
	@system=$(subst clean-,,$@); \
	/bin/rm -rf target/$${system}epics build/$${system}epics; \
	(cd src/fe/$${system}; make clean)

# With this rule one can make any system
# Just say 'make pde', for instance, to make PDE system
# Epics and front-end parts
% :: src/epics/simLink/%.mdl
	(cd src/epics/util; ./feCodeGen.pl ../simLink/$@.mdl $@)
	(/bin/rm -rf target/$@epics build/$@epics; make -f config/Makefile.$@epics)
	(cd src/fe/$@; make clean; make)

% :: config/Makefile.%epics
	(/bin/rm -rf target/$@epics build/$@epics; make -f config/Makefile.$@epics)

# With this rule one can install any system
# By saying 'make install-pde', for example
install-% :: src/epics/simLink/%.mdl
	@system=$(subst install-,,$@); \
	upper_system=`echo $$system | tr a-z A-Z`;\
	site=`grep site target/$${system}epics/$${system}epics*.cmd | sed 's/.*site=\([a-z]*\).*/\1/g'`; \
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | sed 's/.*ifo=\([a-Z0-9]*\).*/\1/g'`;\
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%m%S`;\
	/bin/mkdir -p /cvs/cds/$$site/chans;\
	echo Installing system=$$system site=$$site ifo=$$ifo,$$lower_ifo;\
	echo Installing /cvs/cds/$$site/chans/daq/$${ifo}$${upper_system}.ini;\
	/bin/mkdir -p /cvs/cds/$$site/chans/daq/archive;\
	/bin/mv -f  /cvs/cds/$$site/chans/daq/$${ifo}$${upper_system}.ini /cvs/cds/$$site/chans/daq/archive/$${ifo}$${upper_system}_$${cur_date}.ini;\
	/bin/cp src/epics/util/$${ifo}$${upper_system}.ini /cvs/cds/$$site/chans/daq;\
	echo Installing /cvs/cds/$$site/chans/$${ifo}$${upper_system}.txt;\
	/bin/mkdir -p /cvs/cds/$$site/chans/filter_archive/$$lower_ifo/$$system;\
	/bin/mv -f  /cvs/cds/$$site/chans/$${ifo}$${upper_system}.txt /cvs/cds/$$site/chans/filter_archive/$$lower_ifo/$$system/$${ifo}$${upper_system}_$${cur_date}.txt;\
	/bin/cp src/epics/util/$${ifo}$${upper_system}.txt /cvs/cds/$$site/chans;\
	echo Installing /cvs/cds/$$site/target/$${system}epics;\
	/bin/mkdir -p /cvs/cds/$$site/target_archive;\
	/bin/mv -f /cvs/cds/$$site/target/$${system}epics /cvs/cds/$$site/target_archive/$${system}epics_$$cur_date;\
	/bin/mkdir -p /cvs/cds/$$site/target;\
	/bin/cp -pr target/$${system}epics /cvs/cds/$$site/target;\
	/bin/mv -f /cvs/cds/$$site/target/$${system}epics/db/*/autoBurt.req /cvs/cds/$$site/target/$${system}epics;\
	echo Installing /cvs/cds/$$site/target/$${system};\
	/bin/mkdir -p /cvs/cds/$$site/target/$${system};\
	/bin/mv -f /cvs/cds/$$site/target/$${system}/$${system}fe.rtl /cvs/cds/$$site/target/$${system}/$${system}fe_$${cur_date}.rtl;\
	/bin/cp -pr src/fe/$${system}/$${system}fe.rtl /cvs/cds/$$site/target/$${system};\
	echo Installing Epics screens;\
	/bin/mv -f /cvs/cds/$$site/medm/$${lower_ifo}/$${system} /cvs/cds/$$site/medm/$${lower_ifo}/$${system}_$${cur_date};\
	/bin/mkdir -p /cvs/cds/$$site/medm/$${lower_ifo};\
	/bin/cp -pr src/epics/util/$${system} /cvs/cds/$$site/medm/$${lower_ifo}

install-% :: config/Makefile.%epics
	@system=$(subst install-,,$@); \
	site=`grep SITE $< | sed 's/.*SITE\s*=\s*\([a-z]*\).*/\1/g'`; \
	ifo=`grep ifo target/$${system}epics/$${system}epics*.cmd | sed 's/.*ifo=\([a-Z0-9]*\).*/\1/g'`;\
	lower_ifo=`echo $$ifo | tr A-Z a-z`;\
	cur_date=`date +%y%m%d_%H%m%S`;\
	echo Installing /cvs/cds/$$site/target/$${system}epics;\
	/bin/mkdir -p /cvs/cds/$$site/target_archive;\
	/bin/mv -f /cvs/cds/$$site/target/$${system}epics /cvs/cds/$$site/target_archive/$${system}epics_$$cur_date;\
	/bin/mkdir -p /cvs/cds/$$site/target;\
	/bin/cp -pr target/$${system}epics /cvs/cds/$$site/target;\
	/bin/mv -f /cvs/cds/$$site/target/$${system}epics/db/*/autoBurt.req /cvs/cds/$$site/target/$${system}epics;\

#dbbepics: config/Makefile.dbbepics
#	@echo Making dbbepics...
#	@make -f $<  >& dbbepics.makelog || \
#	(echo "FAILED: check dbbepics.makelog"; false)

# Show all predefined preprocessor definitions
dump_predefines:
	gcc -E -dM -x c /dev/null

