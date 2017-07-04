# LWNN - The Language With No Name*

\**(no name yet)*

The Language With No Name is yet another embryonic programming language that really isn't anything at all yet.  LWNN will hopefully be renamed once a name has actually been chosen.

## Building

The script `tools/build-all-dependencies` will build [ANTLR4](https://github.com/antlr/antlr4) and [LLVM](https://github.com/llvm-mirror/llvm).

### Dependencies First

You'll need git, [cmake](https://cmake.org/) (3.4.3 or later and gcc.  gcc 7.1.1 is known to work however earlier versions are likely to as well.

To build ANTLR4's parser generator [maven](https://maven.apache.org/what-is-maven.html) (version 3.5.0 is known to work) and Java 7 or later is also required.

The script `tools/build-all-dependencies` will clone LLVM and ANTLR4's git repositories and build everything with all the neccessary options, placing all the source codes and intermediate files into `externs/scratch`.  This directory may be deleted to conserve disk space after everything has succesfully built, if desired.  If successful, the libraries and headers will be installed in `externs/llvm` and `externs/antlr4`.  These must exist prior to attempting to build LWNN itself.

### Building LWNN

    mkdir cmake-build
    cd cmake-build
    cmake ../

