all:
	cd Lib/UTC_GPS; make
	-cd Th; make
	-cd Display; make framer4 frameMemRead
	cd Control; make dc3

install:
	install Control/dc3 /opt/apps/dataviewer
	install Control/dataviewer /opt/apps/dataviewer
	install Display/framer4 /opt/apps/dataviewer
	install Display/frameMemRead /opt/apps/dataviewer
	
clean:
	cd Lib/UTC_GPS; make clean
	cd Th; make clean
	cd Display; make clean
	cd Control; make clean
