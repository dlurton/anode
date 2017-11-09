# Anode Tests

Herein is contained Anode's automated tests.

- `simple_tests.cpp` uses [catch.hpp](https://github.com/catchorg/Catch2) to prove a bare minimum amount of the Anode compiler works.  This is a bit more than enough to ensure the JIT compiler, basic expressions and `assert` function are working.  The rest of the tests are written in Anode.  
- The files within `suites/*.an` (will) make up the bulk of the tests for language functionality.
- The files within `negative-suites/*.nts` test syntax and semantic error reporting of the compiler.
