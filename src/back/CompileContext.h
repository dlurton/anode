
#pragma once

#include "back/compile.h"

#include "llvm.h"
#include "common/containers.h"


namespace anode { namespace back {

/** CompileContext is a place to keep track of values that need to be shared among the AstVisitors that make up the
 * IR generation phase.  A new CompileContext must be created for each module being compiled.
 */
class CompileContext : no_copy, no_assign {
    llvm::LLVMContext &llvmContext_;
    llvm::Module &llvmModule_;
    llvm::IRBuilder<> &irBuilder_;
    TypeMap &typeMap_;
    gc_unordered_map<scope::Symbol *, llvm::Value *> symbolValueMap_;
    llvm::Function *assertFailFunc_ = nullptr;
    llvm::Function *assertPassFunc_ = nullptr;
    std::unordered_map<std::string, llvm::Value*> stringConstants_;
public:

    CompileContext(llvm::LLVMContext &llvmContext, llvm::Module &llvmModule, llvm::IRBuilder<> &irBuilder_, TypeMap &typeMap)
        : llvmContext_(llvmContext), llvmModule_(llvmModule), irBuilder_(irBuilder_), typeMap_{typeMap} {
    }

    llvm::LLVMContext &llvmContext() { return llvmContext_; }

    llvm::Module &llvmModule() { return llvmModule_; }

    llvm::IRBuilder<> &irBuilder() { return irBuilder_; }

    llvm::Function *assertFailFunc() {
        ASSERT(assertFailFunc_);
        return assertFailFunc_;
    }

    void setAssertFailFunc(llvm::Function *assertFunc) {
        assertFailFunc_ = assertFunc;
    }

    void mapSymbolToValue(scope::Symbol *symbol, llvm::Value *value) {
        ASSERT(value);
        symbolValueMap_[symbol] = value;
    }

    llvm::Function *assertPassFunc() {
        ASSERT(assertPassFunc_);
        return assertPassFunc_;
    }

    void setAssertPassFunc(llvm::Function *assertFunc) {
        assertPassFunc_ = assertFunc;
    }

    
    llvm::Value *getMappedValue(scope::Symbol *symbol) {
        llvm::Value *found = symbolValueMap_[symbol];
        ASSERT(found && "Symbol must be mapped to an LLVM value.");
        return found;
    }

    TypeMap &typeMap() { return typeMap_; }

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

    llvm::Value *getDeduplicatedStringConstant(const std::string &value) {
        llvm::Value *&found = stringConstants_[value];
        if(!found) {
            found = irBuilder_.CreateGlobalStringPtr(value);
        }
        return found;
    }
};

}}