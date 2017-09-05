
#include "back/compile.h"
#include "front/ast.h"
#include "emit.h"
#include "CompileContext.h"
#include "CompileAstVisitor.h"
#include "CreateStructsAstVisitor.h"
#include "llvm.h"


using namespace anode::ast;

/**
 * The best way I have found to figure out what instructions to use with LLVM is to
 * write some c++ that does the equivalent of what want to do and compile it here: http://ellcc.org/demo/index.cgi
 * then examine the IR that's generated.
 */

namespace anode { namespace back {

void createLlvmStructsForClasses(ast::Module *anodeModule, CompileContext &cc) {
    CreateStructsAstVisitor visitor{cc};
    anodeModule->accept(&visitor);
}

class ModuleAstVisitor : public CompileAstVisitor {
    llvm::TargetMachine &targetMachine_;

    llvm::Function *initFunc_;
    int resultExprStmtCount_ = 0;
    llvm::Function *resultFunc_;
    llvm::Value *executionContextPtrValue_;

public:
    ModuleAstVisitor(CompileContext &cc, llvm::TargetMachine &targetMachine)
        : CompileAstVisitor{cc},
          targetMachine_{targetMachine} {}

    virtual bool visitingClassDefinition(ClassDefinition *) {
        return false;
    }

    virtual bool visitingFuncDefStmt(FuncDefStmt *) override {
        return false;
    }

    virtual bool visitingExprStmt(ExprStmt *expr) override {

        // Skip globally scoped CompoundExpr and allow it's children to be visited
        auto maybeCompoundExpr = dynamic_cast<ast::CompoundExpr*>(expr);
        if(maybeCompoundExpr) {
            if(maybeCompoundExpr->scope()->storageKind() == scope::StorageKind::Global)
                return true;
        }

        if (!expr->type()->isPrimitive()) {
            //eventually, we'll call toString() or somesuch.
            return false;
        }

        llvm::Value *llvmValue = emitExpr(expr, cc());
        if (llvmValue && !llvmValue->getType()->isVoidTy()) {
            resultExprStmtCount_++;
            std::string variableName = string::format("result_%d", resultExprStmtCount_);
            llvm::AllocaInst *resultVar = cc().irBuilder().CreateAlloca(llvmValue->getType(), nullptr, variableName);

            cc().irBuilder().CreateStore(llvmValue, resultVar);

            std::string bitcastVariableName = string::format("bitcasted_%d", resultExprStmtCount_);
            auto bitcasted = cc().irBuilder().CreateBitCast(resultVar, llvm::Type::getInt64PtrTy(cc().llvmContext()));

            std::vector<llvm::Value *> args{
                //ExecutionContext pointer
                executionContextPtrValue_,
                //PrimitiveType,
                llvm::ConstantInt::get(cc().llvmContext(), llvm::APInt(32, (uint64_t) expr->type()->primitiveType(), true)),
                //Pointer to value.
                bitcasted
            };
            cc().irBuilder().CreateCall(resultFunc_, args);
        }

        return false;
    }

    virtual bool visitingModule(Module *module) override {
        cc().llvmModule().setDataLayout(targetMachine_.createDataLayout());

        declareAssertFunctions();
        declareResultFunction();

        startModuleInitFunc(module);

        declareExecutionContextGlobal();

        createLlvmStructsForClasses(module, cc());

        //Define all the global variables now...
        //The reason for doing this here instead of in visitVariableDeclExpr is we do not
        //have VariableDeclExprs for the imported global variables but they do exist as symbols in the global scope.
        gc_vector<scope::VariableSymbol *> globals = module->scope()->variables();
        for (scope::VariableSymbol *symbol : globals) {
            //next step:  make this call to .toLlvmType() return the previously created llvm::StructType
            llvm::Type *llvmType = cc().typeMap().toLlvmType(symbol->type());
            cc().llvmModule().getOrInsertGlobal(symbol->name(), llvmType);
            llvm::GlobalVariable *globalVar = cc().llvmModule().getNamedGlobal(symbol->name());
            cc().mapSymbolToValue(symbol, globalVar);

            if(symbol->type()->isClass()) {

                if (symbol->isExternal()) {
                    //ExternalWeakLinkage defines a symbol in the current module that can be
                    //overridden by a regular ExternalLinkage, it would seem from the LLVM language reference.
                    //https://llvm.org/docs/LangRef.html#linkage-types
                    //HOWEVER...  in my experience this acts *much* more like the C "extern" keyword,
                    //meaning that LLVM seems to expect global variables with ExternalWeakLinkage to be defined
                    //in another module, period.  I also note that ExternalWinkLinkage global variables
                    //cannot have an initializer without causing assertion failure within the LLVM source.
                    globalVar->setLinkage(llvm::GlobalValue::ExternalWeakLinkage);
                } else {
                    //Fills the struct instance with zeros
                    globalVar->setInitializer(llvm::ConstantAggregateZero::get(llvmType));

                    //ExternalLinkage as far as I can tell is not to be confused with the "extern" C keyword.
                    //It seems to mean that the symbol is exposed to other modules, like when the "extern"
                    //and "static" keywords are omitted in C.
                    globalVar->setLinkage(llvm::GlobalValue::ExternalLinkage);
                }
            }

            globalVar->setAlignment(ALIGNMENT);
        }
        emitFuncDefs(module, cc());

        return true;
    }

    virtual void visitedModule(Module *) override {
        cc().irBuilder().CreateRetVoid();
        llvm::raw_ostream &os = llvm::errs();
        if (llvm::verifyModule(cc().llvmModule(), &os)) {
            std::cerr << "Module dump: \n";
            std::cerr.flush();
            cc().llvmModule().dump();
            ASSERT_FAIL("Failed LLVM module verification.");
        }
    }

private:
    void startModuleInitFunc(const Module *module) {
        auto initFuncRetType = llvm::Type::getVoidTy(cc().llvmContext());
        initFunc_ = llvm::cast<llvm::Function>(
            cc().llvmModule().getOrInsertFunction(module->name() + MODULE_INIT_SUFFIX, initFuncRetType));

        initFunc_->setCallingConv(llvm::CallingConv::C);
        auto initFuncBlock = llvm::BasicBlock::Create(cc().llvmContext(), "begin", initFunc_);

        cc().irBuilder().SetInsertPoint(initFuncBlock);
    }

    void declareResultFunction() {
        resultFunc_ = llvm::cast<llvm::Function>(cc().llvmModule().getOrInsertFunction(
            RECEIVE_RESULT_FUNC_NAME,
            llvm::Type::getVoidTy(cc().llvmContext()),        //Return type
            llvm::Type::getInt64PtrTy(cc().llvmContext()),    //Pointer to ExecutionContext
            llvm::Type::getInt32Ty(cc().llvmContext()),       //type::PrimitiveType
            llvm::Type::getInt64PtrTy(cc().llvmContext())));  //Pointer to value


        auto paramItr = resultFunc_->arg_begin();
        llvm::Value *executionContext = paramItr++;
        executionContext->setName("executionContext");

        llvm::Value *primitiveType = paramItr++;
        primitiveType->setName("primitiveType");

        llvm::Value *valuePtr = paramItr;
        valuePtr->setName("valuePtr");
    }

    void declareExecutionContextGlobal() {
        cc().llvmModule().getOrInsertGlobal(EXECUTION_CONTEXT_GLOBAL_NAME, llvm::Type::getInt64Ty(cc().llvmContext()));
        llvm::GlobalVariable *globalVar = cc().llvmModule().getNamedGlobal(EXECUTION_CONTEXT_GLOBAL_NAME);
        globalVar->setLinkage(llvm::GlobalValue::ExternalLinkage);
        globalVar->setAlignment(ALIGNMENT);

        executionContextPtrValue_ = globalVar;
    }

    void declareAssertFunctions() {
        llvm::Function *assertFunc = llvm::cast<llvm::Function>(cc().llvmModule().getOrInsertFunction(
            ASSERT_FAILED_FUNC_NAME,
            llvm::Type::getVoidTy(cc().llvmContext()),      //Return type
            llvm::Type::getInt8PtrTy(cc().llvmContext()),   //char * to source filename
            llvm::Type::getInt32Ty(cc().llvmContext())     //line number
        ));

        auto paramItr = assertFunc->arg_begin();
        llvm::Value *executionContext = paramItr++;
        executionContext->setName("filename");

        llvm::Value *primitiveType = paramItr;
        primitiveType->setName("lineNo");

        cc().setAssertFailFunc(assertFunc);

        assertFunc = llvm::cast<llvm::Function>(cc().llvmModule().getOrInsertFunction(
            ASSERT_PASSED_FUNC_NAME,
            llvm::Type::getVoidTy(cc().llvmContext())      //Return type
        ));
        cc().setAssertPassFunc(assertFunc);

    }
};

std::unique_ptr<llvm::Module> emitModule(
    anode::ast::Module *module,
    anode::back::TypeMap &typeMap,
    llvm::LLVMContext &llvmContext,
    llvm::TargetMachine *targetMachine
) {

    std::unique_ptr<llvm::Module> llvmModule = std::make_unique<llvm::Module>(module->name(), llvmContext);
    llvm::IRBuilder<> irBuilder{llvmContext};

    CompileContext cc{llvmContext, *llvmModule.get(), irBuilder, typeMap};
    ModuleAstVisitor visitor{cc, *targetMachine};

    module->accept(&visitor);
    return std::move(llvmModule);
}

}}
