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
 - Classes:
    ```
        class Widget {
            id:int;
            weight:flaot;
            isInStock:bool;
        }
    ```
 - Stack allocated objects:
    `someWidget:Widget;`
 - Dot operator:
    `someWidget.weight = 12.53;`
 - Class fields with a class type:
    ```
    class WidgetPair {
        first:Widget;
        second:Widget; 
    }
    aPairOfWidgets:WidgetPair;
    aPairOfWidgets.first.id = 1;
    aPairOfWidgets.second.id = 2;
    ```
 - Can define functions: `func someFunction:int() 10 + 12;` (if there is only one expression in the function body)
    - Or: `func someFunction:int() { someGlobal = someGlobal + 1; someGlobal + 12; }` (for multiple expressions in the function body)
    - The result of the last expression in the function body is the return value.
    - Functions may return nothing: `func someFunction:void() someExpression;` in which cast the last expression in the function body is ignored.
    - Local variables also can be defined within functions.
    - Functions may be invoked:  `anInt:int = someFunction();`    
    - Primitive types may be used as function arguments:
        - `func someFunc:void(arg1:int, arg2:float, arg3:bool) soomeExpression`
        
#### Really Really Rough Feature Backlog

These are listed in roughly the order they will be implemented.

- `assert` function for testing
- JIT & execute entire files at command-line
    - i.e. `lwnn file_to_exec.lwn`
    - Also can use shebang in first line of a file:  `#!/path/to/lwnn/executable`
- New testing infrastructure
    - Reads list of files from directory and executes them in alphabetical order, uses new `assert` keyword  
- Explicit return, for when an exit before the last expression of the function body is desired.  Will use keyword `ret`
- Pass class instances by value
- Declare references to objects (something like `anObjectReference:SomeClass*`) 
- Pass any type by reference 
- Function overloading
- Generate "object_init" which initializes fields to their defaults
- Member functions
- Constructors 
- Inheritance
- Virtual functions
- Interfaces / abstract functions
- Modules
- Pre-compile groups of modules into shared libraries for faster loading and interop with other languages such as C 
- Compiler generated metadata about all compiled language constructs
  - Can be used by the compiler when importing a module *and* for reflection
- Generics
- Reflection
- Multiple return value / unpacking:  `(firstValue:int, secondValue:int) = functionReturningPair()`
  - Probably uses tuples
- Numerous other ideas too amorphous to mention

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

Building the source documentation requires doxygen.

    cd $project_root/src
    doxygen doxygen.cfg
    
 