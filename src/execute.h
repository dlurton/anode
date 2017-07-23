
#pragma once

#include "ast.h"

namespace lwnn {
    namespace execute {
        typedef float (*FloatFuncPtr)(void);
        typedef int (*IntFuncPtr)(void);
        typedef void (*VoidFuncPtr)();

        class ExecutionException : public std::runtime_error {
        public:
            ExecutionException(const std::string &what) : std::runtime_error(what) {

            }
        };

        class ExecutionContext {
        protected:
            /** JIT compiles a module and returns its global initialization function.*/
            virtual uint64_t loadModule(std::unique_ptr<ast::Module> module) = 0;
        public:
            virtual ~ExecutionContext() { }

            /** Loads a symbol by name from the previously loaded modules. */
            virtual uint64_t getSymbolAddress(const std::string &name) = 0;

            virtual void setPrettyPrintAst(bool value) = 0;
            virtual void setDumpIROnLoad(bool value) = 0;

            virtual void prepareModule(ast::Module *) = 0;

            /** Loads the specified module and executes its initialization returns its result.
             * This variant of executeModule() is meant to be used by the REPL. */
            template<typename TResult>
            TResult executeModuleWithResult(std::unique_ptr<ast::Module> module) {
                ASSERT(module);
                uint64_t funcPtr = loadModule(std::move(module));
                ASSERT(funcPtr > 0 && "loadModule should not return null");
                //TResult (*func)() = reinterpret_cast<__attribute__((cdecl))TResult (*)(void)>(funcPtr);
                TResult (*func)() = reinterpret_cast<TResult (*)(void)>(funcPtr);
                TResult result = func();
                return result;
            };

            /** Loads the specified module and executes its initialization function. */
            void executeModule(std::unique_ptr<ast::Module> module) {
                ASSERT(module);
                uint64_t funcPtr = loadModule(std::move(module));
                //void (*func)() = reinterpret_cast<__attribute__((cdecl)) void (*)(void)>(funcPtr);
                void (*func)() = reinterpret_cast<void (*)(void)>(funcPtr);
                func();
            };
        };

        std::unique_ptr<ExecutionContext> createExecutionContext();
    }
}
