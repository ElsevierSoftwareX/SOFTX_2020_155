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
FrameCPP 2.6+ and the standalone_edc uses the boost libraries.  Boost is also hard requirement for the daqd, the edcu, and the run number server.  To install boost see (https://www.boost.org/doc/libs/1_69_0/more/getting_started/unix-variants.html)
You may need to pass a parameter to cmake -DBOOST_ROOT=<boost install prefix> to help cmake find boost.


You can make specific targets in cmake.  One useful command is 'make help' from the build directory.  This lists all the targets that are available. Another method is to go to the appropariate location
in the build directory and do a local make there, it will not rebuild the world.

Notes:
RelWithDebInfo does a release build with debug.  On gcc this means -O -g.
-j 8 is to do parallel builds with 8 cores.  Replace the 8 with the number of cores on your build box.

The following components will be built:

awgtpman
daqd (if the compiler is new enough)
nds
dataviewer (in its many pieces)
omx_xmit
omx_recv
dix_xmit
dix_recv
cps_xmit
cps_recv
local_dc
run_number_server

standalone_edc

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

The cmake development is primarily done on Debian 10.

The CDS debian repositories are used to provide some of the software.  See the following resources for information in setting up these repositories:

http://apt.ligo-wa.caltech.edu/debian/README.txt

On Debian the following packages are used:

 bison,
 cmake,
 debhelper (>= 9),
 dkms,
 dolphin-sisci-ix-devel,
 epics-dev,
 flex,
 grace,
 ldas-tools-al-dev,
 ldas-tools-framecpp-dev,
 libboost-all-dev,
 libbz2-dev,
 libc-dev-bin,
 libcds-pubsub-dev,
 libfl-dev,
 libmotif-dev,
 libnds2-client-dev,
 libtool,
 libxpm-dev,
 libxt-dev,
 libzmq3-dev,
 pkg-config,
 rapidjson-dev,

You will may also wish to instll
Open-MX
Dolphin

Open-MX is available in the CDS jessie-restricted repository (Debian 8) or may be built by hand.  For Debian 10 it must be built by hand.

FE -> DAQD Transport

The transport layer to be used with daqd is a modular system based around xmit and recv processes. Currently the following components are used:

Using OpenMX and IX Dolphin

local_dc and omx_xmit on the FE computers
omx_recv and dix_xmit on the data concentrator
dix_recv on the daqd machines

Using CDS Pub/Sub and IX Dolphin

local_dc and cps_xmit on the FE computers
cps_recv and dix_xmit on the data concentrator
dix_recv on the daqd machines


local_dc reads the individual model shared memory sections and concentrates it into one machine wide share memory block.  Then the *_xmit programs read the shared machine block and transmit the data.  The *_xmit processes receive data from the machines.

local_dc -b local_dc -m 100 -s "x1iop x1model1 x1model2"
 -b name of the local buffer to put the data into
 -m size in MB of the local buffer
 -s list of models to concentrate data from

omx_xmit  -b local_dc -m 100 -e 1 -r 1 -t x1dc0:0
 -b name of the local buffer to read data from
 -m size of the local buffer
 -e local MX endpoint
 -r remote MX endpoint
 -t target device

omx_recv -s 32 -b local_dc -m 100 -d 10
 -s the number of systems to listen for
 -b the name of the local buffer to write data to
 -m the size of the local buffer
 -d number of ms to wait for all the data to arrive

dix_xmit -b local_dc -m 100 -g 0 -p X1:CDS-DIX_
 -b the name of the buffer to read data from
 -m the size of the buffer in MB
 -g the IX memory window/group number to transfer data over
 -p prefix for EPICS debug information

dix_recv -b local_dc -m 100 -g 0
 -b the name of the buffer to write data to
 -m the size of the buffer in MB
 -g the IX memory window/group number to transfer data over

cps_xmit -b local_dc -m 100 -p "tcp://10.11.0.7:9000"
 -b the name of the buffer to read data from
 -m the size of the buffer in MB
 -p the publish method, in this example tcp unicast from 10.11.0.7:9000

cps_recv -b local_dc -m 100 -s "tcp://10.11.0.7:9000 tcp://10.11.0.11:9000"
 -b the name of the buffer to write data to
 -m the size of the buffer in MB
 -s the systems to retrieve data from

Configuring daqd

Daqd needs to know which mbuf to read from and its size.

set parameter "shmem_input" = "local_dc";
set parameter "shmem_size" = "104857600";

This MUST be set prior to the producer being started, and should just be set before any start ... calls in the daqdrc.
