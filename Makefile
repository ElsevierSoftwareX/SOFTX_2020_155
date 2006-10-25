all: pde omc dbb sus hepi

clean:
	/bin/rm -rf build
realclean:
	/bin/rm -rf build target


# With this rule one can make any system
# Just say 'make pde', for instance, to make PDE system
# Epics and front-end parts
% :: src/epics/simLink/%.mdl
	(cd src/epics/util; ./feCodeGen.pl ../simLink/$@.mdl $@)
	(/bin/rm -rf target/$@epics build/$@epics; make -f config/Makefile.$@epics)
	(cd src/fe/$@; make clean; make)

#dbbepics: config/Makefile.dbbepics
#	@echo Making dbbepics...
#	@make -f $<  >& dbbepics.makelog || \
#	(echo "FAILED: check dbbepics.makelog"; false)

# Show all predefined preprocessor definitions
dump_predefines:
	gcc -E -dM -x c /dev/null

