# Anode Programming Language

[![Build Status](https://travis-ci.org/dlurton/anode.svg?branch=master)](https://travis-ci.org/dlurton/anode)

Anode is yet another embryonic programming language using LLVM as a back-end.

There's a basic REPL.  Statements entered there will be parsed and then:

 - The AST will be displayed.
 - If all passes against the AST succeed LLVM IR will be generated and displayed.
 - The statement will be executed and the result will be displayed.
 
It's also possible to execute scripts directly, i.e.

    ./bin/<build type>/anode path/to/source_file.an
    
Where `<build type>` is either `Release` or `Debug`.

A `.an` file may also include a shebang line, i.e.

    #!/path/to/anode/executable 
 

Goals of the language:

- Will be initially targeted at application developers (as opposed to systems developers)
- Easy to just jump in and start doing stuff like Python
- Strongly typed like C#/Java
- High performance (utilizes LLVM back-end which has an extensive suite of compiler optimizations)  
- Don't needlessly exclude features (post/pre increment operators, ternary operator, generics, etc)

The compiler and compiled code use the [Boehm-Demers-Weiser conservative garbage collector](https://github.com/ivmai/bdwgc). 

Examination of [simple_tests.cpp](https://github.com/dlurton/anode/blob/master/src/tests/simple_tests.cpp) and the [these files](https://github.com/dlurton/anode/tree/master/src/tests/suites) 
will give a complete and up-to-date picture of supported syntax and features, however, here's a summary:

- Single-line comment:
    ```
    # this is a single line comment
    ```
- Nestable, multi-line comment:
    ```
    (# this is a multiline comment
        (# this is a nested comment which doesn't break the outer comment. #) 
    #)
    ```
- Data types: `bool`, `int` and `float`.
- Literal ints (`123`), floats (`123.0`) and booleans (`true` or `false`).
- Global variable declarations: `someVariable:int`
    - Variables are strongly typed.
    - Variables must be declared before use.
    - New variables are always initialized to 0.
    - Variables may have an initializer: `someVariable:int = 1 + 2 * anyExpressionHere`
- Binary Operators:
    - +, -, /, *, =, !=, ==, >, >=, <, <=, &&, ||
 - Casting:
    - Implicit casting happens when there is no precision loss between float and int:
    - For example in the expression:  `1.0 + 2` the `2` is cast to a `float` and the expression's result is a `float`.
    - Explicit casting is required when there is a precision loss, for example:
        ```         
        someFloatValue:float = 3.14
        someIntValue:int = cast<int>(someFloatvalue)
        ```
      The the fractional portion of `someFloatValue` is truncated and `someIntValue` becomes `3`.
 - Compound Expressions:
    ```
    someInt:int = { 1 2 3 }
    ``` 
    The last expression within the compound expression (`3`) is the value assigned to `someInt`.
 - Ternary expressions: `(? condition, trueValue, falseValue )`
    - When `condition` evaluates to true, `trueValue` is evaluated otherwise `falseValue` is evaluated.
    - This is short circuiting!
 - If expressions:
    - Like ternary expressions but more powerful because they also serve as traditional if statements.  
    - For example `a = if(a == b) 1 else 2"` works just like ternary. 
    - Also note that due to how compound expressions return the last value, more complex logic can be used to determine the 
    values returned by each branch.  For example:  `a = if(a == b) { 1 2 } else { 3 4 }` In this case `a` will become `2` 
    when `a == b` or `4` when `a != b`.
 - Built-in assert function: `assert(someExpression)`
    - This is not an actual function, it's part of the syntax!
    - If `someExpression` is non-zero, execution continues unimpeded
    - If `someExpression` is zero, an error message is printed to stderr including the anode source file, line number, and expression that 
        evaluated to false and the process is terminated.
    - This expression is/will be heavily used during testing of Andoe languages features. 
 - While loops:
    - `while(condition) expression` or `while(condition) { expression1 expression2 ...}`
 - Classes:
    ```
        class Widget {
            id:int
            weight:float
            quantity:int
            func calculateTotalWeight:float() quantity * weight
        }
    ```
 - Heap allocated, garbage collected objects: `someWidget:Widget = new Widget()`
    - `someWidget` is a reference.    
 - Dot operator:
    `someWidget.weight = 12.53`
 - Class fields with a class type:
    ```
    class WidgetPair {
        first:Widget
        second:Widget 
    }
    aPairOfWidgets:WidgetPair
    aPairOfWidgets.first.id = 1
    aPairOfWidgets.second.id = 2
    ```
 - Can define functions: `func someFunction:int() 10 + 12` (if there is only one expression in the function body)
    - Or use curly braces when there is more than one expression in the function body: 
    ```
        func someFunction:int() { 
            someGlobal = someGlobal + 1 
            someGlobal + 12 
        }
    ```
    - The result of the last expression in the function body is the return value.
    - Functions may return nothing: `func someFunction:void() someExpression` in which cast the last expression in the function body is ignored.
    - Local variables also can be defined within functions.
    - Functions may be invoked:  `anInt:int = someFunction()`    
    - Primitive types may be used as function arguments:
        - `func someFunc:void(arg1:int, arg2:float, arg3:bool) someExpression`
 - Namespaces
```
    namespace Foo::Bar {
        class SomeClass { ... }
    } 
    anInstance:Foo::Bar::SomeClass = new Foo::Bar::SomeClass()
```
- If a namespace contains only a single item, curly braces may be omitted:
```
    namespace Foo::Bar 
        class SomeClass { }
```
 - Templates!  Anode's templates are most similar in function to D's mixins but are not as yet as flexible. 
 Templates can be used as a means of reducing code duplication.  "Expanding" the template essentially causes the template's contents to be
  copied to the expansion site just as if the code had been copied and pasted.  Templates may have arguments. Currently, only types may 
  be used as arguments for templates.  The snippet below gives the 
 `FooDocumentItem` and `BarDocumentItem` classes the `documentItemId` field and `assignDocumentItemId()` method.
```
    template CommonDocumentItemMembers { 
        documentItemId:int
        func assignDocumentItemId() {
            ...generate unique documentItemId...
        }
    }
    class FooDocumentItem {
        expand CommonDocumentItemMembers
    }
    class BarDocumentItem {
        expand CommonDocumentItemMembers
    }
```
- Templates can be used to create variations of the same class.   Also note that templates consisting of only a single 
expression do not need to be wrapped in `{}`.
```
    template SomeTemplate<TArg> 
    class SomeClass {
        someValue:TArg
    }

    namespace SomeNamespaceInt expand SomeTemplate<int>
    namespace SomeNamespaceFloat expand SomeTemplate<float>
    
    a:SomeNamespaceInt::SomeClass = ...
    b:SomeNamespaceFloat::SomeClass = ...
    a.someValue = 10    # a.someValue is an int
    b.someValue = 1.01  # b.someValue is a float
 ```
 - Or of the same function: (note that once function overloading is completed, the expansion sites may exist in the same namespace)
```
    template AbsoluteValue<TNumber> {
        func abs(n:TNumber) (? n > 0 ; n; 0 - n;) 
    }
    namespace math::int_funcs expand AbsoluteValue<int>
    namespace math::float_funcs expand AbsoluteValue<float>
```
 - Generics! Generic functions are based on the template feature.  Classes that are placed within an anonymous template
 can be referenced along with their type arguments similar to C++, Java or C#.
 ``` 
     template <TItem>
     class Node {
         item:TItem
         next:Node
     }
     
     
     n1:Node<int> = new Node<int>()
     n1.next = new Node<int>()
     n1.item = 10
     n1.next.item = 11
 ```
 
#### Really Really Rough Feature Backlog

These are listed in roughly the order they will be implemented.  The basic plan is to implement a core set of features found in most 
languages and that are needed for basic usefulness and then come back and add some (perhaps functional) special sauce. 

- Generate and invoke "object_init" which initializes class fields to their defaults when instantiated.
- Strings and their various operations
- Explicit return, for when an exit before the last expression of the function body is desired.  Will use keyword `ret`
- `for` loop
- `switch` maybe with pattern matching.
- Bitwise operators
- Unary operators (`++`, `--`, `!` etc)
- Compound assignment(`*=`, `+-`, `/=`, etc)
- Pass class instances by value
- Declare references to objects (something like `anObjectReference:SomeClass*`) 
- Pass any type by reference 
- Function overloading
- Member functions
- Member access levels (`private`, `public`, `protected` etc)
- Constructors 
- Inheritance
- Exceptions
- Virtual functions
- Interfaces / abstract functions
- Generics
- Lambdas
- Modules
- Pre-compile groups of modules into shared libraries for faster loading and interop with other languages such as C 
- Compiler generated metadata about all compiled language constructs
  - Can be used by the compiler when importing a module *and* for reflection
- Reflection
- Multiple return value / unpacking:  `(firstValue:int, secondValue:int) = functionReturningPair()`
  - Probably uses tuples
- Numerous other ideas too amorphous to mention

#### Other desired features of unspecified importance

- `debug_assert` function, similar to the `assert` function, but is removed during release builds.

## Building

### Prerequisites

The following must be installed and available prior to building Anode.

 - [cmake](https://cmake.org/) (3.4.3 or later) 
 - gcc 6 or clang-4.0 or later. clang-4.0 is recommended.
    - Note that if building on Ubuntu 14.04 (and possibly later), gcc-6 is always required--even when building with clang because 
    clang needs more recent libstdc++ headers in order to provide up-to-date C++14 support.  (Headers from gcc-4.8 do not work.)
 - libuuid
 - cmake 3.4.3 or later
 - autoconf
 - libatomic (if using a version of gcc that doesn't have atomic operations built-in, required by libgc)
 - libtool
 - About 50gb+ of free disk space (for building LLVM, mainly)

### Building Dependencies

A subset of Anode's dependencies must be built from source before Anode itself can be built. In the case
of LLVM, this is because Anode builds against LLVMs master branch in order to keep up more easily with LLVM's frequent
API changes.  The other dependencies are either non-standard in linux distributions, (e.g. liblinenoise-ng) or are more
recent versions than is frequently found in linux distributions (e.g. libgc). 
    
The build scripts will clone the repositories of each of these dependencies and build them with the necessary options, 
placing all the source codes and intermediate files into `externs/scratch`. If successful, the libraries and headers of each dependency 
will be installed in sub-directories of `externs/release` or `externs/debug`, depending on if a release build has been selected.

### First Time Building

Use the script `tools/build-all-debug` or `tools/build-all-release` to build all dependencies and Anode in Debug or Release modes, 
respectively.

### Building Anode

After building the first time, you can build just Anode like so:

    ./tools/build-anode 
    
The default is to perform a release build if the environment variable `ANODE_BUILD_TYPE` is not set.  To perform a debug build, set
`ANODE_BUILD_TYPE` to `Debug` before executing `./tools/build-anode`

To run the all the tests, from the `cmake-build-debug` or `cmake-build-release` directories, execute:

    ctest --output-on-failure

Building the source documentation requires doxygen.  This is not currently built by any of the build scripts.

    cd $project_root/src
    doxygen doxygen.cfg
    
Documentation will be generated and stored in the project root.