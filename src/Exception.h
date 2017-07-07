#pragma once

#include <string>
#include <iostream>

#define ASSERT_NOT_NULL(arg) if((arg) == nullptr) throw lwnn::AssertionException("Expression cannot be null.", #arg);
#define ASSERT(arg) if((arg)) throw lwnn::InvalidArgumentException(#arg);

#ifdef LWNN_DEBUG
#define DEBUG_ASSERT(arg,) if(!(arg)) { throw lwnn::DebugAssertionFailedException( \
    std::string("Debug assertion failed!") + \
    std::string("\nFile       : ") + std::string(__FILE__) + \
    std::string("\nLine       : ") + std::to_string(__LINE__) + \
    std::string("\nExpression : ") + std::string(#arg)); }
#else
#define DEBUG_ASSERT(arg) //no op
#endif
#define UNUSED(x) ((void)(x))
namespace lwnn {

    class Exception : public std::runtime_error {
    public:
        Exception(const std::string &message) : runtime_error(message) {

            //TODO: store traceback/stacktrace/whatever.
            // http://stackoverflow.com/questions/3151779/how-its-better-to-invoke-gdb-from-program-to-print-its-stacktrace/4611112#4611112
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

#ifdef LWNN_DEBUG
    class DebugAssertionFailedException : public FatalException {
    public:
        DebugAssertionFailedException(const std::string &message) : FatalException(message) {

        }
    };
#endif

    class AssertionException : public Exception {
    public:
        AssertionException(const std::string &argumentName)
                : Exception("Invalid value: " + argumentName) {
        }
        AssertionException(const std::string &message, const std::string &argumentName)
                : Exception(message + " Expression:" +  argumentName) {
        }
    };

    class UnhandledSwitchCase : public FatalException {
    public:
        UnhandledSwitchCase() : FatalException("Ruh roh.  There was an unhandled switch case.") {}

        virtual ~UnhandledSwitchCase() {

        }
    };

    class InvalidStateException : public FatalException {
    public:
        InvalidStateException(const std::string &message) : FatalException(message) {

        }

        virtual ~InvalidStateException() {

        }

    };
}