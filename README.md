# LWNN - The Language With No Name

The Language With No Name is yet another embryonic programming language using LLVM as a back-end.
  
There's a basic REPL you can use.  Statements entered there will be parsed and then:

 - The LWNN AST will be displayed.
 - If all passes against the AST succeed LLVM IR will be generated and displayed.
 - The statement will be executed and the result will be displayed.
 
The compiler uses the [Boehm-Demers-Weiser conservative garbage collector](http://www.hboehm.info/gc/) and eventually, the compiled code will too. 
 
There's only a very limited set of functionality that works right now.  Examination of [integration_tests.cpp](https://github.com/dlurton/lwnn/blob/master/src/tests/integration_tests.cpp) will give a more complete
and up-to-date picture of supported syntax and features, however, here's a short summary:

- Data types: `bool`, `int` and `float`.
- Literal ints (`123`), floats (`123.0`) and booleans (`true` or `false`).
- Global variable declarations: `someVariable:int;`
    - Variables are strongly typed.
    - Variables must be declared before use.
    - New variables are always initialized to 0.
    - Variables may have an initializer: `someVariable:int = 1 + 2 * anyExpressionHere;`
- Binary Operators:
    - +, -, /, *, =, !=, ==, >, >=, <, <=, &&, ||
 - Casting:
    - Implicit casting happens when there is no precision loss between float and int:
    - For example in the expression:  `1.0 + 2` the `2` is cast to a `float` and the expression's result is a `float`.
    - Explicit casting is required when there is a precision loss, for example:
        ```         
        someFloatValue:float = 3.14;
        someIntValue:int = cast<int>(someFloatvalue);
        ```
      The the fractional portion of `someFloatValue` is truncated and `someIntValue` becomes `3`.
 - Compound Expressions:
    ```
    someInt:int = { 1; 2; 3; };
    ``` 
    The last expression within the compound expression (`3`) is the value assigned to `someInt`.
 - Ternary expressions: `(? condition, trueValue, falseValue )`
    - When `condition` evaluates to true, `trueValue` is evaluated otherwise `falseValue` is evaluated.
    - This is short circuiting!
 - If expressions:
    - Like ternary expressions but more powerful because they also serve as traditional if statements.  
    - For example `a = if(a == b) 1; else 2;"` works just like ternary. 
    - Also note that due to how compound expressions return the last value, more complex logic can be used to determine the 
    values returned by each branch.  For example:  `a = if(a == b) { 1; 2; } else { 3; 4; };` In this case `a` will become `2` 
    when `a == b` or `4` when `a != b`.
 - While loops:
    - `while(condition) expression;` or `while(condition) { expression1; expression2; ...}`


## Tenets

These ideas are to be kept in mind while designing the LWNN language: 

 1. Be opinionated about not being too opinionated - there are many ways to do the same thing, most of which are *not* objectively better than the others.  If you think your ways is better than everyone else's be prepared with objective arguments.
 1. Use objective and rational arguments for proposals to change the status quo.  Except in rare circumstances, subjective arguments are given little precedence.
 1. Don't try to think for the users of the language.
 1. Don't introduce unnecessary complexity.  The K.I.S.S. method is preferred.  However, complexity is tolerated if simplicity doesn't suffice.
 1. "Best practices" are not best practices if they don't provide objectively measurable benefit.  This is especially true for "best practices" that require significant time investment.
 1. No [bikeshedding](https://blog.codinghorror.com/procrastination-and-the-bikeshed-effect/).  Focus on important details and learn to compromise on trivialities.
 1. Be pragmatic.
 1. And most important of all: **get stuff done**.
 1. Iterate.  
   
 
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

