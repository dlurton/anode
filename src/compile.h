
#pragma once

#include "ast.h"
#include "llvm/IR/Module.h"

namespace lwnn {
    namespace compile {
        const char * const MODULE_INIT_SUFFIX = "_initModule";
        std::unique_ptr<llvm::Module> generateCode(lwnn::ast::Module *module, llvm::LLVMContext &context, llvm::TargetMachine *targetMachine);
    }
}