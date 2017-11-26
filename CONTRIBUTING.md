
This is a living document that will be expanded as issues and questions arise.

Just some simple notes fow now since I am the only contributor at the moment.
 
Before you begin work on any code that you would like to contribute to Anode, please send a message to anode.build@gmail.com to run
it past me first.  This will greatly increase the odds of me accepting a pull request.  

## Philosophy

Above all, Anode is meant to be practical in all aspects, from the design of the language to the implementation of the compiler.  I'm 
not out prove some esoteric computer language theory or write a paper on some heretofore unsolvable computer science problem.  This 
isn't part of some esoteric PhD thesis.

## Coding Style


When in Rome, do as the Romans do.  Some things however, do need to be said:

- Try to make your changes look like the existing code.
- Don't make style changes just because you like your way.
- Spaces, not tabs.
- Curly braces at end of line.
- Long lines aren't the end of the world, but I suggest wrapping them at 140 characters.

## Coding Conventions

This code is running under the [bdwgc collector](https://github.com/ivmai/bdwgc).  This means we can do certain things that some 
C++ programmers might find distasteful, like never using `delete` keyword.  

- Prefer `{}` initializers in member initialization lists to initialize member variables, `()` when calling base class constructors.
- Don't access class member variables directly.  Use accessors instead.
- The naming convention for a set accessor includes the "set" prefix, i.e. `void setWidgetId(int widgetId) { widgetId_ = widgetId; }`
- The naming convention for a get accessor has no prefix, i.e. `int widgetId() { return widgetId; }`
- When deciding between pointer and reference types:
    - Use references for:
        - Variables and member variables that are never `nullptr` and never need to point to a different instance after initialization.
        - Get accessors and other references that return values which are never `nullptr`.
    - Use pointers for:
        - Variables and member variables which may be null or whose instances need to be changed after being initialized.
        - Get accessors and other references that might return `nullptr.`

Standard library and language features up to C++14 are encouraged.
 
If any part of the existing codebase doesn't meet these guidelines, feel free to change it so that it does!

