There are cmake build files in the source tree.

The current effort is to update the daqd to be built via cmake, the main motivations are:
 * we are using autotools wrong
 * to allow full out of source tree builds for daqd
 * to allow the use of external tools in development that can consume projects in a cmake world
 * to build named versions of each daq variant (daq_fw, daq_standiop, ...)

At this time the RCG and real time modules are not being converted to cmake.

Building daqd via cmake.

1. create a build directory
2. change directory to the build directory
3. Make sure that there are package config files available for framecpp & EPICS
4. cmake <path to advLigoRTS>/
5. make -DCMAKE_BUILD_TYPE=RelWithDebInfo -j 8

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
the zmq_stream components