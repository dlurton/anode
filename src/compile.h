
#pragma once

#include "ast.h"
#include "llvm/IR/Module.h"

namespace lwnn {
    namespace compile {
        const char * const MODULE_INIT_SUFFIX = "_initModule";
        const char * const RECEIVE_RESULT_FUNC_NAME = "__receive_result__";
        const char * const EXECUTION_CONTEXT_GLOBAL_NAME = "__execution__context__";
        std::unique_ptr<llvm::Module> compileModule(lwnn::ast::Module *module, llvm::LLVMContext &llvmContext,
                                                    llvm::TargetMachine *targetMachine);
    }
}