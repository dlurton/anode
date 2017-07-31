
#include "compile.h"

#include "ast.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wunused-parameter"


#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Verifier.h"

#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/DynamicLibrary.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/RuntimeDyld.h"

#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"

#pragma GCC diagnostic pop

using namespace lwnn::ast;

/**
 * The best way I have found to figure out what instructions to use with LLVM is to
 * write some c++ that does the equivalent of what want to do and compile it here: http://ellcc.org/demo/index.cgi
 * then examine the IR that's generated.
 */

namespace lwnn {
    namespace compile {
        const int ALIGNMENT = 4;


        class CompileContext {
            llvm::LLVMContext &llvmContext_;
            llvm::Module &llvmModule_;
            llvm::IRBuilder<> &irBuilder_;
            std::stack<scope::SymbolTable*> symbolTableStack_;
        public:
            CompileContext(llvm::LLVMContext &llvmContext,
                           llvm::Module &llvmModule,
                           llvm::IRBuilder<> &irBuilder_)
                : llvmContext_(llvmContext), llvmModule_(llvmModule),
                    irBuilder_(irBuilder_) { }

            void pushScope(scope::SymbolTable *scope) {
                ASSERT(scope);
                symbolTableStack_.push(scope);
            }

            void popScope() {
                ASSERT(symbolTableStack_.size() > 0);
                symbolTableStack_.pop();
            }

            llvm::LLVMContext &llvmContext() { return llvmContext_; };
            llvm::Module &llvmModule() { return llvmModule_; }
            llvm::IRBuilder<> &irBuilder() { return irBuilder_; };
        };

        class CompileAstVisitor : public AstVisitor {
            CompileContext cc_;
        protected:
            CompileContext &cc() { return cc_; }
        public:
            CompileAstVisitor(CompileContext &cc) : cc_(cc) {

            }
        };

        llvm::Value *compileExpr(ExprStmt *exprStmt, CompileContext &);
        llvm::Value *compileIfExpr(IfExpr *exprStmt, CompileContext &);

        llvm::Type *to_llvmType(type::PrimitiveType primitiveType, llvm::LLVMContext &llvmContext) {
            ASSERT(primitiveType != type::PrimitiveType::Void && "Expected a primitive data type here");
            switch (primitiveType) {
                case type::PrimitiveType::Void:
                    return llvm::Type::getVoidTy(llvmContext);
                case type::PrimitiveType::Bool:
                    return llvm::Type::getInt1Ty(llvmContext);
                case type::PrimitiveType::Int32:
                    return llvm::Type::getInt32Ty(llvmContext);
                case type::PrimitiveType::Float:
                    return llvm::Type::getFloatTy(llvmContext);
                case type::PrimitiveType::Double:
                    return llvm::Type::getDoubleTy(llvmContext);
                default:
                    ASSERT_FAIL("Unhandled PrimitiveType");
            }
        }

        llvm::Type *to_llvmType(type::Type *type, llvm::LLVMContext &llvmContext) {
            ASSERT(type);
            type::PrimitiveType primitiveType = type->primitiveType();
            return to_llvmType(primitiveType, llvmContext);
        }

        class ExprAstVisitor : public CompileAstVisitor {
            std::stack<llvm::Value *> valueStack_;

        public:
            ExprAstVisitor(CompileContext &cc) : CompileAstVisitor(cc) { }

            bool hasValue() {
                return valueStack_.size() > 0;
            }

            llvm::Value *llvmValue() {
                ASSERT(valueStack_.size() == 1);
                return valueStack_.top();
            }

            virtual void visitedCastExpr(CastExpr *expr) {
                llvm::Value *value = valueStack_.top();
                valueStack_.pop();

                ASSERT(expr->type()->isPrimitive());
                ASSERT(expr->valueExpr()->type()->isPrimitive());

                llvm::Value* castedValue = nullptr;
                llvm::Type *destLlvmType = to_llvmType(expr->type(), cc().llvmContext());
                switch(expr->valueExpr()->type()->primitiveType()) {
                    //From int32
                    case type::PrimitiveType::Int32:
                        switch(expr->type()->primitiveType()) {
                            //To bool
                            case type::PrimitiveType::Bool: {
                                castedValue = cc().irBuilder().CreateICmpNE(value, getDefaultValueForType(expr->valueExpr()->type()));
                                break;
                            }
                            //To int
                            case type::PrimitiveType::Int32:
                                castedValue = value;
                                break;
                            //to float
                            case type::PrimitiveType::Float:
                                castedValue = cc().irBuilder().CreateSIToFP(value, destLlvmType);
                                break;
                            default:
                                ASSERT_FAIL("Unknown cast from int32");
                        }
                        break;
                    //From float
                    case type::PrimitiveType::Float:
                        switch(expr->type()->primitiveType()) {
                            //To to int32 or bool
                            case type::PrimitiveType::Bool:
                                castedValue = cc().irBuilder().CreateFCmpUNE(value, getDefaultValueForType(expr->valueExpr()->type()));
                                break;
                            case type::PrimitiveType::Int32:
                                castedValue = cc().irBuilder().CreateFPToSI(value, destLlvmType);
                                break;
                            case type::PrimitiveType::Float:
                                castedValue = value;
                                break;
                            default:
                                ASSERT_FAIL("Unknown cast from float");
                        }
                        break;
                    //From bool
                    case type::PrimitiveType::Bool:
                        ASSERT_FAIL("cannot cast from bool to anything");
//                        switch(expr->type()->primitiveType()) {
//                            //To bool
//                            case type::PrimitiveType::Bool:
//                                castedValue = value;
//                                break;
//                            //To to int32
//                            case type::PrimitiveType::Int32:
//                                castedValue = irBuilder_.CreateSExt(value, destLlvmType);
//                                break;
//                                //to float
//                            case type::PrimitiveType::Float:
//                                castedValue = irBuilder_.CreateSIToFP(value, destLlvmType);
//                                break;
//                            default:
//                                ASSERT_FAIL("Unknown cast from bool");
//                        }
                        break;
                    default:
                        ASSERT_FAIL("Unhandled type::PrimitiveType")
                }
                ASSERT(castedValue && "Cannot fail to create casted value...");
                valueStack_.push(castedValue);
            }

            virtual void visitedVariableDeclExpr(VariableDeclExpr *expr) override {
                //For now assume global variables.
                //llvm::AllocaInst *allocaInst = irBuilder_.CreateAlloca(type, nullptr, var->name());
                llvm::GlobalVariable *globalVariable = cc().llvmModule().getNamedGlobal(expr->name());
                ASSERT(globalVariable);
                globalVariable->setAlignment(ALIGNMENT);
                globalVariable->setInitializer(getDefaultValueForType(expr->type()));

                 visitVariableRefExpr(expr);
            }

            virtual void visitLiteralInt32Expr(LiteralInt32Expr *expr) override {
                valueStack_.push(llvm::ConstantInt::get(cc().llvmContext(), llvm::APInt(32, (uint64_t)expr->value(), true)));
            }

            virtual void visitLiteralFloatExpr(LiteralFloatExpr *expr) override {
                valueStack_.push(llvm::ConstantFP::get(cc().llvmContext(), llvm::APFloat(expr->value())));
            }

            virtual void visitLiteralBoolExpr(LiteralBoolExpr *expr) {
                valueStack_.push(llvm::ConstantInt::get(cc().llvmContext(), llvm::APInt(1, (uint64_t)expr->value(), true)));
            }

            virtual void visitVariableRefExpr(VariableRefExpr *expr) override {
                //We are assuming global variables for now
                //llvm::Value *allocaInst = lookupVariable(expr->name());

                llvm::GlobalVariable *globalVar = cc().llvmModule().getNamedGlobal(expr->name());
                ASSERT(globalVar);

                if(expr->variableAccess() == VariableAccess::Write) {
                    valueStack_.push(globalVar);
                    return;
                }

                llvm::LoadInst *loadInst = cc().irBuilder().CreateLoad(globalVar);
                loadInst->setAlignment(ALIGNMENT);
                valueStack_.push(loadInst);

            }

            virtual void visitedBinaryExpr(BinaryExpr *expr) override {
                ASSERT(expr->lValue()->type() == expr->rValue()->type() && "data types must match");

                llvm::Value *rValue = valueStack_.top();
                valueStack_.pop();

                llvm::Value *lValue = valueStack_.top();
                valueStack_.pop();

                llvm::Value *result = createOperation(lValue, rValue, expr->operation(), expr->operandsType());
                valueStack_.push(result);
            }

            virtual bool visitingIfExpr(IfExpr *expr) override {
                llvm::Value *value = compileIfExpr(expr, cc());
                valueStack_.push(value);
                return false;
            }


            virtual bool visitingCompoundExpr(CompoundExpr *expr) override {

                llvm::Value *lastValue = nullptr;
                for(ast::ExprStmt *expr : expr->statements()) {
                    ExprAstVisitor exprAstVisitor{cc()};
                    expr->accept(&exprAstVisitor);
                    ASSERT(exprAstVisitor.hasValue());
                    lastValue = exprAstVisitor.llvmValue();
                }
                valueStack_.push(lastValue);

                return false;
            }

        private:

            llvm::Constant *getDefaultValueForType(type::Type *type) {
                ASSERT(type->isPrimitive());
                return getDefaultValueForType(type->primitiveType());
            }

            llvm::Constant *getDefaultValueForType(type::PrimitiveType primitiveType) {
                ASSERT(primitiveType != type::PrimitiveType::Void);
                switch(primitiveType) {
                    case type::PrimitiveType::Bool:
                        return llvm::ConstantInt::get(cc().llvmContext(), llvm::APInt(1, 0, false));
                    case type::PrimitiveType::Int32:
                        return llvm::ConstantInt::get(cc().llvmContext(), llvm::APInt(32, 0, true));
                    case type::PrimitiveType::Float:
                        //APFloat has different constructors depending on if you want a float or a double...
                        return llvm::ConstantFP::get(cc().llvmContext(), llvm::APFloat((float)0.0));
                    case type::PrimitiveType::Double:
                        return llvm::ConstantFP::get(cc().llvmContext(), llvm::APFloat(0.0));
                    default:
                        ASSERT_FAIL("Unhandled PrimitiveType");
                }
            }

            llvm::Value *
            createOperation(llvm::Value *lValue, llvm::Value *rValue, BinaryOperationKind op, const type::Type *type) {
                ASSERT(type->isPrimitive() && "Only primitive types currently supported here");

                if(op == BinaryOperationKind::Assign) {
                    cc().irBuilder().CreateStore(rValue, lValue); //(args swapped on purpose)
                    //Note:  I think this will call rValue a second time if rValue is a call site.
                    return rValue;
                }
                switch (type->primitiveType()) {
                    case type::PrimitiveType::Bool:
                        switch(op) {
                            case BinaryOperationKind::Eq:
                                return cc().irBuilder().CreateICmpEQ(lValue, rValue);
//                            case BinaryOperationKind::And:
//                                return irBuilder_.CreateICmpEQ(lValue, rValue);
//                            case BinaryOperationKind::Or:
//                                return irBuilder_.CreateICmpEQ(lValue, rValue);
                            default:
                                ASSERT_FAIL("Unsupported BinaryOperationKind for bool primitive type")
                        }
                    case type::PrimitiveType::Int32:
                        switch (op) {
                            case BinaryOperationKind::Eq:
                                return cc().irBuilder().CreateICmpEQ(lValue, rValue);
                            case BinaryOperationKind::Add:
                                return cc().irBuilder().CreateAdd(lValue, rValue);
                            case BinaryOperationKind::Sub:
                                return cc().irBuilder().CreateSub(lValue, rValue);
                            case BinaryOperationKind::Mul:
                                return cc().irBuilder().CreateMul(lValue, rValue);
                            case BinaryOperationKind::Div:
                                return cc().irBuilder().CreateSDiv(lValue, rValue);
                            default:
                                ASSERT_FAIL("Unhandled BinaryOperationKind");
                        }
                    case type::PrimitiveType::Float:
                        switch (op) {
                            case BinaryOperationKind::Eq:
                                return cc().irBuilder().CreateFCmpOEQ(lValue, rValue);
                            case BinaryOperationKind::Add:
                                return cc().irBuilder().CreateFAdd(lValue, rValue);
                            case BinaryOperationKind::Sub:
                                return cc().irBuilder().CreateFSub(lValue, rValue);
                            case BinaryOperationKind::Mul:
                                return cc().irBuilder().CreateFMul(lValue, rValue);
                            case BinaryOperationKind::Div:
                                return cc().irBuilder().CreateFDiv(lValue, rValue);
                            default:
                                ASSERT_FAIL("Unhandled BinaryOperationKind");
                        }
                    default:
                        ASSERT_FAIL("Unhandled PrimitiveType");
                }
            }
        };

        llvm::Value *compileExpr(ExprStmt *exprStmt, CompileContext &cc) {
            ExprAstVisitor visitor{cc};
            exprStmt->accept(&visitor);

            return visitor.hasValue() ? visitor.llvmValue() : nullptr;
        }

        llvm::Value *compileIfExpr(IfExpr *selectExpr, CompileContext &cc) {
            //This function is modeled after: https://llvm.org/docs/tutorial/LangImpl08.html (ctrl-f for "IfExprAST::codegen")

            //Emit the condition
            llvm::Value *condValue = compileExpr(selectExpr->condition(), cc);

            //Prepare the BasicBlocks
            llvm::Function *currentFunc = cc.irBuilder().GetInsertBlock()->getParent();
            llvm::BasicBlock *thenBlock = llvm::BasicBlock::Create(cc.llvmContext(), "thenBlock");
            llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(cc.llvmContext(), "elseBlock");
            llvm::BasicBlock *endBlock = llvm::BasicBlock::Create(cc.llvmContext(), "endBlock");

            //Branch to then or else blocks, depending on condition.
            cc.irBuilder().CreateCondBr(condValue, thenBlock, elseBlock);

            //Emit the thenBlock
            currentFunc->getBasicBlockList().push_back(thenBlock);
            cc.irBuilder().SetInsertPoint(thenBlock);
            llvm::Value *thenValue = compileExpr(selectExpr->thenExpr(), cc);
            thenBlock = cc.irBuilder().GetInsertBlock();

            //Emit the endBlock, skipping the elseBlock
            cc.irBuilder().CreateBr(endBlock);

            //Emit the elseBlock
            currentFunc->getBasicBlockList().push_back(elseBlock);
            cc.irBuilder().SetInsertPoint(elseBlock);
            llvm::Value *elseValue = compileExpr(selectExpr->elseExpr(), cc);
            cc.irBuilder().CreateBr(endBlock);
            elseBlock = cc.irBuilder().GetInsertBlock();

            //Emit the endBlock
            currentFunc->getBasicBlockList().push_back(endBlock);
            cc.irBuilder().SetInsertPoint(endBlock);
            llvm::PHINode *phi = cc.irBuilder().CreatePHI(to_llvmType(selectExpr->type(), cc.llvmContext()), 2, "iftmp");
            phi->addIncoming(thenValue, thenBlock);
            phi->addIncoming(elseValue, elseBlock);

            return phi;
        }
        class ModuleAstVisitor : public CompileAstVisitor {
            llvm::TargetMachine &targetMachine_;

            llvm::Function *initFunc_;
            int resultExprStmtCount_ = 0;
            llvm::Function *resultFunc_;
            llvm::Value *executionContextPtrValue_;

            ast::CompoundExpr *rootCompoundExpr_ = nullptr;
        public:
            ModuleAstVisitor(CompileContext &cc, llvm::TargetMachine &targetMachine)
                : CompileAstVisitor{cc},
                  targetMachine_{targetMachine} { }

            virtual bool visitingExprStmt(ExprStmt *expr) override {
                if(rootCompoundExpr_ == expr) return true;

                //opeFollowingVisitor::visitingExprStmt(expr);
                llvm::Value *llvmValue = compileExpr(expr, cc());
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

            virtual bool visitingModule(Module *module) override {
                rootCompoundExpr_ = module->body();
                cc().llvmModule().setDataLayout(targetMachine_.createDataLayout());

                declareResultFunction();

                startModuleInitFunc(module);

                declareExecutionContextGlobal();

                //Define all the global variables now...
                //The reason for doing this here instead of in visitVariableDeclExpr is we do not
                //have VariableDeclExprs for the imported global variables.
                std::vector<scope::Symbol*> globals = module->scope()->symbols();
                for(scope::Symbol *symbol : globals) {
                    cc().llvmModule().getOrInsertGlobal(symbol->name(), to_llvmType(symbol->type(), cc().llvmContext()));
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

                llvm::Value *valuePtr = paramItr++;
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

        std::unique_ptr<llvm::Module> compileModule(Module *module, llvm::LLVMContext &llvmContext, llvm::TargetMachine *targetMachine) {
            std::unique_ptr<llvm::Module> llvmModule = std::make_unique<llvm::Module>(module->name(), llvmContext);
            llvm::IRBuilder<> irBuilder{llvmContext};
            
            CompileContext cc{llvmContext, *llvmModule.get(), irBuilder};
            ModuleAstVisitor visitor{cc, *targetMachine};

            module->accept(&visitor);
            return std::move(llvmModule);
        }

    } //namespace compile
} //namespace lwnn