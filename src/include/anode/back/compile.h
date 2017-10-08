
#pragma once
#include "anode.h"
#include "front/ast.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wgnu-statement-expression"
#include <cstring>
#include <llvm/Target/TargetMachine.h>
#include "llvm/IR/Module.h"
#pragma GCC diagnostic pop

//namespace llvm {
//    class Module;
//    class LLVMContext;
//    class TargetMachine;
//}

namespace anode { namespace back {
    const char * const MODULE_INIT_SUFFIX = "__initModule__";
    const char * const RECEIVE_RESULT_FUNC_NAME = "__receive_result__";
    const char * const ASSERT_FAILED_FUNC_NAME = "__assert_failed__";
    const char * const ASSERT_PASSED_FUNC_NAME = "__assert_passed__";
    const char * const MALLOC_FUNC_NAME = "__malloc__";
    const char * const EXECUTION_CONTEXT_GLOBAL_NAME = "__execution__context__";

    class TypeMap : no_assign, no_copy {
        gc_unordered_map<const front::type::Type *, llvm::Type *> typeMap_;
    public:
        TypeMap(llvm::LLVMContext &llvmContext) {

            mapTypes(&front::type::Primitives::Void, llvm::Type::getVoidTy(llvmContext));
            mapTypes(&front::type::Primitives::Bool, llvm::Type::getInt1Ty(llvmContext));
            mapTypes(&front::type::Primitives::Int32, llvm::Type::getInt32Ty(llvmContext));
            mapTypes(&front::type::Primitives::Float, llvm::Type::getFloatTy(llvmContext));
            mapTypes(&front::type::Primitives::Double, llvm::Type::getDoubleTy(llvmContext));
        }

        void mapTypes(front::type::Type *anodeType, llvm::Type *llvmType) {
            typeMap_[anodeType] = llvmType;
        }

        llvm::Type *toLlvmType(front::type::Type *anodeType) {
            ASSERT(anodeType);
            llvm::Type *foundType = typeMap_[anodeType->actualType()];

            ASSERT(foundType && "Anode type to LLVM type Mapping must exist!");
            return foundType;
        }
    };

    std::unique_ptr<llvm::Module> emitModule(
        anode::front::ast::AnodeWorld &world,
        anode::front::ast::Module *module,
        anode::back::TypeMap &typeMap,
        llvm::LLVMContext &llvmContext,
        llvm::TargetMachine *targetMachine
    );

}}