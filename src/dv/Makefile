# Dataviewer Makefile

# Dataviewer 'version'
VERSION=3.2

# Installation Location
INSTALL_PREFIX=/ligo/apps

# Host OS/Architecture selection
# Supported systems are Darwin, Linux, SunOS
# There's probably a more elegant way to do this, but this works reasonably
# enough.

ifeq ($(shell uname), Darwin)
BUILD_OS=darwin
endif

ifeq ($(shell uname), Linux)
BUILD_OS=linux
endif

ifeq ($(shell uname), SunOS)
BUILD_OS=solaris
endif

ifeq ($(BUILD_OS),)
$(error Build OS is unsupported.)
endif

APPLOC=${APPSROOT}
RTAPPLOC=${RTAPPSROOT}

all:
	@echo Building for ${BUILD_OS} ...
	cd Lib/UTC_GPS; make
	cd Th; make
	cd Display; make -f Makefile.${BUILD_OS} framer4 frameMemRead
	cd Control/java_widgets/lib; make -f Makefile.${BUILD_OS}
	cd Control; make -f Makefile.${BUILD_OS} dc3

rtinstall:
	mkdir -p $(RTAPPLOC)/dv-${VERSION}
	install Control/dc3 $(RTAPPLOC)/dv-${VERSION}/dv
	sed 's:INSTALL_PATH:$(RTAPPLOC)/dv-${VERSION}:' Control/dataviewer.${BUILD_OS} | sed 's:LOCAL_GRACE:$(RTAPPLOC):' > $(RTAPPLOC)/dv-${VERSION}/dataviewer
	chmod a+x $(RTAPPLOC)/dv-${VERSION}/dataviewer
	install Display/framer4 $(RTAPPLOC)/dv-${VERSION}
	install Display/frameMemRead $(RTAPPLOC)/dv-${VERSION}

install:
	mkdir -p $(APPLOC)/dv-${VERSION}
	install Control/dc3 $(APPLOC)/dv-${VERSION}/dv
	sed 's:INSTALL_PATH:$(APPLOC)/dv-${VERSION}:' Control/dataviewer.${BUILD_OS} | sed 's:LOCAL_GRACE:$(APPLOC):' > $(APPLOC)/dv-${VERSION}/dataviewer
	chmod a+x $(APPLOC)/dv-${VERSION}/dataviewer
	sed 's:INSTALL_PATH:$(APPLOC)/dv-${VERSION}:' Control/dataviewer_msgwin.${BUILD_OS} | sed 's:LOCAL_GRACE:$(APPLOC):' > $(APPLOC)/dv-${VERSION}/dataviewer_msgwin
	chmod a+x $(APPLOC)/dv-${VERSION}/dataviewer_msgwin
	install Display/framer4 $(APPLOC)/dv-${VERSION}
	install Display/frameMemRead $(APPLOC)/dv-${VERSION}
	cp Lib/UTC_GPS/leapsecs.dat $(APPLOC)/dv-${VERSION}

clean:
	cd Lib/UTC_GPS; make clean
	cd Th; make clean
	cd Display; make -f Makefile.${BUILD_OS} clean
	cd Control/java_widgets/lib; make -f Makefile.${BUILD_OS} clean
	cd Control; make -f Makefile.${BUILD_OS} clean
