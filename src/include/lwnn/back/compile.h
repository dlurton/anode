
#pragma once

#include "front/ast.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <cstring>
#include "llvm/IR/Module.h"
#pragma GCC diagnostic pop

//namespace llvm {
//    class Module;
//    class LLVMContext;
//    class TargetMachine;
//}

namespace lwnn {
    namespace back {
        const char * const MODULE_INIT_SUFFIX = "__initModule__";
        const char * const RECEIVE_RESULT_FUNC_NAME = "__receive_result__";
        const char * const EXECUTION_CONTEXT_GLOBAL_NAME = "__execution__context__";
        std::unique_ptr<llvm::Module> emitModule(lwnn::ast::Module *module, llvm::LLVMContext &llvmContext,
                                                 llvm::TargetMachine *targetMachine);
    }
}