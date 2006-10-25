# Will do different things on Solaris and Linux
ifeq ($(shell uname), SunOS)
# Things to do on Solaris
all: fe
clean:
	cd src/fe; make clean
else
# Things to do on Linux
all: epics
clean:
	/bin/rm -rf build
realclean:
	/bin/rm -rf build target
endif

# Build Epics IOC systems
epics: susepics hepiepics pnmepics

pnmepics: config/Makefile.pnmepics
	@echo Making pnmepics...
	@make -f $<  >& pnmepics.makelog || \
	(echo "FAILED: check pnmepics.makelog"; false)

susepics: config/Makefile.susepics
	@echo Making susepics...
	@make -f $<  >& susepics.makelog || \
	(echo "FAILED: check susepics.makelog"; false)

hepiepics: config/Makefile.hepiepics
	@echo Making hepiepics...
	@make -f $<  >& hepiepics.makelog || \
	(echo "FAILED: check hepiepics.makelog"; false)

pdeepics: config/Makefile.pdeepics
	@echo Making pdeepics...
	@make -f $<  >& pdeepics.makelog || \
	(echo "FAILED: check pdeepics.makelog"; false)

omcepics: config/Makefile.omcepics
	@echo Making omcepics...
	@make -f $<  >& omcepics.makelog || \
	(echo "FAILED: check omcepics.makelog"; false)

# Build front-end systems 
fe:
	cd src/fe; $(MAKE) all

# Show all predefined preprocessor definitions
dump_predefines:
	gcc -E -dM -x c /dev/null


% :: src/epics/simLink/%.mdl
	(cd src/epics/util; ./feCodeGen.pl ../simLink/$@.mdl $@  10 G1 32K)
#pde:
#	./feCodeGen.pl ../simLink/pde.mdl pde 10 G1 32K
#	(cd ../../../; /bin/rm -rf target/pdeepics /build/pdeepics; make -f config/Makefile.pdeepics)
#	(cd ../../fe/pde; make clean; make)

