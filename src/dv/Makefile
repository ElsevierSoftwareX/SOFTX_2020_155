# Dataviewer Makefile

# Dataviewer 'version'
VERSION=2.8

# Installation Location
INSTALL_PREFIX=/ligo/apps

# Host OS/Architecture selection
# Supported systems are Darwin, Linux, SunOS
# There's probably a more elegant way to do this, but this works reasonably
# enough.

ifeq ($(shell uname), Darwin)
BUILD_OS=darwin
BUILD_ARCH=darwin-x86_64
endif

ifeq ($(shell uname), Linux)
BUILD_OS=linux
BUILD_ARCH=${EPICS_HOST_ARCH}
endif

ifeq ($(shell uname), SunOS)
BUILD_OS=solaris
BUILD_ARCH=${EPICS_HOST_ARCH}
endif

ifeq ($(BUILD_OS),)
$(error Build OS is unsupported.)
endif

all:
	@echo Building for ${BUILD_OS}, arch: ${BUILD_ARCH}...
	cd Lib/UTC_GPS; make
	cd Th; make
	cd Display; make -f Makefile.${BUILD_OS} framer4 frameMemRead
	cd Control/java_widgets/lib; make -f Makefile.${BUILD_OS}
	cd Control; make -f Makefile.${BUILD_OS} dc3

install:
	mkdir -p ${INSTALL_PREFIX}/${BUILD_ARCH}/dv-${VERSION}
	install Control/dc3 ${INSTALL_PREFIX}/${BUILD_ARCH}/dv-${VERSION}/dv
	sed 's:INSTALL_PATH:${INSTALL_PREFIX}/${BUILD_ARCH}/dv-${VERSION}:' Control/dataviewer.${BUILD_OS} | sed 's:LOCAL_GRACE:${INSTALL_PREFIX}/${BUILD_ARCH}:' > ${INSTALL_PREFIX}/${BUILD_ARCH}/dv-${VERSION}/dataviewer
	chmod a+x ${INSTALL_PREFIX}/${BUILD_ARCH}/dv-${VERSION}/dataviewer
	install Display/framer4 ${INSTALL_PREFIX}/${BUILD_ARCH}/dv-${VERSION}
	install Display/frameMemRead ${INSTALL_PREFIX}/${BUILD_ARCH}/dv-${VERSION}

clean:
	cd Lib/UTC_GPS; make clean
	cd Th; make clean
	cd Display; make -f Makefile.${BUILD_OS} clean
	cd Control/java_widgets/lib; make -f Makefile.${BUILD_OS} clean
	cd Control; make -f Makefile.${BUILD_OS} clean
