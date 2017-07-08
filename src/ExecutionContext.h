
#pragma once

#include "ast.h"

namespace lwnn {

    typedef float (*FloatFuncPtr)(void);
    typedef int (*IntFuncPtr)(void);

    class ExecutionContext {
    public:
        virtual uint64_t getSymbolAddress(const std::string &name) = 0;
        virtual void addModule(const Module *module) = 0;
    };

    std::unique_ptr<ExecutionContext> createExecutionContext();
    void initializeJitCompiler();


    namespace ExprRunner {

        void init();

        float runFloatExpr(std::unique_ptr<const Expr> expr);
        int runInt32Expr(std::unique_ptr<const Expr> expr);

        std::string compile(std::unique_ptr<const Expr> expr);
        std::string compile(std::unique_ptr<const Module> expr);
    }
}

