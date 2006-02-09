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

pnmepics: pnmsusepics

pnmsusepics: config/Makefile.pnmsusepics
	@echo Making pnmsusepics...
	@make -f $<  >& pnmsusepics.makelog || \
	(echo "FAILED: check pnmsusepics.makelog"; false)

susepics: config/Makefile.susepics
	@echo Making susepics...
	@make -f $<  >& susepics.makelog || \
	(echo "FAILED: check susepics.makelog"; false)

hepiepics: config/Makefile.hepiepics
	@echo Making hepiepics...
	@make -f $<  >& hepiepics.makelog || \
	(echo "FAILED: check hepiepics.makelog"; false)

# Build front-end systems 
fe:
	cd src/fe; $(MAKE) all

# Show all predefined preprocessor definitions
dump_predefines:
	gcc -E -dM -x c /dev/null


