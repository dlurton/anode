
#pragma once
#include "lwnn.h"
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

namespace lwnn { namespace back {
    const char * const MODULE_INIT_SUFFIX = "__initModule__";
    const char * const RECEIVE_RESULT_FUNC_NAME = "__receive_result__";
    const char * const ASSERT_FAILED_FUNC_NAME = "__assert_failed__";
    const char * const ASSERT_PASSED_FUNC_NAME = "__assert_passed__";
    const char * const EXECUTION_CONTEXT_GLOBAL_NAME = "__execution__context__";

    class TypeMap : no_assign, no_copy {
        gc_unordered_map<const type::Type *, llvm::Type *> typeMap_;
    public:
        TypeMap(llvm::LLVMContext &llvmContext) {

            mapTypes(&type::Primitives::Void, llvm::Type::getVoidTy(llvmContext));
            mapTypes(&type::Primitives::Bool, llvm::Type::getInt1Ty(llvmContext));
            mapTypes(&type::Primitives::Int32, llvm::Type::getInt32Ty(llvmContext));
            mapTypes(&type::Primitives::Float, llvm::Type::getFloatTy(llvmContext));
            mapTypes(&type::Primitives::Double, llvm::Type::getDoubleTy(llvmContext));
        }

        void mapTypes(type::Type *lwnnType, llvm::Type *llvmType) {
            typeMap_[lwnnType] = llvmType;
        }

        llvm::Type *toLlvmType(type::Type *lwnnType) {
            ASSERT(lwnnType);
            llvm::Type *foundType = typeMap_[lwnnType->actualType()];

            ASSERT(foundType && "LWNN type to LLVM type Mapping must exist!");
            return foundType;
        }
    };

    std::unique_ptr<llvm::Module> emitModule(
        lwnn::ast::Module *module,
        TypeMap &typeMap,
        llvm::LLVMContext &llvmContext,
        llvm::TargetMachine *targetMachine);

}}