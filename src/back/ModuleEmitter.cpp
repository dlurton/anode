
#include "back/compile.h"
#include "front/ast.h"
#include "emit.h"
#include "CompileContext.h"
#include "CompileAstVisitor.h"
#include "llvm.h"
#include "GlobalVariableAstVisitor.h"


/**
 * The best way I have found to figure out what instructions to use with LLVM is to
 * write some c++ that does the equivalent of what want to do and compile it here: http://ellcc.org/demo/index.cgi
 * then examine the IR that's generated.
 */

namespace anode { namespace back {
using namespace anode::front;
using namespace anode::front::ast;

class ModuleEmitter : public gc {
    CompileContext &cc_;
    llvm::TargetMachine &targetMachine_;

    llvm::Function *initFunc_ = nullptr;
    int resultExprStmtCount_ = 0;
    llvm::Function *resultFunc_ = nullptr;
    llvm::Value *executionContextPtrValue_ = nullptr;

public:
    ModuleEmitter(CompileContext &cc, llvm::TargetMachine &targetMachine)
        : cc_{cc}, targetMachine_{targetMachine} {}


    void emitModuleLevelExprStmt(ExprStmt &expr)  {
        llvm::Value *llvmValue = emitExpr(expr, cc_);

        if (!expr.type().isPrimitive()) {
            //eventually, we'll call toString() or somesuch.
            return;
        }

        if (llvmValue && !llvmValue->getType()->isVoidTy()) {
            resultExprStmtCount_++;
            std::string variableName = string::format("result_%d", resultExprStmtCount_);
            llvm::AllocaInst *resultVar = cc_.irBuilder().CreateAlloca(llvmValue->getType(), nullptr, variableName);

            cc_.irBuilder().CreateStore(llvmValue, resultVar);

            std::string bitcastVariableName = string::format("bitcasted_%d", resultExprStmtCount_);
            auto bitcasted = cc_.irBuilder().CreateBitCast(resultVar, llvm::Type::getInt64PtrTy(cc_.llvmContext()));

            std::vector<llvm::Value *> args{
                //ExecutionContext pointer
                executionContextPtrValue_,
                //PrimitiveType,
                llvm::ConstantInt::get(cc_.llvmContext(), llvm::APInt(32, (uint64_t) expr.type().primitiveType(), true)),
                //Pointer to value.
                bitcasted
            };
            cc_.irBuilder().CreateCall(resultFunc_, args);
        }
    }

public:
    void emitModule(Module *module)  {

        cc_.llvmModule().setDataLayout(targetMachine_.createDataLayout());

        declareResultFunction();
        startModuleInitFunc(module);
        declareExecutionContextGlobal();
        emitGlobals(module, cc_);
        emitFuncDefs(module, cc_);

        for(ExprStmt &exprStmt : module->body().expressions()) {
            emitModuleLevelExprStmt(exprStmt);
        }

        cc_.irBuilder().CreateRetVoid();
        llvm::raw_ostream &os = llvm::errs();
        if (llvm::verifyModule(cc_.llvmModule(), &os)) {
            std::cerr << "Module dump: \n";
            std::cerr.flush();
#ifdef ANODE_DEBUG
            cc_.llvmModule().dump();
#endif
            ASSERT_FAIL("Failed LLVM module verification.");
        }

        //Copy global variables to the global scope so they can be shared among modules..
        for(scope::Symbol &symbolToExport : module->scope().symbols()) {
            //TODO:  naming collisions here?
            cc_.world().globalScope().addSymbol(symbolToExport.cloneForExport());
        }
    }

private:
    void startModuleInitFunc(const Module *module) {
        auto initFuncRetType = llvm::Type::getVoidTy(cc_.llvmContext());
        initFunc_ = llvm::cast<llvm::Function>(
            cc_.llvmModule().getOrInsertFunction(module->name() + MODULE_INIT_SUFFIX, initFuncRetType));

        initFunc_->setCallingConv(llvm::CallingConv::C);
        auto initFuncBlock = llvm::BasicBlock::Create(cc_.llvmContext(), "begin", initFunc_);

        cc_.irBuilder().SetInsertPoint(initFuncBlock);
    }

    void declareResultFunction() {
        resultFunc_ = llvm::cast<llvm::Function>(cc_.llvmModule().getOrInsertFunction(
            RECEIVE_RESULT_FUNC_NAME,
            llvm::Type::getVoidTy(cc_.llvmContext()),        //Return type
            llvm::Type::getInt64PtrTy(cc_.llvmContext()),    //Pointer to ExecutionContext
            llvm::Type::getInt32Ty(cc_.llvmContext()),       //type::PrimitiveType
            llvm::Type::getInt64PtrTy(cc_.llvmContext())));  //Pointer to value


        auto paramItr = resultFunc_->arg_begin();
        llvm::Value *executionContext = paramItr++;
        executionContext->setName("executionContext");

        llvm::Value *primitiveType = paramItr++;
        primitiveType->setName("primitiveType");

        llvm::Value *valuePtr = paramItr;
        valuePtr->setName("valuePtr");
    }

    void declareExecutionContextGlobal() {
        cc_.llvmModule().getOrInsertGlobal(EXECUTION_CONTEXT_GLOBAL_NAME, llvm::Type::getInt64Ty(cc_.llvmContext()));
        llvm::GlobalVariable *globalVar = cc_.llvmModule().getNamedGlobal(EXECUTION_CONTEXT_GLOBAL_NAME);
        globalVar->setLinkage(llvm::GlobalValue::ExternalLinkage);
        globalVar->setAlignment(ALIGNMENT);

        executionContextPtrValue_ = globalVar;
    }
};

std::unique_ptr<llvm::Module> emitModule(
    anode::front::ast::AnodeWorld &world,
    anode::front::ast::Module *module,
    anode::back::TypeMap &typeMap,
    llvm::LLVMContext &llvmContext,
    llvm::TargetMachine *targetMachine
) {

    std::unique_ptr<llvm::Module> llvmModule = std::make_unique<llvm::Module>(module->name(), llvmContext);
    llvm::IRBuilder<> irBuilder{llvmContext};

    CompileContext cc{world, llvmContext, *llvmModule.get(), irBuilder, typeMap};
    ModuleEmitter visitor{cc, *targetMachine};
    visitor.emitModule(module);

    return llvmModule;
}

}}
