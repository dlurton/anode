
# Negative Test Suites

This directory contains "Negative Test Suites."  A negative test suite is a file containing snippets of anode source code
which will generate specific error messages when compiled.  The goal of a negative test suite is to assert that the compiler 
handles error conditions correctly and to allow the creation of these test cases to be more efficient than in standard unit
tests such as `catch.hpp`.

Any `.nts` file that exists in this directory will be executed during the build by the `negative_test` program specified in 
`../negative_test.cpp`.  To run an individual suite, execute `negative_test <path to .nts file>`.  (The `negative_test` executable
will be located in the `bin/Debug` or `bin/Release` directory after successful compilation.)

## File Format

Each test within a `.nts` file contains one or more of the following test cases:

```
<possibly multiple lines of anode source code which will generate an error when compiled>
?<Expected anode::front::ErrorKind>, <expected line>, <expected char index>
```

A test case is considered failed whenever the first error of any test case does not match the expected `ErrorKind`, line or char index.

The `?` delimits test cases.  The preceding lines will be be parsed and semantic checked but not fully compiled.  

A test may be ignored by preceding placing a `#` immediately after the `?`.  The test will be skipped. 

Any failing test case will cause the build to fail.  A test that does not result in any errors will also be considered failed.
