all:
	cd Lib/UTC_GPS; make
	-cd Th; make
	-cd Display; make framer4 frameMemRead
	cd Control; make dc3

install:
	mkdir -p /opt/rtapps/dv
	install Control/dc3 /opt/rtapps/dv
	install Control/dataviewer /opt/rtapps/dv
	install Display/framer4 /opt/rtapps/dv
	install Display/frameMemRead /opt/rtapps/dv
	
clean:
	cd Lib/UTC_GPS; make clean
	cd Th; make clean
	cd Display; make clean
	cd Control; make clean
