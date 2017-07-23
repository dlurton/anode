TODO:
    
 - See if it's possible to reduce size of LLVM shared libraries and lwnn executables... (llvm is almost a gig!)
 - Functions
 - Send results to an external function instead of returning them from the module initializer
 - Negative literal ints and floats.
 
Really should be using this:

    https://github.com/toomuchatonce/cmake-examples/blob/master/superbuild-importedtargets/CMakeLists.txt
    https://cmake.org/cmake/help/v3.2/module/ExternalProject.html

Building dependencies by script just doesn't *feel* right and I suspect using this super-build pattern will.

After implementing the cmake super build pattern (in link above), implement travis CI.

http://lint.travis-ci.org/
https://docs.travis-ci.com/user/apps/

Ubuntu packages needed:
 - gcc-7
 - g++-7 
 - (set CC & CXX)
 - uuid-dev

In order to support a REPL thingy, will need to support executing any statement in the global scope like any 
interpreted language.  These types of statements can generate errors while compiling to object code if desired.

Idea for compiler warning:
    when cast<>() is used but the result is then implicitly cast again...

### Principals of the language:

 - An effective module system! 
 - All is immutable (i.e. const) by default
 - Has only classes (no struct)
 - Objects can be allocated on the stack or the heap (like C++)
 - Has syntactic support for thread synchronization (i.e. java's synchronized or C# lock(...) { ... } only better)
 - C# inspired with functional, LINQ-like features
 - Single inheritance, interfaces (if you don't like inheritence, you are free not to use it)
 - Garbage collected but also with optional deterministic destruction
    - A GC which moves stuff around might interfere with C interop without special considerations.  
    - The [Boehm collector](https://github.com/ivmai/bdwgc) might be the best approach for because it doesn't move 
        stuff around.
    - Also see: http://llvm.org/docs/GarbageCollection.html
 - Targeted at application development (i.e. GUI apps, web applications, daemons, etc.)
 - Script-like in the sense that it is JIT compiled
 - But can also be compiled, enabling more optimizations.
 - Interoperates with C
    - Can consume C libraries
    - Can expose functions to C
 - Unicode on by default

Non-Goals:

 - Package management system (i.e. rust's cargo, or node's npm)
 - Generating code suitable for an OS kernel

### Features I'd like to include:

 - Generics!
    - However, nested generics are not allowed!
    - If nested generics are desired one must alias the inner generic first:
        alias list<Widget> WidgetList;
        widgetGroup:map<string, WidgetList>;
    
 - Return tuple as an alternative to output parameters:
    (a, b) = someFunc();
 - Syntactic sugar for properties (a la C#'s getters & setters)
 - Lazy function argument evaluation: https://dlang.org/lazy-evaluation.html
 - Conditional compilation
 - Exception handling
 - Operator overloading
 - Language support for assert, a la D.
 - Some limited form of Mixin support, not as sophisticated as D's.
    - I like the idea of being able to define a "template mixin" as a set of methods and allowing classes to essentially "paste" in the template
    - Provides a better alternative than using inheritance for DRY principal for a series of otherwise unrelated types.
 
### Decisions, decisions:

 - Boxing/unboxing?  How would that even work?
 - Everything is a class like .Net or primitives are not like C++?
 - All operator are call sites to inline functions
    - Can we force LLVM to inline?  Otherwise this might not work...
 
### data type system ideas:

In the code generator, all data types of all operations must match.  When a data type may be coerced into another data type (say when assigning an int to a float variable), the expression tree must be modified to include a CoerceExpr.
A CoerceExpr, when generated, is responsible for converting coercible from one data type to another.  
A CoerceExpr is logically the same as a C style cast and may in fact be the same with the exception that types which are implicitly coercible automatically get wrapped in a CoerceExpr.
Logically, in this snippet:

    int x = 1;
    float n = x;

The second line automatically becomes (in the AST representation):

    float i = (int)x;

However if the type of x and y are reversed:

    float x = 1.0;
    int y = x;
        
Then the second line generates a compile error because data float is not automatically coercible to int because of the loss of precision.  
Similarly, if class B inherits from class A, the cast here is added automatically:

    A instanceOfA = (A)B;

A class declaration needed to support this might be something like:

    class DataType {
    public:
        bool isSameDataTypeAs(DataType dataType);
        bool isCoercibleTo(DataType dataType);
        CoerceExpr tryCoerceTo(DataType dataType); //returns null if can't coerce.
        Expr maybeCoerce(Expr expr, DataType dataType); //returns expr if can't coerce, otherwise expr wrapped in a CoerceExpr coercing to the correct type.

        //Other methods which could be useful:
        bool isPrimitive();
        PrimitiveType primitiveType();
    }

Stack based allocation, deterministic destruction:

    Basically, stack based allocation will have deterministic destruction, similar to C++.
    Heap-based allocation can also be used.
    There is no need to differentiate between classes and structs.  There will only be classes.
    

example code:

    class SomeClass : SomeSuperClass { 

        public construct() { //construct and destroy will be keywords reserved for constructor and destructors.

        }

        public destroy() {

        }
    }

Idea for synchronization primitive:

    class SomeClass {

        // This is a synchronization group.
        // Variables defined within are scoped to the class, not the group.
        spinlock coordinates { 
            int x = 1;  
            int y = 2;  
        }

        void someMethod() {
            x = 1; // Nope - compile error - or maybe runtime error if 
            atomic coordinates {
                x = x + 1;  //Ok because the group is locked.
                y = y + 2;
            }
        }
    }

Primitives:

    i32, i64, ui32, ui64, float, double, char, uchar, byte, string

Declaring variables:

    widgetCount:i32;            //Gets default value of 0
    widgetCount:i32 = 200;      

    //Stack allocation:
    widgetInstance:Widget = salloc(...);  

    //Heap allocation:
    widgetInstance:Widget = halloc(...); 
           

