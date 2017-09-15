
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
    llvm::Function *mallocFunc_ = nullptr;
    std::unordered_map<std::string, llvm::Value*> stringConstants_;

public:

    CompileContext(llvm::LLVMContext &llvmContext, llvm::Module &llvmModule, llvm::IRBuilder<> &irBuilder_, TypeMap &typeMap)
        : llvmContext_(llvmContext), llvmModule_(llvmModule), irBuilder_(irBuilder_), typeMap_{typeMap} {
    }

    llvm::LLVMContext &llvmContext() { return llvmContext_; }

    llvm::Module &llvmModule() { return llvmModule_; }

    llvm::IRBuilder<> &irBuilder() { return irBuilder_; }

    void mapSymbolToValue(scope::Symbol *symbol, llvm::Value *value) {
        ASSERT(value);
        symbolValueMap_[symbol] = value;
    }

    llvm::Function *assertFailFunc() {
        if(!assertFailFunc_) {
            assertFailFunc_ = llvm::cast<llvm::Function>(llvmModule().getOrInsertFunction(
                ASSERT_FAILED_FUNC_NAME,
                llvm::Type::getVoidTy(llvmContext()),      //Return type
                llvm::Type::getInt8PtrTy(llvmContext()),   //char * to source filename
                llvm::Type::getInt32Ty(llvmContext())     //line number
            ));

            auto paramItr = assertFailFunc_->arg_begin();
            llvm::Value *executionContext = paramItr++;
            executionContext->setName("filename");

            llvm::Value *primitiveType = paramItr;
            primitiveType->setName("lineNo");

        }
        return assertFailFunc_;
    }

    llvm::Function *assertPassFunc() {
        if(!assertPassFunc_) {
            assertPassFunc_ = llvm::cast<llvm::Function>(llvmModule().getOrInsertFunction(
                ASSERT_PASSED_FUNC_NAME,
                llvm::Type::getVoidTy(llvmContext())      //Return type
            ));
        }
        return assertPassFunc_;
    }

    llvm::Function *mallocFunc() {
        if(!mallocFunc_) {
            mallocFunc_ = llvm::cast<llvm::Function>(llvmModule().getOrInsertFunction(
                MALLOC_FUNC_NAME,
                llvm::Type::getInt8PtrTy(llvmContext()),   //Return value is pointer to i8*
                llvm::Type::getInt32Ty(llvmContext())      //Size to allocate, in bytes
            ));

            llvm::Value *executionContext = mallocFunc_->arg_begin();
            executionContext->setName("size");
        }
        return mallocFunc_;
    }

    llvm::Value *getMappedValue(scope::Symbol *symbol) {
        llvm::Value *found = symbolValueMap_[symbol];
        ASSERT(found && "Symbol must be mapped to an LLVM value.");
        return found;
    }

    TypeMap &typeMap() { return typeMap_; }

    llvm::Constant *getDefaultValueForType(type::Type *type) {
        if(type->isClass()) {
            return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(typeMap_.toLlvmType(type)));
        }
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