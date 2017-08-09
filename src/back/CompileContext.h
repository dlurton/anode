
#pragma once

#include "llvm.h"

#include <map>
#include <unordered_map>

namespace lwnn {
namespace back {

class CompileContext : no_copy, no_assign {
    llvm::LLVMContext &llvmContext_;
    llvm::Module &llvmModule_;
    llvm::IRBuilder<> &irBuilder_;
    std::stack<scope::SymbolTable *> symbolTableStack_;

    std::unordered_map<type::Type *, llvm::Type *> typeMap_;
public:
    CompileContext(llvm::LLVMContext &llvmContext, llvm::Module &llvmModule, llvm::IRBuilder<> &irBuilder_)
        : llvmContext_(llvmContext), llvmModule_(llvmModule), irBuilder_(irBuilder_) {

        mapTypes(&type::Primitives::Void, llvm::Type::getVoidTy(llvmContext_));
        mapTypes(&type::Primitives::Bool, llvm::Type::getInt1Ty(llvmContext_));
        mapTypes(&type::Primitives::Int32, llvm::Type::getInt32Ty(llvmContext_));
        mapTypes(&type::Primitives::Float, llvm::Type::getFloatTy(llvmContext_));
        mapTypes(&type::Primitives::Double, llvm::Type::getDoubleTy(llvmContext_));
    }

    llvm::LLVMContext &llvmContext() { return llvmContext_; };

    llvm::Module &llvmModule() { return llvmModule_; }

    llvm::IRBuilder<> &irBuilder() { return irBuilder_; };

    void pushScope(scope::SymbolTable *scope) {
        ASSERT(scope);
        symbolTableStack_.push(scope);
    }

    void popScope() {
        ASSERT(symbolTableStack_.size() > 0);
        symbolTableStack_.pop();
    }

    void mapTypes(type::Type *lwnnType, llvm::Type *llvmType) {
        typeMap_[lwnnType] = llvmType;
    }

    llvm::Type *toLlvmType(type::Type *lwnnType) {
        ASSERT(lwnnType);
        llvm::Type *foundType = typeMap_[lwnnType];

        ASSERT(foundType && "LWNN type to LLVM type Mapping must exist!");
        return foundType;
    }

    llvm::Constant *getDefaultValueForType(type::Type *type) {
        ASSERT(type->isPrimitive());
        return getDefaultValueForType(type->primitiveType());
    }

    llvm::Constant *getDefaultValueForType(type::PrimitiveType primitiveType) {
        ASSERT(primitiveType != type::PrimitiveType::Void);
        switch (primitiveType) {
            case type::PrimitiveType::Bool:
                return llvm::ConstantInt::get(llvmContext_, llvm::APInt(1, 0, false));
            case type::PrimitiveType::Int32:
                return llvm::ConstantInt::get(llvmContext_, llvm::APInt(32, 0, true));
            case type::PrimitiveType::Float:
                //APFloat has different constructors depending on if you want a float or a double...
                return llvm::ConstantFP::get(llvmContext_, llvm::APFloat((float) 0.0));
            case type::PrimitiveType::Double:
                return llvm::ConstantFP::get(llvmContext_, llvm::APFloat(0.0));
            default:
                ASSERT_FAIL("Unhandled PrimitiveType");
        }
    }
};

}}