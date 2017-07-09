
#pragma once

#include "ast.h"

namespace lwnn {
    namespace execute {
        typedef float (*FloatFuncPtr)(void);
        typedef int (*IntFuncPtr)(void);

        class ExecutionContext {
        public:
            virtual uint64_t getSymbolAddress(const std::string &name) = 0;

            virtual void addModule(const ast::Module *module) = 0;
        };

        std::unique_ptr<ExecutionContext> createExecutionContext();

        void initializeJitCompiler();
    }
}
