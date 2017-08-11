Build Instructions
==================
% mkdir build
% cd build
% cmake ..
% make install

The installed binary will be in inst/

The sample data files are in data/

The sample solution binaries are in sample_solution/

Platforms
=========
The build was tested on the Athena cluster and under Mac OS X. 
Mac OS X may not provide FLTK by default, so, you may need to 
either install it via fink or MacPorts or build it yourself from source.

To run the sample binary under Windows, you will likely either need to 
run it from that directory or set your PATH to include that directory 
so that the runtime linker can find the freeglut DLL.

The windows build for this assignment has not been tested. The
included binaries for the FLTK library are a bit out of date, so, 
there may likely be compatiblity issues with newer versions of 
Visual Studio. If you really do want to develop under Windows, 
we suggest you build your own version of FLTK from source:

http://www.fltk.org/

Submission
==========
We only provide official support for developing under the Athena cluster.
Your submission has to build and run over there to receive credit.

Please submit your entire source directory (excluding the build
directory) and compiled binary in inst/.
