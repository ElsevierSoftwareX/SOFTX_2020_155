There are cmake build files in the source tree.  These are not production ready yet.

The current effort is to update the daqd to be built via cmake, the main motivations are:
 * we are using autotools wrong
 * to allow full out of source tree builds for daqd
 * to allow the use of external tools in development that can consume projects in a cmake world
 * to build named versions of each daq variant (daq_fw, daq_standiop, ...)

At this time the RCG, real time modules, and other code are not changing to the use of cmake.

As mentioned to build daqd for production the autotools methods should probably still be used (at least until the cmake
build supports all the required targets).

Building daqd via cmake.

1. create a build directory
2. change directory to the build directory
3. Make sure that there are package config files available for framecpp & EPICS
4. cmake <path to advLigoRTS>/
5. make daqd_fw
