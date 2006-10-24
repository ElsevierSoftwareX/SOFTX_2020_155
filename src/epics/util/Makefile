
pde:
	./feCodeGen.pl ../simLink/pde.mdl pde 10 M1 32K
	(cd ../../../; /bin/rm -rf target/pdeepics /build/pdeepics; make -f config/Makefile.pdeepics)
	(cd ../../fe/pde; make clean; make)

