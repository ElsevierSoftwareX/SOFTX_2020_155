all:
	cd Lib/UTC_GPS; make
	-cd Th; make
	-cd Display; make framer4 frameMemRead
	cd Control; make dc3

install:
	mkdir -p /opt/rtapps/dv
	install Control/dc3 /opt/rtapps/dv
	sed 's:INSTALL_PATH:/opt/rtapps/dv:' Control/dataviewer | sed 's:LOCAL_GRACE:/opt/rtapps:' > /opt/rtapps/dv/dataviewer
	chmod a+x /opt/rtapps/dv/dataviewer
#	install Control/dataviewer /opt/rtapps/dv
	install Display/framer4 /opt/rtapps/dv
	install Display/frameMemRead /opt/rtapps/dv
	
ligoapps:
	mkdir -p /ligo/apps/linux-x86_64/dv-2.3
	install Control/dc3 /ligo/apps/linux-x86_64/dv-2.3
	sed 's:INSTALL_PATH:/ligo/apps/linux-x86_64/dv-2.3:' Control/dataviewer | sed 's:LOCAL_GRACE:/ligo/apps/linux-x86_64:' > /ligo/apps/linux-x86_64/dv-2.3/dataviewer
	chmod a+x /ligo/apps/linux-x86_64/dv-2.3/dataviewer
#	install Control/dataviewer /ligo/apps/linux-x86_64/dv-2.3
	install Display/framer4 /ligo/apps/linux-x86_64/dv-2.3
	install Display/frameMemRead /ligo/apps/linux-x86_64/dv-2.3

clean:
	cd Lib/UTC_GPS; make clean
	cd Th; make clean
	cd Display; make clean
	cd Control; make clean
