# LWNN - The Language With No Name*

\**(no name yet)*

The Language With No Name is yet another embryonic programming language that really isn't anything at all yet.  LWNN 
will hopefully be renamed once a name has actually been chosen.

## Building

### Dependencies First

#### Install Build Dependencies

 - [cmake](https://cmake.org/) (3.4.3 or later) 
 - gcc.  (7.1.1 is known to work however earlier versions are likely to as well.)
 - [maven](https://maven.apache.org/what-is-maven.html) (version 3.5.0 is known to work) 
 - Java 7 or later

The script `tools/build-all-dependencies` will clone all of the repositories of each `lwnn` dependency and build 
all of them with the necessary options, placing all the source codes and intermediate files into `externs/scratch`.  
This directory may be deleted to conserve disk space after everything has successfully built, if desired.  If 
successful, the libraries and headers of each dependency will be installed in sub-directories of `externs`. 

### Building LWNN

    mkdir cmake-build
    cd cmake-build
    cmake ..

