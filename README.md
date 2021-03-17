# ReiserRT_Core
Frank Reiser's C++11 components for multi-threaded, realtime 
embedded systems.

## Supported Platforms
This project is a CMake project. At present, GNU Linux is
the only supported platform. Versions of this project exist
on Windows but the CMake work is not quite in place yet.
Note: This project requires CMake v3.17 or higher.

## Example Usage
Example hookup and usage can be found in the various
tests that are present in the tests folder.

## Building and Installation
Roughly as follows:
1) Obtain a copy of the project
2) Create a build folder within the project root folder.
3) Switch directory to the build folder and run the following 
   to configure and build the project for you platform:
   ```
   cmake ..
   cmake --build .
   ```
4) Test the library
   ```
   ctest
   ```
5) Install the library as follows (You'll need root permissions)
   to do this:
   ```
   sudo cmake --install .
   ```
