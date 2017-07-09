
#pragma once

#include "ast.h"
#include "llvm/IR/Module.h"

namespace lwnn {
    namespace compile {
        std::unique_ptr<llvm::Module> generateCode(const lwnn::ast::Module *module, llvm::LLVMContext *context, llvm::TargetMachine *targetMachine);
    }
}