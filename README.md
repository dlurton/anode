# LWNN - The Language With No Name

The Language With No Name is yet another embryonic programming language using LLVM as a back-end.
  
There's a basic REPL you can use.  Statements entered there will be parsed and then:

 - The LWNN AST will be displayed.
 - If all passes against the AST succeed LLVM IR will be generated and displayed.
 - The statement will be executed and the result will be displayed.
 
There's only a very limited set of functionality that works right now:

 - Calculate any expression containing only integers and floats +, -, * or / operators:
        
        lwnn> 10 + 20 * 3 / (5 - 2);
        LWNN AST:
        Module: temp_module_name
        BinaryExpr: Add
          LiteralInt32Expr: 10
          BinaryExpr: Div
            BinaryExpr: Mul
              LiteralInt32Expr: 20
              LiteralInt32Expr: 3
            BinaryExpr: Sub
              LiteralInt32Expr: 5
              LiteralInt32Expr: 2
        
        LLVM IR:
        ; ModuleID = 'temp_module_name'
        source_filename = "temp_module_name"
        target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
        
        define i32 @initModule() {
        EntryBlock:
          ret i32 30
        }
        30

 - Declare a global variables, optionally assigning their default value:
         
        lwnn> imaglobalyo:int = 5 * 20 / (2 + 3);
        LWNN AST:
        Module: temp_module_name
        VariableDeclExpr: (imaglobalyo:int)
          BinaryExpr: Div
            BinaryExpr: Mul
              LiteralInt32Expr: 5
              LiteralInt32Expr: 20
            BinaryExpr: Add
              LiteralInt32Expr: 2
              LiteralInt32Expr: 3
        
        LLVM IR:
        ; ModuleID = 'temp_module_name'
        source_filename = "temp_module_name"
        target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
        
        @imaglobalyo = common global i32 0, align 4
        
        define i32 @initModule() {
        EntryBlock:
          store i32 20, i32* @imaglobalyo
          %0 = load i32, i32* @imaglobalyo
          ret i32 %0
        }
        20

The above expression:  `imaglobalyo:int = 5 * 20 / (2 + 3);` declares a variable named "imaglobalyo" of type "int" and
assigns a default value.  

Literal floats expressed normally: i.e. `1.0` is a literal float while `1` is a literal integer;

## Building

### Dependencies First

#### Install Build Dependencies

These are needed to build ANTLR4 and LLVM which must be done once before LWNN can be built.

 - [cmake](https://cmake.org/) (3.4.3 or later) 
 - gcc.  (7.1.1 is known to work however earlier versions are likely to as well.)
 - [maven](https://maven.apache.org/what-is-maven.html) (version 3.5.0 is known to work) 
 - Java 7 or later
 - libuuid (and development headers, under Fedora the package name is libuuid-devel)
 
The script `tools/build-all-dependencies` will clone all of the repositories of each `lwnn` dependency and build 
all of them with the necessary options, placing all the source codes and intermediate files into `externs/scratch`.  
This directory may be deleted to conserve disk space after everything has successfully built, if desired.  If 
successful, the libraries and headers of each dependency will be installed in sub-directories of `externs`. 

### Building LWNN

    mkdir cmake-build
    cd cmake-build
    cmake ..

