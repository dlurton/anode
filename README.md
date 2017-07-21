# LWNN - The Language With No Name

The Language With No Name is yet another embryonic programming language using LLVM as a back-end.
  
There's a basic REPL you can use.  Statements entered there will be parsed and then:

 - The LWNN AST will be displayed.
 - If all passes against the AST succeed LLVM IR will be generated and displayed.
 - The statement will be executed and the result will be displayed.
 
There's only a very limited set of functionality that works right now.  Examination of `tests.cpp` will give a more complete
and up-to-date picture of supported syntax and features, however, here's a short summary:

Calculate any expression containing only integers and floats +, -, * or / operators:
        
        10 + 20 * 3 / (5 - 2);

Declare a global variables, optionally assigning their default value:
         
        imaglobalyo:int = 5 * 20 / (2 + 3);

The above expression declares a variable named "imaglobalyo" of type "int" and assigns a default value.
Note that variables don't yet persist between statements entered at the REPL.  I expect to add support for this
within the next several commits.

Implicit type conversion between float and int:

        5.0 + 3

The resulting value is of type float.  When the data types of the operands differ, the compiler checks to see if one can
be implicitly cast to the other.  In the example above, because `int` type can be cast to `float`, the `3` is implicitly
cast to `float` and the result is a `float`. Rules for implicit casting are/will be similar to C's...  Essentially,
implicit casting can occur as long as there is no data loss.

Literal floats are expressed normally: i.e. `1.0` is a literal float while `1` is a literal integer.

## Building

### Dependencies First

#### Install Build Dependencies

These are needed to build ANTLR4 and LLVM which must be done once before LWNN can be built.

 - [cmake](https://cmake.org/) (3.4.3 or later) 
 - gcc.  (7.1.1 is known to work however earlier versions are likely to as well.)
 - libuuid (and development headers, under Fedora the package name is libuuid-devel)
 - For building ANTLR4:
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

