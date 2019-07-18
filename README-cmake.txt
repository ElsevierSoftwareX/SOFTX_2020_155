There are cmake build files in the source tree.

The current effort is to update the daqd and most of the advligorts system to build via cmake.  The main motivations are:
 * we are using autotools wrong
 * to allow full out of source tree builds for daqd (no building in src/gds, or auto tools files generated in src/daqd)
 * to provide a unified build system for most components
 * to allow the use of external tools in development that can consume projects in a cmake world
 * to build named versions of each daq variant (daq_fw, daq_standiop, ...)

At this time the RCG and real time modules are not being converted to cmake.

Building daqd via cmake.  You will need cmake >= 3.0.  The build has been tested with cmake 3.0.2, 3.7.2, 3.9.2.

0. Make sure your PKG_CONFIG_PATH is set right, most packages are discovered
via pkg-config
1. create a build directory
2. change directory to the build directory
3. Make sure that there are package config files available for framecpp & EPICS
4. cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBOOST_ROOT=<path to boost*> <path to advLigoRTS>/
5. make -j 8

* Note on Boost.
FrameCPP 2.6+ uses the boost libraries shared_ptr.  To install boost see (https://www.boost.org/doc/libs/1_69_0/more/getting_started/unix-variants.html)
You may need to pass a parameter to cmake -DBOOST_ROOT=<boost install prefix> to help cmake find boost.
This is not required for FrameCPP < 2.6.


You can make specific targets in cmake.  One useful command is 'make help' from the build directory.  This lists all the targets that are available. Another method is to go to the appropariate location
in the build directory and do a local make there, it will not rebuild the world.

Notes:
RelWithDebInfo does a release build with debug.  On gcc this means -O -g.
-j 8 is to do parallel builds with 8 cores.  Replace the 8 with the number of cores on your build box.

The following components will be built:

awgtpman
daqd_fw
daqd_dc_mx
daqd_rcv
daqd_mx_symm
daqd_standiop
daqd_bcst
daqd_shmem (if the compiler is new enough)
nds
dataviewer (in its many pieces)
mx_stream
run_number_server
the zmq_stream components
zmq_fe
zmq_rcv_ix_xmit_delay


If you need to install a copy of cmake you can retrieve the source from kitware.

https://cmake.org/download/
https://cmake.org/files/

As an example, to install cmake 3.7.2, you may do the following.

1. wget https://cmake.org/files/v3.7/cmake-3.7.2.tar.gz
2. Verify the checksum.  The sha256 checksum of this file is
 dc1246c4e6d168ea4d6e042cfba577c1acd65feea27e56f5ff37df920c30cae0
3. tar -zxf cmake-3.7.2.tar.gz
4. cd cmake-3.7.2
5. make build
6. cd build
7. ../bootstrap --prefix=/opt/cmake-3.7.2
8. make && make install

You can now use cmake as '/opt/cmake-3.7.2/bin/cmake'

Building on Debian

The cmake development is primarily done on Debian (both 8 & 9).  Though with an updated gentoo system it works there.

The LSCSoft and CDS debian repositories are used to provide some of the software.  See the following resources for information in setting up these repositories:

https://wiki.ligo.org/Computing/DASWG/DebianJessie
http://apt.ligo-wa.caltech.edu/debian/README.txt

On Debian the following packages are used:

build-essential
cmake
bison
flex
libzmq3-dev
pkg-config
ldas-tools-al-dev
ldas-tools-framecpp-dev
epics-dev
libbz2-dev
libmotif-dev
libxpm-dev
libxt-dev
grace
pcaspy
libboost-all-dev    (Required for FrameCPP >= 2.6.0)

You will also need to install
MX/Open-MX
Dolphin

MX and open-MX are available in the CDS jessie-restricted repository (Debian 8) or may be built by hand.

On Debian 8 we have back ported the main zmq package from Debian 9 so that 4.2.1 is available on Debian 8, 9, and
the gentoo systems.

ZMQ/Dolphin IX transport

The transport layer to be used with daqd_shmem is in flux.  Currently the following components are used:

zmq_fe on the FE computers
zmq_rcv_ix_xmit_delay on the data concentrator
ix_fb_recv on the daqd machines

We also have a rebuild of the mx_streamer to work with the daqd_shmem system.  This is NOT ready yet.

mx_stream on the FE computers
mx_rcv on the data concentrator (eventually mx_rcv_ix_xmit)
ix_fb_recv on the daqd machines eventually

Running zmq_fe

zmq_fe is invoked like this:

zmq_fe -s "system list here... x1iopasc0 x1asc..." -D <optional delay in ms> -d <path to gds param dir> -e eth1

You can get epics data out of the system as well, to do so add:

-p <some EPICS variable name prefix>
-P <path to a fifo>

Then run the src/zmq_stream/scripts/dc_cas.py <path to fifo used by zmq_fe>.  This will export some information about the sender over EPICS.


Receiving ZMQ on the data concentrator.

The receiver process on the data concentrator is zmq_rcv_ix_xmit, it is run as:

zmq_rcv_ix_xmit-delay -s "fe endpoints to subscribe to" -g 0 -b ifo -p <epics prefix> -P <path to fifo> -X <timing debug file>

The -p & -P parameters used the same as with zmq_fe.  You must run a copy of dc_cas.py to export this data.
The -g field is the dolphin group number.
-b ifo tells the system to use the "ifo" named mbuf
-X dumps information about what has been received around a timing/input glitch in zmq to a file, it is not required.


Receiving IX on the daqds

Run the ix_fb_rcv on the daqd systems (fw, tw, nds, ...)

ix_fb_rcv -g 0 -b ifo

Tell it which dolphin group to listen to, and where to put the data.

Configuring daqd_shmem

Daqd_shmem needs to know which mbuf to read from and its size.

set parameter "shmem_input" = "ifo";
set parameter "shmem_size" = "104857600";

This MUST be set prior to the producer being started, and should just be set before any start ... calls in the daqdrc.
