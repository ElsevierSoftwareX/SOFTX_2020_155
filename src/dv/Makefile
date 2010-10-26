all:
	cd Lib/UTC_GPS; make
	-cd Th; make
	-cd Display; make framer4 frameMemRead
	cd Control; make dc3

install:
	install Control/dc3 /usr/local/bin
	install Control/dataviewer /usr/local/bin
	install Display/framer4 /usr/local/bin
	install Display/frameMemRead /usr/local/bin
	
clean:
	cd Lib/UTC_GPS; make clean
	cd Th; make clean
	cd Display; make clean
	cd Control; make clean
