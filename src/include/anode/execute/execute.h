
#pragma once
#include "anode.h"
#include "ast/ast.h"

#include <functional>

namespace anode { namespace execute {
//        typedef float (*FloatFuncPtr)(void);
//        typedef int (*IntFuncPtr)(void);
//        typedef void (*VoidFuncPtr)(void);


union ResultStorage {
    bool boolResult;
    int int32Result;
    float floatReslt;
    double doubleResult;
};

struct StmtResult {
    front::type::PrimitiveType primitiveType;
    ResultStorage storage;

    StmtResult(front::type::PrimitiveType primitiveType, void *valuePtr) {
        set(primitiveType, valuePtr);
    }

    template<typename T>
    T get() {
        if (typeid(T) == typeid(bool)) {
            ASSERT(primitiveType == front::type::PrimitiveType::Bool);
            return storage.boolResult;
        }
        if(typeid(T) == typeid(int)) {
            ASSERT(primitiveType == front::type::PrimitiveType::Int32);
            return storage.int32Result;
        }
        if(typeid(T) == typeid(float)) {
            ASSERT(primitiveType == front::type::PrimitiveType::Float);
            return storage.floatReslt;
        }
        if(typeid(T) == typeid(double)) {
            ASSERT(primitiveType == front::type::PrimitiveType::Double);
            return storage.doubleResult;
        }

        ASSERT_FAIL("T may be only bool, int, float or double");
    }
    void set(anode::front::type::PrimitiveType primitiveType, void *valuePtr) {
        this->primitiveType = primitiveType;
        switch (primitiveType) {
            case front::type::PrimitiveType::Bool:
                storage.boolResult = *reinterpret_cast<bool*>(valuePtr);
                break;
            case front::type::PrimitiveType::Int32:
                storage.int32Result = *reinterpret_cast<int*>(valuePtr);
                break;
            case front::type::PrimitiveType::Float:
                storage.floatReslt = *reinterpret_cast<float*>(valuePtr);
                break;
            case front::type::PrimitiveType::Double:
                storage.doubleResult = *reinterpret_cast<double*>(valuePtr);
                break;
            default:
                ASSERT_FAIL("Unhandled PrimitiveType");
        }
    }
};


class ExecutionException : public std::runtime_error {
public:
    ExecutionException(const std::string &what) : std::runtime_error(what) {

    }
};

class ExecutionContext {
protected:
    /** JIT compiles a module and returns its global initialization function.*/
    virtual uint64_t loadModule(front::ast::Module *module) = 0;
public:
    virtual ~ExecutionContext() { }

    /** Loads a symbol by name from the previously loaded modules. */
    virtual uint64_t getSymbolAddress(const std::string &name) = 0;

    virtual void setPrettyPrintAst(bool value) = 0;
    virtual void setDumpIROnLoad(bool value) = 0;
    virtual bool prepareModule(front::ast::Module *) = 0;

    typedef std::function<void(ExecutionContext*, front::type::PrimitiveType, void*)> ResultCallbackFunctor;

    virtual void setResultCallback(ResultCallbackFunctor functor) = 0;

    /** Loads the specified module and executes its initialization function. */
    void executeModule(front::ast::Module *module) {
        ASSERT(module);
        uint64_t funcPtr = loadModule(module);
        //void (*func)() = reinterpret_cast<__attribute__((cdecl)) void (*)(void)>(funcPtr);
        void (*func)() = reinterpret_cast<void (*)(void)>(funcPtr);
        func();
    };
};

std::unique_ptr<ExecutionContext> createExecutionContext();
}}
