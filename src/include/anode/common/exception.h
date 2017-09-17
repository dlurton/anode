#pragma once
#include "anode.h"
#include <string>
#include <iostream>
#include <common/string_format.h>

#define ASSERT_FAIL(message) \
    throw ::anode::exception::DebugAssertionFailedException(anode::string::format("%s:%d: Debug assertion failed: %s", __FILE__, __LINE__, message));

#define ASSERT(arg) if(!(arg)) { \
    throw ::anode::exception::DebugAssertionFailedException( \
        anode::string::format("%s:%d: Debug assertion failed.  Expression: %s", __FILE__, __LINE__, #arg)); };

namespace anode { namespace exception {

class Exception : public std::runtime_error {
public:
    Exception(const std::string &message) : runtime_error(message) {
        //TODO: store traceback/stacktrace/whatever
        //http://stackoverflow.com/questions/3151779/how-its-better-to-invoke-gdb-from-program-to-print-its-stacktrace/4611112#4611112
    }

    ~Exception() override = default;

    void dump() {
        std::cout << what() << "\n";
    }
};

/** Thrown by runtime libraries when an `assert()` has failed. */
class AnodeAssertionFailedException : public std::runtime_error {
public:
    explicit AnodeAssertionFailedException(const std::string &message) : runtime_error(message) {
    }
};

class FatalException : public Exception {
public:
    explicit FatalException(const std::string &message) : Exception(message) { }

    ~FatalException() override = default;
};


/** Thrown by the compiler whenever a DEBUG_ASSERT has failed.
 * This perhaps should not exist and we should dumping a message to stderr and abort()ing instead? */
class DebugAssertionFailedException : public FatalException {
public:
    explicit DebugAssertionFailedException(const std::string &message) : FatalException(message) {
        std::cerr << message << "\n";
    }
};

}}
