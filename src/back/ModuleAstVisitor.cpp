
#include "back/compile.h"
#include "front/ast.h"


#include "emit.h"
#include "CompileContext.h"
#include "CompileAstVisitor.h"

#include "llvm.h"


using namespace lwnn::ast;

/**
 * The best way I have found to figure out what instructions to use with LLVM is to
 * write some c++ that does the equivalent of what want to do and compile it here: http://ellcc.org/demo/index.cgi
 * then examine the IR that's generated.
 */

namespace lwnn {
    namespace back {

        class ModuleAstVisitor : public CompileAstVisitor {
            llvm::TargetMachine &targetMachine_;

            llvm::Function *initFunc_;
            int resultExprStmtCount_ = 0;
            llvm::Function *resultFunc_;
            llvm::Value *executionContextPtrValue_;

            ast::CompoundStmt *rootCompoundStmt_ = nullptr;
        public:
            ModuleAstVisitor(CompileContext &cc, llvm::TargetMachine &targetMachine)
                : CompileAstVisitor{cc},
                  targetMachine_{targetMachine} { }

            virtual bool visitingExprStmt(ExprStmt *expr) override {

                llvm::Value *llvmValue = emitExpr(expr, cc());
                if(llvmValue) {
                    resultExprStmtCount_++;
                    std::string variableName = string::format("result_%d", resultExprStmtCount_);
                    llvm::AllocaInst *resultVar = cc().irBuilder().CreateAlloca(llvmValue->getType(), nullptr, variableName);

                    cc().irBuilder().CreateStore(llvmValue, resultVar);

                    std::string bitcastVariableName = string::format("bitcasted_%d", resultExprStmtCount_);
                    auto bitcasted = cc().irBuilder().CreateBitCast(resultVar, llvm::Type::getInt64PtrTy(cc().llvmContext()));
//
                    std::vector<llvm::Value*> args {
                        //ExecutionContext pointer
                        executionContextPtrValue_,
                        //PrimitiveType,
                        llvm::ConstantInt::get(cc().llvmContext(), llvm::APInt(32, (uint64_t)expr->type()->primitiveType(), true)),
                        //Pointer to value.
                        bitcasted
                    };
                    cc().irBuilder().CreateCall(resultFunc_, args);
                }

                return false;
            }


            virtual bool visitingClassDefinition(ClassDefinition *cd) {
                std::vector<scope::Symbol*> symbols = cd->body()->scope()->symbols();
                std::vector<llvm::Type*> memberTypes;

                memberTypes.reserve(symbols.size());
                for(auto s : symbols) {
                    //TODO: store field indexes for later reference...
                    memberTypes.push_back(cc().toLlvmType(s->type()));
                }

                llvm::StructType* structType = llvm::StructType::create(cc().llvmContext(), memberTypes, cd->name(), false);

                //LLVM will not emit an unused struct so create a dummy variable...
                cc().llvmModule().getOrInsertGlobal(string::format("use_%s", cd->name().c_str()), structType);

                //TODO:  store structType for later reference...
                return false;
            }

            virtual bool visitingModule(Module *module) override {
                rootCompoundStmt_ = module->body();
                cc().llvmModule().setDataLayout(targetMachine_.createDataLayout());

                declareResultFunction();

                startModuleInitFunc(module);

                declareExecutionContextGlobal();

                //Define all the global variables now...
                //The reason for doing this here instead of in visitVariableDeclExpr is we do not
                //have VariableDeclExprs for the imported global variables.
                std::vector<scope::Symbol*> globals = module->scope()->symbols();
                for(scope::Symbol *symbol : globals) {
                    cc().llvmModule().getOrInsertGlobal(symbol->name(), cc().toLlvmType(symbol->type()));
                    llvm::GlobalVariable *globalVar = cc().llvmModule().getNamedGlobal(symbol->name());
                    globalVar->setLinkage(llvm::GlobalValue::ExternalLinkage);
                    globalVar->setAlignment(ALIGNMENT);
                }

                return true;
            }

            virtual void visitedModule(Module *) override {
                cc().irBuilder().CreateRetVoid();
                llvm::raw_ostream &os = llvm::errs();
                if(llvm::verifyModule(cc().llvmModule(), &os)) {
                    cc().llvmModule().dump();
                    ASSERT_FAIL();
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
        };

        std::unique_ptr<llvm::Module> emitModule(lwnn::ast::Module *module, llvm::LLVMContext &llvmContext,
                                                 llvm::TargetMachine *targetMachine) {
            std::unique_ptr<llvm::Module> llvmModule = std::make_unique<llvm::Module>(module->name(), llvmContext);
            llvm::IRBuilder<> irBuilder{llvmContext};

            CompileContext cc{llvmContext, *llvmModule.get(), irBuilder};
            ModuleAstVisitor visitor{cc, *targetMachine};

            module->accept(&visitor);
            return std::move(llvmModule);
        }

    } //namespace compile
} //namespace lwnn