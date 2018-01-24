
#pragma once

#define NO_COPY(className) className(const className &) = delete;
#define NO_ASSIGN(className) className &operator=(const className &) = delete;
#define NO_COPY_NO_ASSIGN(className) NO_COPY(className) NO_ASSIGN(className)


#include "common/string.h"
//TO DO:  make these no-ops for release builds.
#define ASSERT_FAIL(message) \
    throw ::anode::exception::DebugAssertionFailedException(::anode::string::format("%s:%d: Debug assertion failed: %s", __FILE__, __LINE__, message));

#define ASSERT(arg) if(!(arg)) { \
    throw ::anode::exception::DebugAssertionFailedException( \
        ::anode::string::format("%s:%d: Debug assertion failed.  Expression: %s", __FILE__, __LINE__, #arg)); };


//Basic Boehm garbage collector stuff.
#ifdef ANODE_DEBUG
#define GC_DEBUG
#endif
#include <gc.h>
//Defines an allocator for the STD library so that objects included within containers are not collected prematurely.
#include <gc/gc_allocator.h>
//Includes gc and gc_cleanup classes.
#include <gc/gc_cpp.h>

namespace anode {

class Object : public gc {
public:
    virtual ~Object() { }
};


namespace exception {
class Exception : public std::runtime_error {
public:
    Exception(const std::string &message) : runtime_error(message) {
        //TODO: store traceback/stacktrace/whatever
        //http://stackoverflow.com/questions/3151779/how-its-better-to-invoke-gdb-from-program-to-print-its-stacktrace/4611112#4611112
    }

    ~Exception() override = default;

    void dump() {
        //std::cout << what() << "\n";
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


/** Thrown whenever a DEBUG_ASSERT has failed.
 * This perhaps should not exist and we should dumping a message to stderr and abort()ing instead? */
class DebugAssertionFailedException : public FatalException {
public:
    explicit DebugAssertionFailedException(const std::string &message) : FatalException(message) {
        //std::cerr << message << "\n";
    }
};


} //end namespace exception

template<typename TObject>
bool isInstanceOf(Object *node) {
    return dynamic_cast<TObject*>(node);
}

template<typename TObject>
bool isInstanceOf(Object &node) {
    return dynamic_cast<TObject*>(&node);
}

template<typename TObject>
TObject *downcast(Object *node) {
    TObject *upcasted = dynamic_cast<TObject*>(node);
    if(!upcasted){
        ASSERT_FAIL("Attempted to perform an invalid downcast");
    }

    return upcasted;
}


template<typename TObject>
TObject &downcast(Object &node) {
    TObject *upcasted = dynamic_cast<TObject*>(&node);
    if(!upcasted){
        ASSERT_FAIL("Attempted to perform an invalid downcast");
    }

    return *upcasted;
}


}
