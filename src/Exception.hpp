#pragma once

#include <string>
#include <iostream>

#define ARG_NOT_NULL(arg) if((arg) == nullptr) throw lwnn::InvalidArgumentException(#arg);

#ifdef LWNN_DEBUG
#define DEBUG_ASSERT(arg, reason) if(!(arg)) { throw lwnn::DebugAssertionFailedException( \
    std::string("Debug assertion failed!") + \
    std::string("\nFile       : ") + std::string(__FILE__) + \
    std::string("\nLine       : ") + std::to_string(__LINE__) + \
    std::string("\nExpression : ") + std::string(#arg) + \
    std::string("\nReason     : ") + std::string(reason)); }
#else
#define DEBUG_ASSERT(arg, reason) //no op
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

    class InvalidArgumentException : public Exception {
    public:
        InvalidArgumentException(const std::string &argumentName)
                : Exception("Invalid value for argument " + argumentName) {

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