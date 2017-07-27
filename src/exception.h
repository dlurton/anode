#pragma once

#include <string>
#include <iostream>

#define ASSERT_FAIL(message) throw lwnn::exception::DebugAssertionFailedException( \
    std::string(message) + \
    std::string("\nFile       : ") + std::string(__FILE__) + \
    std::string("\nLine       : ") + std::to_string(__LINE__));

#define ASSERT(arg) if(!(arg)) { throw lwnn::exception::DebugAssertionFailedException( \
    std::string("Debug assertion failed!") + \
    std::string("\nFile       : ") + std::string(__FILE__) + \
    std::string("\nLine       : ") + std::to_string(__LINE__) + \
    std::string("\nExpression : ") + std::string(#arg)); }

namespace lwnn {
    namespace exception {
    class Exception : public std::runtime_error {
    public:
        Exception(const std::string &message) : runtime_error(message) {
            //TODO: store traceback/stacktrace/whatever
            //http://stackoverflow.com/questions/3151779/how-its-better-to-invoke-gdb-from-program-to-print-its-stacktrace/4611112#4611112
        }

        virtual ~Exception() { }

        void dump() {
            std::cout << what() << "\n";
        }
    };

    class FatalException : public Exception {
    public:
        FatalException(const std::string &message) : Exception(message) { }

        virtual ~FatalException() {

        }
    };

    class DebugAssertionFailedException : public FatalException {
    public:
        DebugAssertionFailedException(const std::string &message) : FatalException(message) {

        }
    };

    class AssertionException : public Exception {
    public:
        AssertionException(const std::string &argumentName)
                : Exception("Invalid value: " + argumentName) {
        }
        AssertionException(const std::string &message, const std::string &argumentName)
                : Exception(message + " Expression:" +  argumentName) {
        }
    };

    class InvalidStateException : public FatalException {
    public:
        InvalidStateException(const std::string &message) : FatalException(message) {

        }

        virtual ~InvalidStateException() {

        }

    };
    } //namespace exception
}//namespace lwnn
