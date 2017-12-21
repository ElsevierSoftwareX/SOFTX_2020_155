There are cmake build files in the source tree.

The current effort is to update the daqd and most of the advligorts system to build via cmake.  The main motivations are:
 * we are using autotools wrong
 * to allow full out of source tree builds for daqd (no building in src/gds, or auto tools files generated in src/daqd)
 * to provide a unified build system for most components
 * to allow the use of external tools in development that can consume projects in a cmake world
 * to build named versions of each daq variant (daq_fw, daq_standiop, ...)

At this time the RCG and real time modules are not being converted to cmake.

Building daqd via cmake.  You will need cmake >= 3.0.  The build has been tested with cmake 3.0.2, 3.7.2, 3.9.2.

1. create a build directory
2. change directory to the build directory
3. Make sure that there are package config files available for framecpp & EPICS
4. cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo <path to advLigoRTS>/
5. make -j 8

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
nds
dataviewer (in its many pieces)
mx_stream
run_number_server
the zmq_stream components


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

