Build Instructions
==================
% mkdir build
% cd build
% cmake ..
% make install

The installed binary will be in inst/

The sample solution binaries are in sample_solution/

Platforms
=========
The build was tested on the Athena cluster and under Mac OS X. 

To run the sample binary under Windows, you will likely either need to 
run it from that directory or set your PATH to include that directory 
so that the runtime linker can find the freeglut DLL.

The windows build for this assignment has not been tested. Unlike
assignment 2, we don't use FLTK in this assignment, so, it should 
be easier to get the build to work over there. You will need to make
sure that both the freeglut and the RK4 libraries are linked in properly
in order to make the build work. The RK4 library should be a 32-bit
build, so, try that first.

Submission
==========
We only provide official support for developing under the Athena cluster.
Your submission has to build and run over there to receive credit.

Please submit your entire source directory (excluding the build
directory) and compiled binary in inst/.
